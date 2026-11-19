<p align="center">
  <img src="assets/icon_source.png" alt="AgrabahPort" width="160">
</p>

<h1 align="center">AgrabahPort</h1>

<p align="center">
  <strong>Port Nativo para PC de Disney's Aladdin (Super Nintendo, 1993)</strong><br>
  Construído via recompilação estática para Windows, Linux e macOS.
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
  <a href="README.md">English</a> | <strong>Português (Brasil)</strong> | <a href="CHANGELOG.md">Registro de Atualizações</a>
</p>

---

## 🌟 Sobre o Jogo

O **AgrabahPort** é uma adaptação nativa para computadores do clássico **Disney's Aladdin** (Super Nintendo / SNES, lançado em 1993 pela lendária Capcom).

Diferente de emuladores convencionais, o AgrabahPort traduz todo o código original de montagem do processador do SNES diretamente para a linguagem C nativa. O resultado é um executável de computador ultra-otimizado que roda diretamente no seu sistema operacional com **60 FPS perfeitos**, resposta instantânea aos controles, suporte automático a gamepads modernos, filtros de TV de tubo (CRT) e carregamento sem engasgos.

> ⚖️ **Traga Sua Própria ROM (Bring Your Own ROM):** Este projeto **não distribui** nenhum arquivo protegido por direitos autorais (ROM, músicas ou artes da Capcom/Disney). O jogador deve fornecer o arquivo do seu próprio cartucho original do Super Nintendo.

---

## ⚡ Como Jogar em 3 Passos Simples

1. **Baixe o jogo:** Baixe o arquivo `.zip` da versão mais recente na página de [Releases](https://github.com/testsenvergonha-sys/AgrabahPort/releases) (ou instale com 1 clique pelo [RecompLauncher](https://recomplauncher.com)).
2. **Coloque sua ROM:** Extraia a pasta em qualquer lugar e coloque sua ROM de Aladdin do SNES (`Aladdin.sfc` ou `Aladdin.smc`) dentro da mesma pasta do jogo.
3. **Divirta-se:** Dê dois cliques em `AgrabahPort.exe`. O jogo detecta sua ROM sozinho e abre imediatamente!

*(Se você não colocar a ROM antes, não tem problema: uma janela se abrirá na primeira vez pedindo para você selecionar o arquivo).*

---

## 🎮 Controles

Os controles funcionam automaticamente assim que conectados (Xbox, PlayStation, Nintendo Switch e controles genéricos USB).

| Ação no Jogo | Teclado | Controle Xbox | Controle PlayStation | Nintendo Switch Pro |
|---|---|---|---|---|
| **Mover / Agachar** | Setas direcionais / `W, A, S, D` | D-Pad / Analógico Esquerdo | D-Pad / Analógico Esquerdo | D-Pad / Analógico Esquerdo |
| **Pular** | `Z` ou `Espaço` | Botão `A` | Botão `X` (Cross) | Botão `B` |
| **Atacar com a Espada** | `X` | Botão `X` | Botão `Quadrado` | Botão `Y` |
| **Arremessar Maçã** | `C` | Botão `Y` | Botão `Triângulo` | Botão `X` |
| **Correr** | `Shift` | Botão `B` | Botão `Círculo` | Botão `A` |
| **Pausar** | `Enter` | `Start` / `Menu` | `Options` | `+` (Mais) |
| **Menu de Configurações (`ESC`)** | `ESC` | `Back` + `LB` | `Share` + `L1` | `-` + `L` |
| **Alternar Tela Cheia** | `Alt + Enter` | — | — | — |
| **Salvar Estado Rápido** | `Shift + F1` até `F10` | — | — | — |
| **Carregar Estado Rápido** | `F1` até `F10` | — | — | — |
| **Galeria Visual de Saves** | `F11` | — | — | — |
| **Rebobinar em Tempo Real** | `F12` | `Back` + `R3` | `Share` + `R3` | `-` + `R-Stick` |

---

## ✨ Recursos Exclusivos da Versão PC

- **100% Portátil:** Salve o jogo num pendrive e jogue em qualquer PC. Todos os arquivos de configuração (`config.ini`), saves de bateria e estados salvos (`saves/`) ficam organizados na pasta do próprio jogo, sem espalhar nada pelo seu computador.
- **Latência Quase Zero:** A execução direta no hardware da sua máquina elimina o atraso de áudio e comandos típico de emuladores.
- **Filtros CRT de Alta Definição:** Inclui filtros visuais retrô clássicos: *Sharp Pixel* (pixel art nítido), *CRT Soft Scanlines* (linhas de TV de tubo), *Warm Composite TV* (TV analógica vintage) e *LCD Grid*.
- **Sem Limite de Sprites:** O limite clássico do Super Nintendo de 32 sprites por linha horizontal foi removido, eliminando completamente aquele efeito de personagens e maçãs piscando ou sumindo da tela.
- **Menu ao Vivo no Jogo (`ESC`):** Ajuste volume, filtros, escala de resolução e controles em tempo real sem precisar reiniciar a partida.
- **Rebobinamento (`F12`):** Errou aquele pulo difícil? Aperte `F12` para voltar alguns segundos no tempo!

---

## ❓ Perguntas Frequentes (FAQ)

<details>
<summary><strong>Qual versão da ROM de Aladdin é necessária?</strong></summary>

Você precisa de uma cópia legítima do cartucho americano (USA) de *Disney's Aladdin* do Super Nintendo:
- **Formato:** `.sfc` (sem cabeçalho de copiador) ou `.smc`
- **Tamanho:** `1.310.720` bytes (sem cabeçalho)
- **CRC32:** `124d8e4d`
- **SHA-256:** `aa768b8b00123717c8d49f2c6731cdbfd80ab6a54128bae7594e93f45e38a19e`
</details>

<details>
<summary><strong>Onde meus saves ficam gravados?</strong></summary>

Tudo fica guardado na subpasta `saves/` dentro da própria pasta onde o jogo está instalado. Seus arquivos de progresso (`save.srm`) e fotos de save state são fáceis de copiar e transportar para outro PC.
</details>

<details>
<summary><strong>Posso reconfigurar os botões do teclado e controle?</strong></summary>

Sim! Você pode personalizar as teclas diretamente pelo arquivo `keybinds.ini` ao lado do executável ou apertando `ESC` durante a partida para abrir o menu do launcher.
</details>

---

## 🛠️ Para Desenvolvedores e Compilação do Código-Fonte

<details>
<summary><strong>Clique para expandir instruções técnicas de compilação</strong></summary>

### Pré-requisitos
- Compilador Clang ou GCC com suporte a C11 e C++17
- CMake 3.20 ou superior
- Ninja Build
- Python 3.9+
- Toolchain Rust (`stable-x86_64-pc-windows-gnullvm` ou nativa da sua máquina)
- Bibliotecas SDL3 e zlib

### Compilando Localmente
```bash
git clone --recurse-submodules https://github.com/testsenvergonha-sys/AgrabahPort.git
cd AgrabahPort

# 1. Coloque a sua ROM Aladdin.sfc na pasta roms/
cp /caminho/para/Aladdin.sfc roms/Aladdin.sfc

# 2. Execute a recompilação estática dos códigos 65816
bash tools/regen.sh

# 3. Configure e compile o binário nativo
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target AladdinSNESRecomp
```

O executável compilado estará pronto em `build/AgrabahPort.exe`.
</details>

---

## 📜 Aviso Legal

O AgrabahPort é uma recriação de código aberto desenvolvida por fãs via tecnologia de recompilação estática. Este projeto não possui afiliação, vínculo ou endosso de Capcom Co., Ltd., Disney Enterprises, Inc. ou Nintendo Co., Ltd. *Aladdin* e todas as marcas, personagens e artes correlatas pertencem aos seus respectivos titulares de direitos autorais.
