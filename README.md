# AgrabahPort

<p align="center">
  <img src="assets/icon_source.png" alt="AgrabahPort Icon" width="160">
</p>

<p align="center">
  <strong>Native PC Static Recompilation of the 1993 16-bit SNES Platformer</strong><br>
  Built with <a href="https://github.com/RetroPortingToolKit/snesrecomp">SNESRecomp</a> &amp; <a href="https://github.com/mstan/recomp-ui">recomp-ui</a> (SDL3 + OpenGL + Dear ImGui)
</p>

<p align="center">
  <a href="#english">English</a> • <a href="#português-br">Português (BR)</a>
</p>

---

> [!IMPORTANT]
> **Legal Notice / Aviso Legal (Copyright & DMCA)**
>
> - **No ROM files, game assets, artwork, or recompiled game code (`src/gen/`) are included** in this repository or in any GitHub Release.
> - **You must legally own and supply your own original SNES cartridge dump** (`.sfc` / `.smc`, USA revision) to build or run this project.
> - All recompiled C code is generated **locally on your machine** from your own ROM copy and is excluded by `.gitignore`.
> - This is an independent, non-commercial community preservation project and is **not affiliated with, endorsed by, or connected to Capcom, Disney, or Nintendo**.

---

<a id="english"></a>
## English

### Overview

**AgrabahPort** is a native Windows/Linux/macOS PC port built via **static 65816-to-C recompilation** using the [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp) framework. Instead of traditional full-CPU emulation, the game's original 65816 machine code is translated ahead-of-time into C and compiled into a standalone native executable powered by SDL3 and OpenGL.

### Features

- **Native PC Executable (`Windows GUI`):** Standalone portable build with static runtime (no external MinGW/SDL DLLs required and no stray command prompt window on Windows).
- **Integrated Pre-Boot & In-Game Launcher (`ESC`):** Dear ImGui interface for ROM verification, video/audio/input configuration, mods, save states, and clean game exit (`QUIT GAME` / `RESUME`).
- **Built-In Live Log Viewer:** Captures `stdout`/`stderr` in real time into a clean in-launcher log panel (with color coding, auto-scroll, copy, and clear) — no external console window needed.
- **OpenGL & GLSL Shaders:** Bundled CC0 shader presets (`sharp.glslp`, `crt-soft.glslp`, `warm-composite.glslp`, `lcd-grid.glslp`), VSync, Mode 7 HD scaling (`8x` with supersampling), and sprite-per-scanline limit removal.
- **Experimental Widescreen Support:** Widescreen & Display Aspect controls exposed in the launcher Display settings.
- **Save States & Rewind:** Instant save states (`F11` browser / `F1`–`F10` quick slots) and real-time rewind (`F12`).
- **Original Custom App Icon:** 100% original 16-bit-inspired icon embedded into the Windows executable.

### Supported ROM (Bring Your Own ROM)

Verify that your legally dumped USA ROM matches these headerless digests (an `.smc` file with a 512-byte copier header should be stripped to a clean `.sfc` before running `tools/regen.sh`):

| Field | Expected Value |
|---|---|
| Region | USA (North America) |
| Mapping | LoROM |
| Coprocessor | None |
| Headerless Size | `1,310,720` bytes (1.25 MiB) |
| CRC32 | `124d8e4d` |
| SHA-256 | `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e` |

### Building From Source

#### Prerequisites
- **Git** (with Git Bash on Windows)
- **Python 3.11+**
- **Rust (`rustup`, 1.85+)** — used to build the `snesrecomp` static analyzer
- **C/C++ Compiler + CMake + Ninja + SDL3** — on Windows, the easiest option is the portable [RetroPorting-Toolchains (`cmake-clang-v1-windows-x64`)](https://github.com/RetroPortingToolKit/RetroPorting-Toolchains/releases) pack (includes Clang, CMake, Ninja, zlib, and SDL3) with Rust target `stable-x86_64-pc-windows-gnullvm`, or Visual Studio 2022 Build Tools.

#### Steps

```sh
# 1. Clone with submodules
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

# 2. Apply the integrated Live Log patch to recomp-ui
sh tools/apply_patches.sh

# 3. Generate recompiled C code locally from YOUR legal ROM (.sfc without 512-byte header)
bash tools/regen.sh --rom /path/to/your_rom.sfc

# 4. Configure and build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

> **Windows (Clang/MinGW + Rust `gnullvm`) note:** If you use the portable `cmake-clang-v1` toolchain pack instead of MSVC, set `RUSTUP_TOOLCHAIN=stable-x86_64-pc-windows-gnullvm` before running `tools/regen.sh` and pass `-DSDL3_DIR=<pack>/deps/lib/cmake/SDL3` to CMake.

### Default Shortcuts

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

<a id="português-br"></a>
## Português (BR)

### Visão Geral

O **AgrabahPort** é um **port nativo para PC** construído via **recompilação estática (65816 → C)** usando o framework [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp). Em vez de emulação tradicional de CPU, o código original do cartucho é traduzido localmente para C e compilado em um executável nativo com **SDL3**, **OpenGL** e launcher **Dear ImGui**.

### Recursos

- **Executável nativo e portátil (`Windows GUI`):** compilação estática (sem DLLs externas e **sem janela preta de Prompt de Comando** atrás do jogo).
- **Menu In-Game no `ESC`:** pause a qualquer momento para ajustar vídeo, áudio, controles, abrir os logs ou fechar o jogo (`QUIT GAME` / `RESUME`).
- **Live Log integrado no Launcher:** captura `stdout`/`stderr` em tempo real dentro do próprio launcher (com cores por nível, auto-scroll, copiar e limpar).
- **OpenGL + Shaders GLSL:** presets inclusos (`sharp.glslp`, `crt-soft.glslp`, `warm-composite.glslp`, `lcd-grid.glslp`), VSync, Mode 7 em HD (`8x`) e remoção do limite de sprites por linha.
- **Widescreen experimental:** opção liberada na aba **Display** do launcher.
- **Save States (`F11`) e Rewind (`F12`).**
- **Ícone próprio 100% autoral** embutido no executável `.exe`.

### ROM Necessária (Você deve fornecer a sua)

Nenhuma ROM é distribuída neste projeto. Você precisa da sua própria cópia legal da ROM **USA** limpa (`.sfc` sem cabeçalho antigo de 512 bytes):

- **Tamanho:** `1.310.720` bytes (1,25 MiB)
- **CRC32:** `124d8e4d`
- **SHA-256:** `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e`

*(Se a sua ROM `.smc` tiver `1.311.232` bytes, remova os primeiros 512 bytes de cabeçalho copier para obter o `.sfc` limpo de `1.310.720` bytes.)*

### Como Compilar

```sh
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort
sh tools/apply_patches.sh
bash tools/regen.sh --rom /caminho/para/sua_rom.sfc
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## Credits & Licenses

- Framework: [RetroPortingToolKit/snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) (see `snesrecomp/LICENSE` and `snesrecomp/THIRD_PARTY_ATTRIBUTION.md`)
- Launcher UI: [mstan/recomp-ui](https://github.com/mstan/recomp-ui)
- Project source code: see [LICENSE](LICENSE). No copyrighted game data is included or licensed here.
