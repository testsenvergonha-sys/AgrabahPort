<p align="center">
  <img src="assets/icon_source.png" alt="AgrabahPort" width="136">
</p>

<h1 align="center">AgrabahPort</h1>

<p align="center">
  Native PC port of <em>Aladdin</em> (Super Nintendo, 1993) built via static recompilation.
</p>

<p align="center">
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases"><img src="https://img.shields.io/github/downloads/testsenvergonha-sys/AgrabahPort/total?style=flat-square&amp;label=downloads&amp;color=4B2E83" alt="Downloads"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases/latest"><img src="https://img.shields.io/github/v/release/testsenvergonha-sys/AgrabahPort?display_name=release&amp;label=release&amp;style=flat-square&amp;color=4B2E83" alt="Latest Release"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/stargazers"><img src="https://img.shields.io/github/stars/testsenvergonha-sys/AgrabahPort?style=flat-square&amp;color=4B2E83" alt="Stars"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/issues"><img src="https://img.shields.io/github/issues/testsenvergonha-sys/AgrabahPort?style=flat-square&amp;color=4B2E83" alt="Issues"></a>
</p>

<p align="center">
  <a href="https://recomplauncher.com"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Available on RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="44"></picture></a>
</p>

<p align="center">
  <strong>English</strong> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases">Releases</a> · <a href="CHANGELOG.md">Changelog</a>
</p>

---

## Overview

**AgrabahPort** statically recompiles the original 65816 assembly of the 1993 SNES release of *Aladdin* into native C executables for **Windows**, **Linux**, and **macOS**, powered by [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) and [recomp-ui](https://github.com/mstan/recomp-ui).

Instead of emulating SNES hardware at runtime, the game logic runs directly on the host CPU with SDL3 input/audio and OpenGL presentation, enabling native widescreen support, customizable CRT shaders, low-latency controls, save states, and real-time rewind.

> **Bring Your Own ROM:** This repository and all public releases contain **no ROMs**, **no generated game C code (`src/gen/`)**, and **no copyrighted game assets**. Public releases ship as a **Setup Host** (`-DSNESRECOMP_SETUP_HOST=ON`); you must supply your own legal copy of the US cartridge dump to build and play locally.

---

## Launcher

<p align="center">
  <img src="docs/screenshots/launcher-play.png" alt="AgrabahPort Launcher — Play" width="49%">
  <img src="docs/screenshots/launcher-settings.png" alt="AgrabahPort Launcher — Settings" width="49%">
</p>

---

## Features

- **Native Execution & Frame Pacing:** Locked `60.0 Hz` simulation with hybrid high-resolution wait timing, VSync support, and optional `Tab` fast-forward.
- **Adaptive Widescreen & Classic Ratios:** Supports adaptive `16:9` rendering alongside authentic `4:3` CRT and `8:7` square-pixel modes, plus removal of the original 32-sprites-per-scanline flicker limit.
- **Integrated Launcher UI:** Dedicated graphical interface for launch management, video/audio settings, controller profiles, mod packages, live runtime logs, and built-in GitHub update checks with automatic save backups.
- **Video Shaders:** Includes *Sharp Pixel*, *CRT Soft Scanlines*, *Warm Composite TV*, and *LCD Grid* GLSL presets.
- **Save States & Rewind:** Visual save-state browser (`F11`), 10 quick-save slots (`Shift+F1`–`F10` / `F1`–`F10`), battery SRAM (`saves/save.srm`), and real-time rewind (`F12`).
- **Portable Storage:** Creating an empty `portable.txt` next to the executable keeps `config.ini`, `keybinds.ini`, `saves/`, `mods/`, and `backups/` inside the game directory.

---

## Getting Started

1. **Download:** Grab the Setup Pack for your platform from [Releases](https://github.com/testsenvergonha-sys/AgrabahPort/releases) or install directly through [RecompLauncher](https://recomplauncher.com).
2. **Provide a verified ROM:** Extract the archive and supply your legal USA headerless `.sfc` dump using the launcher's setup prompt (or place `Aladdin.sfc` in the game folder).
3. **Build & Play:** On first run, the Setup Host validates your dump, compiles the native executable locally, and launches the game.

### Verified ROM Dump

| Field | Expected Value |
|---|---|
| Format | Headerless `.sfc` (USA) |
| Size | `1,310,720` bytes |
| CRC32 | `124d8e4d` |
| SHA-256 | `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e` |

---

## Controls & Shortcuts

USB and Bluetooth controllers (SNES, Xbox, PlayStation, Switch Pro, and generic SDL gamepads) are detected automatically and can be remapped in the **Controls** tab or `keybinds.ini`.

### Gameplay

| Action | Keyboard | SNES | Xbox | PlayStation |
|---|---|---|---|---|
| Move / Crouch | Arrow Keys | D-Pad | D-Pad / Left Stick | D-Pad / Left Stick |
| Jump | `Z` | `B` | `A` | Cross |
| Sword Slash | `X` | `Y` | `X` | Square |
| Throw Apple | `C` | `X` | `Y` | Triangle |
| Sprint / Run | `Shift` | `A` | `B` | Circle |
| Pause | `Enter` | `Start` | Menu / Start | Options |

### System Shortcuts

| Shortcut | Action |
|---|---|
| `ESC` | Open in-game launcher / menu |
| `Alt + Enter` | Toggle fullscreen / windowed |
| `Tab` | Hold for Turbo fast-forward |
| `F` | Toggle FPS overlay |
| `F11` | Open Save State browser |
| `F12` | Rewind |
| `Shift + F1` … `F10` | Quick save slot |
| `F1` … `F10` | Quick load slot |

---

## Building from Source

<details>
<summary>Prerequisites and build commands</summary>

### Prerequisites

- C11 / C++17 compiler (Clang, GCC, or MinGW-w64)
- CMake 3.20+ and Ninja
- Python 3.9+ and Rust toolchain
- SDL3 and zlib development libraries

### Local Build (with your own ROM)

```bash
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

sh tools/apply_patches.sh
cp /path/to/Aladdin.sfc roms/Aladdin.sfc
bash tools/regen.sh

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AladdinSNESRecomp
```

### Setup Host Build (for redistribution without ROM-derived code)

```bash
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

sh tools/apply_patches.sh
cmake -S . -B build-setup -G Ninja -DCMAKE_BUILD_TYPE=Release -DSNESRECOMP_SETUP_HOST=ON
cmake --build build-setup --target AladdinSNESRecomp
```

</details>

---

## Credits & Legal Notice

- Built on [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) and [recomp-ui](https://github.com/mstan/recomp-ui).
- AgrabahPort is an unofficial, non-commercial fan project. It is not affiliated with, endorsed by, or sponsored by Disney Enterprises, Inc., Capcom Co., Ltd., or Nintendo Co., Ltd. No ROM files, game code derived from ROMs, audio, or copyrighted artwork are distributed in this repository or its releases.
