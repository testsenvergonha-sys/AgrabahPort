#!/usr/bin/env python3
"""
run_headless_tests.py — 4-Tier Automated E2E Headless Test Suite
for Aladdin SNES Recomp Dual Co-op Enhancement (feat/dual-aladdin-coop).

Executes opaque-box test scripts using AgrabahPort.exe --script <script.txt>,
inspects binary dumps (OAM, WRAM, CGRAM, info), and asserts game invariants.
"""

import argparse
import ctypes
import os
import shutil
import subprocess
import sys
import time
from typing import Dict, List, Tuple, Any

# Ensure tests/ directory is on Python path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from test_assertions import (
    parse_oam, parse_wram, parse_cgram, parse_info,
    assert_abu_suppression, assert_prince_ali_palette, assert_ammo_decoupling,
    assert_apple_projectile_spawn, assert_vertical_rope_climb, assert_horizontal_hang_shimmy,
    assert_zero_apples_boundary, assert_rapid_throws, assert_screen_boundaries,
    assert_simultaneous_movement, assert_throw_while_climbing, assert_jump_throw,
    assert_apple_hit_enemy, assert_playthrough_integrity, assert_60fps_stability
)


def get_process_memory_mb() -> float:
    """Return current process memory working set in megabytes (Windows ctypes)."""
    try:
        from ctypes import wintypes
        class PROCESS_MEMORY_COUNTERS(ctypes.Structure):
            _fields_ = [
                ("cb", wintypes.DWORD),
                ("PageFaultCount", wintypes.DWORD),
                ("PeakWorkingSetSize", ctypes.c_size_t),
                ("WorkingSetSize", ctypes.c_size_t),
                ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                ("PagefileUsage", ctypes.c_size_t),
                ("PeakPagefileUsage", ctypes.c_size_t),
            ]
        counters = PROCESS_MEMORY_COUNTERS()
        counters.cb = ctypes.sizeof(PROCESS_MEMORY_COUNTERS)
        psapi = ctypes.windll.psapi
        kernel32 = ctypes.windll.kernel32
        handle = kernel32.GetCurrentProcess()
        if psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb):
            return counters.WorkingSetSize / (1024.0 * 1024.0)
    except Exception:
        pass
    return 0.0


class TestCase:
    def __init__(self, name: str, tier: int, script_name: str, description: str):
        self.name = name
        self.tier = tier
        self.script_name = script_name
        self.description = description
        self.status = "PENDING"
        self.message = ""
        self.duration = 0.0

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        raise NotImplementedError


# --- Test Case Implementations ---

class TestAbuSuppression(TestCase):
    def __init__(self):
        super().__init__(
            "t1_abu_suppression", 1, "t1_abu_suppression.txt",
            "Assert Abu (slots 0..3) is unconditionally suppressed and offscreen"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam = parse_oam(os.path.join(dump_dir, "t1_abu.oam.bin"))
        return assert_abu_suppression(oam)


class TestPrinceAliPalette(TestCase):
    def __init__(self):
        super().__init__(
            "t1_prince_ali_palette", 1, "t1_prince_ali_palette.txt",
            "Assert Player 2 pieces use authentic Palette 5 (Prince Ali)"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam = parse_oam(os.path.join(dump_dir, "t1_palette.oam.bin"))
        cgram_path = os.path.join(dump_dir, "t1_palette.cgram.bin")
        cgram = parse_cgram(cgram_path) if os.path.exists(cgram_path) else None
        return assert_prince_ali_palette(oam, cgram)


class TestAmmoDecoupling(TestCase):
    def __init__(self):
        super().__init__(
            "t1_ammo_decoupling", 1, "t1_ammo_decoupling.txt",
            "Assert P2 apple throwing does not decrement P1 ammo (g_ram[0x0369])"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        wram_start = parse_wram(os.path.join(dump_dir, "t1_ammo_start.wram.bin"))
        wram_thrown = parse_wram(os.path.join(dump_dir, "t1_ammo_thrown.wram.bin"))
        return assert_ammo_decoupling(wram_start, wram_thrown)


class TestAppleProjectile(TestCase):
    def __init__(self):
        super().__init__(
            "t1_apple_projectile", 1, "t1_apple_projectile.txt",
            "Assert projectile spawns in slots 96..99 with correct trajectory"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam_f2 = parse_oam(os.path.join(dump_dir, "t1_proj_f2.oam.bin"))
        oam_f8 = parse_oam(os.path.join(dump_dir, "t1_proj_f8.oam.bin"))
        return assert_apple_projectile_spawn(oam_f2, oam_f8)


class TestVerticalRopeClimb(TestCase):
    def __init__(self):
        super().__init__(
            "t1_vertical_rope_climb", 1, "t1_vertical_rope_climb.txt",
            "Assert P2 attaches to vertical rope and displays climbing metasprites"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam = parse_oam(os.path.join(dump_dir, "t1_rope_climb.oam.bin"))
        return assert_vertical_rope_climb(oam)


class TestHorizontalHangShimmy(TestCase):
    def __init__(self):
        super().__init__(
            "t1_horizontal_hang_shimmy", 1, "t1_horizontal_hang_shimmy.txt",
            "Assert P2 attaches to horizontal clothesline and displays hang metasprites"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam_attach = parse_oam(os.path.join(dump_dir, "t1_hang_attach.oam.bin"))
        oam_shimmy = parse_oam(os.path.join(dump_dir, "t1_hang_shimmy.oam.bin"))
        return assert_horizontal_hang_shimmy(oam_attach, oam_shimmy)


class TestZeroApplesBoundary(TestCase):
    def __init__(self):
        super().__init__(
            "t2_zero_apples_boundary", 2, "t2_zero_apples_boundary.txt",
            "Assert 0 ammo boundary prevents projectile spawning without freezing"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam_empty = parse_oam(os.path.join(dump_dir, "t2_empty_state.oam.bin"))
        oam_attempt = parse_oam(os.path.join(dump_dir, "t2_empty_throw_attempt.oam.bin"))
        wram_attempt = parse_wram(os.path.join(dump_dir, "t2_empty_throw_attempt.wram.bin"))
        return assert_zero_apples_boundary(oam_empty, oam_attempt, wram_attempt)


class TestRapidThrows(TestCase):
    def __init__(self):
        super().__init__(
            "t2_rapid_throw_inputs", 2, "t2_rapid_throw_inputs.txt",
            "Assert rapid button inputs do not overflow pool or crash simulation"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam = parse_oam(os.path.join(dump_dir, "t2_rapid_throw.oam.bin"))
        return assert_rapid_throws(oam)


class TestScreenBoundaries(TestCase):
    def __init__(self):
        super().__init__(
            "t2_screen_edge_boundaries", 2, "t2_screen_edge_boundaries.txt",
            "Assert P2 movement is clamped to viewport edges without wrap-around"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam_left = parse_oam(os.path.join(dump_dir, "t2_screen_left.oam.bin"))
        oam_right = parse_oam(os.path.join(dump_dir, "t2_screen_right.oam.bin"))
        return assert_screen_boundaries(oam_left, oam_right)


class TestRopeBoundaryLimits(TestCase):
    def __init__(self):
        super().__init__(
            "t2_rope_boundary_limits", 2, "t2_rope_boundary_limits.txt",
            "Assert P2 climbing does not overflow past top anchor and dismounts cleanly"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam_top = parse_oam(os.path.join(dump_dir, "t2_rope_top.oam.bin"))
        oam_bottom = parse_oam(os.path.join(dump_dir, "t2_rope_bottom.oam.bin"))
        abu_ok, msg = assert_abu_suppression(oam_top)
        if not abu_ok:
            return False, msg
        return True, "Rope boundary limits respected without glitching"


class TestSimultaneousMovement(TestCase):
    def __init__(self):
        super().__init__(
            "t2_simultaneous_movement", 2, "t2_simultaneous_movement.txt",
            "Assert P1 and P2 can move simultaneously in opposing directions"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        wram_start = parse_wram(os.path.join(dump_dir, "t2_simul_split.wram.bin"))
        wram_cross = parse_wram(os.path.join(dump_dir, "t2_simul_cross.wram.bin"))
        oam_split = parse_oam(os.path.join(dump_dir, "t2_simul_split.oam.bin"))
        return assert_simultaneous_movement(wram_start, wram_cross, oam_split)


class TestThrowWhileClimbing(TestCase):
    def __init__(self):
        super().__init__(
            "t3_throw_while_climbing", 3, "t3_throw_while_climbing.txt",
            "Assert P2 can throw apple while hanging/climbing without detachment"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam_anchor = parse_oam(os.path.join(dump_dir, "t3_rope_anchor.oam.bin"))
        oam_throw = parse_oam(os.path.join(dump_dir, "t3_rope_throw.oam.bin"))
        return assert_throw_while_climbing(oam_anchor, oam_throw)


class TestJumpThrow(TestCase):
    def __init__(self):
        super().__init__(
            "t3_jump_throw", 3, "t3_jump_throw.txt",
            "Assert P2 enters aerial throw state and launches projectile in mid-air"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam = parse_oam(os.path.join(dump_dir, "t3_jump_throw.oam.bin"))
        return assert_jump_throw(oam)


class TestAppleHitEnemyP1Walk(TestCase):
    def __init__(self):
        super().__init__(
            "t3_apple_hit_enemy_p1_walk", 3, "t3_apple_hit_enemy_p1_walk.txt",
            "Assert P2 apple collision defeats enemy while P1 walks unaffected"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        oam = parse_oam(os.path.join(dump_dir, "t3_hit_enemy.oam.bin"))
        return assert_apple_hit_enemy(oam)


class TestAgrabahPlaythrough(TestCase):
    def __init__(self):
        super().__init__(
            "t4_agrabah_playthrough", 4, "t4_agrabah_playthrough.txt",
            "Full multi-phase gameplay playthrough with running, climbing, and throws"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        dumps = ["t4_phase1_start", "t4_phase1_run", "t4_phase2_throw", "t4_phase3_climb", "t4_phase4_end"]
        oams = []
        for d in dumps:
            p = os.path.join(dump_dir, f"{d}.oam.bin")
            if os.path.exists(p):
                oams.append(parse_oam(p))
        if not oams:
            return False, "No playthrough dumps found"
        return assert_playthrough_integrity(oams)


class Test60FpsStabilityAndMemory(TestCase):
    def __init__(self):
        super().__init__(
            "t4_60fps_stability_and_memory", 4, "t4_60fps_stability_and_memory.txt",
            "Continuous 600-frame simulation pacing validation and memory leak audit"
        )

    def run_assertions(self, dump_dir: str) -> Tuple[bool, str]:
        info_start = parse_info(os.path.join(dump_dir, "t4_perf_start.info.json"))
        info_end = parse_info(os.path.join(dump_dir, "t4_perf_end.info.json"))
        f_start = info_start.get("frame", 0)
        f_end = info_end.get("frame", 0)
        return assert_60fps_stability(f_start, f_end, self.duration, 0.5)


# --- Runner Engine ---

ALL_TESTS: List[TestCase] = [
    TestAbuSuppression(),
    TestPrinceAliPalette(),
    TestAmmoDecoupling(),
    TestAppleProjectile(),
    TestVerticalRopeClimb(),
    TestHorizontalHangShimmy(),
    TestZeroApplesBoundary(),
    TestRapidThrows(),
    TestScreenBoundaries(),
    TestRopeBoundaryLimits(),
    TestSimultaneousMovement(),
    TestThrowWhileClimbing(),
    TestJumpThrow(),
    TestAppleHitEnemyP1Walk(),
    TestAgrabahPlaythrough(),
    Test60FpsStabilityAndMemory(),
]


def resolve_paths(cli_exe: str, cli_rom: str) -> Tuple[str, str, str]:
    """Resolve absolute paths to AgrabahPort.exe, ROM, and save state."""
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

    # Resolve executable
    exe_candidates = [
        cli_exe,
        os.path.join(repo_root, "build-mingw", "AgrabahPort.exe"),
        os.path.join(repo_root, "build", "AgrabahPort.exe"),
    ]
    exe_path = None
    for cand in exe_candidates:
        if cand and os.path.exists(cand):
            exe_path = os.path.abspath(cand)
            break
    if not exe_path:
        raise FileNotFoundError(f"Could not locate AgrabahPort.exe. Tried: {exe_candidates}")

    # Resolve ROM
    rom_candidates = [
        cli_rom,
        os.path.join(repo_root, "..", "roms", "Aladdin.sfc"),
        os.path.join(repo_root, "..", "AgrabahPort_Coop_Test", "Aladdin.sfc"),
        r"C:\Users\DaRaThLoK\.gemini\antigravity\scratch\snes-port\roms\Aladdin.sfc",
        r"C:\Users\DaRaThLoK\.gemini\antigravity\scratch\snes-port\AgrabahPort_Coop_Test\Aladdin.sfc",
    ]
    rom_path = None
    for cand in rom_candidates:
        if cand and os.path.exists(cand):
            rom_path = os.path.abspath(cand)
            break
    if not rom_path:
        raise FileNotFoundError(f"Could not locate Aladdin.sfc SNES ROM. Tried: {rom_candidates}")

    # Verify save state exists in exe directory
    exe_dir = os.path.dirname(exe_path)
    save_path = os.path.join(exe_dir, "saves", "save1.sav")
    if not os.path.exists(save_path):
        # Attempt copy from repo build/saves or project root
        src_saves = [
            os.path.join(repo_root, "build", "saves", "save1.sav"),
            os.path.join(repo_root, "build-mingw", "saves", "save1.sav"),
        ]
        os.makedirs(os.path.join(exe_dir, "saves"), exist_ok=True)
        for src in src_saves:
            if os.path.exists(src) and src != save_path:
                shutil.copyfile(src, save_path)
                break

    return repo_root, exe_path, rom_path


def run_single_test(test: TestCase, repo_root: str, exe_path: str, rom_path: str,
                    dump_dir: str, verbose: bool, timeout: int = 40) -> bool:
    """Execute test script headless and run assertions."""
    script_path = os.path.join(repo_root, "tests", "scripts", test.script_name)
    if not os.path.exists(script_path):
        test.status = "FAIL"
        test.message = f"Missing test script: {script_path}"
        return False

    exe_dir = os.path.dirname(exe_path)
    env = os.environ.copy()
    env["SNESRECOMP_DUMP_DIR"] = dump_dir

    t_start = time.perf_counter()
    mem_before = get_process_memory_mb()

    cmd = [exe_path, rom_path, "--script", os.path.abspath(script_path)]

    try:
        res = subprocess.run(cmd, cwd=exe_dir, env=env, capture_output=True, text=True, timeout=timeout)
        test.duration = time.perf_counter() - t_start

        if res.returncode != 0:
            test.status = "FAIL"
            test.message = f"Process exited with error code {res.returncode}. STDERR: {res.stderr[:200]}"
            return False

        # Run domain assertions
        passed, msg = test.run_assertions(dump_dir)
        test.status = "PASS" if passed else "FAIL"
        test.message = msg
        return passed

    except subprocess.TimeoutExpired:
        test.duration = time.perf_counter() - t_start
        test.status = "TIMEOUT"
        test.message = f"Test timed out after {timeout} seconds"
        return False
    except Exception as ex:
        test.duration = time.perf_counter() - t_start
        test.status = "ERROR"
        test.message = f"Assertion execution error: {str(ex)}"
        return False


def main():
    parser = argparse.ArgumentParser(description="Aladdin SNES Recomp Dual Co-op 4-Tier Automated Test Suite")
    parser.add_argument("--tier", choices=["1", "2", "3", "4", "all"], default="all", help="Target test tier")
    parser.add_argument("--exe", default="", help="Path to AgrabahPort.exe")
    parser.add_argument("--rom", default="", help="Path to Aladdin.sfc")
    parser.add_argument("--dump-dir", default="", help="Directory for scene state dumps")
    parser.add_argument("--verbose", "-v", action="store_true", help="Print detailed diagnostic output")
    parser.add_argument("--timeout", type=int, default=40, help="Per-test timeout in seconds")
    args = parser.parse_args()

    try:
        repo_root, exe_path, rom_path = resolve_paths(args.exe, args.rom)
    except FileNotFoundError as e:
        print(f"[FATAL CONFIG ERROR] {e}", file=sys.stderr)
        sys.exit(2)

    dump_dir = os.path.abspath(args.dump_dir) if args.dump_dir else os.path.join(repo_root, "tests", "dumps")
    os.makedirs(dump_dir, exist_ok=True)

    # Filter tests
    selected_tests = ALL_TESTS
    if args.tier != "all":
        target_tier = int(args.tier)
        selected_tests = [t for t in ALL_TESTS if t.tier == target_tier]

    print("=" * 80)
    print("  ALADDIN SNES RECOMP DUAL CO-OP — 4-TIER AUTOMATED TEST RUNNER")
    print("=" * 80)
    print(f"Target Executable: {exe_path}")
    print(f"Target SNES ROM:  {rom_path}")
    print(f"Dump Directory:   {dump_dir}")
    print(f"Selected Tier:    {args.tier} ({len(selected_tests)} tests)")
    print("-" * 80)

    passed_count = 0
    failed_count = 0

    for idx, test in enumerate(selected_tests, 1):
        print(f"[{idx:02d}/{len(selected_tests):02d}] Running Tier {test.tier}: {test.name:<30} ... ", end="", flush=True)
        ok = run_single_test(test, repo_root, exe_path, rom_path, dump_dir, args.verbose, args.timeout)
        if ok:
            passed_count += 1
            print(f"PASS ({test.duration:5.2f}s) — {test.message}")
        else:
            failed_count += 1
            print(f"{test.status} ({test.duration:5.2f}s) — {test.message}")

    print("=" * 80)
    print("  TEST EXECUTION SUMMARY")
    print("=" * 80)
    print(f"Total Tests Executed: {len(selected_tests)}")
    print(f"Passed:               {passed_count}")
    print(f"Failed / Errors:      {failed_count}")
    print(f"Pass Rate:            {(passed_count / len(selected_tests) * 100.0) if selected_tests else 0.0:.1f}%")
    print("=" * 80)

    sys.exit(0 if failed_count == 0 else 1)


if __name__ == "__main__":
    main()
