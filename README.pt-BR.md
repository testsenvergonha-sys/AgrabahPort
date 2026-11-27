<p align="center">
  <img src="assets/icon_source.png" alt="AgrabahPort" width="136">
</p>

<h1 align="center">AgrabahPort</h1>

<p align="center">
  Port nativo de <em>Aladdin</em> (Super Nintendo, 1993) para PC construído por recompilação estática.
</p>

<p align="center">
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases"><img src="https://img.shields.io/github/downloads/testsenvergonha-sys/AgrabahPort/total?style=flat-square&amp;label=downloads&amp;color=4B2E83" alt="Downloads"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases/latest"><img src="https://img.shields.io/github/v/release/testsenvergonha-sys/AgrabahPort?display_name=release&amp;label=release&amp;style=flat-square&amp;color=4B2E83" alt="Última Release"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/stargazers"><img src="https://img.shields.io/github/stars/testsenvergonha-sys/AgrabahPort?style=flat-square&amp;color=4B2E83" alt="Estrelas"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/issues"><img src="https://img.shields.io/github/issues/testsenvergonha-sys/AgrabahPort?style=flat-square&amp;color=4B2E83" alt="Issues"></a>
</p>

<p align="center">
  <a href="https://recomplauncher.com"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Disponível no RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="44"></picture></a>
</p>

<p align="center">
  <a href="README.md">English</a> · <strong>Português (Brasil)</strong> · <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases">Releases</a> · <a href="CHANGELOG.md">Changelog</a>
</p>

---

## Visão Geral

O **AgrabahPort** recompila estaticamente as instruções originais 65816 de *Aladdin* (SNES, 1993) para código C nativo em **Windows**, **Linux** e **macOS**, utilizando [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) e [recomp-ui](https://github.com/mstan/recomp-ui).

Em vez de emular o hardware do Super Nintendo em tempo de execução, a lógica do jogo roda diretamente no processador do computador com entrada/áudio via SDL3 e apresentação em OpenGL, permitindo widescreen adaptativo, filtros CRT, baixa latência, save states e rebobinamento em tempo real.

> **Traga Sua Própria ROM:** Este repositório e todas as releases públicas **não contêm ROMs**, **não contêm código C gerado da ROM (`src/gen/`)** e **não distribuem assets protegidos por direitos autorais**. As releases públicas usam exclusivamente o modo **Setup Host** (`-DSNESRECOMP_SETUP_HOST=ON`); o jogador deve fornecer sua própria cópia legal americana para compilar e jogar localmente.

---

## Launcher

<p align="center">
  <img src="docs/screenshots/launcher-play.png" alt="AgrabahPort Launcher — Play" width="49%">
  <img src="docs/screenshots/launcher-settings.png" alt="AgrabahPort Launcher — Settings" width="49%">
</p>

---

## Recursos

- **Execução Nativa e Ritmo de Quadros:** Simulação travada em `60.0 Hz` com limitador híbrido de alta precisão, suporte a VSync e fast-forward opcional em `Tab`.
- **Widescreen Adaptativo e Proporções Clássicas:** Suporte a exibição `16:9` adaptativa junto aos modos autênticos `4:3` (CRT) e `8:7` (pixels quadrados), além da remoção do limite original de 32 sprites por linha.
- **Launcher Gráfico Integrado:** Interface dedicada para configuração de vídeo, áudio, perfis de controle, pacotes de mods, visualização de logs ao vivo e verificação de atualizações no GitHub com backup automático de saves.
- **Shaders de Vídeo:** Presets GLSL inclusos (*Sharp Pixel*, *CRT Soft Scanlines*, *Warm Composite TV* e *LCD Grid*).
- **Save States e Rewind:** Galeria visual de estados (`F11`), 10 slots rápidos (`Shift+F1`–`F10` / `F1`–`F10`), save SRAM (`saves/save.srm`) e rebobinamento em tempo real (`F12`).
- **Modo Portátil:** Com um arquivo `portable.txt` ao lado do executável, `config.ini`, `keybinds.ini`, `saves/`, `mods/` e `backups/` permanecem isolados dentro da pasta do jogo.

---

## Como Jogar

1. **Baixe:** Pegue o Setup Pack do seu sistema na página de [Releases](https://github.com/testsenvergonha-sys/AgrabahPort/releases) ou instale diretamente pelo [RecompLauncher](https://recomplauncher.com).
2. **Forneça uma ROM verificada:** Extraia o pacote e informe sua cópia legal americana sem cabeçalho (`.sfc`) no assistente do launcher (ou coloque `Aladdin.sfc` na pasta do jogo).
3. **Compile e Jogue:** Na primeira execução, o Setup Host valida o arquivo, compila o executável nativo localmente e inicia o jogo.

### Especificação da ROM Verificada

| Campo | Valor Esperado |
|---|---|
| Formato | `.sfc` sem cabeçalho (USA) |
| Tamanho | `1.310.720` bytes |
| CRC32 | `124d8e4d` |
| SHA-256 | `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e` |

---

## Controles e Atalhos

Controles USB e Bluetooth (SNES, Xbox, PlayStation, Switch Pro e gamepads SDL genéricos) são reconhecidos automaticamente e podem ser remapeados na aba **Controls** ou em `keybinds.ini`.

### Gameplay

| Ação | Teclado | SNES | Xbox | PlayStation |
|---|---|---|---|---|
| Mover / Agachar | Setas | D-Pad | D-Pad / Analógico | D-Pad / Analógico |
| Pular | `Z` | `B` | `A` | Cruz (X) |
| Golpear (Espada) | `X` | `Y` | `X` | Quadrado |
| Jogar Maçã | `C` | `X` | `Y` | Triângulo |
| Correr | `Shift` | `A` | `B` | Círculo |
| Pausar | `Enter` | `Start` | Menu / Start | Options |

### Atalhos do Sistema

| Atalho | Ação |
|---|---|
| `ESC` | Abrir launcher / menu durante o jogo |
| `Alt + Enter` | Alternar tela cheia / janela |
| `Tab` | Segurar para acelerar (Turbo) |
| `F` | Exibir ou ocultar contador de FPS |
| `F11` | Abrir navegador de Save States |
| `F12` | Rebobinar (Rewind) |
| `Shift + F1` … `F10` | Salvar estado rápido |
| `F1` … `F10` | Carregar estado rápido |

---

## Compilando do Código-Fonte

<details>
<summary>Pré-requisitos e comandos de compilação</summary>

### Pré-requisitos

- Compilador C11 / C++17 (Clang, GCC ou MinGW-w64)
- CMake 3.20+ e Ninja
- Python 3.9+ e toolchain Rust
- Bibliotecas SDL3 e zlib

### Build Local (com a sua própria ROM)

```bash
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

sh tools/apply_patches.sh
cp /caminho/para/Aladdin.sfc roms/Aladdin.sfc
bash tools/regen.sh

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AladdinSNESRecomp
```

### Build Setup Host (para distribuição sem código derivado da ROM)

```bash
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

sh tools/apply_patches.sh
cmake -S . -B build-setup -G Ninja -DCMAKE_BUILD_TYPE=Release -DSNESRECOMP_SETUP_HOST=ON
cmake --build build-setup --target AladdinSNESRecomp
```

</details>

---

## Créditos e Aviso Legal

- Baseado em [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) e [recomp-ui](https://github.com/mstan/recomp-ui).
- O AgrabahPort é um projeto independente e não comercial desenvolvido por fãs. Não possui afiliação, vínculo nem endosso da Disney Enterprises, Inc., Capcom Co., Ltd. ou Nintendo Co., Ltd. Nenhum arquivo de ROM, código gerado a partir de ROM, áudio ou arte protegida por direitos autorais é distribuído neste repositório ou em suas releases.
