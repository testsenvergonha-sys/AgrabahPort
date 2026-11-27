<p align="center">
  <img src="assets/icon_source.png" alt="Emblema do AgrabahPort" width="148">
</p>

<h1 align="center">AgrabahPort</h1>

<p align="center">
  <strong>Port Nativo Não Oficial de Aladdin (Super Nintendo, 1993) para PC</strong><br>
  Recompilação estática para Windows, Linux e macOS — <em>v1.0.0</em>
</p>

<p align="center">
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases"><img src="https://img.shields.io/github/downloads/testsenvergonha-sys/AgrabahPort/total?style=flat-square&amp;label=downloads&amp;color=4B2E83" alt="Downloads"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/releases/latest"><img src="https://img.shields.io/github/v/release/testsenvergonha-sys/AgrabahPort?display_name=release&amp;label=version&amp;style=flat-square&amp;color=4B2E83" alt="Versão"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/stargazers"><img src="https://img.shields.io/github/stars/testsenvergonha-sys/AgrabahPort?label=stars&amp;style=flat-square&amp;color=4B2E83" alt="Estrelas"></a>
  <a href="https://github.com/testsenvergonha-sys/AgrabahPort/issues"><img src="https://img.shields.io/github/issues/testsenvergonha-sys/AgrabahPort?style=flat-square&amp;color=4B2E83" alt="Issues"></a>
</p>

<p align="center">
  <a href="https://recomplauncher.com"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Disponível no RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="46"></picture></a>
</p>

<p align="center">
  <a href="README.md">English</a> | <strong>Português (Brasil)</strong> | <a href="CHANGELOG.md">Changelog</a>
</p>

---

## 🖼️ Visual do Novo Launcher

<p align="center">
  <img src="docs/screenshots/launcher-play.png" alt="AgrabahPort v1.0.0 Launcher — Aba Play" width="49%">
  <img src="docs/screenshots/launcher-settings.png" alt="AgrabahPort v1.0.0 Launcher — Aba Settings" width="49%">
</p>

<p align="center">
  <em>Esquerda: Aba Play no tema autoral Palace Night com status da ROM e botões do Super Nintendo. Direita: Aba Settings (vídeo, proporção, shaders, exibição de FPS, áudio e Rewind).</em>
</p>

---

## 🌟 Sobre o AgrabahPort v1.0.0

O **AgrabahPort** é um port nativo não oficial do clássico de Super Nintendo *Aladdin* (1993) para computadores, construído com [snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp) e [recomp-ui](https://github.com/mstan/recomp-ui).

Em vez de emular o console em tempo real, o AgrabahPort recompila estaticamente as instruções do processador 65816 para código C nativo executando com SDL3, OpenGL e temporização de alta precisão. Na versão **v1.0.0**, o projeto deixa a fase de testes com um **launcher autoral completo**, **ícones visuais de controle do Super Nintendo**, **widescreen adaptativo 16:9** corrigido em fases e chefes, modo **Turbo (`Tab`) fluido** e **auto-updater integrado ao GitHub**.

> ⚖️ **Traga Sua Própria ROM — Sem Conteúdo Protegido Incluso:** Este repositório e todas as releases públicas **não distribuem** arquivos de ROM, código gerado derivado da ROM (`src/gen/`), músicas ou sprites protegidos por direitos autorais. As releases públicas usam exclusivamente o modo **Setup Host** (`-DSNESRECOMP_SETUP_HOST=ON`), exigindo que você forneça sua própria cópia legal americana (`.sfc`).

---

## ✨ Novidades e Recursos da v1.0.0

- **Novo Launcher Autoral (*Palace Night*):** Visual exclusivo com arcos árabes, lanternas animadas, cinco abas (**Play**, **Settings**, **Controls**, **Mods**, **About**) e rodapé com botões reais do **Super Nintendo** (**`L`/`R`** abas, **`Enter`/`B`** confirmar, **`A`** voltar, **`Esc`** fechar).
- **Auto-Updater Seguro via GitHub Releases:** Verifica automaticamente `testsenvergonha-sys/AgrabahPort` ao abrir, exibe o modal com notas da versão, cria backup automático de `saves/` em `backups/pre_update_<versão>/`, mantém `AgrabahPort.exe.old` para segurança e nunca toca em `saves/`, `mods/`, `states/`, `config.ini`, `keybinds.ini`, `rom.cfg`, `portable.txt` ou sua ROM.
- **Widescreen Adaptativo 16:9 + Modos Clássicos:** Ampliação dinâmica para telas largas com provedores dedicados por fase (Caverna das Maravilhas, Escape no Tapete, fase da Lava e chefes finais) e fallback limpo em telas estáticas, além dos modos `4:3` e `8:7`.
- **60.0 FPS Travados & Turbo (`Tab`) Fluido:** Sincronia de simulação em `60.0 Hz` e controle desacoplado durante o fast-forward com `Tab` para evitar travamentos, voltando imediatamente ao ritmo normal ao soltar a tecla.
- **Padrões de Fábrica Refinados:** Inicia em **Tela Cheia (Borderless)** (`Fullscreen = 1`) com o contador de FPS (`DisplayPerfInTitle = 0`) desativado por padrão — ambos configuráveis diretamente na aba **Settings**.
- **Shaders CRT, Save States e Rewind:** Presets *Sharp Pixel*, *CRT Soft Scanlines*, *Warm Composite TV* e *LCD Grid*, galeria visual de Save States (`F11`), salvamento rápido (`Shift+F1..F10` / `F1..F10`) e rebobinamento (`F12`).
- **Modo 100% Portátil (`portable.txt`):** Mantenha saves, mods, backups e configurações dentro da própria pasta do jogo.

---

## ⚡ Como Jogar em 3 Passos

1. **Baixe:** Faça o download do Setup Pack `.zip` para o seu sistema na página de [Releases](https://github.com/testsenvergonha-sys/AgrabahPort/releases) (ou instale direto pelo [RecompLauncher](https://recomplauncher.com)).
2. **Forneça sua ROM:** Extraia o arquivo e coloque sua ROM americana sem cabeçalho (`.sfc`) ao lado do executável ou selecione-a no assistente inicial do launcher.
3. **Jogue:** Abra o `AgrabahPort.exe` (ou binário Linux/macOS). Na primeira execução do Setup Pack, o assistente gera e compila localmente o executável a partir da sua ROM.

---

## 🎮 Controles e Atalhos

Controles USB e Bluetooth são reconhecidos automaticamente (Super Nintendo USB, Xbox, PlayStation, Switch Pro e genéricos SDL).

| Ação | Teclado | Padrão SNES | Xbox / XInput | PlayStation |
|---|---|---|---|---|
| **Mover / Agachar** | Setas / `W A S D` | D-Pad | D-Pad / Analógico | D-Pad / Analógico |
| **Pular** | `Z` ou `Espaço` | `B` | `A` | `X` (Cruz) |
| **Golpear (Espada)** | `X` | `Y` | `X` | `Quadrado` |
| **Jogar Maçã** | `C` | `X` | `Y` | `Triângulo` |
| **Correr** | `Shift` | `A` | `B` | `Círculo` |
| **Pausar** | `Enter` | `Start` | `Menu / Start` | `Options` |
| **Modo Turbo** | `Tab` | Atalho Assist | Atalho Assist | Atalho Assist |
| **Alternar Tela Cheia** | `Alt + Enter` | — | — | — |
| **Menu / Launcher** | `ESC` | — | — | — |
| **Save States / Rewind** | `F11` / `F12` | — | — | — |
| **Salvar / Carregar Rápido** | `Shift+F1..F10` / `F1..F10` | — | — | — |

---

## ❓ Perguntas Frequentes (FAQ)

<details>
<summary><strong>Qual ROM é aceita?</strong></summary>

Uma cópia legítima USA (sem cabeçalho de copiador) de *Aladdin* para Super Nintendo (`Aladdin.sfc`):
- **Tamanho:** `1.310.720` bytes
- **CRC32:** `124d8e4d`
- **SHA-256:** `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e`
</details>

<details>
<summary><strong>O auto-updater apaga meus saves, configs ou mods?</strong></summary>

Não. Antes de aplicar a atualização, o launcher faz backup de `saves/` em `backups/pre_update_<versão>/` e exclui da pasta temporária de atualização qualquer arquivo ou pasta protegida (`saves/`, `backups/`, `mods/`, `states/`, `config.ini`, `config.local.ini`, `keybinds.ini`, `rom.cfg`, `portable.txt`, `AgrabahPort_window.cfg` e arquivos `.sfc`/`.smc`/`.srm`/`.sav`).
</details>

---

## 🛠️ Compilando do Código-Fonte

<details>
<summary><strong>Instruções para Desenvolvedores e Setup Host</strong></summary>

### Pré-requisitos
- Compilador C11 / C++17 (Clang / GCC / MinGW-w64), CMake 3.20+, Ninja, Python 3.9+, Rust toolchain, SDL3 e zlib.

### Build local com sua própria ROM (apenas uso pessoal local)
```bash
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

cp /caminho/para/Aladdin.sfc roms/Aladdin.sfc
bash tools/regen.sh

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AladdinSNESRecomp
```

### Build Setup Host (modelo seguro para distribuição pública)
```bash
cmake -S . -B build-setup -G Ninja -DCMAKE_BUILD_TYPE=Release -DSNESRECOMP_SETUP_HOST=ON
cmake --build build-setup --target AladdinSNESRecomp
```
</details>

---

## 📜 Aviso Legal

O AgrabahPort é um projeto independente e não comercial criado por fãs usando recompilação estática. Não possui afiliação, vínculo nem endosso da Disney Enterprises, Inc., Capcom Co., Ltd. ou Nintendo Co., Ltd. Nenhum arquivo de ROM, áudio ou sprite protegido por direitos autorais é distribuído por este projeto; o usuário deve fornecer sua própria cópia legal.
