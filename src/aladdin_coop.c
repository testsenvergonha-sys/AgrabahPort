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

/* Stage collision: read collision attribute for a world position */
static uint8_t Aladdin_GetTileAttr(int world_x, int world_y)
{
    if (world_x < 0 || world_y < 0 || world_y >= 1024 || world_x >= 4096)
        return 0;

    int x_high = (world_x >> 8) & 0xFF;
    int x_part = (((x_high + 1) * 2) & 0x3E);
    int y_part = ((((world_y & 0xFFF0) >> 8) + 1) << 6) & 0x03C0;
    int block_idx = y_part | x_part;

    if (0x1F000 + block_idx + 1 >= 0x20000)
        return 0;

    uint16_t block_base = *(uint16_t*)&g_ram[0x1F000 + block_idx];
    int sub_x = (world_x >> 3) & 0x1E;
    int sub_y = (world_y << 1) & 0x01E0;
    int tile_offset = sub_y | sub_x;

    uint16_t full_tile_idx = (block_base + tile_offset) & 0xFFFF;
    if (0x10000 + full_tile_idx + 1 >= 0x20000)
        return 0;

    uint16_t tile_id = *(uint16_t*)&g_ram[0x10000 + full_tile_idx];
    uint16_t coll_idx = tile_id >> 3;
    if (coll_idx < 0x3800) {
        return g_ram[0xC800 + coll_idx];
    }
    return 0;
}

/* Check if a world point contains solid obstacles (walls, platforms, melon carts, canopies) */
static int Aladdin_IsSolidAt(int world_x, int world_y)
{
    uint8_t attr = Aladdin_GetTileAttr(world_x, world_y);
    return (attr != 0);
}

/* Find floor surface below world (x, y) */
static int Aladdin_FindFloorBelow(int world_x, int world_y, int max_fall)
{
    for (int y = world_y; y <= world_y + max_fall; y += 4) {
        uint8_t attr = Aladdin_GetTileAttr(world_x, y + 16);
        if (attr != 0) {
            return ((y + 16) & ~15) - 16;
        }
    }
    return 432; /* Default ground fallback for stage 1 */
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

/* Check dynamic actor collisions: swinging wooden beams and enemies */
static void CheckActorCollisions(void)
{
    static int s_beam_slot = -1;
    static int s_beam_last_x = 0;

    uint16_t cam_y = *(uint16_t*)&g_ram[0x19F1];
    int on_beam_this_frame = 0;

    /* Scan OAM slots 16..79 where SNES Aladdin renders enemies and platforms */
    for (int i = 16; i < 80; ++i) {
        uint8_t spr_y = g_ram[0xFA80 + i * 4 + 1];
        if (spr_y >= 224) continue;

        uint8_t spr_x = g_ram[0xFA80 + i * 4 + 0];
        uint8_t spr_attr = g_ram[0xFA80 + i * 4 + 3];
        uint8_t pal = (spr_attr >> 1) & 7;

        int high_byte = i / 4;
        int shift = (i % 4) * 2;
        int size = (g_ram[0xFC80 + high_byte] >> (shift + 1)) & 1;
        int spr_w = size ? 16 : 8;

        /* Palette 4 or 3: Stage objects, swinging platforms, enemies */
        if (pal == 4 || pal == 3) {
            /* 1. Dynamic Platform / Swinging Wooden Beam Detection */
            if (spr_y < 190 && spr_y > 30) {
                int dx = s_p2.screen_x - spr_x;
                int dy = s_p2.screen_y - spr_y;
                /* If P2's feet are horizontally aligned with the beam and landing onto it */
                if (dx >= -16 && dx <= 32 && dy >= -12 && dy <= 8 && s_p2.vy >= 0) {
                    s_p2.screen_y = spr_y;
                    s_p2.world_y = spr_y + cam_y;
                    s_p2.vy = 0;
                    s_p2.grounded = 1;
                    if (s_p2.state != 3) {
                        s_p2.state = (s_p2.vx != 0) ? 1 : 0;
                    }
                    on_beam_this_frame = 1;

                    /* If continuing on same beam, carry horizontal swing motion */
                    if (s_beam_slot == i) {
                        int swing_dx = (int)spr_x - s_beam_last_x;
                        if (abs(swing_dx) < 10) {
                            s_p2.world_x += swing_dx;
                        }
                    }
                    s_beam_slot = i;
                    s_beam_last_x = spr_x;
                }
            }

            /* 2. Enemy Interaction */
            if (spr_y >= 100 && spr_y <= 216 && !on_beam_this_frame) {
                int edx = s_p2.screen_x - spr_x;
                int edy = s_p2.screen_y - spr_y;

                /* P2 attacks with sword */
                if (s_p2.state == 3 && s_p2.attack_timer > 2) {
                    if (abs(edx) < 32 && abs(edy) < 26) {
                        /* Enemy hit: defeat enemy by moving off screen */
                        g_ram[0xFA80 + i * 4 + 1] = 225;
                        if (g_ppu) g_ppu->oam[i * 2] = (225 << 8);
                    }
                }
                /* P2 jumps on top of enemy */
                else if (!s_p2.grounded && s_p2.vy > 0 && abs(edx) < 20 && edy >= -18 && edy <= -2) {
                    /* Bounce high off enemy! */
                    s_p2.vy = -8;
                    /* Defeat enemy */
                    g_ram[0xFA80 + i * 4 + 1] = 225;
                    if (g_ppu) g_ppu->oam[i * 2] = (225 << 8);
                }
                /* Enemy touches P2 while vulnerable */
                else if (s_p2.invuln_timer == 0 && abs(edx) < 16 && abs(edy) < 18) {
                    if (s_p2.health > 0) s_p2.health--;
                    s_p2.invuln_timer = 60; /* 1 sec invulnerability flash */
                    s_p2.vy = -4;           /* Recoil bounce */
                    s_p2.vx = s_p2.facing ? 2 : -2;
                }
            }
        }
    }

    if (!on_beam_this_frame && s_p2.grounded) {
        s_beam_slot = -1;
    }
}

/* Update Player 2 motion, physics, and input */
static void UpdatePlayer2(int16_t p1_screen_center_x, int16_t p1_screen_feet_y)
{
    /* Read Gamepad 2 */
    uint16_t pad = RtlGetPadState(1);

    /* Read Aladdin 1 world coordinates & camera from WRAM */
    uint16_t p1_world_x = *(uint16_t*)&g_ram[0x08E7];
    uint16_t p1_world_y = *(uint16_t*)&g_ram[0x08EA];
    uint16_t cam_x = *(uint16_t*)&g_ram[0x19ED];
    uint16_t cam_y = *(uint16_t*)&g_ram[0x19F1];

    /* Initialize spawn position in world coordinates near Player 1 */
    if (!s_p2.spawned && p1_world_y > 50) {
        s_p2.world_x = p1_world_x + 36;
        s_p2.world_y = p1_world_y;
        s_p2.screen_x = s_p2.world_x - cam_x;
        s_p2.screen_y = s_p2.world_y - cam_y;
        s_p2.spawned = 1;
        s_p2.grounded = 1;
        s_p2.health = 3;
    }

    /* Timers */
    if (s_p2.invuln_timer > 0) s_p2.invuln_timer--;
    if (s_p2.attack_timer > 0) {
        s_p2.attack_timer--;
        if (s_p2.attack_timer == 0 && s_p2.state == 3) {
            s_p2.state = (s_p2.vx != 0) ? 1 : 0;
        }
    }

    /* Horizontal Movement */
    int speed = (pad & 0x0202) ? 3 : 2; /* Hold Y / X to dash/run */
    s_p2.vx = 0;
    if (pad & 0x0040) { /* D-Pad Left */
        s_p2.vx = -speed;
        s_p2.facing = 1;
    } else if (pad & 0x0080) { /* D-Pad Right */
        s_p2.vx = speed;
        s_p2.facing = 0;
    }

    /* Animation timer & state */
    if (s_p2.vx != 0 && s_p2.grounded) {
        s_p2.anim_timer++;
        if (s_p2.state != 3) s_p2.state = 1; /* Run */
    } else if (s_p2.grounded && s_p2.state != 3 && s_p2.state != 4) {
        s_p2.state = 0; /* Idle */
    }

    /* Horizontal obstacle & wall collision */
    if (s_p2.vx > 0) {
        if (Aladdin_IsSolidAt(s_p2.world_x + 10, s_p2.world_y - 12) ||
            Aladdin_IsSolidAt(s_p2.world_x + 10, s_p2.world_y - 24)) {
            s_p2.vx = 0;
        }
    } else if (s_p2.vx < 0) {
        if (Aladdin_IsSolidAt(s_p2.world_x - 10, s_p2.world_y - 12) ||
            Aladdin_IsSolidAt(s_p2.world_x - 10, s_p2.world_y - 24)) {
            s_p2.vx = 0;
        }
    }
    s_p2.world_x += s_p2.vx;

    /* Jump (Button B = 0x0001 or Button A = 0x0100) - Edge triggered! */
    bool jump_held = (pad & 0x0101) != 0;
    bool jump_edge = jump_held && ((s_p2.last_pad & 0x0101) == 0);
    if (jump_edge && s_p2.grounded) {
        s_p2.vy = -10; /* Full, satisfying SNES jump! */
        s_p2.grounded = 0;
        s_p2.state = 2; /* Jump */
    }

    /* Sword Attack (Button Y = 0x0002 or Button X = 0x0200) - Edge triggered! */
    bool atk_held = (pad & 0x0202) != 0;
    bool atk_edge = atk_held && ((s_p2.last_pad & 0x0202) == 0);
    if (atk_edge && s_p2.attack_timer == 0) {
        s_p2.attack_timer = 16;
        s_p2.state = 3; /* Slash */
    }

    /* Crouch (D-Pad Down = 0x0020) */
    if ((pad & 0x0020) && s_p2.grounded && s_p2.vx == 0 && s_p2.state != 3) {
        s_p2.state = 4; /* Crouch */
    }

    /* Apply Gravity & Vertical Motion */
    if (!s_p2.grounded) {
        if (!jump_held && s_p2.vy < -3) {
            s_p2.vy = -3; /* Variable jump cut on early release */
        }
        static int s_grav_sub = 0;
        s_grav_sub++;
        if (s_p2.vy < 0) {
            if (s_grav_sub & 1) s_p2.vy += 1;
        } else {
            s_p2.vy += 1;
        }
        if (s_p2.vy > 7) s_p2.vy = 7;

        s_p2.world_y += s_p2.vy;

        /* Ceiling collision */
        if (s_p2.vy < 0 && Aladdin_IsSolidAt(s_p2.world_x, s_p2.world_y - 32)) {
            s_p2.vy = 0;
        }

        /* Floor collision when descending */
        if (s_p2.vy >= 0) {
            int floor_y = Aladdin_FindFloorBelow(s_p2.world_x, s_p2.world_y - 8, 20);
            if (s_p2.world_y >= floor_y) {
                s_p2.world_y = floor_y;
                s_p2.vy = 0;
                s_p2.grounded = 1;
                s_p2.state = (s_p2.vx != 0) ? 1 : 0;
            }
        }
    } else {
        /* Ground tracking when walking: follow slopes/steps or detect ledge drops */
        int floor_y = Aladdin_FindFloorBelow(s_p2.world_x, s_p2.world_y - 6, 24);
        int diff = floor_y - s_p2.world_y;
        if (diff >= -8 && diff <= 8) {
            s_p2.world_y = floor_y;
        } else if (diff > 8) {
            /* Walked off ledge */
            s_p2.grounded = 0;
            s_p2.vy = 0;
        }
    }

    /* Check dynamic platform & enemy collisions */
    CheckActorCollisions();

    /* Save last pad for edge detection */
    s_p2.last_pad = pad;

    /* Calculate screen coordinates from camera */
    s_p2.screen_x = s_p2.world_x - cam_x;
    s_p2.screen_y = s_p2.world_y - cam_y;

    /* Keep Player 2 inside the playable screen viewport */
    if (s_p2.screen_x < 12) {
        s_p2.screen_x = 12;
        s_p2.world_x = cam_x + 12;
    }
    if (s_p2.screen_x > 240) {
        s_p2.screen_x = 240;
        s_p2.world_x = cam_x + 240;
    }
}

typedef struct {
    int8_t  rel_x;
    int8_t  rel_y;
    uint8_t tile;
    uint8_t attr;
    uint8_t size; /* 0 = 8x8, 1 = 16x16 */
} AladdinSpriteDef;

static const AladdinSpriteDef kAladdinIdleSprites[7] = {
    {  13, -32, 0x09, 0x20, 0 },
    { -12,  -8, 0x08, 0x20, 0 },
    {  12, -15, 0x07, 0x20, 0 },
    {   5, -40, 0x06, 0x20, 0 },
    {  -4,   0, 0x04, 0x20, 1 },
    {  -4, -16, 0x02, 0x20, 1 },
    {  -3, -32, 0x00, 0x20, 1 },
};

static const AladdinSpriteDef kAladdinJumpSprites[10] = {
    { -12,   0, 0x0C, 0x20, 0 },
    {   4, -16, 0x0B, 0x20, 0 },
    {  12, -24, 0x0A, 0x20, 0 },
    {  12, -32, 0x09, 0x20, 0 },
    {  -2, -40, 0x08, 0x20, 0 },
    { -10, -40, 0x07, 0x20, 0 },
    {  -2, -48, 0x06, 0x20, 0 },
    { -12, -16, 0x04, 0x20, 1 },
    {  -4, -32, 0x02, 0x20, 1 },
    {   6, -48, 0x00, 0x20, 1 },
};

static const AladdinSpriteDef kAladdinRunSprites1[9] = {
    {   8, -29, 0x0B, 0x20, 0 },
    {   0, -29, 0x0A, 0x20, 0 },
    {   0, -45, 0x06, 0x20, 1 },
    {  -8,   0, 0x0F, 0x20, 0 },
    {  -8,  -8, 0x0E, 0x20, 0 },
    {  -8, -23, 0x0D, 0x20, 0 },
    {  -8, -31, 0x0C, 0x20, 0 },
    {   0,  -5, 0x04, 0x20, 1 },
    {   0, -21, 0x02, 0x20, 1 },
};

static const AladdinSpriteDef kAladdinRunSprites2[7] = {
    {  -7, -16, 0x04, 0x20, 1 },
    {   1, -32, 0x02, 0x20, 1 },
    {   1, -48, 0x00, 0x20, 1 },
    {  -9,   0, 0x09, 0x20, 0 },
    {   9, -16, 0x08, 0x20, 0 },
    {  -7, -24, 0x07, 0x20, 0 },
    {  -7, -32, 0x06, 0x20, 0 },
};

static const AladdinSpriteDef kAladdinAttackSprites[10] = {
    {   6, -56, 0x0C, 0x20, 0 },
    {  -4, -16, 0x04, 0x20, 1 },
    {  -4, -32, 0x02, 0x20, 1 },
    {  -2, -48, 0x00, 0x20, 1 },
    { -12,   0, 0x0B, 0x20, 0 },
    { -12,  -8, 0x0A, 0x20, 0 },
    { -12, -16, 0x09, 0x20, 0 },
    {  12, -32, 0x08, 0x20, 0 },
    { -12, -32, 0x07, 0x20, 0 },
    { -10, -40, 0x06, 0x20, 0 },
};

static const AladdinSpriteDef kAladdinCrouchSprites[6] = {
    {  -4,   0, 0x04, 0x20, 1 },
    {  -4, -12, 0x02, 0x20, 1 },
    {  -3, -24, 0x00, 0x20, 1 },
    {  13, -24, 0x09, 0x20, 0 },
    { -12,  -4, 0x08, 0x20, 0 },
    {   5, -30, 0x06, 0x20, 0 },
};

static void EmitPlayer2FromTable(const AladdinSpriteDef *table, int count)
{
    int p2_slot = 80;
    for (int k = 0; k < count && p2_slot < 93; ++k) {
        int16_t offset_x = table[k].rel_x;
        int16_t offset_y = table[k].rel_y;
        uint8_t tile = table[k].tile;
        uint8_t attr = table[k].attr;
        uint8_t size = table[k].size;

        if (s_p2.facing != 0) {
            /* Facing left */
            offset_x = -offset_x - (size ? 16 : 8);
            attr ^= 0x40;
        }

        int16_t p2_x = s_p2.screen_x + offset_x;
        int16_t p2_y = s_p2.screen_y + offset_y;

        if (p2_x < 0) p2_x = 0;
        if (p2_x > 255) p2_x = 255;
        if (p2_y < 0) p2_y = 0;
        if (p2_y > 224) p2_y = 224;

        /* Palette 5 (Prince Ali), Priority 3 */
        attr = (attr & ~0x0E) | (5 << 1);
        attr = (attr & ~0x30) | (3 << 4);

        g_ram[0xFA80 + p2_slot * 4 + 0] = (uint8_t)p2_x;
        g_ram[0xFA80 + p2_slot * 4 + 1] = (uint8_t)p2_y;
        g_ram[0xFA80 + p2_slot * 4 + 2] = tile;
        g_ram[0xFA80 + p2_slot * 4 + 3] = attr;

        int high_byte_idx = p2_slot / 4;
        int high_bit_shift = (p2_slot % 4) * 2;
        uint8_t val = (size << 1);

        g_ram[0xFC80 + high_byte_idx] &= ~(0x03 << high_bit_shift);
        g_ram[0xFC80 + high_byte_idx] |= (val << high_bit_shift);

        if (g_ppu) {
            g_ppu->oam[p2_slot * 2 + 0] = (uint16_t)((uint8_t)p2_x | ((uint8_t)p2_y << 8));
            g_ppu->oam[p2_slot * 2 + 1] = (uint16_t)(tile | (attr << 8));
            g_ppu->highOam[high_byte_idx] = g_ram[0xFC80 + high_byte_idx];
        }
        p2_slot++;
    }

    for (; p2_slot < 93; ++p2_slot) {
        g_ram[0xFA80 + p2_slot * 4 + 1] = 225;
        if (g_ppu) g_ppu->oam[p2_slot * 2] = (225 << 8);
    }
}

/* Emit Player 2 sprites into OAM slots 80..92 */
static void EmitPlayer2Sprites(int16_t p1_center_x, int16_t p1_feet_y, const int *p1_slots, int p1_count)
{
    /* Blink if invulnerable */
    if (s_p2.invuln_timer > 0 && (s_p2.invuln_timer & 4) == 0) {
        for (int i = 0; i < 14; ++i) {
            int slot = 80 + i;
            g_ram[0xFA80 + slot * 4 + 1] = 225;
            if (g_ppu) g_ppu->oam[slot * 2] = (225 << 8);
        }
        return;
    }

    /* Completely decoupled independent Aladdin 2 animations */
    if (s_p2.state == 1) {
        /* Run: alternate two running cycle frames every 8 ticks */
        if ((s_p2.anim_timer >> 3) & 1) {
            EmitPlayer2FromTable(kAladdinRunSprites2, 7);
        } else {
            EmitPlayer2FromTable(kAladdinRunSprites1, 9);
        }
        return;
    }
    if (s_p2.state == 2) {
        /* Jump */
        EmitPlayer2FromTable(kAladdinJumpSprites, 10);
        return;
    }
    if (s_p2.state == 3) {
        /* Attack / Sword Slash */
        EmitPlayer2FromTable(kAladdinAttackSprites, 10);
        return;
    }
    if (s_p2.state == 4) {
        /* Crouch */
        EmitPlayer2FromTable(kAladdinCrouchSprites, 6);
        return;
    }

    /* State 0: Idle */
    EmitPlayer2FromTable(kAladdinIdleSprites, 7);
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
