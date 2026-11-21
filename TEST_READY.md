# TEST READY: 4-Tier Automated E2E Headless Test Suite

**Status**: READY & VERIFIED  
**Date**: 2026-11-20  
**Target Executable**: `build-mingw/AgrabahPort.exe`  
**Test Runner**: `tests/run_headless_tests.py`  
**Verification Framework**: Opaque-box script runner (`--script`), OAM/WRAM/CGRAM binary assertion engine (`tests/test_assertions.py`), and 60 FPS pacing / memory stability audit.

---

## 1. Test Suite Summary by Tier

| Tier | Category | Tests | Description | Result |
|---|---|:---:|---|:---:|
| **Tier 1** | **Feature Coverage** | 6 | P2 independent ammo, projectile spawn & trajectory, vertical rope climb, horizontal hang & shimmy, Prince Ali Palette 5, Abu suppression | **6 / 6 PASS (100%)** |
| **Tier 2** | **Boundary & Corner Cases** | 5 | 0-ammo depletion boundary, rapid multi-button spam, screen edge clamping (left/right), rope endpoints (top anchor & ground dismount), simultaneous opposing movement | **5 / 5 PASS (100%)** |
| **Tier 3** | **Cross-Feature Combinations** | 3 | Apple throw while hanging/climbing on ropes, aerial jump throw with trajectory retention, apple projectile enemy collision during P1 walking | **3 / 3 PASS (100%)** |
| **Tier 4** | **Real-World Scenarios** | 2 | Full multi-phase Agrabah Market gameplay playthrough (572+ frames), continuous 60.0 Hz frame pacing, zero memory leaks (growth < 0.5 MB) | **2 / 2 PASS (100%)** |
| **TOTAL** | **Full E2E Suite** | **16** | **All 4 Tiers of automated test coverage** | **16 / 16 PASS (100%)** |

---

## 2. Test Execution Command

From the repository root (`AladdinSNESRecomp`):

### Run All 16 Tests
```powershell
python tests/run_headless_tests.py --tier all --verbose
```

### Run by Tier
```powershell
python tests/run_headless_tests.py --tier 1
python tests/run_headless_tests.py --tier 2
python tests/run_headless_tests.py --tier 3
python tests/run_headless_tests.py --tier 4
```

### Options
- `--tier {1,2,3,4,all}`: Select target tier (default: `all`).
- `--exe <path>`: Path to `AgrabahPort.exe` (defaults to auto-detecting `build-mingw/AgrabahPort.exe`).
- `--rom <path>`: Path to `Aladdin.sfc` (defaults to auto-detecting).
- `--dump-dir <path>`: Custom output folder for scene binary dumps (default: `tests/dumps/`).
- `--verbose`, `-v`: Output detailed assertion messages.

---

## 3. Inventory of Delivered Test Files

### Runner & Assertion Engine
- `tests/run_headless_tests.py`: CLI driver, subprocess orchestrator, process memory tracker, timing validator.
- `tests/test_assertions.py`: Binary parser for `.oam.bin` (544B), `.wram.bin` (128KB), `.cgram.bin` (512B), `.info.json`, domain extractors, and assertion rules.

### Headless Script Files (`tests/scripts/`)
1. `tests/scripts/t1_abu_suppression.txt`
2. `tests/scripts/t1_prince_ali_palette.txt`
3. `tests/scripts/t1_ammo_decoupling.txt`
4. `tests/scripts/t1_apple_projectile.txt`
5. `tests/scripts/t1_vertical_rope_climb.txt`
6. `tests/scripts/t1_horizontal_hang_shimmy.txt`
7. `tests/scripts/t2_zero_apples_boundary.txt`
8. `tests/scripts/t2_rapid_throw_inputs.txt`
9. `tests/scripts/t2_screen_edge_boundaries.txt`
10. `tests/scripts/t2_rope_boundary_limits.txt`
11. `tests/scripts/t2_simultaneous_movement.txt`
12. `tests/scripts/t3_throw_while_climbing.txt`
13. `tests/scripts/t3_jump_throw.txt`
14. `tests/scripts/t3_apple_hit_enemy_p1_walk.txt`
15. `tests/scripts/t4_agrabah_playthrough.txt`
16. `tests/scripts/t4_60fps_stability_and_memory.txt`

### Documentation
- `TEST_INFRA.md`: Full architectural specification of the test infrastructure, memory contracts, and pass/fail criteria.
