<p align="center">
  <img src="assets/icon_source.png" alt="AgrabahPort" width="160">
</p>

<h1 align="center">AgrabahPort</h1>

<p align="center">
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases"><img src="https://img.shields.io/github/downloads/testsenvergonha-sys/AgrabahPort/total?style=flat-square&amp;label=downloads&amp;color=4B2E83" alt="downloads"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases/latest"><img src="https://img.shields.io/github/v/release/testsenvergonha-sys/AgrabahPort?display_name=release&amp;label=version&amp;style=flat-square&amp;color=4B2E83" alt="version"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/stargazers"><img src="https://img.shields.io/github/stars/testsenvergonha-sys/AgrabahPort?label=stars&amp;style=flat-square&amp;color=4B2E83" alt="stars"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/issues"><img src="https://img.shields.io/github/issues/testsenvergonha-sys/AgrabahPort?style=flat-square&amp;color=4B2E83" alt="issues"></a>
</p>

<p align="center">
  <a href="https://recomplauncher.com"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Get it on RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="48"></picture></a>
</p>

<p align="center">
  <strong>English</strong> | <a href="README.pt-BR.md">Português (Brasil)</a>
</p>

**AgrabahPort** is an unofficial native PC port of **Disney's Aladdin** (Super Nintendo / SNES, 1993 — developed by Capcom) for Windows, Linux, and macOS, built via static 65816-to-C recompilation on top of [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp) and [recomp-ui](https://github.com/mstan/recomp-ui).

> **No game files or ROMs are included.** You must supply a legal dump from your own SNES cartridge (tested: **Disney's Aladdin (USA)**, headerless `.sfc` — CRC32 `124d8e4d`, SHA-256 `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e`).
> All recompiled C code (`src/gen/`) is generated locally on your machine from your own ROM and is never committed or distributed in this repository or in any release.
> This project is not affiliated with or endorsed by Capcom, Disney, or Nintendo. *Aladdin* and related marks are trademarks of their respective owners.

---

## Features

- **Native PC Execution (Static Recompilation):** Translates the original SNES 65816 CPU code ahead-of-time into C and builds a standalone native executable with **SDL3** and **OpenGL**.
- **Clean Windows GUI Build:** Standalone static binary (`AgrabahPort.exe`) with no external MinGW/SDL DLLs and no background command prompt window.
- **Pre-Boot & In-Game Launcher (`ESC`):** Dear ImGui launcher with ROM verification, video/audio/controller configuration, mods support, save states, and instant `RESUME` / `QUIT GAME` menu mid-game.
- **Built-In Live Log Viewer:** Captures `stdout`/`stderr` in real time directly inside the launcher (with color-coded log lines, auto-scroll, copy, and clear).
- **OpenGL & GLSL Shaders:** Includes CC0 shader presets (`sharp.glslp`, `crt-soft.glslp`, `warm-composite.glslp`, `lcd-grid.glslp`), VSync, Mode 7 HD scaling (`8x` with supersampling), and sprite-per-scanline limit removal.
- **Experimental Widescreen Support:** Widescreen and Display Aspect options available in the launcher Display settings.
- **Save States & Rewind:** Visual save-state browser (`F11`), quick slots (`F1`–`F10`), and real-time rewind (`F12`).
- **Original 16-bit App Icon:** Custom original pixel-art icon embedded directly in the Windows executable.

---

## Supported ROM (Bring Your Own ROM)

Verify that your legally dumped **Disney's Aladdin (USA)** SNES ROM matches these headerless digests. If your dump is an `.smc` file with a 512-byte copier header (`1,311,232` bytes), strip the first 512 bytes to obtain a clean `.sfc` (`1,310,720` bytes) before running `tools/regen.sh`.

| Field | Expected Value |
|---|---|
| Game | **Disney's Aladdin (USA)** — SNES (Capcom, 1993) |
| Region | USA (North America) |
| Mapping | LoROM |
| Coprocessor | None |
| Headerless Size | `1,310,720` bytes (1.25 MiB) |
| CRC32 | `124d8e4d` |
| SHA-256 | `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e` |

---

## Building From Source

### Prerequisites
- **Git** (with Git Bash on Windows)
- **Python 3.11+**
- **Rust (`rustup`, 1.85+)** — builds the `snesrecomp` static analyzer
- **C/C++ Compiler + CMake + Ninja + SDL3** — on Windows, you can use the portable [RetroPorting-Toolchains (`cmake-clang-v1-windows-x64`)](https://github.com/RetroPortingToolKit/RetroPorting-Toolchains/releases) pack (bundles Clang, CMake, Ninja, zlib, and SDL3) paired with Rust `stable-x86_64-pc-windows-gnullvm`, or Visual Studio 2022 Build Tools.

### Steps

```sh
# 1. Clone with submodules
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

# 2. Apply the integrated Live Log patch to recomp-ui
sh tools/apply_patches.sh

# 3. Generate recompiled C code locally from YOUR legal Aladdin (USA) ROM (.sfc)
bash tools/regen.sh --rom /path/to/Aladdin.sfc

# 4. Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

> **Windows (Clang/MinGW + Rust `gnullvm`) note:** When using the portable `cmake-clang-v1` toolchain pack instead of MSVC, set `RUSTUP_TOOLCHAIN=stable-x86_64-pc-windows-gnullvm` before running `tools/regen.sh` and pass `-DSDL3_DIR=<pack>/deps/lib/cmake/SDL3` to CMake.

---

## Default Shortcuts

| Shortcut | Action |
|---|---|
| `ESC` | Open In-Game Menu (Resume / Settings / Live Log / Quit Game) |
| `Alt + Enter` | Toggle Fullscreen / Windowed |
| `F11` | Open Save-State Browser |
| `Shift + F1..F10` / `F1..F10` | Quick Save / Quick Load Slots |
| `F12` | Rewind |
| `Tab` | Turbo / Fast-Forward |
| `Ctrl + R` | Reset |

---

## Credits & Licenses

- **Static Recompiler Framework:** [RetroPortingToolKit/snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) (see `snesrecomp/LICENSE` and `snesrecomp/THIRD_PARTY_ATTRIBUTION.md`)
- **Launcher UI:** [mstan/recomp-ui](https://github.com/mstan/recomp-ui)
- **Project Source:** see [LICENSE](LICENSE). No copyrighted game data, ROMs, or official artwork are included or licensed here.
