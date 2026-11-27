/*
 * Aladdin — desktop host shim.
 *
 * Identity and hooks, nothing else. The host itself is the framework's
 * (snesrecomp/runner/src/desktop/host_main.h): the pre-boot launcher, ROM
 * resolution and digest checks, config.ini and keybinds.ini, the window and
 * the SDL / OpenGL presenters, audio, gamepads, the in-game save-state
 * browser and rewind filmstrip, the OSD, the pacing clock, crash handlers and
 * the post-mortem report, mod packages, Generate & rebuild, and the netplay
 * barrier when the project is built with it. A fix there reaches this
 * project on a submodule pull; nothing in this file needs to change for it.
 *
 * This file used to BE the host -- a ~700-line copy made at scaffold time --
 * so every project scaffolded before a fix kept the bug. Two ports of the
 * same game, a week apart, had different hosts.
 *
 * Nothing here ever ships a ROM. The player supplies one; the host makes
 * that easy (launcher, then positional argument, then a copy beside the
 * executable, then the rom.cfg cache, then a file picker) and checks what it
 * is handed against the digests in rom_identity.txt.
 *
 * Grow this file only with what is specific to THIS title: a custom
 * presenter (prepare_frame / draw_frame), an SPC player, a pacing rule
 * (keep_pacing_debt), a Mods provider. See SnesDesktopHostGame for the
 * complete list of hooks and what each is for.
 */

#include "host_main.h"
#include "recomp_launcher.h"
#include "game_rtl.h"
#include "live_log.h"
#include "snesrecomp_rom_identity.h"  /* generated from rom_identity.txt */
#include "desktop/config.h"
#include "desktop/display_aspect.h"
#include "snes/cart.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/ws_shadow.h"
#include "widescreen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __ANDROID__
#define SDL_MAIN_HANDLED 1
#endif
#include "desktop/sdl_compat.h"
#ifdef __ANDROID__
/* SDLActivity loads libmain.so and dlsyms its SDL_main. */
#define main SDL_main
#endif

#ifndef SNES_GAME_VERSION
#define SNES_GAME_VERSION "dev"
#endif

extern Ppu *g_ppu;
extern Snes *g_snes;
extern uint8_t g_ram[0x20000];

/* Frame-local memo cache for Aladdin 8x8 tile lookups from $7F:F000 + $9C metatile ROM table. */
static uint32_t s_ws_tile_keys[4096];
static uint16_t s_ws_tile_vals[4096];
static uint16_t s_ws_bg1_bias = 0;
static uint16_t s_ws_bg2_bias = 0x0400;
static bool     s_ws_bg2_loop512 = true;

static uint16_t AladdinWram16(uint32_t addr)
{
    return (uint16_t)(g_ram[addr] | (g_ram[addr + 1] << 8));
}

static uint16_t AladdinReadRomTile16(uint8_t bank, uint16_t addr)
{
    if (!g_snes || !g_snes->cart)
        return 0;
    uint8_t lo = cart_read(g_snes->cart, bank, addr);
    uint8_t hi = cart_read(g_snes->cart, bank, (uint16_t)(addr + 1));
    return (uint16_t)(lo | (hi << 8));
}

/* Exact tilemap lookup matching Aladdin ROM $81:C1A2 + $81:C315 ($7F:F000 screen table). */
static uint16_t AladdinWorldLayerTile(uint32_t tx, uint32_t ty, uint16_t bias)
{
    uint32_t table_idx = (((((ty >> 5) + 1u) & 0xFFu) << 6)
                        + ((((tx >> 5) + 1u) & 0x7Fu) << 1)
                        + (uint32_t)bias) & 0x0FFEu;
    uint16_t scr_off = AladdinWram16(0x1F000u + table_idx);
    if (scr_off & 0x8000u)
        scr_off = 0;

    uint32_t wram_off = 0x10000u + (uint32_t)scr_off
                      + (((ty >> 1) & 0x0Fu) << 5)
                      + (((tx >> 1) & 0x0Fu) << 1);
    if (wram_off + 1 >= sizeof(g_ram))
        return 0;

    uint16_t mt_disp = AladdinWram16(wram_off);
    uint8_t mt_bank = g_ram[0x005C];
    if (mt_bank == 0)
        return 0;

    uint16_t rom_off = (uint16_t)(mt_disp | 0x8000u)
                     + (uint16_t)(((ty & 1u) << 2) | ((tx & 1u) << 1));
    return AladdinReadRomTile16(mt_bank, rom_off);
}

static uint16_t AladdinCachedLayerTile(int layer, uint32_t tx, uint32_t ty, uint16_t bias)
{
    uint32_t key = (tx & 1023u) | ((ty & 1023u) << 10) | ((uint32_t)(layer & 1) << 20);
    unsigned slot = (key * 2654435761u) >> 20;
    if (s_ws_tile_keys[slot] == key + 1u)
        return s_ws_tile_vals[slot];

    uint16_t val = AladdinWorldLayerTile(tx, ty, bias);
    s_ws_tile_keys[slot] = key + 1u;
    s_ws_tile_vals[slot] = val;
    return val;
}

static uint16_t AladdinVramWord(uint16_t map_base, bool wide, uint32_t tx, uint32_t ty)
{
    uint16_t x_hi = wide ? (uint16_t)(((tx >> 5) & 1u) * 0x400u) : 0u;
    return (uint16_t)(map_base + x_hi + (ty & 31u) * 32u + (tx & 31u));
}

/* Verify that $7F:F000 + $9C matches live native VRAM for the requested BG layer. */
static bool AladdinLayerStreamValid(int layer, int32_t wx, int32_t wy, uint16_t bias)
{
    if (wx < 0 || wy < 0 || !g_ppu || !g_snes || !g_snes->cart)
        return false;
    uint16_t mt_addr = AladdinWram16(0x005A);
    uint8_t mt_bank = g_ram[0x005C];
    if (mt_addr < 0x8000 || mt_bank == 0)
        return false;

    uint16_t map_base = (uint16_t)PPU_bgTilemapAdr(g_ppu, layer);
    bool wide = PPU_bgTilemapWider(g_ppu, layer) != 0;
    int miss = 0;
    for (int i = 0; i < 12; i++) {
        uint32_t tx = ((uint32_t)wx + 12u + (uint32_t)i * 20u) >> 3;
        uint32_t ty = ((uint32_t)wy + 16u + (uint32_t)(i % 6) * 34u) >> 3;
        uint16_t want = AladdinCachedLayerTile(layer, tx, ty, bias);
        uint16_t got = g_ppu->vram[AladdinVramWord(map_base, wide, tx, ty) & 0x7FFFu];
        if (want != got) {
            if (++miss > 2)
                return false;
        }
    }
    return true;
}

static int32_t s_ws_bg1_min_tx = 0;
static int32_t s_ws_bg1_max_tx = 1024;
static bool s_ws_bg1_is_shaft = false;

static WsShadowSource AladdinBg1WorldProvider(void *context, uint32_t tx, uint32_t ty,
                                              uint16_t *entry)
{
    (void)context;
    /* BG1 worldX is biased by +512 (+64 tiles) so left margins when wx < 43
     * stay >= 0 for WsShadowTile instead of forcing ViewAnchor HUD sliding. */
    int32_t real_tx = (int32_t)tx - 64;
    if (ty >= 512)
        return kWsShadowMiss;
    int32_t sample_tx = real_tx;
    uint16_t flip = 0;
    if (real_tx < s_ws_bg1_min_tx) {
        if (s_ws_bg1_is_shaft) {
            sample_tx = s_ws_bg1_min_tx;
        } else {
            int32_t d = s_ws_bg1_min_tx - 1 - real_tx;
            if (d > 8)
                return kWsShadowMiss;
            sample_tx = s_ws_bg1_min_tx + d;
            flip = 0x4000u;
        }
    } else if (real_tx >= s_ws_bg1_max_tx) {
        if (s_ws_bg1_is_shaft) {
            sample_tx = s_ws_bg1_max_tx - 1;
        } else {
            int32_t d = real_tx - s_ws_bg1_max_tx;
            if (d > 8)
                return kWsShadowMiss;
            sample_tx = s_ws_bg1_max_tx - 1 - d;
            flip = 0x4000u;
        }
    }
    if (sample_tx < 0 || sample_tx >= 1024)
        return kWsShadowMiss;
    uint16_t tile = AladdinCachedLayerTile(0, (uint32_t)sample_tx, ty, s_ws_bg1_bias);
    *entry = (tile != 0) ? (tile ^ flip) : 0;
    /* Return MarginOverride so native VRAM pixels stay untouched while margins always
     * sample the live $7F:F000 map without stale circular-VRAM columns. */
    return kWsShadowMarginOverride;
}

static WsShadowSource AladdinBg2WaveProvider(void *context, uint32_t tx, uint32_t ty,
                                             uint16_t *entry)
{
    (void)context;
    if (!g_ppu)
        return kWsShadowMiss;
    uint32_t hs1 = g_ppu->hScroll[1] & 511u;
    /* Before the Stage 3-2 lava wave enters native view (hScroll[1] >= 256, blank R page),
     * keep both widescreen margins transparent so L's solid 0x7500 fill never bleeds. */
    if (hs1 >= 256u) {
        *entry = 0x0000;
        return kWsShadowMarginOverride;
    }
    int32_t real_tx = (int32_t)tx - 64;
    if (real_tx >= 32) {
        *entry = 0x0000;
        return kWsShadowMarginOverride;
    }
    uint32_t sample_col = (real_tx < 0) ? 0u : (uint32_t)real_tx;
    uint32_t base2 = (uint32_t)(g_ppu->bgXsc[1] & 0xFCu) << 8;
    uint32_t voff = ((g_ppu->bgXsc[1] & 0x02u) && (ty & 32u)) ? 0x800u : 0u;
    *entry = g_ppu->vram[(base2 + voff + ((ty & 31u) << 5) + sample_col) & 0x7FFFu];
    return kWsShadowMarginOverride;
}

static int32_t s_ws_bg2_min_tx = 0;
static int32_t s_ws_bg2_max_tx = 1024;

static WsShadowSource AladdinBg2WorldProvider(void *context, uint32_t tx, uint32_t ty,
                                              uint16_t *entry)
{
    (void)context;
    /* BG2 worldX was biased by +512 (+64 tiles) so negative left-margin parallax
     * coordinates stay >= 0 for WsShadowTile. Convert back to real world tile X. */
    int32_t real_tx = (int32_t)tx - 64;
    if (ty >= 512)
        return kWsShadowMiss;
    int32_t sample_tx;
    uint16_t flip = 0;
    if (s_ws_bg2_loop512) {
        sample_tx = real_tx & 63;
    } else {
        sample_tx = real_tx;
        if (real_tx < s_ws_bg2_min_tx) {
            int32_t d = s_ws_bg2_min_tx - 1 - real_tx;
            if (d > 8)
                return kWsShadowMiss;
            sample_tx = s_ws_bg2_min_tx + d;
            flip = 0x4000u;
        } else if (real_tx >= s_ws_bg2_max_tx) {
            int32_t d = real_tx - s_ws_bg2_max_tx;
            if (d > 8)
                return kWsShadowMiss;
            sample_tx = s_ws_bg2_max_tx - 1 - d;
            flip = 0x4000u;
        }
        if (sample_tx < 0 || sample_tx >= 1024)
            return kWsShadowMiss;
    }
    uint16_t tile = AladdinCachedLayerTile(1, (uint32_t)sample_tx, ty, s_ws_bg2_bias);
    *entry = (tile != 0) ? (tile ^ flip) : 0;
    return kWsShadowMarginOverride;
}

static WsShadowSource AladdinBg2FireProvider(void *context, uint32_t tx, uint32_t ty,
                                             uint16_t *entry)
{
    (void)context;
    if (!g_ppu)
        return kWsShadowMiss;
    /* Stage 6-3 rising/falling fire (0x10): repeat live 8-tile cycles from the native
     * viewport so sky rows stay clean and R[30..31] stale fire garbage is never folded. */
    uint8_t sc = g_ppu->bgXsc[1];
    uint32_t width_mask = (sc & 0x01u) ? 63u : 31u;
    uint32_t hs1 = g_ppu->hScroll[1] & (width_mask * 8u + 7u);
    int32_t nat_col = (int32_t)(hs1 >> 3);
    int32_t real_tx = (int32_t)tx - 64;
    int32_t rel = (real_tx - nat_col) & 7;
    uint32_t base2 = (uint32_t)(sc & 0xFCu) << 8;
    uint32_t voff = ((sc & 0x02u) && (ty & 32u)) ? ((sc & 0x01u) ? 0x800u : 0x400u) : 0u;
    uint32_t row_off = (ty & 31u) << 5;
    uint16_t out = 0;
    if (real_tx < nat_col || real_tx >= nat_col + 32) {
        int step = (real_tx < nat_col) ? 1 : -1;
        int start = (real_tx < nat_col) ? 0 : 3;
        for (int i = 0; i < 4; i++) {
            uint32_t c = (uint32_t)(nat_col + (start + i * step) * 8 + rel) & width_mask;
            uint32_t page = (c >= 32u) ? 0x400u : 0u;
            uint16_t t = g_ppu->vram[(base2 + page + voff + row_off + (c & 31u)) & 0x7FFFu];
            if (t != 0u) {
                out = t;
                break;
            }
        }
    } else {
        uint32_t c = (uint32_t)real_tx & width_mask;
        uint32_t page = (c >= 32u) ? 0x400u : 0u;
        out = g_ppu->vram[(base2 + page + voff + row_off + (c & 31u)) & 0x7FFFu];
    }
    *entry = out;
    return kWsShadowMarginOverride;
}

static WsShadowSource AladdinJafarArenaProvider(void *context, uint32_t tx, uint32_t ty,
                                                uint16_t *entry)
{
    if (!g_ppu)
        return kWsShadowMiss;
    /* Stage 6-4 Wizard & Cobra Jafar throne room (0x11): L[0..31] holds animated 4-tile
     * repeating arches/floor while R[0..1] has 0x0000 holes; wrap cleanly within L[0..31]. */
    uint32_t layer = (uint32_t)(uintptr_t)context;
    uint8_t sc = g_ppu->bgXsc[layer];
    uint32_t base = (uint32_t)(sc & 0xFCu) << 8;
    int32_t real_tx = (int32_t)tx - 64;
    uint32_t sample_col = (uint32_t)(real_tx & 31);
    *entry = g_ppu->vram[(base + ((ty & 31u) << 5) + sample_col) & 0x7FFFu];
    return kWsShadowMarginOverride;
}

static bool AladdinScreenColValid(uint32_t col, uint32_t row_a, uint32_t row_b, uint16_t bias, bool is_bg2)
{
    if (col < 1u || col >= 31u)
        return false;
    for (uint32_t row = row_a; row <= row_b; row++) {
        uint32_t table_idx = ((((row & 0xFFu) << 6) + (col << 1) + (uint32_t)bias) & 0x0FFEu);
        uint16_t scr_off = AladdinWram16(0x1F000u + table_idx);
        if ((scr_off & 0x8000u) == 0 && (0x10000u + (uint32_t)scr_off + 512u) <= sizeof(g_ram)) {
            /* scr_off == 0 aliases $7F:0000 (stage start room). Reject duplicate 0x0000 on
             * later columns (or BG2 trailing cols) so Stage 2-2 & Stage 5 never wrap to start. */
            if (scr_off == 0u && (col > 1u || is_bg2)) {
                bool seen_zero = false;
                if (is_bg2) {
                    for (uint32_t r = 1u; r <= 16u && !seen_zero; r++) {
                        for (uint32_t c = 1u; c <= 30u; c++) {
                            uint32_t p_idx = ((((r & 0xFFu) << 6) + (c << 1) + (uint32_t)s_ws_bg1_bias) & 0x0FFEu);
                            if (AladdinWram16(0x1F000u + p_idx) == 0u) {
                                seen_zero = true;
                                break;
                            }
                        }
                    }
                }
                for (uint32_t r = 1u; r <= 16u && !seen_zero; r++) {
                    for (uint32_t c = 1u; c < col; c++) {
                        uint32_t p_idx = ((((r & 0xFFu) << 6) + (c << 1) + (uint32_t)bias) & 0x0FFEu);
                        if (AladdinWram16(0x1F000u + p_idx) == 0u) {
                            seen_zero = true;
                            break;
                        }
                    }
                }
                if (seen_zero)
                    continue;
            }
            return true;
        }
    }
    return false;
}

static bool AladdinBg1ScreenColValid(uint32_t col, uint32_t row_a, uint32_t row_b, uint16_t bias)
{
    return AladdinScreenColValid(col, row_a, row_b, bias, false);
}

static bool AladdinIsLiveGameplay(void)
{
    /* Do NOT reject 0x0367 == 0 or 0x0364 == 0 here: losing a heart or dropping into a pit
     * zeros 0x0367/0x0364 while the live stage graphics are still visible on screen during
     * the death animation. Game Over / Continue (0x035E == 0x25) and Title/Cards (0x35/0x40)
     * are filtered cleanly below and by AladdinLayerStreamValid(0). */

    /* Game Over screen */
    if (g_ram[0x08E2] == 0x0F)
        return false;

    /* Playable stages in $80:B73E use room IDs 0x00..0x1C; menus/cutscenes/title/intro
     * cards use IDs >= 0x20 (e.g. 0x35 Title, 0x40 Abu-Lamp transition). */
    if (g_ram[0x035E] > 0x1Cu)
        return false;

    /* Note: Do NOT reject 0x03FA == 1 here! Stage exit animations (Abu clapping,
     * carpet lift, white/black fadeout) set 0x03FA = 1 while the live stage graphics
     * and $7F:F000 tilemap are still active. Screen mode checks and AladdinLayerStreamValid
     * keep non-stage cutscenes/Stage Clear in 4:3. */

    /* Require live hardware Mode 1 (or Mode 2 OPT on Stage 6-4 Cobra Jafar boss)
     * with a 64-tile-wide BG1 tilemap (excludes Mode 7 cutscenes, Bonus Wheel, static screens). */
    uint8_t mode = g_ppu->bgmode & 0x07u;
    bool mode_ok = (mode == 1u) || (mode == 2u && g_ram[0x035E] == 0x11u);
    if (!mode_ok || (g_ppu->bgXsc[0] & 0x03u) != 1u)
        return false;

    /* Aladdin must be spawned inside an active stage */
    uint16_t al_x = AladdinWram16(0x08E7);
    uint16_t al_y = AladdinWram16(0x08EA);
    if (al_x == 0 && al_y == 0)
        return false;

    return true;
}

static void AladdinClearUnusedPillarboxes(void)
{
    if (!g_ppu || !g_ppu->renderBuffer || !g_ppu->renderPitch)
        return;
    int frame_w = 256 + (int)g_ppu->extraFrameLeft + (int)g_ppu->extraFrameRight;
    int left_bar = (int)g_ppu->extraFrameLeft - (int)g_ppu->extraLeftCur;
    int right_start = (int)g_ppu->extraFrameLeft + 256 + (int)g_ppu->extraRightCur;
    int right_bar = frame_w - right_start;
    if (left_bar <= 0 && right_bar <= 0)
        return;
    for (int y = 0; y < 224; y++) {
        uint8_t *row = g_ppu->renderBuffer + (size_t)y * g_ppu->renderPitch;
        if (left_bar > 0)
            memset(row, 0, (size_t)left_bar * 4);
        if (right_bar > 0)
            memset(row + (size_t)right_start * 4, 0, (size_t)right_bar * 4);
    }
}

static void AladdinBeginFrame(unsigned number)
{
    (void)number;
    static bool s_ws_shadow_was_active = false;
    static uint32_t s_last_stage_key = 0;
    if (!g_ppu || !g_config.widescreen) {
        if (s_ws_shadow_was_active) {
            WsShadowReset();
            s_ws_shadow_was_active = false;
        }
        return;
    }

    uint32_t stage_key = ((uint32_t)g_ram[0x035C] << 24) |
                         ((uint32_t)g_ram[0x035D] << 16) |
                         (uint32_t)AladdinWram16(0x005A);
    if (stage_key != s_last_stage_key) {
        if (s_ws_shadow_was_active) {
            WsShadowReset();
            s_ws_shadow_was_active = false;
        }
        s_last_stage_key = stage_key;
    }

    /* Non-gameplay screens (Title, Logos, Story Cutscenes, Bonus Wheel, Stage Clear, Game Over):
     * centered 4:3 pillarboxes. Live stages (including Stage 1-2, 035D == 1) flow into the
     * $7F:F000 verification path below. */
    if (!AladdinIsLiveGameplay()) {
        if (s_ws_shadow_was_active) {
            WsShadowReset();
            s_ws_shadow_was_active = false;
        }
        /* Stage Clear screen (0x035E == 0x24): extend dark-purple backdrop cleanly to 16:9
         * while keeping centered BG1/BG2/BG3 borders unstretched and Genie sprites unclipped. */
        if (g_ram[0x035E] == 0x24u && (g_ppu->bgmode & 0x07) == 1) {
            PpuSetExtraSpace(g_ppu, (uint16_t)g_ws_extra);
            PpuSetWidescreenViewAnchor(g_ppu, false);
            PpuSetExtraSideSpace(g_ppu, g_ws_extra, g_ws_extra, 0);
            PpuSetWidescreenLayerClamp(g_ppu, 0x07);
            PpuSetWidescreenWindowExpansion(g_ppu, 0x00, 0x00);
            return;
        }
        PpuSetExtraSpaceCentered(g_ppu, (uint16_t)g_ws_extra);
        PpuSetWidescreenWindowExpansion(g_ppu, 0x00, 0x00);
        AladdinClearUnusedPillarboxes();
        return;
    }

    /* Active scrolling stage: keep the 256px viewport centered (ViewAnchor off) so the HUD
     * (lives, hearts, apples, gems) and background never slide sideways near stage edges. */
    PpuSetExtraSpace(g_ppu, (uint16_t)g_ws_extra);
    PpuSetWidescreenViewAnchor(g_ppu, false);

    /* Align WRAM BG1 camera origin to PPU's hardware scroll phase (P1 invariant). */
    int32_t wx = (int32_t)AladdinWram16(0x176C);
    if (wx == 0)
        wx = (int32_t)AladdinWram16(0x19ED);
    int32_t wy = (int32_t)AladdinWram16(0x1777);

    int32_t dx = ((int32_t)g_ppu->hScroll[0] - wx) & 511;
    if (dx >= 256) dx -= 512;
    int32_t dy = ((int32_t)g_ppu->vScroll[0] - wy) & 255;
    if (dy >= 128) dy -= 256;
    wx += dx;
    wy += dy;
    if (wx < 0) wx = 0;
    if (wy < 0) wy = 0;

    /* Read $7F:F000 screen table biases before clamping bounds so void sentinel screens
     * (0xFE00 / bit 15 set) stop widescreen expansion cleanly, while ignoring dynamic
     * boss/room camera locks ($1A40/$1A42) so boss arenas keep full authored 16:9 width. */
    s_ws_bg1_bias = AladdinWram16(0x177A);
    s_ws_bg2_bias = AladdinWram16(0x18D0);

    uint16_t lim1 = AladdinWram16(0x1A04);
    int level_max_x = (lim1 > 0 && lim1 < 0x7000) ? (int)lim1 : 0x7000;
    uint32_t row_a = ((uint32_t)wy >> 8) + 1u;
    uint32_t row_b = (((uint32_t)wy + 223u) >> 8) + 1u;

    uint32_t min_col = ((uint32_t)wx >> 8) + 1u;
    while (min_col > 1u && AladdinBg1ScreenColValid(min_col - 1u, row_a, row_b, s_ws_bg1_bias))
        min_col--;
    int cam_min_x = (int)(min_col - 1u) * 256;
    if (cam_min_x > wx)
        cam_min_x = (int)wx;

    uint32_t max_level_col = ((uint32_t)level_max_x >> 8) + 1u;
    uint32_t max_col = ((uint32_t)(wx + 255) >> 8) + 1u;
    while (max_col < max_level_col && AladdinBg1ScreenColValid(max_col + 1u, row_a, row_b, s_ws_bg1_bias))
        max_col++;
    int cam_max_x = (int)(max_col - 1u) * 256;
    if (cam_max_x > level_max_x)
        cam_max_x = level_max_x;
    if (cam_max_x < (int)wx)
        cam_max_x = (int)wx;

    s_ws_bg1_min_tx = (int32_t)((min_col - 1u) << 5);
    s_ws_bg1_max_tx = (int32_t)(max_col << 5);
    s_ws_bg1_is_shaft = (min_col == max_col);
    PpuSetExtraSideSpace(g_ppu, g_ws_extra, g_ws_extra, 0);

    /* Configure WsShadow world-keyed tilemap provider for BG1 (layer 0) and BG2 (layer 1)
     * from Aladdin's unified $7F:F000 map table. */
    memset(s_ws_tile_keys, 0, sizeof(s_ws_tile_keys));

    if (g_ram[0x035E] == 0x11u && wx >= 256) {
        /* Stage 6-4 Wizard & Cobra Jafar throne room: animate both layers from L[0..31] */
        for (uint32_t l = 0; l < 2u; l++) {
            uint32_t hs = g_ppu->hScroll[l] & 511u;
            WsShadowSetWorld(l, hs + 512u, (uint32_t)g_ppu->vScroll[l]);
            WsShadowSetScroll(l, hs, (uint32_t)g_ppu->vScroll[l]);
            WsShadowSetWorldProvider(l, &AladdinJafarArenaProvider, (void *)(uintptr_t)l, false);
            WsShadowSetBlankTile(l, -1);
            WsShadowSetRespectGameWrites(l, 0);
        }
        WsShadowFrame(g_ppu);
        s_ws_shadow_was_active = true;
    } else if (AladdinLayerStreamValid(0, wx, wy, s_ws_bg1_bias)) {
        WsShadowSetWorld(0, (uint32_t)(wx + 512), (uint32_t)wy);
        WsShadowSetScroll(0, g_ppu->hScroll[0], g_ppu->vScroll[0]);
        WsShadowSetWorldProvider(0, &AladdinBg1WorldProvider, NULL, false);
        WsShadowSetBlankTile(0, -1);
        WsShadowSetRespectGameWrites(0, 60);

        /* BG2 (layer 1): verify against $7F + bias (e.g. $7F:F400). Use 0x18C2 as baseline
         * worldX (since BG2HOFS is HDMA-driven per scanline) and align wy2 to vScroll[1]. */
        int32_t wx2 = (int32_t)AladdinWram16(0x18C2);
        int32_t wy2 = (int32_t)AladdinWram16(0x18CD);
        int32_t dy2 = ((int32_t)g_ppu->vScroll[1] - wy2) & 255;
        if (dy2 >= 128) dy2 -= 256;
        wy2 += dy2;
        if (wy2 < 0) wy2 = 0;

        if (g_ram[0x035E] == 0x10u) {
            /* Stage 6-3 Rising/Falling Fire: always sample live 8-tile cycles from VRAM so
             * partial $7F:F400 fire/empty tables never steal BG2 and fold corner fire. */
            uint32_t hs1 = (g_ppu->bgXsc[1] & 0x01u) ? (g_ppu->hScroll[1] & 511u) : (g_ppu->hScroll[1] & 255u);
            WsShadowSetWorld(1, hs1 + 512u, (uint32_t)g_ppu->vScroll[1]);
            WsShadowSetScroll(1, hs1, (uint32_t)g_ppu->vScroll[1]);
            WsShadowSetWorldProvider(1, &AladdinBg2FireProvider, NULL, false);
            WsShadowSetBlankTile(1, -1);
            WsShadowSetRespectGameWrites(1, 0);
        } else if (AladdinLayerStreamValid(1, wx2, wy2, s_ws_bg2_bias)) {
            uint32_t row_off = ((((uint32_t)wy2 >> 8) + 1u) & 0xFFu) << 6;
            uint16_t c1 = AladdinWram16(0x1F000u + ((row_off + 2u + s_ws_bg2_bias) & 0x0FFEu));
            uint16_t c2 = AladdinWram16(0x1F000u + ((row_off + 4u + s_ws_bg2_bias) & 0x0FFEu));
            uint16_t c3 = AladdinWram16(0x1F000u + ((row_off + 6u + s_ws_bg2_bias) & 0x0FFEu));
            uint16_t c4 = AladdinWram16(0x1F000u + ((row_off + 8u + s_ws_bg2_bias) & 0x0FFEu));
            s_ws_bg2_loop512 = (c1 == c3 && c2 == c4);

            if (!s_ws_bg2_loop512) {
                uint32_t r2a = ((uint32_t)wy2 >> 8) + 1u;
                uint32_t r2b = (((uint32_t)wy2 + 223u) >> 8) + 1u;
                uint32_t min_c2 = ((uint32_t)wx2 >> 8) + 1u;
                while (min_c2 > 1u && AladdinScreenColValid(min_c2 - 1u, r2a, r2b, s_ws_bg2_bias, true))
                    min_c2--;
                uint32_t max_c2 = ((uint32_t)(wx2 + 255) >> 8) + 1u;
                while (max_c2 < 30u && AladdinScreenColValid(max_c2 + 1u, r2a, r2b, s_ws_bg2_bias, true))
                    max_c2++;
                s_ws_bg2_min_tx = (int32_t)((min_c2 - 1u) << 5);
                s_ws_bg2_max_tx = (int32_t)(max_c2 << 5);
            }

            WsShadowSetWorld(1, (uint32_t)(wx2 + 512), (uint32_t)wy2);
            WsShadowSetScroll(1, (uint32_t)(wx2 & 511), g_ppu->vScroll[1]);
            WsShadowSetWorldProvider(1, &AladdinBg2WorldProvider, NULL, false);
            WsShadowSetBlankTile(1, -1);
            WsShadowSetRespectGameWrites(1, 0);
        } else if (g_ram[0x035E] == 7u) {
            /* Stage 3-2 Carpet + Lava wave: localized wave on L page, blank R page. */
            uint32_t hs1 = g_ppu->hScroll[1] & 511u;
            WsShadowSetWorld(1, hs1 + 512u, (uint32_t)g_ppu->vScroll[1]);
            WsShadowSetScroll(1, hs1, (uint32_t)g_ppu->vScroll[1]);
            WsShadowSetWorldProvider(1, &AladdinBg2WaveProvider, NULL, false);
            WsShadowSetBlankTile(1, -1);
            WsShadowSetRespectGameWrites(1, 0);
        } else {
            WsShadowSetPeriodicFold(1);
        }

        WsShadowFrame(g_ppu);
        s_ws_shadow_was_active = true;
    } else {
        if (s_ws_shadow_was_active) {
            WsShadowReset();
            s_ws_shadow_was_active = false;
        }
        /* Without verified streaming map, fall back to centered 4:3 to prevent VRAM garbage */
        PpuSetExtraSpaceCentered(g_ppu, (uint16_t)g_ws_extra);
    }

    /* Clear any unrendered outer black pillarbox strips so previous frames never ghost */
    AladdinClearUnusedPillarboxes();

    /* BG3 (layer 2): clamp to native 256 columns */
    PpuSetWidescreenLayerClamp(g_ppu, 0x04);

    /* Keep internal windows (lamp light beam, cutscene cutouts) unexpanded; hardware
     * edges pinned to 0/255 already stretch automatically in ppu.c. */
    PpuSetWidescreenWindowExpansion(g_ppu, 0x00, 0x00);
}

static void AladdinPrepareFrame(int dw, int dh, int *w, int *h)
{
    if (!g_config.widescreen) {
        *w = 256;  /* authentic 4:3 SNES */
        *h = 224;
        return;
    }
    int wide_w = SnesDisplayAspect_ComputeWideFrameWidth(256);  /* 342: 16:9 baseline */
    if (dw > 0 && dh > 0) {
        SnesDisplayFrame f = SnesDisplayAspect_ComputeAdaptiveFrame(
            256, 224, 384, (double)dw / (double)dh,
            SnesDisplayAspect_Clamp(g_config.display_aspect));
        if (f.width > wide_w)
            wide_w = f.width;
    }
    *w = wide_w;
    *h = 224;
}

static int AladdinWindowBaseWidth(int frame_w)
{
    if (g_config.widescreen) {
        return SnesDisplayAspect_ComputeWindowWidth(
            342, 224, 240, SnesDisplayAspect_Clamp(g_config.display_aspect));
    }
    return (frame_w * 5 + 2) / 4;
}

static const char *const kAladdinAspectLabels[] = {
    "16:9 Widescreen (Recomendado)",
    "8:7 Pixels Quadrados",
    "1:1 Quadro Original",
};

static void AladdinConfigureLauncher(RecompLauncherCGameInfo *info)
{
    if (info)
        info->theme = "agrabah";
}

static const SnesDesktopHostGame kGameHost = {
    .display_name        = "AgrabahPort",
    .window_title        = "AgrabahPort",
    .region              = SNESRECOMP_ROM_REGION,
    .rom_file            = SNESRECOMP_ROM_FILE,
    .expected_sha256_hex = SNESRECOMP_ROM_EXPECTED_SHA256,
    .expected_crc32_hex  = SNESRECOMP_ROM_EXPECTED_CRC32,
    .game_id             = SNESRECOMP_ROM_GAME_ID,
    .build_version       = SNES_GAME_VERSION,
    .game_info           = &kGameInfo,
    .num_players         = 1,
    .simulation_hz       = 60.0,
    .widescreen_supported     = 1,
    .native_widescreen        = 1,
    .display_aspect_supported = 1,
    .prepare_frame            = &AladdinPrepareFrame,
    .begin_sim_frame          = &AladdinBeginFrame,
    .window_base_width        = &AladdinWindowBaseWidth,
    .configure_launcher       = &AladdinConfigureLauncher,
    .shader_supported         = 1,
    .rewind_settings          = 1,
    .in_game_launcher         = 1,
    /* Battery-backed SRAM shows the launcher's SAVES panel. Leave NULL for a
     * title without one. The path is exe-relative. */
    .sram_path           = "saves/save.srm",
};

#ifndef __ANDROID__
#undef main   /* desktop: keep plain main() even if SDL_main.h remapped it */
#endif
int main(int argc, char **argv)
{
    aladdin_live_log_init();

#if defined(SNESRECOMP_SETUP_HOST)
    /* A setup host contains no recompiled game code and cannot boot straight
     * into gameplay. If a front-end like RecompLauncher passes a ROM path
     * via --rom <path> without --launcher, force the launcher so the setup /
     * rebuild wizard opens cleanly instead of aborting. */
    int has_launcher_flag = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--launcher") == 0 || strcmp(argv[i], "--no-launcher") == 0) {
            has_launcher_flag = 1;
            break;
        }
    }
    if (!has_launcher_flag) {
        char **new_argv = (char **)malloc((argc + 2) * sizeof(char *));
        if (new_argv) {
            new_argv[0] = argv[0];
            new_argv[1] = (char *)"--launcher";
            for (int i = 1; i < argc; ++i) {
                new_argv[i + 1] = argv[i];
            }
            new_argv[argc + 1] = NULL;
            return snesrecomp_desktop_main(&kGameHost, argc + 1, new_argv);
        }
    }
#endif

    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}
