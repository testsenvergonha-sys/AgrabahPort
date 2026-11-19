import os
import zipfile

dist_dir = r"C:\Users\Samsung\.gemini\antigravity\scratch\snes-port\AladdinSNESRecomp\dist"
os.makedirs(dist_dir, exist_ok=True)
zip_path = os.path.join(dist_dir, "agrabahport-0.1.3-windows-x64-portable.zip")
if os.path.exists(zip_path):
    os.remove(zip_path)

build_dir = r"C:\Users\Samsung\.gemini\antigravity\scratch\snes-port\AladdinSNESRecomp\build"
project_dir = r"C:\Users\Samsung\.gemini\antigravity\scratch\snes-port\AladdinSNESRecomp"
prefix = "agrabahport-0.1.3-windows-x64-portable"

config_content = """[General]
Autosave = 0
DisableFrameDelay = 1
SkipLauncher = 0
RunAhead = 0
DisplayPerfInTitle = 1

[Graphics]
WindowSize = Auto
Fullscreen = 0
WindowScale = 3
NewRenderer = 1
IgnoreAspectRatio = 0
NoSpriteLimits = 1
Mode7Enabled = 0
Mode7Scale = 8
Mode7Supersample = 1
DisplayAspect = 4:3
OutputMethod = SDL
LinearFiltering = 0
Shader = 
Widescreen = 0
FrameBlend = 0
VSync = 1
Renderer = auto

[Sound]
EnableAudio = 1
Volume = 100
AudioFreq = 32000
AudioChannels = 2
AudioSamples = 512

[KeyMap]
Fullscreen = Alt+Return
Reset = Ctrl+r
Pause = Shift+p
PauseDimmed = p
Turbo = Tab
WindowBigger = Ctrl+Up
WindowSmaller = Ctrl+Down
VolumeUp = Keypad +
VolumeDown = Keypad -
DisplayPerf = f
ToggleRenderer = r
SaveStateMenu = F11
Rewind = F12
OpenLauncher = Escape
Load =      F1,     F2,     F3,     F4,     F5,     F6,     F7,     F8,     F9,     F10
Save = Shift+F1,Shift+F2,Shift+F3,Shift+F4,Shift+F5,Shift+F6,Shift+F7,Shift+F8,Shift+F9,Shift+F10

[GamepadMap]
EnableGamepad1 = true
EnableGamepad2 = false
Controls =   DpadUp, DpadDown, DpadLeft, DpadRight, Back, Start, B, A, Y, X, Lb, Rb
ControlsP2 = DpadUp, DpadDown, DpadLeft, DpadRight, Back, Start, B, A, Y, X, Lb, Rb
GamepadDeadzone = 3277

[Controller]
SourceP1 = 1
SourceP2 = 1

[Netplay]
PlayerName = 

[Rewind]
Enabled = 1
Depth = 50
Interval = 15
"""

how_to_play_pt = """=======================================================
               AGRABAHPORT v0.1.3 (PORTATIL)
      Disney's Aladdin (Super Nintendo, 1993) no PC
=======================================================

COMO JOGAR EM 3 PASSOS:

1. Coloque a sua ROM legal do Aladdin (.sfc ou .smc) nesta mesma pasta.
   - Nome recomendado: Aladdin.sfc ou Aladdin.smc
   - Tamanho exato: 1.310.720 bytes (CRC32: 124d8e4d)

2. De dois cliques em 'AgrabahPort.exe'. O jogo abre direto!

3. Bom jogo! Seus saves e configuracoes ficam 100% salvos aqui mesmo.

-------------------------------------------------------
CONTROLES PADRAO:
-------------------------------------------------------
- Movimentacao:   Setas do Teclado ou D-Pad / Analogico
- Pular:          Z ou Espaco (Controle: Botao A / X)
- Cortar (Espada):X (Controle: Botao X / Quadrado)
- Jogar Maca:     C (Controle: Botao Y / Triangulo)
- Correr:         Shift (Controle: Botao B / Circulo)
- Pausar:         Enter (Controle: Start)
- Menu / Opcoes:  ESC
- Tela Cheia:     Alt + Enter
- Rebobinar:      F12
- Save States:    F11 (Menu Visual) ou Shift+F1..F10
=======================================================
"""

how_to_play_en = """=======================================================
               AGRABAHPORT v0.1.3 (PORTABLE)
      Disney's Aladdin (Super Nintendo, 1993) on PC
=======================================================

HOW TO PLAY IN 3 STEPS:

1. Place your legal Aladdin SNES ROM (.sfc or .smc) in this folder.
   - Recommended filename: Aladdin.sfc or Aladdin.smc
   - Exact size: 1,310,720 bytes (CRC32: 124d8e4d)

2. Double-click 'AgrabahPort.exe'. The game starts instantly!

3. Enjoy! All settings and saves stay 100% inside this folder.

-------------------------------------------------------
DEFAULT CONTROLS:
-------------------------------------------------------
- Movement:       Arrow Keys or D-Pad / Left Stick
- Jump:           Z or Space (Controller: A / Cross)
- Sword Slash:    X (Controller: X / Square)
- Throw Apple:    C (Controller: Y / Triangle)
- Run:            Shift (Controller: B / Circle)
- Pause:          Enter (Controller: Start)
- Settings Menu:  ESC
- Fullscreen:     Alt + Enter
- Rewind:         F12
- Save States:    F11 (Browser) or Shift+F1..F10
=======================================================
"""

with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as z:
    z.write(os.path.join(build_dir, "AgrabahPort.exe"), f"{prefix}/AgrabahPort.exe")
    z.writestr(f"{prefix}/config.ini", config_content)
    keybinds_path = os.path.join(project_dir, "keybinds.ini")
    if os.path.exists(keybinds_path):
        z.write(keybinds_path, f"{prefix}/keybinds.ini")
    z.writestr(f"{prefix}/portable.txt", "portable\n")
    z.write(os.path.join(project_dir, "recomplauncher.json"), f"{prefix}/recomplauncher.json")
    z.writestr(f"{prefix}/COMO JOGAR.txt", how_to_play_pt)
    z.writestr(f"{prefix}/HOW TO PLAY.txt", how_to_play_en)

    assets_dir = os.path.join(project_dir, "assets")
    for root, dirs, files in os.walk(assets_dir):
        for f in files:
            full = os.path.join(root, f)
            rel = os.path.relpath(full, project_dir).replace("\\", "/")
            z.write(full, f"{prefix}/{rel}")

print(f"Zip created: {zip_path} ({os.path.getsize(zip_path)} bytes)")
