# Changelog / Registro de Atualizações

Todas as mudanças notáveis no projeto **AgrabahPort** são documentadas neste arquivo de forma detalhada e transparente.

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