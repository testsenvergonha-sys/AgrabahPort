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
    .rope_cooldown = 0,
    .rope_anchor_x = 0,
    .rope_anchor_y = 0,
};
static int s_coop_enabled = 1;
static AladdinP2Projectile s_p2_apples[MAX_P2_PROJECTILES];

AladdinP2Projectile *AladdinCoop_GetProjectiles(void)
{
    return s_p2_apples;
}

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
    s_p2.rope_cooldown = 0;
    s_p2.rope_anchor_x = 0;
    s_p2.rope_anchor_y = 0;
    memset(s_p2_apples, 0, sizeof(s_p2_apples));
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

/* Dynamic detection of vertical ropes in OAM slots 16..79 */
static int CheckVerticalRopeOAM(int screen_x, int screen_y, int *out_rope_sx)
{
    for (int i = 16; i < 80; ++i) {
        uint8_t spr_y = g_ram[0xFA80 + i * 4 + 1];
        if (spr_y >= 224) continue;

        uint8_t spr_x = g_ram[0xFA80 + i * 4 + 0];
        uint8_t spr_tile = g_ram[0xFA80 + i * 4 + 2];
        uint8_t spr_attr = g_ram[0xFA80 + i * 4 + 3];
        uint8_t pal = (spr_attr >> 1) & 7;

        /* Palette 3 or 7: Vertical rope tiles 0xB0..0xB8, 0xD0, 0xC2..0xCA */
        if (pal == 3 || pal == 7) {
            bool is_vrope = (spr_tile >= 0xB0 && spr_tile <= 0xB8) ||
                            (spr_tile == 0xD0) ||
                            (spr_tile >= 0xC2 && spr_tile <= 0xCA);
            if (is_vrope) {
                int dx = screen_x - (int)spr_x;
                int dy = screen_y - (int)spr_y;
                if (abs(dx) <= 20 && dy >= -40 && dy <= 28) {
                    if (out_rope_sx) *out_rope_sx = spr_x;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Dynamic detection of horizontal clotheslines and nets in OAM slots 16..79 */
static int CheckHorizontalRopeOAM(int screen_x, int screen_y, int *out_rope_sy)
{
    for (int i = 16; i < 80; ++i) {
        uint8_t spr_y = g_ram[0xFA80 + i * 4 + 1];
        if (spr_y >= 224) continue;

        uint8_t spr_x = g_ram[0xFA80 + i * 4 + 0];
        uint8_t spr_tile = g_ram[0xFA80 + i * 4 + 2];
        uint8_t spr_attr = g_ram[0xFA80 + i * 4 + 3];
        uint8_t pal = (spr_attr >> 1) & 7;

        /* Palette 4 or 7: Horizontal clothesline / hanging net tiles 0xC0..0xDC */
        if (pal == 7 || pal == 4) {
            bool is_hrope = (spr_tile >= 0xC0 && spr_tile <= 0xDC);
            if (is_hrope) {
                int dx = screen_x - (int)spr_x;
                int dy = screen_y - (int)spr_y;
                if (abs(dx) <= 28 && dy >= -10 && dy <= 32) {
                    if (out_rope_sy) *out_rope_sy = spr_y + 16;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Spawn a Player 2 Apple projectile */
static void SpawnPlayer2Apple(void)
{
    for (int i = 0; i < MAX_P2_PROJECTILES; ++i) {
        if (!s_p2_apples[i].active) {
            s_p2_apples[i].active = 1;
            s_p2_apples[i].world_x = s_p2.world_x + (s_p2.facing ? -12 : 12);
            s_p2_apples[i].world_y = s_p2.world_y - 20;
            s_p2_apples[i].vx = s_p2.facing ? -5 : 5;
            s_p2_apples[i].vy = -2;
            s_p2_apples[i].anim_timer = 0;
            break;
        }
    }
}

/* Update Player 2 Apple projectile physics, terrain and enemy collision */
static void UpdatePlayer2Projectiles(void)
{
    uint16_t cam_x = *(uint16_t*)&g_ram[0x19ED];
    uint16_t cam_y = *(uint16_t*)&g_ram[0x19F1];

    for (int p = 0; p < MAX_P2_PROJECTILES; ++p) {
        if (!s_p2_apples[p].active) continue;

        AladdinP2Projectile *app = &s_p2_apples[p];

        /* Update projectile flight position */
        app->world_x += app->vx;
        app->world_y += app->vy;
        app->anim_timer++;

        /* Gravity: accelerate vy downward up to terminal velocity 6 */
        if (app->anim_timer & 1) {
            if (app->vy < 6) app->vy += 1;
        }

        /* Screen coordinates */
        app->screen_x = app->world_x - cam_x;
        app->screen_y = app->world_y - cam_y;

        /* Despawn if out of viewport */
        if (app->screen_x < -16 || app->screen_x > 272 ||
            app->screen_y < -16 || app->screen_y > 240) {
            app->active = 0;
            continue;
        }

        /* Solid terrain collision */
        if (Aladdin_IsSolidAt(app->world_x, app->world_y)) {
            app->active = 0;
            continue;
        }

        /* Enemy collision: check dynamic actors in OAM slots 16..79 */
        for (int i = 16; i < 80; ++i) {
            uint8_t spr_y = g_ram[0xFA80 + i * 4 + 1];
            if (spr_y >= 224) continue;

            uint8_t spr_x = g_ram[0xFA80 + i * 4 + 0];
            uint8_t spr_attr = g_ram[0xFA80 + i * 4 + 3];
            uint8_t pal = (spr_attr >> 1) & 7;

            if (pal == 4 || pal == 3) {
                int edx = app->screen_x - spr_x;
                int edy = app->screen_y - spr_y;
                if (abs(edx) < 20 && abs(edy) < 22) {
                    /* Defeat enemy in OAM */
                    g_ram[0xFA80 + i * 4 + 1] = 225;
                    if (g_ppu) g_ppu->oam[i * 2] = (225 << 8);

                    /* Zero HP in dynamic enemy pool at WRAM 0x0B5B..0x1400 */
                    for (int s = 0; s < 27; ++s) {
                        uint16_t base = 0x0B5B + s * 0x50;
                        if (g_ram[base] != 0) {
                            int16_t ex = *(int16_t*)&g_ram[base + 0x0C];
                            int16_t ey = *(int16_t*)&g_ram[base + 0x0F];
                            if (abs(app->world_x - ex) < 28 && abs(app->world_y - ey) < 28) {
                                g_ram[base + 0x42] = 0;
                                g_ram[base + 0x43] = 0x0C; /* Hit by apple */
                            }
                        }
                    }

                    app->active = 0;
                    break;
                }
            }
        }
    }
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
        s_p2.apples = 10;
        s_p2.rope_cooldown = 0;
    }

    /* Timers */
    if (s_p2.invuln_timer > 0) s_p2.invuln_timer--;
    if (s_p2.rope_cooldown > 0) s_p2.rope_cooldown--;

    if (s_p2.attack_timer > 0) {
        s_p2.attack_timer--;
        if (s_p2.attack_timer == 0) {
            if (s_p2.state == ALADDIN_STATE_SLASH || s_p2.state == ALADDIN_STATE_THROW_GROUND) {
                s_p2.state = (s_p2.vx != 0) ? ALADDIN_STATE_RUN : ALADDIN_STATE_IDLE;
            } else if (s_p2.state == ALADDIN_STATE_THROW_AIR) {
                s_p2.state = ALADDIN_STATE_JUMP;
            }
        }
    }

    /* Edge-triggered buttons */
    /* Jump: Button B = 0x0001 */
    bool jump_held = (pad & 0x0001) != 0;
    bool jump_edge = jump_held && ((s_p2.last_pad & 0x0001) == 0);

    /* Apple Throw: Button A = 0x0100, Button X = 0x0200, Button R = 0x0800 */
    bool throw_held = (pad & 0x0B00) != 0;
    bool throw_edge = throw_held && ((s_p2.last_pad & 0x0B00) == 0);

    /* Slash / Melee: Button Y = 0x0002 */
    bool atk_held = (pad & 0x0002) != 0;
    bool atk_edge = atk_held && ((s_p2.last_pad & 0x0002) == 0);

    /* Trigger Apple Throw */
    if (throw_edge && s_p2.attack_timer == 0 && s_p2.apples > 0) {
        s_p2.apples--;
        SpawnPlayer2Apple();
        if (s_p2.grounded) {
            s_p2.attack_timer = 16;
            s_p2.state = ALADDIN_STATE_THROW_GROUND;
            s_p2.vx = 0;
        } else {
            s_p2.attack_timer = 12;
            s_p2.state = ALADDIN_STATE_THROW_AIR;
        }
    }
    /* Trigger Slash */
    else if (atk_edge && s_p2.attack_timer == 0 &&
             s_p2.state != ALADDIN_STATE_CLIMB_VERTICAL && s_p2.state != ALADDIN_STATE_HANG_HORIZONTAL) {
        s_p2.attack_timer = 16;
        s_p2.state = ALADDIN_STATE_SLASH;
    }

    /* State Machine Execution */
    if (s_p2.state == ALADDIN_STATE_CLIMB_VERTICAL) {
        /* Vertical Rope / Net Climbing State */
        s_p2.vx = 0;
        s_p2.vy = 0;
        s_p2.grounded = 0;

        /* D-Pad Up / Down vertical climbing */
        if (pad & 0x0010) { /* Up */
            if (!Aladdin_IsSolidAt(s_p2.world_x, s_p2.world_y - 32)) {
                s_p2.world_y -= 2;
            }
            s_p2.anim_timer++;
        } else if (pad & 0x0020) { /* Down */
            s_p2.world_y += 2;
            s_p2.anim_timer++;

            /* Floor landing check */
            int floor_y = Aladdin_FindFloorBelow(s_p2.world_x, s_p2.world_y - 8, 20);
            if (s_p2.world_y >= floor_y) {
                s_p2.world_y = floor_y;
                s_p2.grounded = 1;
                s_p2.state = ALADDIN_STATE_IDLE;
            }
        }

        /* Exit Transitions from Vertical Rope */
        if (jump_edge) {
            if (pad & 0x0020) {
                /* Drop release */
                s_p2.vy = 1;
                s_p2.state = ALADDIN_STATE_JUMP;
                s_p2.rope_cooldown = 15;
            } else {
                /* Athletic dismount jump */
                s_p2.vy = -8;
                s_p2.vx = (pad & 0x0040) ? -3 : ((pad & 0x0080) ? 3 : (s_p2.facing ? 3 : -3));
                s_p2.world_x += s_p2.vx;
                s_p2.state = ALADDIN_STATE_JUMP;
                s_p2.rope_cooldown = 15;
            }
        }
    }
    else if (s_p2.state == ALADDIN_STATE_HANG_HORIZONTAL) {
        /* Horizontal Rope / Net / Clothesline Hanging State */
        s_p2.vx = 0;
        s_p2.vy = 0;
        s_p2.grounded = 0;

        /* D-Pad Left / Right lateral shimmy */
        if (pad & 0x0040) { /* Left */
            if (!Aladdin_IsSolidAt(s_p2.world_x - 10, s_p2.world_y - 12)) {
                s_p2.world_x -= 2;
            }
            s_p2.facing = 1;
            s_p2.anim_timer++;
        } else if (pad & 0x0080) { /* Right */
            if (!Aladdin_IsSolidAt(s_p2.world_x + 10, s_p2.world_y - 12)) {
                s_p2.world_x += 2;
            }
            s_p2.facing = 0;
            s_p2.anim_timer++;
        }

        /* Exit Transitions from Horizontal Hang */
        if (jump_edge) {
            if (pad & 0x0020) {
                /* Drop release */
                s_p2.vy = 1;
                s_p2.state = ALADDIN_STATE_JUMP;
                s_p2.rope_cooldown = 15;
            } else {
                /* Athletic dismount jump */
                s_p2.vy = -8;
                s_p2.vx = (pad & 0x0040) ? -3 : ((pad & 0x0080) ? 3 : 0);
                s_p2.world_x += s_p2.vx;
                s_p2.state = ALADDIN_STATE_JUMP;
                s_p2.rope_cooldown = 15;
            }
        }
    }
    else {
        /* Normal Grounded & Airborne Physics */

        /* Horizontal Movement (if not locked in ground throw) */
        int speed = (pad & 0x0202) ? 3 : 2; /* Hold Y / X to dash/run */
        s_p2.vx = 0;
        if (s_p2.state != ALADDIN_STATE_THROW_GROUND) {
            if (pad & 0x0040) { /* D-Pad Left */
                s_p2.vx = -speed;
                s_p2.facing = 1;
            } else if (pad & 0x0080) { /* D-Pad Right */
                s_p2.vx = speed;
                s_p2.facing = 0;
            }
        }

        /* Animation timer & state */
        if (s_p2.vx != 0 && s_p2.grounded) {
            s_p2.anim_timer++;
            if (s_p2.state != ALADDIN_STATE_SLASH && s_p2.state != ALADDIN_STATE_THROW_GROUND) {
                s_p2.state = ALADDIN_STATE_RUN;
            }
        } else if (s_p2.grounded && s_p2.state != ALADDIN_STATE_SLASH &&
                   s_p2.state != ALADDIN_STATE_THROW_GROUND && s_p2.state != ALADDIN_STATE_CROUCH) {
            s_p2.state = ALADDIN_STATE_IDLE;
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

        /* Jump (Button B = 0x0001) - Edge triggered! */
        if (jump_edge && s_p2.grounded && s_p2.state != ALADDIN_STATE_THROW_GROUND) {
            s_p2.vy = -10; /* Full, satisfying SNES jump! */
            s_p2.grounded = 0;
            s_p2.state = ALADDIN_STATE_JUMP;
        }

        /* Crouch (D-Pad Down = 0x0020) */
        if ((pad & 0x0020) && s_p2.grounded && s_p2.vx == 0 &&
            s_p2.state != ALADDIN_STATE_SLASH && s_p2.state != ALADDIN_STATE_THROW_GROUND) {
            s_p2.state = ALADDIN_STATE_CROUCH;
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
                    s_p2.state = (s_p2.vx != 0) ? ALADDIN_STATE_RUN : ALADDIN_STATE_IDLE;
                }
            }

            /* Dynamic Rope & Net detection while airborne */
            if (!s_p2.grounded && s_p2.rope_cooldown == 0 && s_p2.state != ALADDIN_STATE_THROW_AIR) {
                int rope_sx = 0;
                if (CheckVerticalRopeOAM(s_p2.screen_x, s_p2.screen_y, &rope_sx)) {
                    if ((pad & 0x0030) != 0 || s_p2.vy > 0) {
                        s_p2.world_x = rope_sx + cam_x;
                        s_p2.rope_anchor_x = s_p2.world_x;
                        s_p2.state = ALADDIN_STATE_CLIMB_VERTICAL;
                        s_p2.vx = 0;
                        s_p2.vy = 0;
                    }
                }

                int rope_sy = 0;
                if (s_p2.state != ALADDIN_STATE_CLIMB_VERTICAL &&
                    CheckHorizontalRopeOAM(s_p2.screen_x, s_p2.screen_y, &rope_sy)) {
                    s_p2.world_y = rope_sy + cam_y;
                    s_p2.rope_anchor_y = s_p2.world_y;
                    s_p2.state = ALADDIN_STATE_HANG_HORIZONTAL;
                    s_p2.vx = 0;
                    s_p2.vy = 0;
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

/* Ground throw wind-up (10 pieces, tag=throw_peak_f16, cx=128, fy=149) */
static const AladdinSpriteDef kAladdinThrowWindupSprites[10] = {
    {   -9,   -8, 0x0B, 0x20, 0 },
    {    9,    0, 0x0D, 0x20, 0 },
    {    1,    0, 0x0C, 0x20, 0 },
    {   -9,  -16, 0x0A, 0x20, 0 },
    {    8,  -32, 0x09, 0x20, 0 },
    {   16,  -40, 0x08, 0x20, 0 },
    {   -1,  -16, 0x04, 0x20, 1 },
    {   -8,  -32, 0x02, 0x20, 1 },
    {  -16,  -48, 0x06, 0x20, 1 },
    {    0,  -48, 0x00, 0x20, 1 },
};

/* Ground throw release (9 pieces, tag=throw_peak_f20, cx=130, fy=161) */
static const AladdinSpriteDef kAladdinThrowReleaseSprites[9] = {
    {    4,    0, 0x0B, 0x20, 0 },
    {   16,  -45, 0x0A, 0x20, 0 },
    {   -8,  -40, 0x09, 0x20, 0 },
    {   -8,  -48, 0x08, 0x20, 0 },
    {   -8,  -56, 0x07, 0x20, 0 },
    {  -16,  -56, 0x06, 0x20, 0 },
    {   -4,  -16, 0x04, 0x20, 1 },
    {   -8,  -32, 0x02, 0x20, 1 },
    {    0,  -48, 0x00, 0x20, 1 },
};

/* Ground throw follow-through (9 pieces, tag=apple_thrown_f76, cx=84, fy=159) */
static const AladdinSpriteDef kAladdinThrowFollowSprites[9] = {
    {    6,  -32, 0x02, 0x20, 1 },
    {    8,  -48, 0x00, 0x20, 1 },
    {   -8,    0, 0x0B, 0x20, 0 },
    {   -8,   -8, 0x0A, 0x20, 0 },
    {   -8,  -16, 0x09, 0x20, 0 },
    {   -2,  -24, 0x08, 0x20, 0 },
    {   -2,  -32, 0x07, 0x20, 0 },
    {    0,  -40, 0x06, 0x20, 0 },
    {    0,  -16, 0x04, 0x20, 1 },
};

/* Vertical rope climb motion cycle 1 (9 pieces, tag=test_climb_rope, cx=131, fy=122) */
static const AladdinSpriteDef kAladdinClimbSprites[9] = {
    {  -16,    0, 0x0A, 0x20, 0 },
    {  -16,   -8, 0x09, 0x20, 0 },
    {   16,  -17, 0x08, 0x20, 0 },
    {    8,  -17, 0x07, 0x20, 0 },
    {    8,  -25, 0x06, 0x20, 0 },
    {    8,  -33, 0x05, 0x20, 0 },
    {    6,  -40, 0x04, 0x20, 0 },
    {   -8,  -16, 0x02, 0x20, 1 },
    {   -8,  -32, 0x00, 0x20, 1 },
};

/* Vertical rope climb motion cycle 2 (7 pieces, tag=stage1_pos8, cx=132, fy=150) */
static const AladdinSpriteDef kAladdinClimbMotionSprites[7] = {
    {    4,    0, 0x09, 0x20, 0 },
    {   12,   -8, 0x08, 0x20, 0 },
    {  -12,  -16, 0x07, 0x20, 0 },
    {  -10,  -40, 0x06, 0x20, 0 },
    {   12,    0, 0x04, 0x20, 1 },
    {   -4,  -16, 0x02, 0x20, 1 },
    {  -12,  -32, 0x00, 0x20, 1 },
};

/* Vertical rope idle grip (11 pieces, tag=test_reach_rope, cx=126, fy=90) */
static const AladdinSpriteDef kAladdinReachRopeSprites[11] = {
    {    5,  -48, 0x0D, 0x20, 0 },
    {   13,    0, 0x0C, 0x20, 0 },
    {    5,    0, 0x0B, 0x20, 0 },
    {    5,  -24, 0x0A, 0x20, 0 },
    {   -3,  -24, 0x09, 0x20, 0 },
    {  -13,  -16, 0x02, 0x20, 1 },
    {    3,  -16, 0x04, 0x20, 1 },
    {  -11,  -32, 0x08, 0x20, 0 },
    {  -11,  -40, 0x07, 0x20, 0 },
    {   13,  -39, 0x06, 0x20, 0 },
    {   -3,  -40, 0x00, 0x20, 1 },
};

/* Horizontal clothesline / rope idle hang (10 pieces, tag=stage1_pos5, cx=125, fy=148) */
static const AladdinSpriteDef kAladdinHangHorizontalIdleSprites[10] = {
    {  -12,    0, 0x0C, 0x20, 0 },
    {    4,  -16, 0x0B, 0x20, 0 },
    {   12,  -24, 0x0A, 0x20, 0 },
    {   12,  -32, 0x09, 0x20, 0 },
    {   -2,  -40, 0x08, 0x20, 0 },
    {  -10,  -40, 0x07, 0x20, 1 },
    {   -2,  -48, 0x06, 0x20, 1 },
    {  -12,  -16, 0x04, 0x20, 1 },
    {   -4,  -32, 0x02, 0x20, 1 },
    {    6,  -48, 0x00, 0x20, 0 },
};

/* Horizontal clothesline / rope shimmy movement (9 pieces, tag=stage1_pos6, cx=126, fy=89) */
static const AladdinSpriteDef kAladdinHangHorizontalShimmySprites[9] = {
    {  -16,    0, 0x0C, 0x20, 0 },
    {   17,  -34, 0x0B, 0x20, 0 },
    {   -7,  -24, 0x0A, 0x20, 0 },
    {   -7,  -32, 0x09, 0x20, 0 },
    {   -7,  -40, 0x08, 0x20, 0 },
    {  -15,  -16, 0x06, 0x20, 1 },
    {    1,  -16, 0x04, 0x20, 1 },
    {    1,  -32, 0x02, 0x20, 1 },
    {    1,  -48, 0x00, 0x20, 1 },
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

    if (s_p2.state == ALADDIN_STATE_RUN) {
        /* Run: alternate two running cycle frames every 8 ticks */
        if ((s_p2.anim_timer >> 3) & 1) {
            EmitPlayer2FromTable(kAladdinRunSprites2, 7);
        } else {
            EmitPlayer2FromTable(kAladdinRunSprites1, 9);
        }
        return;
    }
    if (s_p2.state == ALADDIN_STATE_JUMP) {
        /* Jump */
        EmitPlayer2FromTable(kAladdinJumpSprites, 10);
        return;
    }
    if (s_p2.state == ALADDIN_STATE_SLASH) {
        /* Attack / Sword Slash */
        EmitPlayer2FromTable(kAladdinAttackSprites, 10);
        return;
    }
    if (s_p2.state == ALADDIN_STATE_CROUCH) {
        /* Crouch */
        EmitPlayer2FromTable(kAladdinCrouchSprites, 6);
        return;
    }
    if (s_p2.state == ALADDIN_STATE_THROW_GROUND) {
        /* Ground Apple Throw: Windup (16..12) -> Release (11..6) -> Follow-through (5..1) */
        if (s_p2.attack_timer > 11) {
            EmitPlayer2FromTable(kAladdinThrowWindupSprites, 10);
        } else if (s_p2.attack_timer > 5) {
            EmitPlayer2FromTable(kAladdinThrowReleaseSprites, 9);
        } else {
            EmitPlayer2FromTable(kAladdinThrowFollowSprites, 9);
        }
        return;
    }
    if (s_p2.state == ALADDIN_STATE_THROW_AIR) {
        /* Aerial Apple Throw: Release -> Follow-through */
        if (s_p2.attack_timer > 5) {
            EmitPlayer2FromTable(kAladdinThrowReleaseSprites, 9);
        } else {
            EmitPlayer2FromTable(kAladdinThrowFollowSprites, 9);
        }
        return;
    }
    if (s_p2.state == ALADDIN_STATE_CLIMB_VERTICAL) {
        /* Vertical Rope / Net: Climbing motion or Idle Grip */
        if ((s_p2.last_pad & 0x0030) != 0) {
            if ((s_p2.anim_timer >> 3) & 1) {
                EmitPlayer2FromTable(kAladdinClimbMotionSprites, 7);
            } else {
                EmitPlayer2FromTable(kAladdinClimbSprites, 9);
            }
        } else {
            EmitPlayer2FromTable(kAladdinReachRopeSprites, 11);
        }
        return;
    }
    if (s_p2.state == ALADDIN_STATE_HANG_HORIZONTAL) {
        /* Horizontal Rope / Clothesline / Net: Lateral Shimmy or Idle Hang */
        if ((s_p2.last_pad & 0x00C0) != 0 || s_p2.vx != 0) {
            EmitPlayer2FromTable(kAladdinHangHorizontalShimmySprites, 9);
        } else {
            EmitPlayer2FromTable(kAladdinHangHorizontalIdleSprites, 10);
        }
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

        /* 8x8 sprite */
        int high_byte_idx = slot / 4;
        int high_bit_shift = (slot % 4) * 2;
        g_ram[0xFC80 + high_byte_idx] &= ~(0x03 << high_bit_shift);

        if (g_ppu) {
            g_ppu->oam[slot * 2 + 0] = (uint16_t)((uint8_t)hx | (hud_y << 8));
            g_ppu->oam[slot * 2 + 1] = (uint16_t)(tile | (attr << 8));
            g_ppu->highOam[high_byte_idx] = g_ram[0xFC80 + high_byte_idx];
        }
    }
}

/* Emit Player 2 Apple Projectiles into OAM slots 96..99 */
static void EmitPlayer2Projectiles(void)
{
    for (int p = 0; p < MAX_P2_PROJECTILES; ++p) {
        int slot = 96 + p;
        if (s_p2_apples[p].active) {
            int16_t sx = s_p2_apples[p].screen_x;
            int16_t sy = s_p2_apples[p].screen_y;
            if (sx < 0) sx = 0;
            if (sx > 255) sx = 255;
            if (sy < 0) sy = 0;
            if (sy > 224) sy = 224;

            /* Rotating apple sprite: 0xD3 / 0xD2 */
            uint8_t tile = (s_p2_apples[p].anim_timer & 4) ? 0xD2 : 0xD3;
            uint8_t attr = (5 << 1) | (3 << 4); /* Palette 5, Priority 3 */

            g_ram[0xFA80 + slot * 4 + 0] = (uint8_t)sx;
            g_ram[0xFA80 + slot * 4 + 1] = (uint8_t)sy;
            g_ram[0xFA80 + slot * 4 + 2] = tile;
            g_ram[0xFA80 + slot * 4 + 3] = attr;

            /* 16x16 size (size bit = 1) */
            int high_byte_idx = slot / 4;
            int high_bit_shift = (slot % 4) * 2;
            uint8_t val = (1 << 1);

            g_ram[0xFC80 + high_byte_idx] &= ~(0x03 << high_bit_shift);
            g_ram[0xFC80 + high_byte_idx] |= (val << high_bit_shift);

            if (g_ppu) {
                g_ppu->oam[slot * 2 + 0] = (uint16_t)((uint8_t)sx | ((uint8_t)sy << 8));
                g_ppu->oam[slot * 2 + 1] = (uint16_t)(tile | (attr << 8));
                g_ppu->highOam[high_byte_idx] = g_ram[0xFC80 + high_byte_idx];
            }
        } else {
            g_ram[0xFA80 + slot * 4 + 1] = 225;
            if (g_ppu) g_ppu->oam[slot * 2] = (225 << 8);
        }
    }
}

/* Emit Player 2 Apple HUD into OAM slots 100..102 and suppress 103..127 */
static void EmitPlayer2AppleHud(void)
{
    const int16_t hud_y = 28;
    const int16_t apple_icon_x = 210;
    const int16_t tens_x = 222;
    const int16_t ones_x = 230;

    uint8_t tens = (s_p2.apples / 10) % 10;
    uint8_t ones = s_p2.apples % 10;
    uint8_t attr = (5 << 1) | (3 << 4); /* Palette 5, Priority 3 */

    /* Slot 100: Mini apple icon */
    g_ram[0xFA80 + 100 * 4 + 0] = (uint8_t)apple_icon_x;
    g_ram[0xFA80 + 100 * 4 + 1] = (uint8_t)hud_y;
    g_ram[0xFA80 + 100 * 4 + 2] = 0xD2;
    g_ram[0xFA80 + 100 * 4 + 3] = attr;

    /* Slot 101: Tens digit */
    g_ram[0xFA80 + 101 * 4 + 0] = (uint8_t)tens_x;
    g_ram[0xFA80 + 101 * 4 + 1] = (uint8_t)hud_y;
    g_ram[0xFA80 + 101 * 4 + 2] = 0x70 + tens;
    g_ram[0xFA80 + 101 * 4 + 3] = attr;

    /* Slot 102: Ones digit */
    g_ram[0xFA80 + 102 * 4 + 0] = (uint8_t)ones_x;
    g_ram[0xFA80 + 102 * 4 + 1] = (uint8_t)hud_y;
    g_ram[0xFA80 + 102 * 4 + 2] = 0x70 + ones;
    g_ram[0xFA80 + 102 * 4 + 3] = attr;

    for (int s = 100; s <= 102; ++s) {
        int high_byte_idx = s / 4;
        int high_bit_shift = (s % 4) * 2;
        g_ram[0xFC80 + high_byte_idx] &= ~(0x03 << high_bit_shift);

        if (g_ppu) {
            uint8_t x = g_ram[0xFA80 + s * 4 + 0];
            uint8_t y = g_ram[0xFA80 + s * 4 + 1];
            uint8_t tile = g_ram[0xFA80 + s * 4 + 2];
            uint8_t a = g_ram[0xFA80 + s * 4 + 3];
            g_ppu->oam[s * 2 + 0] = (uint16_t)(x | (y << 8));
            g_ppu->oam[s * 2 + 1] = (uint16_t)(tile | (a << 8));
            g_ppu->highOam[high_byte_idx] = g_ram[0xFC80 + high_byte_idx];
        }
    }

    /* Suppress slots 103..127 */
    for (int s = 103; s < 128; ++s) {
        g_ram[0xFA80 + s * 4 + 1] = 225;
        if (g_ppu) g_ppu->oam[s * 2] = (225 << 8);
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

    /* Update Player 2 Apple projectiles */
    UpdatePlayer2Projectiles();

    /* Apply Prince Ali Palette to CGRAM */
    ApplyPrinceAliPalette();

    /* Suppress Abu the monkey */
    SuppressAbu();

    /* Emit Player 2 sprites */
    EmitPlayer2Sprites(p1_center_x, p1_feet_y, p1_slots, p1_count);

    /* Emit Player 2 HUD Hearts */
    EmitPlayer2Hud();

    /* Emit Player 2 Apple Projectiles */
    EmitPlayer2Projectiles();

    /* Emit Player 2 Apple HUD */
    EmitPlayer2AppleHud();
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
    EmitPlayer2Projectiles();
    EmitPlayer2AppleHud();
}
