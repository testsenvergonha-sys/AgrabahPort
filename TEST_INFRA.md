# Aladdin SNES Recomp: 4-Tier Automated Test Infrastructure (TEST_INFRA)

## 1. Overview & Architecture

This document defines the automated, opaque-box test infrastructure for the **Aladdin SNES Recomp Dual Co-op Enhancement** (`feat/dual-aladdin-coop`).

The test suite validates the decoupling, mechanics, visual integrity, and runtime performance of Player 2 (Prince Ali) without modifying production implementation code. Testing is conducted via headless simulation driven by `AgrabahPort.exe --script <script.txt>`, asserting on binary memory dumps (OAM, WRAM, CGRAM) and runtime telemetry (`info.json`, host execution pacing, memory usage).

```
+-------------------------------------------------------------------------+
|                         Headless Test Runner                            |
|                    (tests/run_headless_tests.py)                        |
+------------------------------------+------------------------------------+
                                     |
                                     v
                   +-----------------------------------+
                   |   AgrabahPort.exe (build-mingw)   |
                   |   CLI: --script <script.txt>      |
                   +-----------------+-----------------+
                                     |
               +---------------------+---------------------+
               |                     |                     |
               v                     v                     v
        +--------------+      +--------------+      +--------------+
        |   OAM Dump   |      |  WRAM Dump   |      |  Telemetry   |
        |  (.oam.bin)  |      | (.wram.bin)  |      | (.info.json) |
        +-------+------+      +------+-------+      +------+-------+
                |                    |                     |
                v                    v                     v
        +----------------------------------------------------------+
        |                    Test Assertions                       |
        |              (tests/test_assertions.py)                  |
        +----------------------------------------------------------+
```

---

## 2. Headless Script Grammar & Driver Interface

The recompiler desktop runner supports deterministic headless scripting via `--script <path>`. All test scripts reside in `tests/scripts/`.

### Supported Commands
| Command | Arguments | Description |
|---|---|---|
| `loadstate` | `<slot>` | Loads save state `saves/save<slot>.sav` (Slot 1: Agrabah Market level 1). |
| `wait` | `<frames>` | Advances simulation by specified frame count without input. |
| `press` | `<mask(+mask...)> [frames]` | Holds button mask for N frames (default 1). Prefix `p2:` targets Controller 2. |
| `dump` | `<tag>` | Dumps full PPU/CPU/WRAM state into `<tag>.{oam,wram,cgram,info}.bin/json`. |
| `quit` | - | Cleanly terminates process with return code 0. |

### Button Mask Grammar
- **Player 1**: `up`, `down`, `left`, `right`, `a`, `b`, `x`, `y`, `l`, `r`, `start`, `select`.
- **Player 2**: `p2:up`, `p2:down`, `p2:left`, `p2:right`, `p2:a`, `p2:b`, `p2:x`, `p2:y`, `p2:l`, `p2:r`, `p2:start`, `p2:select`.
- **Simultaneous Input**: Combinations joined by `+` (e.g. `left+p2:right 30`, `b+p2:b 12`, `p2:up+p2:b 8`).

---

## 3. Binary Memory Layout & Inspection Contracts

### 3.1 OAM Structure (`.oam.bin`, 544 bytes)
- **Primary Table (Offsets 0..511)**: 128 sprites $\times$ 4 bytes:
  - `Byte 0`: Screen X coordinate low 8 bits ($0..255$).
  - `Byte 1`: Screen Y coordinate ($0..223$ visible, $\ge 224$ offscreen/suppressed).
  - `Byte 2`: Tile Index ($0..255$).
  - `Byte 3`: Attribute flags:
    - Bits 1..3: Sprite Palette ($0..7$).
    - Bits 4..5: Priority ($0..3$).
    - Bit 6: Horizontal flip ($0 = \text{normal}, 1 = \text{mirrored}$).
    - Bit 7: Vertical flip ($0 = \text{normal}, 1 = \text{inverted}$).
- **High Table (Offsets 512..543)**: 128 sprites $\times$ 2 bits:
  - Bit 0: X coordinate 9th bit (sign / wrap-around).
  - Bit 1: Size bit ($0 = 8\times 8\text{ px}, 1 = 16\times 16\text{ px}$).

### 3.2 Slot Partitioning Contract
| OAM Slots | Owner / Purpose | Invariants & Expected Values |
|---|---|---|
| **0 .. 3** | Suppressed Abu | $Y \ge 224$ unconditionally (Palette 1 forced offscreen). |
| **4 .. 31** | Player 1 (Classic Aladdin) | Palette 0 (`(attr >> 1) & 7 == 0`). |
| **16 .. 79** | Dynamic actors & enemies | Palettes 3, 4, 7. When defeated, $Y \ge 224$. |
| **80 .. 92** | Player 2 (Prince Ali) | Palette 5 (`(attr >> 1) & 7 == 5`). Active pieces $\ge 6$. |
| **93 .. 95** | Player 2 HUD Hearts | Palette 5, fixed $Y=16$, $X \in \{210, 222, 234\}$. |
| **96 .. 99** | Player 2 Apple Projectiles | 16x16 size (`size_bit == 1`), tiles `0xD2/0xD3`, active during flight. |
| **100 .. 102**| Player 2 Apple HUD | Mini apple icon + decimal digits, top right screen. |

### 3.3 WRAM Addresses (`.wram.bin`, 131,072 bytes)
- `g_ram[0x0369]`: Player 1 Apple ammo in BCD (e.g. `0x10` = 16). **Must remain constant during P2 throws.**
- `g_ram[0x0367]`: Player 1 Health (hearts, 1..3).
- `g_ram[0x08E7..0x08E8]`: Player 1 16-bit World X.
- `g_ram[0x08EA..0x08EB]`: Player 1 16-bit World Y.
- `g_ram[0x19ED..0x19EE]`: Camera Viewport X.
- `g_ram[0x19F1..0x19F2]`: Camera Viewport Y.
- `g_ram[0x0B5B..0x1400]`: Dynamic actor pool (enemies). Offset `+0x42` = Enemy HP.

---

## 4. Test Suite Taxonomy (4 Tiers)

### Tier 1: Feature Coverage (Base Mechanics)
1. `t1_abu_suppression`: Asserts Abu OAM slots 0..3 have $Y \ge 224$.
2. `t1_prince_ali_palette`: Asserts all active P2 OAM slots 80..92 use Palette 5, while P1 slots 4..31 use Palette 0. Asserts CGRAM words 208..223 match Prince Ali colors (Royal Blue, Silk Gold, Crimson Fez).
3. `t1_ammo_decoupling`: Asserts P2 apple throwing does NOT decrement P1 ammo at `g_ram[0x0369]`.
4. `t1_apple_projectile`: Asserts projectile spawn in slots 96..99 with 16x16 size, forward horizontal velocity, and gravitational downward arc.
5. `t1_vertical_rope_climb`: Asserts P2 attaches to vertical rope, locks X axis, and ascends/descends via D-Pad Up/Down with authentic climb metasprites.
6. `t1_horizontal_hang_shimmy`: Asserts P2 attaches to horizontal clothesline/net and shimmies left/right with authentic two-handed hang metasprites.

### Tier 2: Boundary & Corner Cases
1. `t2_zero_apples_boundary`: P2 throws all 10 apples. On 11th throw with 0 ammo, asserts no projectile spawns, ammo does not underflow, and P2 does not freeze.
2. `t2_rapid_throw_inputs`: Consecutive rapid throw inputs across 4 frames. Asserts projectile pool allocates up to 4 slots cleanly without crashing or heap overflow.
3. `t2_screen_edge_boundaries`: P2 moves hard left and hard right. Asserts screen clamping ($12 \le \text{screen\_x} \le 244$) prevents wrapping or visual disintegration.
4. `t2_rope_boundary_limits`: P2 climbs to top anchor and bottom frayed end. Asserts climbing clamps at top anchor and dismounts cleanly at ground floor.
5. `t2_simultaneous_movement`: Simultaneous opposing inputs for P1 and P2 (`left+p2:right`, `b+p2:b`). Asserts both players move independently without state interference.

### Tier 3: Cross-Feature Combinations
1. `t3_throw_while_climbing`: P2 on vertical rope presses Throw. Asserts projectile spawns at rope anchor without detaching P2 or corrupting climbing state.
2. `t3_jump_throw`: P2 jumps into air and presses Throw. Asserts P2 enters Air Throw state, maintains aerial velocity, and launches projectile.
3. `t3_apple_hit_enemy_p1_walk`: P2 throws apple hitting active enemy while P1 walks forward. Asserts enemy sprite moves to $Y=225$, enemy HP clears, projectile despawns, and P1 continues walking smoothly.

### Tier 4: Real-World Scenarios & Performance Stability
1. `t4_agrabah_playthrough`: Full multi-phase gameplay sequence (running, jumping, throwing apples at guards, climbing ropes, navigation) executing cleanly over 600+ frames.
2. `t4_60fps_stability_and_memory`: Validates continuous 60.0 Hz frame pacing and monitors process memory usage before and after execution to guarantee zero memory leaks.

---

## 5. Execution Guide

### Automated Run Command
From repository root:
```powershell
python tests/run_headless_tests.py --tier all --verbose
```

### Filtering by Tier
```powershell
python tests/run_headless_tests.py --tier 1
python tests/run_headless_tests.py --tier 2
python tests/run_headless_tests.py --tier 3
python tests/run_headless_tests.py --tier 4
```

### Custom Executable or ROM
```powershell
python tests/run_headless_tests.py --exe build-mingw/AgrabahPort.exe --rom C:/roms/Aladdin.sfc
```

### Exit Codes
- `0`: All tests passed successfully.
- `1`: One or more test assertions failed.
- `2`: Configuration or environment error (missing executable, ROM, or save state).
