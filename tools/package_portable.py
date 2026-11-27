import os
import sys
import zipfile

project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
build_dir = os.path.join(project_dir, "build")
dist_dir = os.path.join(project_dir, "dist")
cache_path = os.path.join(build_dir, "CMakeCache.txt")

# Copyright / DMCA protection: public distributable archives must NEVER include
# binaries linked against ROM-derived src/gen/*.c code. Only Setup Host builds
# (-DSNESRECOMP_SETUP_HOST=ON) may be packaged for distribution.
setup_host_ok = False
if os.path.exists(cache_path):
    with open(cache_path, "r", encoding="utf-8", errors="ignore") as cf:
        for line in cf:
            if line.strip() == "SNESRECOMP_SETUP_HOST:BOOL=ON":
                setup_host_ok = True
                break

if not setup_host_ok:
    print(
        "ERROR [Copyright/DMCA Guard]: build/CMakeCache.txt does not have "
        "SNESRECOMP_SETUP_HOST:BOOL=ON.\n"
        "Public packages must use Setup Host mode so no ROM-derived src/gen code "
        "is distributed. Configure with -DSNESRECOMP_SETUP_HOST=ON first.",
        file=sys.stderr,
    )
    sys.exit(1)

with open(os.path.join(project_dir, "VERSION"), "r", encoding="utf-8") as vf:
    ver = vf.read().strip()

os.makedirs(dist_dir, exist_ok=True)
prefix = f"agrabahport-{ver}-windows-x64-setup"
zip_path = os.path.join(dist_dir, f"{prefix}.zip")
if os.path.exists(zip_path):
    os.remove(zip_path)

config_content = """[General]
Autosave = 0
DisableFrameDelay = 1
SkipLauncher = 0
RunAhead = 0
DisplayPerfInTitle = 0

[Graphics]
WindowSize = Auto
Fullscreen = 1
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
Shader = assets/shaders/sharp.glslp
Widescreen = 1
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

how_to_play_pt = f"""=======================================================
               AGRABAHPORT v{ver} (SETUP HOST)
          Aladdin (Super Nintendo, 1993) no PC
=======================================================

COMO JOGAR EM 3 PASSOS (SEM ROM INCLUSA):

1. Coloque a sua ROM legal de Aladdin (USA, .sfc/.smc) nesta pasta.
   - Tamanho esperado (headerless): 1.310.720 bytes (CRC32: 124d8e4d)

2. Execute 'AgrabahPort.exe' e siga o assistente inicial do launcher
   para gerar e compilar os arquivos locais a partir da sua ROM.

3. Bom jogo! Seus saves, mods e configuracoes ficam salvos aqui mesmo.
=======================================================
"""

how_to_play_en = f"""=======================================================
               AGRABAHPORT v{ver} (SETUP HOST)
          Aladdin (Super Nintendo, 1993) on PC
=======================================================

HOW TO PLAY IN 3 STEPS (NO ROM INCLUDED):

1. Place your legal Aladdin SNES ROM (USA, .sfc/.smc) in this folder.
   - Expected headerless size: 1,310,720 bytes (CRC32: 124d8e4d)

2. Run 'AgrabahPort.exe' and follow the launcher setup wizard to
   generate and build locally using your own cartridge dump.

3. Enjoy! All settings, mods, and saves stay cleanly in this folder.
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
