#include "live_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#define LIVE_LOG_MAX_LINES 500
#define LIVE_LOG_LINE_CAP  512

typedef struct {
    char lines[LIVE_LOG_MAX_LINES][LIVE_LOG_LINE_CAP];
    size_t head;
    size_t count;
    uint64_t seq;
    char pending[LIVE_LOG_LINE_CAP];
    size_t pending_len;
} LiveLogRing;

static LiveLogRing g_ring;
static int g_initialized = 0;

#ifdef _WIN32
static CRITICAL_SECTION g_lock;
static int g_pipe_read_fd = -1;
static int g_orig_err_fd = -1;
static int g_mirror_orig = 0;

static void lock_ring(void)   { EnterCriticalSection(&g_lock); }
static void unlock_ring(void) { LeaveCriticalSection(&g_lock); }
#else
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_pipe_read_fd = -1;
static int g_orig_err_fd = -1;
static int g_mirror_orig = 0;

static void lock_ring(void)   { pthread_mutex_lock(&g_lock); }
static void unlock_ring(void) { pthread_mutex_unlock(&g_lock); }
#endif

static void push_line_locked(const char *line, size_t len)
{
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n'))
        len--;
    if (len >= LIVE_LOG_LINE_CAP)
        len = LIVE_LOG_LINE_CAP - 1;

    size_t slot;
    if (g_ring.count < LIVE_LOG_MAX_LINES) {
        slot = (g_ring.head + g_ring.count) % LIVE_LOG_MAX_LINES;
        g_ring.count++;
    } else {
        slot = g_ring.head;
        g_ring.head = (g_ring.head + 1) % LIVE_LOG_MAX_LINES;
    }

    memcpy(g_ring.lines[slot], line, len);
    g_ring.lines[slot][len] = '\0';
    g_ring.seq++;
}

static void ingest_bytes(const char *buf, size_t n)
{
    if (n == 0) return;

#ifdef _WIN32
    if (g_mirror_orig && g_orig_err_fd >= 0) {
        (void)_write(g_orig_err_fd, buf, (unsigned int)n);
    }
#else
    if (g_mirror_orig && g_orig_err_fd >= 0) {
        (void)write(g_orig_err_fd, buf, n);
    }
#endif

    lock_ring();
    for (size_t i = 0; i < n; ++i) {
        char c = buf[i];
        if (c == '\r') continue;
        if (c == '\n') {
            g_ring.pending[g_ring.pending_len] = '\0';
            push_line_locked(g_ring.pending, g_ring.pending_len);
            g_ring.pending_len = 0;
        } else {
            if (g_ring.pending_len + 1 < LIVE_LOG_LINE_CAP) {
                g_ring.pending[g_ring.pending_len++] = c;
            } else {
                g_ring.pending[g_ring.pending_len] = '\0';
                push_line_locked(g_ring.pending, g_ring.pending_len);
                g_ring.pending_len = 0;
                g_ring.pending[g_ring.pending_len++] = c;
            }
        }
    }
    unlock_ring();
}

#ifdef _WIN32
static unsigned __stdcall reader_thread_proc(void *arg)
{
    (void)arg;
    char buf[2048];
    for (;;) {
        int r = _read(g_pipe_read_fd, buf, (unsigned int)sizeof(buf));
        if (r <= 0) break;
        ingest_bytes(buf, (size_t)r);
    }
    return 0;
}
#else
static void *reader_thread_proc(void *arg)
{
    (void)arg;
    char buf[2048];
    for (;;) {
        ssize_t r = read(g_pipe_read_fd, buf, sizeof(buf));
        if (r <= 0) break;
        ingest_bytes(buf, (size_t)r);
    }
    return NULL;
}
#endif

void aladdin_live_log_init(void)
{
    if (g_initialized) return;
    g_initialized = 1;

    memset(&g_ring, 0, sizeof(g_ring));

#ifdef _WIN32
    InitializeCriticalSection(&g_lock);

    /* Preserve external redirection (e.g. CLI tests redirecting to a file). */
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    DWORD errType = (hErr && hErr != INVALID_HANDLE_VALUE) ? GetFileType(hErr) : FILE_TYPE_UNKNOWN;
    if (errType == FILE_TYPE_DISK || errType == FILE_TYPE_PIPE) {
        int err_fd = _fileno(stderr);
        if (err_fd >= 0) {
            g_orig_err_fd = _dup(err_fd);
            if (g_orig_err_fd >= 0) g_mirror_orig = 1;
        }
    }

    int fds[2];
    if (_pipe(fds, 65536, _O_BINARY) != 0)
        return;

    g_pipe_read_fd = fds[0];
    int write_fd = fds[1];

    /* Under SUBSYSTEM:WINDOWS (GUI), UCRT starts with stdout/stderr bound to
     * _NO_CONSOLE_FILENO (-2). Re-opening them on NUL gives them real CRT file
     * descriptors before _dup2 redirects them onto our live pipe. */
    fflush(stdout);
    fflush(stderr);
    if (_fileno(stdout) < 0) (void)freopen("NUL", "w", stdout);
    if (_fileno(stderr) < 0) (void)freopen("NUL", "w", stderr);

    int out_fd = _fileno(stdout);
    int err_fd = _fileno(stderr);
    if (out_fd >= 0) _dup2(write_fd, out_fd);
    if (err_fd >= 0) _dup2(write_fd, err_fd);
    if (out_fd != 1) _dup2(write_fd, 1);
    if (err_fd != 2) _dup2(write_fd, 2);
    _close(write_fd);

    HANDLE hWrite = (HANDLE)_get_osfhandle(out_fd >= 0 ? out_fd : 1);
    if (hWrite && hWrite != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, hWrite);
        SetStdHandle(STD_ERROR_HANDLE, hWrite);
    }

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    uintptr_t th = _beginthreadex(NULL, 0, reader_thread_proc, NULL, 0, NULL);
    if (th) CloseHandle((HANDLE)th);
#else
    int fds[2];
    if (pipe(fds) != 0) return;
    g_orig_err_fd = dup(2);
    g_mirror_orig = (g_orig_err_fd >= 0);
    g_pipe_read_fd = fds[0];
    fflush(stdout);
    fflush(stderr);
    dup2(fds[1], 1);
    dup2(fds[1], 2);
    close(fds[1]);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    pthread_t th;
    if (pthread_create(&th, NULL, reader_thread_proc, NULL) == 0)
        pthread_detach(th);
#endif
}

size_t recomp_live_log_copy(char *dst, size_t cap, uint64_t *out_seq, size_t *out_lines)
{
    if (!g_initialized) {
        if (dst && cap > 0) dst[0] = '\0';
        if (out_seq) *out_seq = 0;
        if (out_lines) *out_lines = 0;
        return 0;
    }

    lock_ring();
    if (out_seq) *out_seq = g_ring.seq;
    if (out_lines) *out_lines = g_ring.count;

    size_t pos = 0;
    if (dst && cap > 0) {
        for (size_t i = 0; i < g_ring.count; ++i) {
            size_t idx = (g_ring.head + i) % LIVE_LOG_MAX_LINES;
            const char *line = g_ring.lines[idx];
            size_t len = strlen(line);
            if (pos + len + 2 >= cap) break;
            memcpy(dst + pos, line, len);
            pos += len;
            dst[pos++] = '\n';
        }
        if (g_ring.pending_len > 0 && pos + g_ring.pending_len + 1 < cap) {
            memcpy(dst + pos, g_ring.pending, g_ring.pending_len);
            pos += g_ring.pending_len;
        }
        dst[pos] = '\0';
    }
    unlock_ring();
    return pos;
}

void recomp_live_log_clear(void)
{
    if (!g_initialized) return;
    lock_ring();
    g_ring.head = 0;
    g_ring.count = 0;
    g_ring.pending_len = 0;
    g_ring.seq++;
    unlock_ring();
}

size_t recomp_live_log_line_count(void)
{
    if (!g_initialized) return 0;
    lock_ring();
    size_t c = g_ring.count;
    unlock_ring();
    return c;
}
