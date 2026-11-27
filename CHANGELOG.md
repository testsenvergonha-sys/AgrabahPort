# Changelog / Registro de Atualizações

Todas as mudanças notáveis no projeto **AgrabahPort** são documentadas neste arquivo de forma detalhada e transparente.

---

## [1.0.1] - 2026-11-27

### 🛠️ Correções do Setup Pack, Widescreen 16:9 e Launcher
- **Recompilação local do Setup Host (`target AgrabahPort`):** Adicionei o target `AgrabahPort` em `CMakeLists.txt` mapeado para `AladdinSNESRecomp` para que o `snesrecomp_codegen_host` conclua a compilação local sem falhar com `unknown target 'AgrabahPort'`.
- **Headers autorais versionados em `src/`:** Movi `agrabah_launcher_shell.h` e `agrabah_updater.h` para `src/` (fora do submódulo `recomp-ui`) garantindo que sejam empacotados em todos os Setup Packs gerados pelo CI sem erro de header ausente.
- **Widescreen 16:9 e Fullscreen Borderless padrão out-of-the-box:** Passei a gerar `Widescreen = 1` junto de `Fullscreen = 1` no `config.ini` inicial de portas `native_widescreen` (com fallback automático quando a chave estiver ausente), evitando que o primeiro boot abra preso em 4:3 (`256×224`).
- **Controle real de Widescreen no Launcher:** Sincronizei `widescreen` entre `g_config` e o modelo do launcher, adicionei o checkbox **Widescreen (16:9)** na aba **Settings → Display** e corrigi o badge **ASPECT** da aba **Play** para refletir o estado real do widescreen e do aspecto de pixels.

---

## [1.0.0] - 2026-11-27

### 🏰 Novo Launcher Autoral & Botões do Super Nintendo
- **Interface autoral (*Palace Night*):** Desenhei um novo shell completo em `recomp-ui` com temática própria de palácio árabe (arcos, lanternas, estrela geométrica e emblema da lâmpada), organizado nas abas **Play**, **Settings**, **Controls**, **Mods** e **About**.
- **Controles visuais do Super Nintendo:** Substituí as dicas genéricas no rodapé por ícones visuais dos botões do SNES (**`L` / `R`** para trocar de aba, **`( B )`** para confirmar e **`( A )`** para voltar, além de `Enter` / `Esc`).
- **Configuração inicial refinada:** O jogo passa a iniciar em **Tela Cheia (Borderless)** por padrão (`Fullscreen = 1`, alternável no launcher) e com o contador de FPS na janela (`DisplayPerfInTitle = 0`) desativado por padrão, podendo ser ligado direto na aba **Settings**.

### 🔄 Auto-Updater Seguro via GitHub Releases
- **Verificação automática ao abrir:** Ao iniciar o launcher, consulto a API pública do repositório `testsenvergonha-sys/AgrabahPort` sem travar a interface e exibo o modal de atualização com notas de release quando houver versão mais nova.
- **Preservação rígida dos dados do usuário:** Antes de aplicar qualquer pacote, crio backup automático de `saves/` em `backups/pre_update_<versão>/` e removo da extração qualquer entrada protegida (`saves/`, `mods/`, `states/`, `config.ini`, `keybinds.ini`, `rom.cfg`, `portable.txt`, `.sfc`/`.smc`/`.srm`), gerando também `AgrabahPort.exe.old` para rollback seguro.

### 🎞️ Widescreen Adaptativo 16:9 e Correções Visuais
- **Cobertura widescreen nas fases finais e chefes:** Implementei suporte adaptativo 16:9 com provedores de BG2 em `src/main.c` e correções de PPU/raster em `snesrecomp` para Caverna das Maravilhas (feixe da lâmpada), Escape no Tapete Mágico, fase da Lava (BG2 wave) e confrontos finais contra Jafar, limpando corretamente faixas laterais quando o mapa não realiza streaming contínuo.

### ⚡ Turbo Fluido e Ritmo de Quadros
- **Correção do modo Turbo (`Tab`):** Desacoplei a cadência do VSync durante o fast-forward e introduzi um portão temporal de apresentação (~50 Hz) para evitar engasgos pesados em modo Turbo, restaurando instantaneamente a sincronia de 60.0 FPS ao soltar `Tab`.

---

## [0.1.3] - 2026-11-19

### Framepacing Perfeito e 60.0 FPS Travados
- **Desacoplamento de Frame Delay (`DisableFrameDelay = 1`):** Resolvido o problema de micro-engasgos causados por conflito entre a espera de software (`SDL_Delay`) e o VSync do monitor. A taxa de quadros agora corre sincronizada com o hardware em **60.0 FPS cravados**.
- **Frequência de Simulação Ajustada (`simulation_hz = 60.0`):** Sincronização exata entre o relógio da máquina virtual do SNES e monitores de 60 Hz modernos, eliminando descompassos de taxa de atualização.
- **Backend de Renderização Otimizado (`SDL` / Direct3D 11):** Configuração nativa de hardware flip model no Windows, garantindo altíssimo desempenho mesmo em placas integradas.

### Resolução e Proporção Estável (Zero Artefatos Visuais)
- **Apresentação Autêntica 4:3 e 8:7:** O renderizador agora exibe as proporções fiéis ao hardware original (4:3 TV CRT e 8:7 Pixels Quadrados), perfeitamente centralizadas em telas 16:9 modernas com laterais pretas limpas (*pillarbox*).
- **Fim da Repetição de Tiles nas Bordas:** Eliminação definitiva de artefatos visuais de repetição/wrap do cenário que ocorriam ao forçar 16:9 em um motor de jogo cuja VRAM produz 256 pixels horizontais.
- **Telas Estáticas Limpas:** Logos da Capcom e Disney, bem como tela de título e introdução de fases, agora abrem sem barras de repetição nas margens.

### Correção de Boot / Watchdog no Quadro 784
- **Eliminação de Travamento:** Corrigido o travamento no quadro 784 após o logo da Capcom através da limpeza e regeneração integral das rotinas C puras do recompiler.

---

## [0.1.2] - 2026-11-19

### Modo Portátil e Experiência do Jogador (Player-First)
- **Modo 100% Portátil (`portable.txt`):** O jogo agora pode ser executado a partir de qualquer pasta ou pendrive. Todas as configurações (`config.ini`), estados salvos e saves de bateria (`saves/`) são mantidos dentro do próprio diretório do jogo, sem espalhar arquivos pelo sistema (`AppData` ou `Documents`).
- **Detecção Automática da ROM:** Ao abrir, o executável procura automaticamente pela ROM (`Aladdin.sfc` ou `Aladdin.smc`) na mesma pasta do executável. Se a ROM estiver presente, o jogo inicia instantaneamente em menos de 1 segundo.
- **Executável Jogável Standalone (Windows):** Pacote pré-compilado leve contendo o executável nativo direto, eliminando a necessidade de compilação demorada pelo jogador no primeiro uso.
- **Documentação Redesenhada:** Guias rápidos de início, tabelas visuais de controles para Teclado e Gamepads (Xbox, PlayStation, Switch), especificações de ROM e FAQ.