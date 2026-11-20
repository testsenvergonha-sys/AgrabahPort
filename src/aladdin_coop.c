/*
 * Aladdin SNES Recomp — Dual Aladdin 2-Player Co-op Engine
 *
 * Implements simultaneous cooperative gameplay with two Aladdins:
 *   - Player 1: Classic Aladdin (Purple vest, tan pants, red fez) -> Gamepad 1 / Keys P1
 *   - Player 2: Prince Ali Aladdin (Royal Blue vest, silk gold belt, crimson fez) -> Gamepad 2 / Keys P2
 *
 * Replaces the companion monkey (Abu) with Player 2 as requested:
 * "não usaremos mais o macaco, iremos usar outro aladdin, com roupa diferente!"
 */

#include "aladdin_coop.h"
#include "snes/ppu.h"
#include <string.h>
#include <stdio.h>

extern uint8_t g_ram[0x20000];
extern Ppu *g_ppu;
extern uint16_t RtlGetPadState(int slot);

static AladdinGuestPlayer s_p2 = {
    .active = 1,
    .health = 3,
    .apples = 10,
    .facing = 0,
    .state = 0,
    .grounded = 1,
    .spawned = 0,
    .screen_x = 70,
    .screen_y = 190,
};
static int s_coop_enabled = 1;

/*
 * Prince Ali / Alternate Aladdin Palette (Sprite Palette 5 in CGRAM)
 * SNES BGR555 format: (R >> 3) | ((G >> 3) << 5) | ((B >> 3) << 10)
 *
 * Designed with 1-to-1 correspondence to Aladdin's Sprite Palette 0:
 *   0: Transparent (0x0000)
 *   1: Skin primary (0x2E37 -> RGB 184, 136, 88)
 *   2: Skin highlight (0x42BA -> RGB 208, 168, 128)
 *   3: Skin light (0x637D -> RGB 232, 216, 192)
 *   4: Skin shadow (0x04CF -> RGB 120, 48, 8)
 *   5: Warm shadow (0x0938 -> RGB 192, 72, 16)
 *   6: Fez shadow (0x2A1C -> RGB 224, 128, 80)
 *   7: Bright Gold highlight (0x235F -> RGB 248, 208, 64)
 *   8: Crimson Fez (0x141C -> RGB 224, 0, 40)
 *   9: Royal Blue vest shadow (0x44A2 -> RGB 16, 40, 136)
 *  10: Royal Blue vest primary (0x6964 -> RGB 32, 88, 208)
 *  11: Royal Blue vest highlight (0x7E48 -> RGB 64, 144, 248)
 *  12: Silk Gold belt primary (0x129B -> RGB 216, 160, 32)
 *  13: Silk Gold belt shadow (0x09AC -> RGB 96, 104, 16)
 *  14: Dark outline (0x14A8 -> RGB 64, 40, 40)
 *  15: Jet Black hair/eyes (0x0000)
 */
static const uint16_t kPrinceAliCgram[16] = {
    0x0000, /* 0: Transparent */
    0x2E37, /* 1: Skin primary */
    0x42BA, /* 2: Skin highlight */
    0x637D, /* 3: Skin light */
    0x04CF, /* 4: Skin shadow */
    0x0938, /* 5: Warm shadow */
    0x2A1C, /* 6: Fez shadow */
    0x235F, /* 7: Bright Gold highlight */
    0x141C, /* 8: Crimson Fez */
    0x44A2, /* 9: Royal Blue vest shadow */
    0x6964, /* 10: Royal Blue vest primary */
    0x7E48, /* 11: Royal Blue vest highlight */
    0x129B, /* 12: Silk Gold belt primary */
    0x09AC, /* 13: Silk Gold belt shadow */
    0x14A8, /* 14: Dark outline */
    0x0000  /* 15: Jet Black hair/eyes */
};

void AladdinCoop_Init(void)
{
    memset(&s_p2, 0, sizeof(s_p2));
    s_p2.active = 1;
    s_p2.health = 3;
    s_p2.apples = 10;
    s_p2.facing = 0;
    s_p2.grounded = 1;
    s_p2.spawned = 0;
    s_p2.screen_x = 70;
    s_p2.screen_y = 190;
}

void AladdinCoop_Reset(void)
{
    AladdinCoop_Init();
}

int AladdinCoop_IsActive(void)
{
    if (!s_coop_enabled)
        return 0;
    if (!s_p2.active)
        s_p2.active = 1;

    /* Check if in active gameplay (health > 0 or lives > 0) */
    uint8_t p1_health = g_ram[0x0367];
    uint8_t p1_lives = g_ram[0x0364];
    return (p1_health > 0 || p1_lives > 0);
}

void AladdinCoop_SetEnabled(int enabled)
{
    s_coop_enabled = enabled;
}

AladdinGuestPlayer *AladdinCoop_GetPlayer2(void)
{
    return &s_p2;
}

/* Populate Sprite Palette 5 in CGRAM with Prince Ali colors */
static void ApplyPrinceAliPalette(void)
{
    if (!g_ppu)
        return;

    /* Sprite Palette 5 starts at CGRAM word 128 + 5 * 16 = 208 */
    const unsigned base_word = 128 + 5 * 16;
    for (unsigned i = 0; i < 16; ++i) {
        g_ppu->cgram[base_word + i] = kPrinceAliCgram[i];
    }
}

/* Eliminate Abu the monkey from OAM slots ("não usaremos mais o macaco") */
static void SuppressAbu(void)
{
    /* Scan slots 0..15: hide any slot belonging to Abu (Palette 1) */
    for (int slot = 0; slot < 16; ++slot) {
        uint8_t y    = g_ram[0xFA80 + slot * 4 + 1];
        uint8_t attr = g_ram[0xFA80 + slot * 4 + 3];
        uint8_t pal  = (attr >> 1) & 0x07;

        /* Hide unconditionally slots 0..3, or any slot with Pal 1 */
        if (slot < 4 || pal == 1) {
            g_ram[0xFA80 + slot * 4 + 1] = 225;
            if (g_ppu) {
                g_ppu->oam[slot * 2 + 0] = (225 << 8);
            }
        }
    }
}

/* Find Player 1 center X, feet Y, and sprite pieces */
static int FindPlayer1Pieces(int16_t *out_center_x, int16_t *out_feet_y, int *out_slots, int max_slots)
{
    int count = 0;
    int16_t min_x = 999, max_x = -999;
    int16_t min_y = 999, max_y = -999;

    for (int slot = 4; slot < 32 && count < max_slots; ++slot) {
        uint8_t x    = g_ram[0xFA80 + slot * 4 + 0];
        uint8_t y    = g_ram[0xFA80 + slot * 4 + 1];
        uint8_t attr = g_ram[0xFA80 + slot * 4 + 3];
        uint8_t pal  = (attr >> 1) & 0x07;

        /* Aladdin 1 uses Sprite Palette 0 */
        if (pal == 0 && y < 224) {
            out_slots[count++] = slot;
            if (x < min_x) min_x = x;
            if (x > max_x) max_x = x;
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }
    }

    if (count > 0) {
        *out_center_x = (min_x + max_x) / 2;
        *out_feet_y = max_y;
    } else {
        *out_center_x = 64;
        *out_feet_y = 190;
    }

    return count;
}

/* Update Player 2 motion, physics, and input */
static void UpdatePlayer2(int16_t p1_center_x, int16_t p1_feet_y)
{
    /* Read Gamepad 2 */
    uint16_t pad = RtlGetPadState(1);

    /* Initialize spawn position near Player 1 */
    if (!s_p2.spawned) {
        s_p2.screen_x = p1_center_x + 36;
        if (s_p2.screen_x > 220) s_p2.screen_x = p1_center_x - 36;
        if (s_p2.screen_x < 24)  s_p2.screen_x = 48;
        s_p2.screen_y = p1_feet_y;
        s_p2.spawned = 1;
        s_p2.grounded = 1;
    }

    /* Timers */
    if (s_p2.invuln_timer > 0) s_p2.invuln_timer--;
    if (s_p2.attack_timer > 0) s_p2.attack_timer--;

    /* Horizontal Movement */
    s_p2.vx = 0;
    if (pad & 0x0040) { /* D-Pad Left */
        s_p2.vx = -2;
        s_p2.facing = 1;
    } else if (pad & 0x0080) { /* D-Pad Right */
        s_p2.vx = 2;
        s_p2.facing = 0;
    }

    /* Jump (Button B = 0x0001 or Button A = 0x0100) */
    if ((pad & 0x0101) && s_p2.grounded) {
        s_p2.vy = -7;
        s_p2.grounded = 0;
        s_p2.state = 2; /* Jump */
    }

    /* Sword Attack (Button Y = 0x0002 or Button X = 0x0200) */
    if ((pad & 0x0202) && s_p2.attack_timer == 0) {
        s_p2.attack_timer = 16;
        s_p2.state = 3; /* Slash */
    }

    /* Crouch (D-Pad Down = 0x0020) */
    if ((pad & 0x0020) && s_p2.grounded && s_p2.vx == 0) {
        s_p2.state = 4; /* Crouch */
    } else if (s_p2.grounded && s_p2.attack_timer == 0) {
        if (s_p2.vx != 0) {
            s_p2.state = 1; /* Run */
            s_p2.anim_timer++;
            if (s_p2.anim_timer >= 6) {
                s_p2.anim_timer = 0;
                s_p2.anim_frame = (s_p2.anim_frame + 1) % 4;
            }
        } else {
            s_p2.state = 0; /* Idle */
            s_p2.anim_frame = 0;
        }
    }

    /* Apply Gravity & Vertical Motion */
    if (!s_p2.grounded) {
        s_p2.vy += 1;
        if (s_p2.vy > 7) s_p2.vy = 7;
        s_p2.screen_y += s_p2.vy;

        /* Floor collision against P1 ground level */
        if (s_p2.screen_y >= p1_feet_y) {
            s_p2.screen_y = p1_feet_y;
            s_p2.vy = 0;
            s_p2.grounded = 1;
            s_p2.state = (s_p2.vx != 0) ? 1 : 0;
        }
    } else {
        /* Gently track ground height if close to P1 */
        if (p1_feet_y > 32 && p1_feet_y < 224) {
            int diff = p1_feet_y - s_p2.screen_y;
            if (diff > -24 && diff < 24) {
                s_p2.screen_y = p1_feet_y;
            }
        }
    }

    /* Apply Horizontal Position */
    s_p2.screen_x += s_p2.vx;

    /* Screen clamp (keep on screen) */
    if (s_p2.screen_x < 16)  s_p2.screen_x = 16;
    if (s_p2.screen_x > 238) s_p2.screen_x = 238;
}

/* Emit Player 2 sprites into OAM slots 80..92 */
static void EmitPlayer2Sprites(int16_t p1_center_x, int16_t p1_feet_y, const int *p1_slots, int p1_count)
{
    /* Blink if invulnerable */
    if (s_p2.invuln_timer > 0 && (s_p2.invuln_timer & 2)) {
        for (int i = 0; i < 14; ++i) {
            int slot = 80 + i;
            g_ram[0xFA80 + slot * 4 + 1] = 225;
            if (g_ppu) g_ppu->oam[slot * 2] = (225 << 8);
        }
        return;
    }

    int p2_slot = 80;

    for (int k = 0; k < p1_count && p2_slot < 93; ++k) {
        int src = p1_slots[k];
        uint8_t src_x    = g_ram[0xFA80 + src * 4 + 0];
        uint8_t src_y    = g_ram[0xFA80 + src * 4 + 1];
        uint8_t src_tile = g_ram[0xFA80 + src * 4 + 2];
        uint8_t src_attr = g_ram[0xFA80 + src * 4 + 3];

        int16_t offset_x = (int16_t)src_x - p1_center_x;
        int16_t offset_y = (int16_t)src_y - p1_feet_y;

        uint8_t p1_hflip = (src_attr & 0x40) != 0;
        uint8_t p2_attr = src_attr;

        if (s_p2.facing != 0) {
            /* Facing left */
            if (!p1_hflip) {
                offset_x = -offset_x - 16;
                p2_attr ^= 0x40;
            }
        } else {
            /* Facing right */
            if (p1_hflip) {
                offset_x = -offset_x - 16;
                p2_attr ^= 0x40;
            }
        }

        int16_t p2_x = s_p2.screen_x + offset_x;
        int16_t p2_y = s_p2.screen_y + offset_y;

        /* Clamp */
        if (p2_x < 0) p2_x = 0;
        if (p2_x > 255) p2_x = 255;
        if (p2_y < 0) p2_y = 0;
        if (p2_y > 224) p2_y = 224;

        /* Palette 5 (Prince Ali royal blue/gold) */
        p2_attr = (p2_attr & ~0x0E) | (5 << 1);
        /* Priority 3 (above background) */
        p2_attr = (p2_attr & ~0x30) | (3 << 4);

        /* Write to WRAM low OAM */
        g_ram[0xFA80 + p2_slot * 4 + 0] = (uint8_t)p2_x;
        g_ram[0xFA80 + p2_slot * 4 + 1] = (uint8_t)p2_y;
        g_ram[0xFA80 + p2_slot * 4 + 2] = src_tile;
        g_ram[0xFA80 + p2_slot * 4 + 3] = p2_attr;

        /* Update high OAM (size bit and high X bit) */
        int high_byte_idx = p2_slot / 4;
        int high_bit_shift = (p2_slot % 4) * 2;
        int src_high_byte = src / 4;
        int src_high_shift = (src % 4) * 2;
        uint8_t src_high_val = (g_ram[0xFC80 + src_high_byte] >> src_high_shift) & 0x03;

        g_ram[0xFC80 + high_byte_idx] &= ~(0x03 << high_bit_shift);
        g_ram[0xFC80 + high_byte_idx] |= (src_high_val << high_bit_shift);

        /* Mirror directly to PPU OAM */
        if (g_ppu) {
            g_ppu->oam[p2_slot * 2 + 0] = (uint16_t)((uint8_t)p2_x | ((uint8_t)p2_y << 8));
            g_ppu->oam[p2_slot * 2 + 1] = (uint16_t)(src_tile | (p2_attr << 8));
            g_ppu->highOam[high_byte_idx] = g_ram[0xFC80 + high_byte_idx];
        }

        p2_slot++;
    }

    /* Clear any remaining allocated slots */
    for (; p2_slot < 93; ++p2_slot) {
        g_ram[0xFA80 + p2_slot * 4 + 1] = 225;
        if (g_ppu) g_ppu->oam[p2_slot * 2] = (225 << 8);
    }
}

/* Draw Player 2 HUD (Hearts on top-right of screen) */
static void EmitPlayer2Hud(void)
{
    /* Use OAM slots 93, 94, 95 for Player 2 Hearts at top-right */
    const int16_t hud_y = 16;
    const int16_t start_x = 210;

    for (int h = 0; h < 3; ++h) {
        int slot = 93 + h;
        int16_t hx = start_x + h * 12;

        /* Tile 0x0A / 0x0B in Sprite sheet is heart/jewel */
        uint8_t tile = (h < s_p2.health) ? 0x0B : 0x0A;
        uint8_t attr = (5 << 1) | (3 << 4); /* Pal 5, Pri 3 */

        g_ram[0xFA80 + slot * 4 + 0] = (uint8_t)hx;
        g_ram[0xFA80 + slot * 4 + 1] = (uint8_t)hud_y;
        g_ram[0xFA80 + slot * 4 + 2] = tile;
        g_ram[0xFA80 + slot * 4 + 3] = attr;

        if (g_ppu) {
            g_ppu->oam[slot * 2 + 0] = (uint16_t)((uint8_t)hx | (hud_y << 8));
            g_ppu->oam[slot * 2 + 1] = (uint16_t)(tile | (attr << 8));
        }
    }
}

void AladdinCoop_PostFrame(void)
{
    if (!AladdinCoop_IsActive()) {
        s_p2.spawned = 0;
        return;
    }

    int16_t p1_center_x, p1_feet_y;
    int p1_slots[32];
    int p1_count = FindPlayer1Pieces(&p1_center_x, &p1_feet_y, p1_slots, 32);

    /* Update Player 2 motion, physics, and input */
    UpdatePlayer2(p1_center_x, p1_feet_y);

    /* Apply Prince Ali Palette to CGRAM */
    ApplyPrinceAliPalette();

    /* Suppress Abu the monkey */
    SuppressAbu();

    /* Emit Player 2 sprites */
    EmitPlayer2Sprites(p1_center_x, p1_feet_y, p1_slots, p1_count);

    /* Emit Player 2 HUD */
    EmitPlayer2Hud();
}

void AladdinCoop_PrepareDraw(void)
{
    if (!AladdinCoop_IsActive())
        return;

    /* Ensure Palette 5 is loaded in PPU CGRAM before rasterization */
    ApplyPrinceAliPalette();

    /* Ensure Abu remains suppressed in PPU OAM */
    SuppressAbu();

    /* Ensure P2 sprites and HUD are present in PPU OAM */
    int16_t p1_center_x, p1_feet_y;
    int p1_slots[32];
    int p1_count = FindPlayer1Pieces(&p1_center_x, &p1_feet_y, p1_slots, 32);

    EmitPlayer2Sprites(p1_center_x, p1_feet_y, p1_slots, p1_count);
    EmitPlayer2Hud();
}
