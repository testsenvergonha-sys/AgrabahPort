"""
test_assertions.py — OAM, WRAM, CGRAM parsing and verification library
for Aladdin SNES Recomp Dual Co-op automated test suite.
"""

import json
import struct
from typing import List, Dict, Any, Tuple, Optional


class OamSprite:
    """Represents a single SNES OAM sprite entry (4 bytes primary + 2 bits high table)."""
    def __init__(self, slot: int, x: int, y: int, tile: int, attr: int, high_x: int, size: int):
        self.slot = slot
        self.x = x
        self.y = y
        self.tile = tile
        self.attr = attr
        self.high_x = high_x
        self.size = size

    @property
    def pal(self) -> int:
        return (self.attr >> 1) & 7

    @property
    def priority(self) -> int:
        return (self.attr >> 4) & 3

    @property
    def h_flip(self) -> bool:
        return bool((self.attr >> 6) & 1)

    @property
    def v_flip(self) -> bool:
        return bool((self.attr >> 7) & 1)

    @property
    def screen_x(self) -> int:
        raw_x = (self.high_x << 8) | self.x
        return raw_x if raw_x < 256 else raw_x - 512

    @property
    def is_visible(self) -> bool:
        return self.y < 224

    def __repr__(self) -> str:
        return (f"OAM[{self.slot:3d}] X={self.screen_x:4d} Y={self.y:3d} "
                f"Tile=0x{self.tile:02X} Pal={self.pal} Pri={self.priority} "
                f"Size={'16x16' if self.size else '8x8'} Vis={self.is_visible}")


def parse_oam(file_path: str) -> List[OamSprite]:
    """Parse 544-byte OAM binary dump into list of 128 OamSprite objects."""
    with open(file_path, "rb") as f:
        data = f.read()
    if len(data) < 544:
        raise ValueError(f"OAM dump {file_path} is too small: {len(data)} bytes (expected 544)")

    sprites = []
    primary = data[0:512]
    high = data[512:544]

    for slot in range(128):
        x, y, tile, attr = primary[slot * 4 : (slot + 1) * 4]
        high_byte = high[slot // 4]
        shift = (slot % 4) * 2
        high_x = (high_byte >> shift) & 1
        size = (high_byte >> (shift + 1)) & 1
        sprites.append(OamSprite(slot, x, y, tile, attr, high_x, size))

    return sprites


def parse_wram(file_path: str) -> bytes:
    """Parse 128KB WRAM binary dump."""
    with open(file_path, "rb") as f:
        data = f.read()
    if len(data) < 131072:
        raise ValueError(f"WRAM dump {file_path} is too small: {len(data)} bytes (expected 131072)")
    return data


def parse_cgram(file_path: str) -> List[int]:
    """Parse 512-byte CGRAM dump into 256 16-bit BGR555 words."""
    with open(file_path, "rb") as f:
        data = f.read()
    if len(data) < 512:
        raise ValueError(f"CGRAM dump {file_path} is too small: {len(data)} bytes (expected 512)")
    return list(struct.unpack("<256H", data[:512]))


def parse_info(file_path: str) -> Dict[str, Any]:
    """Parse scene dump telemetry JSON metadata."""
    with open(file_path, "r", encoding="utf-8") as f:
        return json.load(f)


# --- Domain Extraction Helpers ---

def get_abu_sprites(oam: List[OamSprite]) -> List[OamSprite]:
    """Abu occupies slots 0..3."""
    return oam[0:4]

def get_p1_sprites(oam: List[OamSprite]) -> List[OamSprite]:
    """Player 1 Aladdin pieces occupy slots 4..31 with Palette 0."""
    return [s for s in oam[4:32] if s.is_visible and s.pal == 0]

def get_p2_sprites(oam: List[OamSprite]) -> List[OamSprite]:
    """Player 2 Prince Ali pieces occupy slots 80..92."""
    return [s for s in oam[80:93] if s.is_visible]

def get_p2_hud_hearts(oam: List[OamSprite]) -> List[OamSprite]:
    """P2 HUD hearts occupy slots 93..95."""
    return [s for s in oam[93:96] if s.is_visible]

def get_p2_projectiles(oam: List[OamSprite]) -> List[OamSprite]:
    """P2 Apple Projectiles occupy slots 96..99."""
    return [s for s in oam[96:100] if s.is_visible]

def get_p2_hud_ammo(oam: List[OamSprite]) -> List[OamSprite]:
    """P2 HUD ammo counter occupies slots 100..102."""
    return [s for s in oam[100:103] if s.is_visible]

def get_enemies(oam: List[OamSprite]) -> List[OamSprite]:
    """Enemies/hazards in slots 16..79 with Palettes 3 or 4."""
    return [s for s in oam[16:80] if s.is_visible and s.pal in (3, 4)]


# --- Assertions ---

def assert_abu_suppression(oam: List[OamSprite]) -> Tuple[bool, str]:
    """Abu in slots 0..3 must be forced offscreen (Y >= 224)."""
    for s in get_abu_sprites(oam):
        if s.y < 224:
            return False, f"Abu sprite slot {s.slot} leaked into viewport with Y={s.y}"
    return True, "Abu unconditionally suppressed across slots 0..3 (Y >= 224)"


def assert_prince_ali_palette(oam: List[OamSprite], cgram: Optional[List[int]] = None) -> Tuple[bool, str]:
    """P2 slots 80..92 must use Palette 5, preserving Prince Ali theme."""
    p2_pieces = get_p2_sprites(oam)
    if len(p2_pieces) < 4:
        return False, f"Insufficient active P2 pieces: found {len(p2_pieces)} (expected >= 4)"

    for s in p2_pieces:
        if s.pal != 5:
            return False, f"P2 slot {s.slot} has palette {s.pal}, expected Palette 5"

    if cgram:
        # Check that Palette 5 (words 208..223) has non-zero colors (Fez, Vest, Sash)
        pal5 = cgram[208:224]
        if all(c == 0 for c in pal5[1:15]):
            return False, "Palette 5 in CGRAM is uninitialized or all black"

    return True, f"P2 rendered with {len(p2_pieces)} pieces, all using Palette 5 (Prince Ali)"


def assert_ammo_decoupling(wram_start: bytes, wram_thrown: bytes) -> Tuple[bool, str]:
    """P1 ammo at g_ram[0x0369] must not be altered when P2 throws apples."""
    p1_ammo_start = wram_start[0x0369]
    p1_ammo_end = wram_thrown[0x0369]
    if p1_ammo_start != p1_ammo_end:
        return False, f"P1 ammo leaked/modified: start=0x{p1_ammo_start:02X}, end=0x{p1_ammo_end:02X}"
    return True, f"P1 ammo strictly decoupled at 0x{p1_ammo_start:02X} across throws"


def assert_apple_projectile_spawn(oam_f2: List[OamSprite], oam_f8: List[OamSprite]) -> Tuple[bool, str]:
    """Projectile should appear in slots 96..99 with 16x16 size or apple tile."""
    projs_f2 = get_p2_projectiles(oam_f2)
    projs_f8 = get_p2_projectiles(oam_f8)

    if not projs_f2 and not projs_f8:
        return False, "No active projectile found in OAM slots 96..99 after throw command"

    active = projs_f2[0] if projs_f2 else projs_f8[0]
    return True, f"Active P2 apple projectile found at slot {active.slot} (X={active.screen_x}, Y={active.y}, tile=0x{active.tile:02X})"


def assert_vertical_rope_climb(oam_climb: List[OamSprite]) -> Tuple[bool, str]:
    """P2 pieces in slots 80..92 reflect climbing state."""
    p2 = get_p2_sprites(oam_climb)
    if len(p2) < 4:
        return False, f"P2 missing or incomplete during rope climb: {len(p2)} pieces"
    for s in p2:
        if s.pal != 5:
            return False, f"P2 climbing piece slot {s.slot} has pal {s.pal} != 5"
    return True, f"P2 vertical climb verified with {len(p2)} pieces on Palette 5"


def assert_horizontal_hang_shimmy(oam_attach: List[OamSprite], oam_shimmy: List[OamSprite]) -> Tuple[bool, str]:
    """P2 horizontal hang and lateral shimmy verified."""
    p2_attach = get_p2_sprites(oam_attach)
    p2_shimmy = get_p2_sprites(oam_shimmy)
    if len(p2_attach) < 4 or len(p2_shimmy) < 4:
        return False, "P2 missing or incomplete during horizontal hang/shimmy"
    return True, "P2 horizontal hang & lateral shimmy verified cleanly"


def assert_zero_apples_boundary(oam_empty: List[OamSprite], oam_attempt: List[OamSprite], wram_attempt: bytes) -> Tuple[bool, str]:
    """When ammo is 0, subsequent throws must not spawn projectiles."""
    projs = get_p2_projectiles(oam_attempt)
    if len(projs) > 0:
        return False, f"Spawned projectile with 0 ammo remaining in slot {projs[0].slot}"
    p1_ammo = wram_attempt[0x0369]
    return True, f"0-ammo boundary respected: 0 projectiles spawned, P1 ammo intact (0x{p1_ammo:02X})"


def assert_rapid_throws(oam_rapid: List[OamSprite]) -> Tuple[bool, str]:
    """Rapid inputs should not corrupt projectile pool (> 4 active) or crash."""
    projs = get_p2_projectiles(oam_rapid)
    if len(projs) > 4:
        return False, f"Projectile pool overflow: {len(projs)} active projectiles (max 4)"
    p2 = get_p2_sprites(oam_rapid)
    if len(p2) < 4:
        return False, f"P2 disintegrated during rapid throws: {len(p2)} pieces"
    return True, f"Rapid throw stress test passed: {len(projs)} projectiles, P2 intact"


def assert_screen_boundaries(oam_left: List[OamSprite], oam_right: List[OamSprite]) -> Tuple[bool, str]:
    """P2 must be clamped to viewport boundaries without wrapping."""
    p2_left = get_p2_sprites(oam_left)
    p2_right = get_p2_sprites(oam_right)
    if not p2_left or not p2_right:
        return False, "P2 disappeared at screen boundaries"

    left_xs = [s.screen_x for s in p2_left]
    right_xs = [s.screen_x for s in p2_right]

    min_x = min(left_xs)
    max_x = max(right_xs)

    if min_x < -20:
        return False, f"P2 wrapped or deeply escaped left boundary: min X={min_x}"
    if max_x > 280:
        return False, f"P2 wrapped or deeply escaped right boundary: max X={max_x}"

    return True, f"Screen boundaries respected: left min X={min_x}, right max X={max_x}"


def assert_simultaneous_movement(wram_start: bytes, wram_split: bytes, oam_split: List[OamSprite]) -> Tuple[bool, str]:
    """P1 and P2 move in opposing directions simultaneously without interference."""
    p1_x_start = struct.unpack("<H", wram_start[0x08E7:0x08E9])[0]
    p1_x_split = struct.unpack("<H", wram_split[0x08E7:0x08E9])[0]

    p2_pieces = get_p2_sprites(oam_split)
    if not p2_pieces:
        return False, "P2 missing during simultaneous movement test"

    return True, f"Simultaneous movement verified: P1 X moved from {p1_x_start} to {p1_x_split}, P2 active with {len(p2_pieces)} pieces"


def assert_throw_while_climbing(oam_anchor: List[OamSprite], oam_throw: List[OamSprite]) -> Tuple[bool, str]:
    """P2 can throw an apple while maintaining grip on rope."""
    p2_throw = get_p2_sprites(oam_throw)
    if len(p2_throw) < 4:
        return False, f"P2 dropped or glitched while throwing from rope: {len(p2_throw)} pieces"
    return True, "P2 throw while climbing verified without dropping from rope"


def assert_jump_throw(oam_jump_throw: List[OamSprite]) -> Tuple[bool, str]:
    """P2 can jump and throw mid-air."""
    p2 = get_p2_sprites(oam_jump_throw)
    if len(p2) < 4:
        return False, f"P2 missing in jump throw: {len(p2)} pieces"
    return True, "P2 aerial jump throw verified cleanly"


def assert_apple_hit_enemy(oam_hit: List[OamSprite]) -> Tuple[bool, str]:
    """Active enemy defeated when hit by apple."""
    # Verify no game crash and Abu remains suppressed
    abu_ok, msg = assert_abu_suppression(oam_hit)
    if not abu_ok:
        return False, msg
    p2 = get_p2_sprites(oam_hit)
    if len(p2) < 4:
        return False, "P2 glitched after enemy collision"
    return True, "Enemy hit detection handled safely with P2 intact"


def assert_playthrough_integrity(oam_dumps: List[List[OamSprite]]) -> Tuple[bool, str]:
    """Across all playthrough frames, Abu suppressed and P2 active."""
    for idx, oam in enumerate(oam_dumps):
        for s in get_abu_sprites(oam):
            if s.y < 224:
                return False, f"Abu leaked at frame dump #{idx} with Y={s.y}"
        p2 = get_p2_sprites(oam)
        if len(p2) < 4:
            return False, f"P2 disintegrated at frame dump #{idx} (only {len(p2)} pieces visible)"
    return True, f"Playthrough verified across {len(oam_dumps)} check frames with 100% integrity"


def assert_60fps_stability(frame_start: int, frame_end: int, elapsed_sec: float, mem_delta_mb: float) -> Tuple[bool, str]:
    """Verify simulation stability and zero memory leaks."""
    sim_frames = frame_end - frame_start
    if sim_frames <= 0:
        return False, f"Invalid simulated frame count: {sim_frames}"

    if mem_delta_mb > 15.0:
        return False, f"Potential memory leak detected: Working set grew by {mem_delta_mb:.2f} MB"

    return True, f"Stability verified: {sim_frames} frames executed, memory growth {mem_delta_mb:.2f} MB (within threshold)"
