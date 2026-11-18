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
  <a href="README.md">English</a> | <strong>Português (Brasil)</strong>
</p>

O **AgrabahPort** é um port nativo não-oficial para PC de **Disney's Aladdin** (Super Nintendo / SNES, 1993 — desenvolvido pela Capcom) para Windows, Linux e macOS, construído via **recompilação estática (65816 → C)** usando [SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp) e [recomp-ui](https://github.com/mstan/recomp-ui).

> **Nenhum arquivo do jogo ou ROM está incluído.** Você precisa fornecer uma cópia legal do seu próprio cartucho de SNES (testado: **Disney's Aladdin (USA)**, `.sfc` sem cabeçalho — CRC32 `124d8e4d`, SHA-256 `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e`).
> Todo o código C recompilado (`src/gen/`) é gerado localmente na sua máquina a partir da sua própria ROM e nunca é commitado ou distribuído neste repositório ou em qualquer release.
> Este projeto não é afiliado nem endossado pela Capcom, Disney ou Nintendo. *Aladdin* e marcas relacionadas pertencem aos seus respectivos proprietários.

---

## Recursos

- **Execução Nativa em PC (Recompilação Estática):** Traduz antecipadamente o código 65816 original do SNES para C e compila um executável nativo com **SDL3** e **OpenGL**.
- **Executável Windows GUI Portátil:** Binário estático (`AgrabahPort.exe`) sem DLLs externas e **sem janela preta de Prompt de Comando** aberta atrás do jogo.
- **Launcher e Menu In-Game no `ESC`:** Interface Dear ImGui para verificação de ROM, vídeo, áudio, controles, mods, save states e saída limpa do jogo (`RESUME` / `QUIT GAME`).
- **Live Log Integrado:** Captura `stdout`/`stderr` em tempo real dentro do próprio launcher (com linhas coloridas, auto-scroll, copiar e limpar).
- **OpenGL + Shaders GLSL:** Presets CC0 inclusos (`sharp.glslp`, `crt-soft.glslp`, `warm-composite.glslp`, `lcd-grid.glslp`), VSync, Mode 7 em HD (`8x` com supersampling) e remoção do limite de sprites por linha.
- **Widescreen Experimental:** Opções de Widescreen e Proporção de Tela liberadas na aba **Display** do launcher.
- **Save States e Rewind:** Navegador visual de save states (`F11`), slots rápidos (`F1`–`F10`) e rewind em tempo real (`F12`).
- **Ícone 16-bit Original:** Ícone em pixel art 100% autoral embutido no executável `.exe`.

---

## ROM Suportada (Você deve fornecer a sua)

Verifique se a sua cópia legal de **Disney's Aladdin (USA)** para SNES corresponde aos hashes abaixo (sem cabeçalho). Se o seu arquivo `.smc` tiver `1.311.232` bytes (cabeçalho copier antigo de 512 bytes), remova os primeiros 512 bytes para gerar o `.sfc` limpo de `1.310.720` bytes antes de rodar `tools/regen.sh`.

| Campo | Valor Esperado |
|---|---|
| Jogo | **Disney's Aladdin (USA)** — SNES (Capcom, 1993) |
| Região | USA (América do Norte) |
| Mapeamento | LoROM |
| Coprocessador | Nenhum |
| Tamanho (sem cabeçalho) | `1.310.720` bytes (1,25 MiB) |
| CRC32 | `124d8e4d` |
| SHA-256 | `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e` |

---

## Como Compilar do Código-Fonte

### Pré-requisitos
- **Git** (com Git Bash no Windows)
- **Python 3.11+**
- **Rust (`rustup`, 1.85+)** — usado para compilar o analisador estático do `snesrecomp`
- **Compilador C/C++ + CMake + Ninja + SDL3** — no Windows, a forma mais prática é usar o pacote portátil [RetroPorting-Toolchains (`cmake-clang-v1-windows-x64`)](https://github.com/RetroPortingToolKit/RetroPorting-Toolchains/releases) (já traz Clang, CMake, Ninja, zlib e SDL3) junto com Rust `stable-x86_64-pc-windows-gnullvm`, ou Visual Studio 2022 Build Tools.

### Passo a Passo

```sh
# 1. Clonar com submódulos
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

# 2. Aplicar o patch do Live Log integrado ao recomp-ui
sh tools/apply_patches.sh

# 3. Gerar o código C recompilado localmente a partir da SUA ROM legal de Aladdin (USA) (.sfc)
bash tools/regen.sh --rom /caminho/para/Aladdin.sfc

# 4. Configurar e compilar
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

> **Dica no Windows (Clang/MinGW + Rust `gnullvm`):** Se estiver usando o pacote portátil `cmake-clang-v1` no lugar do MSVC, defina `RUSTUP_TOOLCHAIN=stable-x86_64-pc-windows-gnullvm` antes do `tools/regen.sh` e passe `-DSDL3_DIR=<pacote>/deps/lib/cmake/SDL3` no CMake.

---

## Atalhos Padrão

| Atalho | Ação |
|---|---|
| `ESC` | Abrir Menu In-Game (Retomar / Configurações / Live Log / Sair do Jogo) |
| `Alt + Enter` | Alternar Tela Cheia / Janela |
| `F11` | Menu de Save States |
| `Shift + F1..F10` / `F1..F10` | Salvar / Carregar Slots Rápidos |
| `F12` | Rewind (voltar no tempo) |
| `Tab` | Turbo / Avanço Rápido |
| `Ctrl + R` | Reset |

---

## Créditos e Licenças

- **Framework de Recompilação:** [RetroPortingToolKit/snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) (veja `snesrecomp/LICENSE` e `snesrecomp/THIRD_PARTY_ATTRIBUTION.md`)
- **Interface do Launcher:** [mstan/recomp-ui](https://github.com/mstan/recomp-ui)
- **Código do Projeto:** veja [LICENSE](LICENSE). Nenhum dado de jogo protegido por direitos autorais, ROM ou arte oficial está incluído ou licenciado aqui.
