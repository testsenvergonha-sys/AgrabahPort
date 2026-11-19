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
#include "game_rtl.h"
#include "live_log.h"
#include "snesrecomp_rom_identity.h"  /* generated from rom_identity.txt */
#include "desktop/config.h"
#include "desktop/display_aspect.h"
#include "snes/ppu.h"

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
extern uint8_t g_ram[0x20000];

static void AladdinBeginFrame(unsigned number)
{
    (void)number;
    if (!g_ppu || !g_config.widescreen)
        return;

    uint8_t health = g_ram[0x0367];
    uint8_t lives = g_ram[0x0364];

    /* Clamp static screens (Capcom logo, Disney logo, Title screen) to authentic 256
     * with clean black side borders. Unclamp during active gameplay. */
    if (health == 0 && lives == 0) {
        PpuSetWidescreenLayerClamp(g_ppu, 0x0F);
    } else {
        PpuSetWidescreenLayerClamp(g_ppu, 0x00);
    }
}

static void AladdinPrepareFrame(int dw, int dh, int *w, int *h)
{
    (void)dw; (void)dh;
    if (g_config.widescreen) {
        *w = SnesDisplayAspect_ComputeWideFrameWidth(256);  /* 342: 16:9 widescreen */
        *h = 224;
    } else {
        *w = 256;  /* authentic 4:3 SNES */
        *h = 224;
    }
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
    .widescreen_supported     = 0,
    .native_widescreen        = 0,
    .display_aspect_supported = 1,
    .prepare_frame            = &AladdinPrepareFrame,
    .begin_sim_frame          = &AladdinBeginFrame,
    .window_base_width        = &AladdinWindowBaseWidth,
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
