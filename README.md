<p align="center">
  <img src="assets/icon_source.png" alt="AgrabahPort" width="160">
</p>

<h1 align="center">AgrabahPort</h1>

<p align="center">
  <strong>Native PC Port of Disney's Aladdin (Super Nintendo, 1993)</strong><br>
  Built with static recompilation for Windows, Linux, and macOS.
</p>

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
  <strong>English</strong> | <a href="README.pt-BR.md">Português (Brasil)</a> | <a href="CHANGELOG.md">Changelog</a>
</p>

---

## 🌟 About the Game

**AgrabahPort** is an unofficial native PC port of the classic **Disney's Aladdin** (Super Nintendo / SNES, 1993 — developed by Capcom). 

Unlike traditional emulators, AgrabahPort translates the original SNES processor instructions ahead-of-time directly into native C code. The result is a true standalone PC game running directly on your operating system with **flawless 60 FPS**, instantaneous input response, native controller support, modern display scaling, CRT shaders, and save-state capabilities.

> ⚖️ **Bring Your Own ROM:** This project does **not** include copyrighted ROM files, game music, or Disney/Capcom graphics. You provide your own legal copy of the original Super Nintendo cartridge dump.

---

## ⚡ Quick Start: How to Play in 3 Steps

1. **Download:** Grab the latest release `.zip` for your system from the [Releases](https://github.com/testsenvergonha-sys/AgrabahPort/releases) page (or install directly via [RecompLauncher](https://recomplauncher.com)).
2. **Place your ROM:** Extract the zip into any folder and place your legal SNES ROM (`Aladdin.sfc` or `Aladdin.smc`) in the same folder.
3. **Play:** Double-click `AgrabahPort.exe` (or run on Linux/macOS). The game automatically detects your ROM and starts instantly!

*(If you don't place the ROM beforehand, a clean file selector will appear on first launch asking you to pick it).*

---

## 🎮 Controls

Controllers auto-detect when plugged in (Xbox, PlayStation, Nintendo Switch, and generic USB gamepads).

| Action | Keyboard | Xbox Controller | PlayStation Controller | Nintendo Switch Pro |
|---|---|---|---|---|
| **Move / Crouch** | Arrow Keys / `W, A, S, D` | D-Pad / Left Stick | D-Pad / Left Stick | D-Pad / Left Stick |
| **Jump** | `Z` or `Space` | `A` Button | `Cross` (X) | `B` Button |
| **Slash (Sword)** | `X` | `X` Button | `Square` | `Y` Button |
| **Throw Apple** | `C` | `Y` Button | `Triangle` | `X` Button |
| **Sprint / Run** | `Shift` | `B` Button | `Circle` | `A` Button |
| **Pause Game** | `Enter` | `Start` / `Menu` | `Options` | `+` (Plus) |
| **In-Game Settings Menu** | `ESC` | `Back` + `LB` | `Share` + `L1` | `-` + `L` |
| **Toggle Fullscreen** | `Alt + Enter` | — | — | — |
| **Quick Save State** | `Shift + F1` … `F10` | — | — | — |
| **Quick Load State** | `F1` … `F10` | — | — | — |
| **Save State Browser** | `F11` | — | — | — |
| **Real-time Rewind** | `F12` | `Back` + `R3` | `Share` + `R3` | `-` + `R-Stick` |

---

## ✨ Features

- **100% Portable Mode:** Keep your game on a USB flash drive. Saves (`saves/`), settings (`config.ini`), and states stay cleanly in the game folder without cluttering your PC.
- **Ultra-Low Latency:** Native execution eliminates emulator audio buffer lag and input latency.
- **CRT & Video Shaders:** Built-in retro display shaders: *Sharp Pixel*, *CRT Soft Scanlines*, *Warm Composite TV*, and *LCD Grid*.
- **No Sprite Limits:** Removes the SNES 32-sprites-per-scanline hardware limit to eliminate character flickering.
- **Modern Display Support:** Seamless window resizing, crisp aspect ratio correction (4:3 CRT authentic or 8:7 square pixels), and VSync.
- **Live In-Game Menu (`ESC`):** Tweak video, audio, shaders, and controls on the fly without quitting the game.
- **Visual Rewind (`F12`) & Save States (`F11`):** Rewind past missed jumps and manage save slots visually.

---

## ❓ Frequently Asked Questions (FAQ)

<details>
<summary><strong>Which ROM version is required?</strong></summary>

You need a legally dumped USA cartridge of *Disney's Aladdin* for the Super Nintendo:
- **Format:** Headerless `.sfc` (or headered `.smc`)
- **Size:** `1,310,720` bytes (Headerless)
- **CRC32:** `124d8e4d`
- **SHA-256:** `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e`
</details>

<details>
<summary><strong>Where are my saved games stored?</strong></summary>

Everything is kept right inside the game folder in the `saves/` directory. Battery backup saves (`save.srm`) and state files remain portable so you can back them up or transfer them between computers easily.
</details>

<details>
<summary><strong>Can I customize the controls?</strong></summary>

Yes! You can edit `keybinds.ini` located next to the executable or adjust controls from the in-game launcher menu accessed by pressing `ESC`.
</details>

---

## 🛠️ For Developers & Building from Source

<details>
<summary><strong>Click to expand technical build instructions</strong></summary>

### Prerequisites
- Clang / GCC with C11 and C++17 support
- CMake 3.20+
- Ninja build system
- Python 3.9+
- Rust toolchain (`stable-x86_64-pc-windows-gnullvm` or standard host)
- SDL3 and zlib development libraries

### Building Locally
```bash
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

# 1. Provide your clean Aladdin.sfc dump
cp /path/to/Aladdin.sfc roms/Aladdin.sfc

# 2. Run static recompilation
bash tools/regen.sh

# 3. Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AladdinSNESRecomp
```

The resulting executable will be available at `build/AgrabahPort.exe`.
</details>

---

## 📜 Legal Notice

AgrabahPort is a fan recreation and static recompilation engine. It is not affiliated with, endorsed by, or sponsored by Capcom Co., Ltd., Disney Enterprises, Inc., or Nintendo Co., Ltd. *Aladdin* and all associated characters, artwork, and trademarks belong to their respective copyright holders.
