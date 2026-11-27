#ifndef AGRABAH_UPDATER_H
#define AGRABAH_UPDATER_H

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <winhttp.h>
  #include <shellapi.h>
#endif

// Self-updater for AgrabahPort against GitHub Releases
// (testsenvergonha-sys/AgrabahPort). Uses Windows WinHTTP loaded dynamically
// at runtime so zero extra link libraries are required and offline/error runs
// never block the launcher UI. All user files (saves/, config.ini,
// keybinds.ini, rom.cfg, *.sfc/*.smc, mods/) are strictly preserved.

namespace agrabah_upd {

enum State {
    UPD_IDLE = 0,
    UPD_CHECKING,
    UPD_UP_TO_DATE,
    UPD_NO_RELEASE,
    UPD_AVAILABLE,
    UPD_DOWNLOADING,
    UPD_EXTRACTING,
    UPD_READY_APPLY,
    UPD_ERROR
};

struct ReleaseInfo {
    std::string tag_name;
    std::string title;
    std::string notes;
    std::string html_url;
    std::string asset_name;
    std::string asset_url;
    uint64_t    asset_size = 0;
};

struct Snapshot {
    State       state = UPD_IDLE;
    ReleaseInfo rel;
    std::string local_version;
    std::string status_text;
    std::string error_text;
    uint64_t    bytes_done = 0;
    uint64_t    bytes_total = 0;
    bool        should_open_modal = false;
    bool        should_quit_to_apply = false;
};

struct Runtime {
    std::mutex  mu;
    State       state = UPD_IDLE;
    ReleaseInfo rel;
    std::string local_version;
    std::string status_text;
    std::string error_text;
    uint64_t    bytes_done = 0;
    uint64_t    bytes_total = 0;
    bool        auto_checked = false;
    bool        should_open_modal = false;
    bool        should_quit_to_apply = false;
    std::atomic<bool> busy{false};
};

static inline Runtime& rt() {
    static Runtime r;
    return r;
}

static inline std::string to_lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// Parse major.minor.patch integers from strings like "v0.1.4" or "0.1.3-beta".
static inline bool parse_semver(const std::string& raw, int out[3], bool* has_suffix = nullptr) {
    out[0] = out[1] = out[2] = 0;
    if (has_suffix) *has_suffix = false;
    size_t i = 0;
    while (i < raw.size() && (raw[i] == 'v' || raw[i] == 'V' || raw[i] == ' '))
        ++i;
    if (i >= raw.size() || !std::isdigit((unsigned char)raw[i]))
        return false;
    int part = 0;
    while (i < raw.size() && part < 3) {
        if (!std::isdigit((unsigned char)raw[i])) break;
        int val = 0;
        while (i < raw.size() && std::isdigit((unsigned char)raw[i])) {
            val = val * 10 + (raw[i] - '0');
            ++i;
        }
        out[part++] = val;
        if (i < raw.size() && raw[i] == '.') ++i;
        else break;
    }
    if (has_suffix && i < raw.size() && (raw[i] == '-' || raw[i] == '+'))
        *has_suffix = true;
    return true;
}

static inline bool is_newer_version(const std::string& remote, const std::string& local,
                                    bool allow_override) {
    int r[3] = {0}, l[3] = {0};
    bool rsuf = false, lsuf = false;
    if (!parse_semver(remote, r, &rsuf)) return false;
    if (!parse_semver(local, l, &lsuf)) {
        // Non-numeric "dev" builds only show update modal during explicit override tests.
        return allow_override;
    }
    for (int k = 0; k < 3; ++k) {
        if (r[k] != l[k]) return r[k] > l[k];
    }
    // Same numeric core: release 0.1.4 > 0.1.4-dev
    if (!rsuf && lsuf) return true;
    return false;
}

// Minimal JSON string extractor for GitHub Releases payloads.
static inline bool parse_json_string_at(const std::string& json, size_t pos,
                                        std::string& out, size_t* next_pos) {
    if (pos >= json.size() || json[pos] != '"') return false;
    ++pos;
    out.clear();
    while (pos < json.size()) {
        char c = json[pos++];
        if (c == '"') {
            if (next_pos) *next_pos = pos;
            return true;
        }
        if (c == '\\' && pos < json.size()) {
            char e = json[pos++];
            switch (e) {
                case '"':  out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/':  out.push_back('/'); break;
                case 'n':  out.push_back('\n'); break;
                case 'r':  break;
                case 't':  out.push_back('\t'); break;
                case 'u':
                    if (pos + 4 <= json.size()) {
                        unsigned code = 0;
                        if (std::sscanf(json.substr(pos, 4).c_str(), "%4x", &code) == 1) {
                            if (code < 0x80) out.push_back((char)code);
                            else if (code < 0x800) {
                                out.push_back((char)(0xC0 | (code >> 6)));
                                out.push_back((char)(0x80 | (code & 0x3F)));
                            } else {
                                out.push_back((char)(0xE0 | (code >> 12)));
                                out.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
                                out.push_back((char)(0x80 | (code & 0x3F)));
                            }
                        }
                        pos += 4;
                    }
                    break;
                default: out.push_back(e); break;
            }
        } else {
            out.push_back(c);
        }
    }
    return false;
}

static inline size_t skip_ws(const std::string& s, size_t i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n'))
        ++i;
    return i;
}

// Find a top-level (depth == 1) key's string or uint64 value in a JSON object.
static inline bool find_json_field_in_slice(const std::string& json, size_t start, size_t end,
                                            const char* key, std::string* str_out, uint64_t* num_out) {
    int depth = 0;
    bool in_str = false;
    const size_t klen = std::strlen(key);
    for (size_t i = start; i < end; ++i) {
        char c = json[i];
        if (in_str) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') in_str = false;
            continue;
        }
        if (c == '{' || c == '[') { ++depth; continue; }
        if (c == '}' || c == ']') { --depth; continue; }
        if (c == '"') {
            if (depth == 1) {
                std::string k;
                size_t after_k = i;
                if (parse_json_string_at(json, i, k, &after_k)) {
                    size_t p = skip_ws(json, after_k);
                    if (p < end && json[p] == ':') {
                        p = skip_ws(json, p + 1);
                        if (k.size() == klen && k == key) {
                            if (str_out && p < end && json[p] == '"') {
                                return parse_json_string_at(json, p, *str_out, nullptr);
                            }
                            if (num_out && p < end && std::isdigit((unsigned char)json[p])) {
                                *num_out = (uint64_t)std::strtoull(json.c_str() + p, nullptr, 10);
                                return true;
                            }
                            return false;
                        }
                    }
                    i = after_k - 1;
                    continue;
                }
            }
            in_str = true;
        }
    }
    return false;
}

static inline bool parse_github_release_json(const std::string& json, ReleaseInfo& out) {
    out = ReleaseInfo{};
    size_t obj_start = json.find('{');
    if (obj_start == std::string::npos) return false;
    size_t obj_end = json.size();

    if (!find_json_field_in_slice(json, obj_start, obj_end, "tag_name", &out.tag_name, nullptr))
        return false;
    find_json_field_in_slice(json, obj_start, obj_end, "name", &out.title, nullptr);
    find_json_field_in_slice(json, obj_start, obj_end, "body", &out.notes, nullptr);
    find_json_field_in_slice(json, obj_start, obj_end, "html_url", &out.html_url, nullptr);

    // Find top-level "assets": [ ... ]
    int depth = 0;
    bool in_str = false;
    size_t assets_arr = std::string::npos;
    for (size_t i = obj_start; i < obj_end; ++i) {
        char c = json[i];
        if (in_str) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') in_str = false;
            continue;
        }
        if (c == '{' || c == '[') { ++depth; continue; }
        if (c == '}' || c == ']') { --depth; continue; }
        if (c == '"') {
            if (depth == 1) {
                std::string k;
                size_t after_k = i;
                if (parse_json_string_at(json, i, k, &after_k)) {
                    size_t p = skip_ws(json, after_k);
                    if (p < obj_end && json[p] == ':' && k == "assets") {
                        p = skip_ws(json, p + 1);
                        if (p < obj_end && json[p] == '[') {
                            assets_arr = p;
                            break;
                        }
                    }
                    i = after_k - 1;
                    continue;
                }
            }
            in_str = true;
        }
    }

    if (assets_arr != std::string::npos) {
        int best_score = -1;
        size_t i = assets_arr + 1;
        while (i < obj_end) {
            i = skip_ws(json, i);
            if (i >= obj_end || json[i] == ']') break;
            if (json[i] == '{') {
                size_t a_start = i;
                int ad = 0;
                bool astr = false;
                size_t a_end = a_start;
                for (; a_end < obj_end; ++a_end) {
                    char c = json[a_end];
                    if (astr) {
                        if (c == '\\') { ++a_end; continue; }
                        if (c == '"') astr = false;
                        continue;
                    }
                    if (c == '"') { astr = true; continue; }
                    if (c == '{') ++ad;
                    else if (c == '}' && --ad == 0) { ++a_end; break; }
                }
                std::string aname, aurl;
                uint64_t asize = 0;
                find_json_field_in_slice(json, a_start, a_end, "name", &aname, nullptr);
                find_json_field_in_slice(json, a_start, a_end, "browser_download_url", &aurl, nullptr);
                find_json_field_in_slice(json, a_start, a_end, "size", nullptr, &asize);

                std::string low = to_lower(aname);
                if (!aurl.empty() && low.size() >= 4 &&
                    low.rfind(".zip") == low.size() - 4) {
                    int score = 1;
                    if (low.find("agrabah") != std::string::npos) score += 2;
                    if (low.find("win") != std::string::npos ||
                        low.find("portable") != std::string::npos) score += 4;
                    if (low.find("source") != std::string::npos ||
                        low.find("setup") != std::string::npos) score -= 2;
                    if (score > best_score) {
                        best_score = score;
                        out.asset_name = aname;
                        out.asset_url = aurl;
                        out.asset_size = asize;
                    }
                }
                i = a_end;
            } else {
                ++i;
            }
        }
    }
    return true;
}

#if defined(_WIN32)

struct WinHttpApi {
    HMODULE mod = nullptr;
    decltype(&WinHttpOpen) pOpen = nullptr;
    decltype(&WinHttpCloseHandle) pClose = nullptr;
    decltype(&WinHttpCrackUrl) pCrackUrl = nullptr;
    decltype(&WinHttpConnect) pConnect = nullptr;
    decltype(&WinHttpOpenRequest) pOpenRequest = nullptr;
    decltype(&WinHttpSetOption) pSetOption = nullptr;
    decltype(&WinHttpSendRequest) pSendRequest = nullptr;
    decltype(&WinHttpReceiveResponse) pReceiveResponse = nullptr;
    decltype(&WinHttpQueryHeaders) pQueryHeaders = nullptr;
    decltype(&WinHttpQueryDataAvailable) pQueryDataAvailable = nullptr;
    decltype(&WinHttpReadData) pReadData = nullptr;
    decltype(&WinHttpSetTimeouts) pSetTimeouts = nullptr;
    bool ok = false;

    static WinHttpApi& get() {
        static WinHttpApi api = []() {
            WinHttpApi a;
            a.mod = LoadLibraryA("winhttp.dll");
            if (!a.mod) return a;
            a.pOpen = (decltype(&WinHttpOpen))GetProcAddress(a.mod, "WinHttpOpen");
            a.pClose = (decltype(&WinHttpCloseHandle))GetProcAddress(a.mod, "WinHttpCloseHandle");
            a.pCrackUrl = (decltype(&WinHttpCrackUrl))GetProcAddress(a.mod, "WinHttpCrackUrl");
            a.pConnect = (decltype(&WinHttpConnect))GetProcAddress(a.mod, "WinHttpConnect");
            a.pOpenRequest = (decltype(&WinHttpOpenRequest))GetProcAddress(a.mod, "WinHttpOpenRequest");
            a.pSetOption = (decltype(&WinHttpSetOption))GetProcAddress(a.mod, "WinHttpSetOption");
            a.pSendRequest = (decltype(&WinHttpSendRequest))GetProcAddress(a.mod, "WinHttpSendRequest");
            a.pReceiveResponse = (decltype(&WinHttpReceiveResponse))GetProcAddress(a.mod, "WinHttpReceiveResponse");
            a.pQueryHeaders = (decltype(&WinHttpQueryHeaders))GetProcAddress(a.mod, "WinHttpQueryHeaders");
            a.pQueryDataAvailable = (decltype(&WinHttpQueryDataAvailable))GetProcAddress(a.mod, "WinHttpQueryDataAvailable");
            a.pReadData = (decltype(&WinHttpReadData))GetProcAddress(a.mod, "WinHttpReadData");
            a.pSetTimeouts = (decltype(&WinHttpSetTimeouts))GetProcAddress(a.mod, "WinHttpSetTimeouts");
            a.ok = a.pOpen && a.pClose && a.pCrackUrl && a.pConnect &&
                   a.pOpenRequest && a.pSetOption && a.pSendRequest &&
                   a.pReceiveResponse && a.pQueryHeaders && a.pQueryDataAvailable &&
                   a.pReadData;
            return a;
        }();
        return api;
    }
};

static inline std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

static inline std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

static inline bool winhttp_fetch(const std::string& url,
                                 std::string* body_out,
                                 const std::filesystem::path* file_out,
                                 int* http_status,
                                 std::string& err_out,
                                 void (*progress_cb)(uint64_t done, uint64_t total)) {
    WinHttpApi& api = WinHttpApi::get();
    if (!api.ok) {
        err_out = "Windows WinHTTP library is unavailable.";
        return false;
    }

    std::wstring wurl = utf8_to_wide(url);
    URL_COMPONENTS uc;
    std::memset(&uc, 0, sizeof(uc));
    wchar_t host[256] = {0};
    wchar_t path[2048] = {0};
    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 2048;

    if (!api.pCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &uc)) {
        err_out = "Invalid release URL.";
        return false;
    }

    HINTERNET hSession = api.pOpen(L"AgrabahPort-Updater/1.0",
                                   WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                   WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        err_out = "Could not open HTTP session.";
        return false;
    }
    if (api.pSetTimeouts)
        api.pSetTimeouts(hSession, 5000, 7000, 10000, 15000);

    HINTERNET hConn = api.pConnect(hSession, host, uc.nPort, 0);
    if (!hConn) {
        api.pClose(hSession);
        err_out = "Could not connect to update server.";
        return false;
    }

    const DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = api.pOpenRequest(hConn, L"GET", path[0] ? path : L"/",
                                      nullptr, WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) {
        api.pClose(hConn);
        api.pClose(hSession);
        err_out = "Could not open HTTP request.";
        return false;
    }

    DWORD redir = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    api.pSetOption(hReq, WINHTTP_OPTION_REDIRECT_POLICY, &redir, sizeof(redir));

    const wchar_t* hdrs = L"Accept: application/vnd.github+json\r\n";
    if (!api.pSendRequest(hReq, hdrs, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !api.pReceiveResponse(hReq, nullptr)) {
        api.pClose(hReq);
        api.pClose(hConn);
        api.pClose(hSession);
        err_out = "Could not reach GitHub (offline or timed out).";
        return false;
    }

    DWORD status = 0, st_sz = sizeof(status);
    api.pQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX, &status, &st_sz, WINHTTP_NO_HEADER_INDEX);
    if (http_status) *http_status = (int)status;

    DWORD content_len = 0, cl_sz = sizeof(content_len);
    api.pQueryHeaders(hReq, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX, &content_len, &cl_sz, WINHTTP_NO_HEADER_INDEX);

    if (status < 200 || status >= 300) {
        api.pClose(hReq);
        api.pClose(hConn);
        api.pClose(hSession);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Server returned HTTP %lu.", (unsigned long)status);
        err_out = buf;
        return false;
    }

    HANDLE hFile = INVALID_HANDLE_VALUE;
    if (file_out) {
        const std::wstring wpath = file_out->wstring();
        hFile = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD gle = GetLastError();
            api.pClose(hReq);
            api.pClose(hConn);
            api.pClose(hSession);
            err_out = "Could not write staging file [" + wide_to_utf8(wpath) +
                      "] (winerr=" + std::to_string(gle) + ").";
            return false;
        }
    }
    if (body_out) body_out->clear();

    uint64_t total_read = 0;
    std::vector<char> chunk(16384);
    for (;;) {
        DWORD avail = 0;
        if (!api.pQueryDataAvailable(hReq, &avail)) break;
        if (avail == 0) break;
        if (avail > chunk.size()) chunk.resize(avail);
        DWORD read = 0;
        if (!api.pReadData(hReq, chunk.data(), avail, &read) || read == 0) break;
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(hFile, chunk.data(), read, &written, nullptr);
        }
        if (body_out) body_out->append(chunk.data(), read);
        total_read += read;
        if (progress_cb) progress_cb(total_read, (uint64_t)content_len);
    }

    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    api.pClose(hReq);
    api.pClose(hConn);
    api.pClose(hSession);
    return true;
}

static inline std::filesystem::path get_exe_path() {
    wchar_t buf[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return n > 0 ? std::filesystem::path(std::wstring(buf, n)) : std::filesystem::path();
}

static inline bool run_hidden_command(const std::wstring& cmd_line, DWORD* exit_code_out) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    std::memset(&si, 0, sizeof(si));
    std::memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    std::wstring mutable_cmd = cmd_line;
    if (!CreateProcessW(nullptr, &mutable_cmd[0], nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 120000);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    if (exit_code_out) *exit_code_out = code;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code == 0;
}

// Strip any user-owned files from the extracted staging directory before applying,
// so robocopy can never overwrite saves, mods, configs, keybinds, or ROMs even
// if a release zip shipped defaults.
static inline void scrub_protected_user_files(const std::filesystem::path& src_root) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const char* protected_entries[] = {
        "saves", "backups", "mods", "states", "config.ini", "config.local.ini",
        "keybinds.ini", "rom.cfg", "portable.txt", "AgrabahPort_window.cfg"
    };
    for (const char* name : protected_entries) {
        fs::path p = src_root / fs::u8path(name);
        if (fs::exists(p, ec))
            fs::remove_all(p, ec);
    }
    for (fs::recursive_directory_iterator it(src_root, ec), end; !ec && it != end; ) {
        fs::path p = it->path();
        ++it;
        if (!fs::is_regular_file(p, ec)) continue;
        std::string ext = to_lower(p.extension().u8string());
        if (ext == ".sfc" || ext == ".smc" || ext == ".srm" || ext == ".sav")
            fs::remove(p, ec);
    }
}

static inline bool find_extracted_root(const std::filesystem::path& extract_dir,
                                       const std::string& exe_name,
                                       std::filesystem::path& out_root) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path wname = fs::u8path(exe_name);
    if (fs::is_regular_file(extract_dir / wname, ec)) {
        out_root = extract_dir;
        return true;
    }
    for (fs::directory_iterator it(extract_dir, ec); !ec && it != fs::directory_iterator(); ++it) {
        if (it->is_directory(ec) && fs::is_regular_file(it->path() / wname, ec)) {
            out_root = it->path();
            return true;
        }
    }
    return false;
}

#endif // _WIN32

static inline void cleanup_residual_staging() {
#if defined(_WIN32)
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path exe = get_exe_path();
    if (exe.empty()) return;
    fs::path staging = exe.parent_path() / L"_update_staging";
    if (fs::exists(staging, ec))
        fs::remove_all(staging, ec);
#endif
}

static inline void check_async(const std::string& local_version, bool open_modal_if_found) {
    Runtime& r = rt();
    bool expected = false;
    if (!r.busy.compare_exchange_strong(expected, true))
        return;

    {
        std::lock_guard<std::mutex> lk(r.mu);
        r.local_version = local_version;
        r.state = UPD_CHECKING;
        r.status_text = "Checking GitHub Releases...";
        r.error_text.clear();
        r.bytes_done = 0;
        r.bytes_total = 0;
    }

    std::thread([local_version, open_modal_if_found]() {
        Runtime& r = rt();
#if defined(_WIN32)
        const char* override_url = std::getenv("AGRABAH_UPDATE_URL");
        const bool has_override = override_url && override_url[0] != '\0';
        const std::string url = has_override
            ? std::string(override_url)
            : std::string("https://api.github.com/repos/testsenvergonha-sys/AgrabahPort/releases/latest");

        std::string body, err;
        int status = 0;
        bool fetched = winhttp_fetch(url, &body, nullptr, &status, err, nullptr);
        if (!fetched) {
            std::lock_guard<std::mutex> lk(r.mu);
            if (status == 404) {
                r.state = UPD_NO_RELEASE;
                r.status_text = "No public GitHub release found yet.";
            } else {
                r.state = UPD_ERROR;
                r.status_text = "Could not check for updates.";
                r.error_text = err;
            }
            r.busy.store(false);
            return;
        }

        ReleaseInfo rel;
        if (!parse_github_release_json(body, rel)) {
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_ERROR;
            r.status_text = "Invalid response from GitHub.";
            r.error_text = "Release metadata could not be parsed.";
            r.busy.store(false);
            return;
        }

        std::lock_guard<std::mutex> lk(r.mu);
        r.rel = rel;
        if (is_newer_version(rel.tag_name, local_version, has_override)) {
            r.state = UPD_AVAILABLE;
            r.status_text = "Update available: " + rel.tag_name;
            if (open_modal_if_found)
                r.should_open_modal = true;
        } else {
            r.state = UPD_UP_TO_DATE;
            r.status_text = "AgrabahPort is up to date.";
        }
#else
        (void)open_modal_if_found;
        std::lock_guard<std::mutex> lk(r.mu);
        r.state = UPD_UP_TO_DATE;
        r.status_text = "Auto-update is currently Windows-only.";
#endif
        r.busy.store(false);
    }).detach();
}

static inline void check_once_on_boot(const std::string& local_version) {
    cleanup_residual_staging();
    Runtime& r = rt();
    {
        std::lock_guard<std::mutex> lk(r.mu);
        if (r.auto_checked) return;
        r.auto_checked = true;
    }
    check_async(local_version, true);
}

static inline void start_install_async() {
#if defined(_WIN32)
    Runtime& r = rt();
    bool expected = false;
    if (!r.busy.compare_exchange_strong(expected, true))
        return;

    ReleaseInfo rel;
    std::string cur_ver;
    {
        std::lock_guard<std::mutex> lk(r.mu);
        rel = r.rel;
        cur_ver = r.local_version;
        r.state = UPD_DOWNLOADING;
        r.status_text = "Downloading update package...";
        r.error_text.clear();
        r.bytes_done = 0;
        r.bytes_total = rel.asset_size;
    }

    std::thread([rel, cur_ver]() {
        namespace fs = std::filesystem;
        Runtime& r = rt();

        if (rel.asset_url.empty()) {
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_ERROR;
            r.error_text = "No .zip package was attached to this release.";
            r.busy.store(false);
            return;
        }

        fs::path exe_path = get_exe_path();
        if (exe_path.empty()) {
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_ERROR;
            r.error_text = "Could not locate current AgrabahPort executable.";
            r.busy.store(false);
            return;
        }

        const fs::path exe_dir = exe_path.parent_path();
        const std::string exe_name = exe_path.filename().u8string();
        wchar_t tmp_base[MAX_PATH] = {0};
        DWORD tmp_len = GetTempPathW(MAX_PATH, tmp_base);
        const fs::path staging = (tmp_len > 0 && tmp_len < MAX_PATH)
            ? (fs::path(std::wstring(tmp_base, tmp_len)) /
               fs::u8path("AgrabahPort_upd_" + std::to_string((unsigned long)GetCurrentProcessId())))
            : (exe_dir / L"_update_staging");
        const fs::path zip_path = staging / L"package.zip";
        const fs::path extract_dir = staging / L"extracted";

        std::error_code ec;
        fs::remove_all(staging, ec);
        if ((!CreateDirectoryW(staging.wstring().c_str(), nullptr) &&
             GetLastError() != ERROR_ALREADY_EXISTS) ||
            (!CreateDirectoryW(extract_dir.wstring().c_str(), nullptr) &&
             GetLastError() != ERROR_ALREADY_EXISTS)) {
            DWORD gle = GetLastError();
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_ERROR;
            r.error_text = "Could not create staging dir [" + wide_to_utf8(staging.wstring()) +
                           "] (winerr=" + std::to_string(gle) + ").";
            r.busy.store(false);
            return;
        }

        std::string err;
        auto on_prog = [](uint64_t done, uint64_t total) {
            Runtime& rt_ref = rt();
            std::lock_guard<std::mutex> lk(rt_ref.mu);
            rt_ref.bytes_done = done;
            if (total > 0) rt_ref.bytes_total = total;
        };

        if (!winhttp_fetch(rel.asset_url, nullptr, &zip_path, nullptr, err, on_prog)) {
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_ERROR;
            r.error_text = "Download failed: " + err;
            r.busy.store(false);
            return;
        }

        {
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_EXTRACTING;
            r.status_text = "Extracting and verifying package...";
        }

        // Extract with built-in tar.exe first (fast, Windows 10/11), fallback to PowerShell Expand-Archive.
        std::wstring tar_cmd = L"tar.exe -xf \"" + zip_path.wstring() +
                               L"\" -C \"" + extract_dir.wstring() + L"\"";
        DWORD code = 1;
        bool extracted = run_hidden_command(tar_cmd, &code);
        if (!extracted) {
            std::wstring ps_cmd =
                L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "
                L"\"Expand-Archive -LiteralPath '" + zip_path.wstring() +
                L"' -DestinationPath '" + extract_dir.wstring() + L"' -Force\"";
            extracted = run_hidden_command(ps_cmd, &code);
        }

        fs::path src_root;
        if (!extracted || !find_extracted_root(extract_dir, exe_name, src_root)) {
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_ERROR;
            r.error_text = "Downloaded archive did not contain " + exe_name + ".";
            r.busy.store(false);
            return;
        }

        // Protect user data twice:
        // 1. Back up saves/ into backups/pre_update_<version>/
        // 2. Scrub any user-data names from src_root so xcopy cannot overwrite them.
        fs::path saves_dir = exe_dir / L"saves";
        if (fs::exists(saves_dir, ec)) {
            std::string tag = cur_ver.empty() ? "old" : cur_ver;
            fs::path bak_dir = exe_dir / L"backups" / fs::u8path("pre_update_" + tag) / L"saves";
            fs::create_directories(bak_dir, ec);
            fs::copy(saves_dir, bak_dir,
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing, ec);
        }
        scrub_protected_user_files(src_root);

        // Emit update helper batch file next to staging.
        const fs::path cmd_path = staging / L"apply_update.cmd";
        {
            FILE* cf = _wfopen(cmd_path.wstring().c_str(), L"wb");
            if (!cf) {
                std::lock_guard<std::mutex> lk(r.mu);
                r.state = UPD_ERROR;
                r.error_text = "Could not create update helper script.";
                r.busy.store(false);
                return;
            }
            const DWORD pid = GetCurrentProcessId();
            const std::string s_exe_dir = wide_to_utf8(exe_dir.wstring());
            const std::string s_src_root = wide_to_utf8(src_root.wstring());
            const std::string s_exe_path = wide_to_utf8(exe_path.wstring());
            const std::string s_staging = wide_to_utf8(staging.wstring());

            std::fprintf(cf,
                "@echo off\r\n"
                "chcp 65001 >nul\r\n"
                "setlocal\r\n"
                ":wait_exit\r\n"
                "tasklist /FI \"PID eq %lu\" 2>nul | find \"%lu\" >nul\r\n"
                "if not errorlevel 1 (\r\n"
                "  ping -n 2 127.0.0.1 >nul\r\n"
                "  goto wait_exit\r\n"
                ")\r\n"
                "if exist \"%s\" copy /Y \"%s\" \"%s.old\" >nul\r\n"
                "robocopy \"%s\" \"%s\" /E /R:3 /W:1 /NFL /NDL /NJH /NJS /NC /NS /NP >nul\r\n"
                "start \"\" \"%s\" --launcher\r\n"
                "rmdir /S /Q \"%s\" 2>nul\r\n",
                (unsigned long)pid, (unsigned long)pid,
                s_exe_path.c_str(), s_exe_path.c_str(), s_exe_path.c_str(),
                s_src_root.c_str(), s_exe_dir.c_str(),
                s_exe_path.c_str(),
                s_staging.c_str());
            std::fclose(cf);
        }

        // Launch detached apply_update.cmd so it finishes replacing files once we exit.
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        std::memset(&si, 0, sizeof(si));
        std::memset(&pi, 0, sizeof(pi));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::wstring run_cmd = L"cmd.exe /c \"" + cmd_path.wstring() + L"\"";
        if (!CreateProcessW(nullptr, &run_cmd[0], nullptr, nullptr, FALSE,
                            CREATE_NO_WINDOW,
                            nullptr, exe_dir.wstring().c_str(), &si, &pi)) {
            std::lock_guard<std::mutex> lk(r.mu);
            r.state = UPD_ERROR;
            r.error_text = "Could not launch update installer script.";
            r.busy.store(false);
            return;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        std::lock_guard<std::mutex> lk(r.mu);
        r.state = UPD_READY_APPLY;
        r.status_text = "Applying update and restarting...";
        r.should_quit_to_apply = true;
        r.busy.store(false);
    }).detach();
#endif
}

static inline void open_url_in_browser(const std::string& url) {
#if defined(_WIN32)
    if (!url.empty())
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    (void)url;
#endif
}

static inline Snapshot snapshot() {
    Runtime& r = rt();
    std::lock_guard<std::mutex> lk(r.mu);
    Snapshot s;
    s.state = r.state;
    s.rel = r.rel;
    s.local_version = r.local_version;
    s.status_text = r.status_text;
    s.error_text = r.error_text;
    s.bytes_done = r.bytes_done;
    s.bytes_total = r.bytes_total;
    s.should_open_modal = r.should_open_modal;
    s.should_quit_to_apply = r.should_quit_to_apply;
    r.should_open_modal = false;
    return s;
}

} // namespace agrabah_upd

#endif // AGRABAH_UPDATER_H
