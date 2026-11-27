#ifndef AGRABAH_LAUNCHER_SHELL_H
#define AGRABAH_LAUNCHER_SHELL_H

#include <cmath>
#include <cstdio>
#include <cstring>

// Authorial AgrabahPort launcher UI inspired by Arabian palace architecture:
// pointed Moorish arch hero showcase, hanging palace lanterns, subtle geometric
// star-tile backdrop, split editorial Play layout, and cohesive Settings/Controls/
// Mods/About tabs integrated directly inside recomp-ui.

enum AgrabahTab {
    AGR_TAB_GAME = 0,
    AGR_TAB_SETTINGS,
    AGR_TAB_CONTROLS,
    AGR_TAB_MODS,
    AGR_TAB_ABOUT,
    AGR_TAB__COUNT
};

// Hanging golden Arabian lantern with soft swinging light halo.
static inline void draw_agrabah_lantern(ImDrawList* dl, ImVec2 anchor, float len,
                                        float scale, float t, float phase) {
    const float swing = std::sin(t * 1.15f + phase) * px(3.5f * scale);
    const ImVec2 top(anchor.x + swing * 0.4f, anchor.y + len);
    const ImU32 chain_col = IM_COL32(214, 165, 58, 125);
    const ImU32 brass     = IM_COL32(220, 168, 52, 220);
    const ImU32 brass_dk  = IM_COL32(138, 94, 24, 220);

    dl->AddLine(anchor, top, chain_col, px(1.4f));

    // Warm candlelight halo.
    const ImVec2 glow_c(top.x + swing * 0.4f, top.y + px(22.0f * scale));
    const float pulse = 0.88f + 0.12f * std::sin(t * 2.8f + phase);
    dl->AddCircleFilled(glow_c, px(44.0f * scale),
                        IM_COL32(252, 198, 78, (int)(22 * pulse)), 20);
    dl->AddCircleFilled(glow_c, px(22.0f * scale),
                        IM_COL32(255, 224, 130, (int)(44 * pulse)), 18);

    // Ornate octagonal lantern silhouette.
    const float w = px(13.0f * scale);
    const float h = px(28.0f * scale);
    const float cx = glow_c.x;
    const float y0 = top.y;
    dl->AddTriangleFilled(ImVec2(cx, y0 - px(5.0f * scale)),
                          ImVec2(cx - w * 0.8f, y0 + px(5.0f * scale)),
                          ImVec2(cx + w * 0.8f, y0 + px(5.0f * scale)), brass);
    dl->AddQuadFilled(ImVec2(cx - w * 0.85f, y0 + px(6.0f * scale)),
                      ImVec2(cx + w * 0.85f, y0 + px(6.0f * scale)),
                      ImVec2(cx + w * 0.62f, y0 + h),
                      ImVec2(cx - w * 0.62f, y0 + h), IM_COL32(252, 212, 108, 195));
    dl->AddQuad(ImVec2(cx - w * 0.85f, y0 + px(6.0f * scale)),
                ImVec2(cx + w * 0.85f, y0 + px(6.0f * scale)),
                ImVec2(cx + w * 0.62f, y0 + h),
                ImVec2(cx - w * 0.62f, y0 + h), brass_dk, px(1.5f));
    dl->AddLine(ImVec2(cx, y0 + px(6.0f * scale)), ImVec2(cx, y0 + h), brass_dk, px(1.1f));
    dl->AddTriangleFilled(ImVec2(cx - w * 0.5f, y0 + h),
                          ImVec2(cx + w * 0.5f, y0 + h),
                          ImVec2(cx, y0 + h + px(8.0f * scale)), brass);
}

// 8-point Moorish geometric star tile outline (girih motif).
static inline void draw_girih_star(ImDrawList* dl, ImVec2 c, float r, ImU32 col_line) {
    const float s = r * 0.70710678f;
    dl->AddRect(ImVec2(c.x - s, c.y - s), ImVec2(c.x + s, c.y + s), col_line, 0.0f, 0, px(1.0f));
    dl->AddQuad(ImVec2(c.x, c.y - r), ImVec2(c.x + r, c.y),
                ImVec2(c.x, c.y + r), ImVec2(c.x - r, c.y), col_line, px(1.0f));
}

static inline void draw_agrabah_dome(ImDrawList* dl, float cx, float base_y,
                                     float half_w, float height, ImU32 fill, ImU32 rim) {
    dl->AddRectFilled(ImVec2(cx - half_w * 0.90f, base_y - height * 0.46f),
                      ImVec2(cx + half_w * 0.90f, base_y), fill);
    dl->AddCircleFilled(ImVec2(cx, base_y - height * 0.65f), half_w * 0.86f, fill, 22);
    dl->AddTriangleFilled(
        ImVec2(cx - half_w * 0.54f, base_y - height * 0.76f),
        ImVec2(cx + half_w * 0.54f, base_y - height * 0.76f),
        ImVec2(cx, base_y - height * 1.12f), fill);
    dl->AddLine(ImVec2(cx, base_y - height * 1.10f),
                ImVec2(cx, base_y - height * 1.32f), rim, px(1.6f));
    dl->AddCircleFilled(ImVec2(cx, base_y - height * 1.34f), px(2.2f), rim, 8);
}

static inline void draw_agrabah_background(ImDrawList* bg, ImVec2 origin, ImVec2 size, float t) {
    const ImVec2 br(origin.x + size.x, origin.y + size.y);
    const float w = size.x, h = size.y;

    // Deep sapphire night -> royal violet-indigo gradient.
    const ImU32 top_c = IM_COL32(8, 13, 34, 255);
    const ImU32 mid_c = IM_COL32(15, 22, 54, 255);
    const ImU32 bot_c = IM_COL32(28, 19, 48, 255);
    const ImVec2 mid_r(br.x, origin.y + h * 0.55f);
    const ImVec2 mid_l(origin.x, origin.y + h * 0.55f);
    bg->AddRectFilledMultiColor(origin, mid_r, top_c, top_c, mid_c, mid_c);
    bg->AddRectFilledMultiColor(mid_l, br, mid_c, mid_c, bot_c, bot_c);

    // Faint Islamic geometric tile watermark along outer margins.
    const ImU32 tile_col = IM_COL32(214, 165, 58, 16);
    const float step = px(96.0f);
    for (float y = origin.y + px(48.0f); y < br.y; y += step) {
        draw_girih_star(bg, ImVec2(origin.x + px(42.0f), y), px(24.0f), tile_col);
        draw_girih_star(bg, ImVec2(br.x - px(42.0f), y), px(24.0f), tile_col);
    }

    // Subtle Arabian arch framing in top-left/top-right corners + hanging lanterns.
    const ImU32 frame_gold = IM_COL32(214, 165, 58, 55);
    bg->AddRect(ImVec2(origin.x + px(12.0f), origin.y + px(12.0f)),
                ImVec2(br.x - px(12.0f), br.y - px(12.0f)),
                IM_COL32(214, 165, 58, 38), px(14.0f), 0, px(1.0f));
    bg->AddBezierQuadratic(ImVec2(origin.x + px(12.0f), origin.y + px(130.0f)),
                           ImVec2(origin.x + px(28.0f), origin.y + px(24.0f)),
                           ImVec2(origin.x + px(155.0f), origin.y + px(12.0f)),
                           frame_gold, px(1.4f), 16);
    bg->AddBezierQuadratic(ImVec2(br.x - px(12.0f), origin.y + px(130.0f)),
                           ImVec2(br.x - px(28.0f), origin.y + px(24.0f)),
                           ImVec2(br.x - px(155.0f), origin.y + px(12.0f)),
                           frame_gold, px(1.4f), 16);

    draw_agrabah_lantern(bg, ImVec2(origin.x + px(68.0f), origin.y + px(12.0f)),
                         px(26.0f), 0.95f, t, 0.0f);
    draw_agrabah_lantern(bg, ImVec2(br.x - px(68.0f), origin.y + px(12.0f)),
                         px(32.0f), 0.95f, t, 2.1f);

    // Subtle twinkling stars.
    for (int i = 0; i < 28; ++i) {
        float fx = (float)((i * 67 + 13) % 100) * 0.01f;
        float fy = (float)((i * 43 + 7) % 62) * 0.01f;
        float pulse = 0.50f + 0.50f * std::sin(t * 1.3f + (float)i * 1.1f);
        bg->AddCircleFilled(ImVec2(origin.x + w * fx, origin.y + h * fy),
                            px((i % 4 == 0) ? 1.6f : 1.0f),
                            IM_COL32(248, 232, 180, (int)(30 + 95 * pulse)), 6);
    }

    // Soft distant palace silhouette along the very bottom horizon.
    const ImU32 sil_col = IM_COL32(10, 12, 30, 195);
    const ImU32 sil_rim = IM_COL32(214, 165, 58, 45);
    draw_agrabah_dome(bg, origin.x + w * 0.14f, br.y - px(24.0f), px(22.0f), px(58.0f), sil_col, sil_rim);
    draw_agrabah_dome(bg, br.x - w * 0.14f, br.y - px(24.0f), px(24.0f), px(62.0f), sil_col, sil_rim);
}

static inline void draw_magic_lamp_emblem(ImDrawList* dl, ImVec2 center, float radius, float t) {
    // Circular gold coin crest containing a golden magic lamp and genie spark.
    dl->AddCircleFilled(center, radius, IM_COL32(20, 28, 66, 245), 32);
    dl->AddCircle(center, radius, IM_COL32(244, 196, 66, 210), 32, px(1.8f));
    dl->AddCircle(center, radius - px(3.5f), IM_COL32(64, 218, 204, 110), 28, px(1.0f));

    const float s = radius / px(24.0f);
    const ImU32 gold_hi  = IM_COL32(252, 218, 106, 255);
    const ImU32 gold_mid = IM_COL32(232, 174, 48, 255);
    const ImU32 gold_lo  = IM_COL32(170, 114, 22, 255);
    const ImU32 aqua     = IM_COL32(64, 224, 208, 240);

    const float cx = center.x - px(2.0f * s);
    const float by = center.y + px(11.0f * s);
    // Spark rising from spout
    float phase = std::fmod(t * 0.7f, 1.0f);
    dl->AddCircleFilled(ImVec2(center.x + px(12.0f * s), center.y - px((3.0f + phase * 8.0f) * s)),
                        px((2.2f - phase) * s), IM_COL32(72, 232, 214, (int)((1.0f - phase) * 210)), 8);
    // Handle & spout
    dl->AddCircle(ImVec2(cx - px(9.5f * s), by - px(8.0f * s)), px(4.5f * s), gold_lo, 12, px(2.2f * s));
    dl->AddBezierQuadratic(ImVec2(cx + px(5.0f * s), by - px(6.5f * s)),
                           ImVec2(cx + px(11.0f * s), by - px(8.0f * s)),
                           ImVec2(center.x + px(13.0f * s), by - px(13.0f * s)),
                           gold_mid, px(3.6f * s), 10);
    // Pedestal & body
    dl->AddTriangleFilled(ImVec2(cx - px(5.0f * s), by),
                          ImVec2(cx + px(5.0f * s), by),
                          ImVec2(cx, by - px(5.0f * s)), gold_lo);
    dl->AddEllipseFilled(ImVec2(cx, by - px(7.5f * s)),
                         ImVec2(px(8.5f * s), px(4.8f * s)), gold_mid, 0.0f, 16);
    dl->AddEllipseFilled(ImVec2(cx - px(1.5f * s), by - px(8.8f * s)),
                         ImVec2(px(4.0f * s), px(1.8f * s)), gold_hi, 0.0f, 12);
    dl->AddCircleFilled(ImVec2(cx, by - px(12.8f * s)), px(2.4f * s), aqua, 10);
}

// Pointed Moorish Arch Hero Scene (Cave of Wonders + Agrabah Moonlight + Carpet).
static inline void draw_agrabah_arch_showcase(ImDrawList* dl, ImVec2 mn, ImVec2 mx,
                                              bool verified, float t) {
    const float w = mx.x - mn.x;
    const float h = mx.y - mn.y;
    const float r = px(18.0f);

    // Build pointed Arabian arch perimeter points for background & border.
    const float spring_y = mn.y + h * 0.34f;
    const float cx = (mn.x + mx.x) * 0.5f;
    ImVec2 arch[34];
    int n_pts = 0;
    arch[n_pts++] = ImVec2(mn.x, mx.y - r);
    arch[n_pts++] = ImVec2(mn.x, spring_y);
    for (int i = 1; i <= 12; ++i) {
        float u = (float)i / 12.0f;
        float x = mn.x + (cx - mn.x) * u;
        float y = spring_y - std::sin(u * 1.570796f) * (spring_y - mn.y - px(6.0f))
                           - u * (1.0f - u) * px(18.0f);
        arch[n_pts++] = ImVec2(x, y);
    }
    for (int i = 1; i <= 12; ++i) {
        float u = (float)i / 12.0f;
        float v = 1.0f - u;
        float x = cx + (mx.x - cx) * u;
        float y = spring_y - std::sin(v * 1.570796f) * (spring_y - mn.y - px(6.0f))
                           - v * (1.0f - v) * px(18.0f);
        arch[n_pts++] = ImVec2(x, y);
    }
    arch[n_pts++] = ImVec2(mx.x, mx.y - r);
    arch[n_pts++] = ImVec2(mx.x - r * 0.4f, mx.y);
    arch[n_pts++] = ImVec2(mn.x + r * 0.4f, mx.y);

    // Clip scene inside bounding rect + mask exterior subtly via dark surround.
    dl->AddRectFilled(mn, mx, IM_COL32(10, 14, 34, 255), r);
    dl->PushClipRect(mn, mx, true);

    // Rich Agrabah night-to-emerald-magic sky inside the arch.
    const float split = mn.y + h * 0.58f;
    dl->AddRectFilledMultiColor(mn, ImVec2(mx.x, split),
                                IM_COL32(16, 22, 62, 255), IM_COL32(16, 22, 62, 255),
                                IM_COL32(42, 34, 88, 255), IM_COL32(42, 34, 88, 255));
    dl->AddRectFilledMultiColor(ImVec2(mn.x, split), mx,
                                IM_COL32(42, 34, 88, 255), IM_COL32(42, 34, 88, 255),
                                IM_COL32(28, 72, 86, 255), IM_COL32(28, 72, 86, 255));

    // Luminous full moon behind Sultan's Palace.
    const ImVec2 moon(cx + w * 0.14f, mn.y + h * 0.38f);
    dl->AddCircleFilled(moon, px(64.0f), IM_COL32(252, 224, 138, 28), 28);
    dl->AddCircleFilled(moon, px(42.0f), IM_COL32(254, 232, 162, 72), 28);
    dl->AddCircleFilled(moon, px(28.0f), IM_COL32(255, 240, 190, 235), 28);

    // Twinkling stars in upper dome.
    for (int i = 0; i < 16; ++i) {
        float sx = mn.x + w * (0.12f + 0.052f * (float)i);
        float sy = mn.y + h * (0.14f + 0.06f * (float)((i * 5) % 5));
        float pulse = 0.55f + 0.45f * std::sin(t * 1.9f + (float)i * 0.7f);
        dl->AddCircleFilled(ImVec2(sx, sy), px((i % 3 == 0) ? 1.7f : 1.1f),
                            IM_COL32(255, 246, 210, (int)(185 * pulse)), 6);
    }

    // Palace silhouette cluster centered beneath the moon.
    const ImU32 pal = IM_COL32(14, 18, 44, 248);
    const ImU32 pal_rim = IM_COL32(245, 200, 92, 185);
    const float pal_y = mx.y - h * 0.17f;
    draw_agrabah_dome(dl, cx + w * 0.11f, pal_y, px(30.0f), px(82.0f), pal, pal_rim);
    draw_agrabah_dome(dl, cx - w * 0.03f, pal_y, px(18.0f), px(54.0f), pal, pal_rim);
    draw_agrabah_dome(dl, cx + w * 0.24f, pal_y, px(16.0f), px(48.0f), pal, pal_rim);
    draw_agrabah_dome(dl, cx + w * 0.34f, pal_y, px(11.0f), px(38.0f), pal, pal_rim);

    // Glowing Cave of Wonders / Genie turquoise light beam on left pedestal.
    const float ped_x = mn.x + w * 0.24f;
    const float ped_y = mx.y - h * 0.20f;
    dl->AddRectFilledMultiColor(
        ImVec2(ped_x - px(18.0f), mn.y + h * 0.20f),
        ImVec2(ped_x + px(18.0f), ped_y),
        IM_COL32(64, 228, 212, 8), IM_COL32(64, 228, 212, 8),
        IM_COL32(64, 228, 212, 92), IM_COL32(64, 228, 212, 92));
    dl->AddRectFilled(ImVec2(ped_x - px(22.0f), ped_y),
                      ImVec2(ped_x + px(22.0f), mx.y - h * 0.06f),
                      IM_COL32(28, 34, 62, 255), px(4.0f));
    dl->AddEllipseFilled(ImVec2(ped_x, ped_y), ImVec2(px(22.0f), px(5.0f)),
                         IM_COL32(72, 232, 214, 180), 0.0f, 16);
    // Tiny golden lamp silhouette on the pedestal.
    dl->AddEllipseFilled(ImVec2(ped_x, ped_y - px(6.0f)), ImVec2(px(10.0f), px(5.5f)),
                         IM_COL32(246, 198, 66, 255), 0.0f, 14);
    dl->AddCircleFilled(ImVec2(ped_x + px(9.0f), ped_y - px(10.0f)), px(2.2f),
                        IM_COL32(252, 222, 120, 255), 8);

    // Magic Carpet gliding across the moonlit sky.
    const float mcp_y = mn.y + h * 0.28f + std::sin(t * 2.0f) * px(4.0f);
    const float mcp_x = cx - w * 0.08f;
    dl->AddQuadFilled(ImVec2(mcp_x - px(24.0f), mcp_y + px(5.0f)),
                      ImVec2(mcp_x + px(20.0f), mcp_y - px(4.0f)),
                      ImVec2(mcp_x + px(26.0f), mcp_y + px(2.0f)),
                      ImVec2(mcp_x - px(15.0f), mcp_y + px(11.0f)),
                      IM_COL32(138, 40, 168, 245));
    dl->AddLine(ImVec2(mcp_x - px(24.0f), mcp_y + px(5.0f)),
                ImVec2(mcp_x + px(20.0f), mcp_y - px(4.0f)),
                IM_COL32(246, 198, 72, 235), px(1.8f));
    for (int i = 0; i < 4; ++i) {
        dl->AddCircleFilled(ImVec2(mcp_x - px(30.0f + 9.0f * i), mcp_y + px(8.0f + 1.8f * i)),
                            px(2.1f - 0.35f * i), IM_COL32(72, 232, 214, 195 - i * 42), 6);
    }

    // Layered foreground dunes + scattered gems along bottom edge.
    dl->AddEllipseFilled(ImVec2(mn.x + w * 0.72f, mx.y + px(10.0f)),
                         ImVec2(w * 0.48f, px(44.0f)), IM_COL32(24, 28, 56, 255), 0.0f, 24);
    dl->AddEllipseFilled(ImVec2(mn.x + w * 0.28f, mx.y + px(14.0f)),
                         ImVec2(w * 0.50f, px(42.0f)), IM_COL32(14, 18, 40, 255), 0.0f, 24);

    // Decorative corner spandrels so the Moorish arch outline frames the scene cleanly.
    dl->AddTriangleFilled(mn, ImVec2(cx, mn.y), ImVec2(mn.x, spring_y), IM_COL32(11, 16, 38, 235));
    dl->AddTriangleFilled(ImVec2(mx.x, mn.y), ImVec2(cx, mn.y), ImVec2(mx.x, spring_y), IM_COL32(11, 16, 38, 235));

    dl->PopClipRect();

    // Double golden filigree arch border.
    dl->AddRect(mn, mx, IM_COL32(214, 165, 58, 125), r, 0, px(1.6f));
    dl->AddPolyline(arch, n_pts - 2, IM_COL32(244, 196, 66, 200), 0, px(1.8f));

    // Bottom status ribbon inside the showcase.
    const char* status_txt = verified ? "VERIFIED ROM READY" : "SELECT ALADDIN ROM";
    const ImVec2 ts = ImGui::CalcTextSize(status_txt);
    const float ph = px(24.0f);
    const float pw = ts.x + px(30.0f);
    const ImVec2 pm(mn.x + px(14.0f), mx.y - ph - px(12.0f));
    const ImVec2 pmx(pm.x + pw, pm.y + ph);
    dl->AddRectFilled(pm, pmx, IM_COL32(10, 14, 30, 224), px(6.0f));
    dl->AddRect(pm, pmx, verified ? IM_COL32(70, 227, 155, 195) : IM_COL32(245, 178, 60, 210),
                px(6.0f), 0, px(1.1f));
    dl->AddCircleFilled(ImVec2(pm.x + px(11.0f), pm.y + ph * 0.5f), px(3.8f),
                        verified ? IM_COL32(70, 227, 155, 255) : IM_COL32(245, 178, 60, 255), 8);
    dl->AddText(ImVec2(pm.x + px(20.0f), pm.y + (ph - ts.y) * 0.5f),
                IM_COL32(244, 240, 230, 255), status_txt);
}

static inline bool agrabah_gold_cta(const char* id, const char* label, ImVec2 size,
                                    bool enabled, bool arrow) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    if (!enabled) ImGui::BeginDisabled();
    bool pressed = ImGui::InvisibleButton(id, size, ImGuiButtonFlags_EnableNav);
    bool hovered = enabled && ImGui::IsItemHovered();
    bool held = enabled && ImGui::IsItemActive();
    bool focused = enabled && ImGui::IsItemFocused();
    if (!enabled) ImGui::EndDisabled();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float r = px(10.0f);
    const ImVec2 mx(p.x + size.x, p.y + size.y);
    if (enabled) {
        if (hovered || focused) {
            dl->AddRectFilled(ImVec2(p.x - px(4.0f), p.y - px(4.0f)),
                              ImVec2(mx.x + px(4.0f), mx.y + px(4.0f)),
                              IM_COL32(244, 196, 66, held ? 28 : 54), r + px(4.0f));
        }
        const LngColor top = held ? lng_rgba(0.84f, 0.62f, 0.14f, 1.0f)
                           : hovered ? lng_rgba(1.00f, 0.86f, 0.38f, 1.0f)
                                     : lng_rgba(0.96f, 0.78f, 0.26f, 1.0f);
        const LngColor bot = held ? lng_rgba(0.70f, 0.48f, 0.08f, 1.0f)
                                  : lng_rgba(0.82f, 0.58f, 0.12f, 1.0f);
        grad_rect(dl, p, mx, r, top, bot);
        dl->AddRect(p, mx, IM_COL32(255, 236, 168, hovered || focused ? 240 : 160),
                    r, 0, px(focused ? 2.0f : 1.2f));
        // Double inner gold-leaf border for royal palace feel.
        dl->AddRect(ImVec2(p.x + px(3.0f), p.y + px(3.0f)),
                    ImVec2(mx.x - px(3.0f), mx.y - px(3.0f)),
                    IM_COL32(120, 78, 12, 95), r - px(2.0f), 0, px(1.0f));
    } else {
        dl->AddRectFilled(p, mx, IM_COL32(28, 32, 58, 220), r);
        dl->AddRect(p, mx, IM_COL32(100, 96, 128, 120), r, 0, px(1.0f));
    }

    ImU32 txt_col = enabled ? IM_COL32(22, 16, 6, 255) : IM_COL32(140, 138, 160, 220);
    ImGui::SetWindowFontScale(1.14f);
    ImVec2 ts = ImGui::CalcTextSize(label);
    float icon_w = arrow ? px(16.0f) : 0.0f;
    float total_w = icon_w + ts.x;
    float sx = p.x + (size.x - total_w) * 0.5f;
    float sy = p.y + (size.y - ts.y) * 0.5f;
    if (arrow) {
        float tri = px(10.0f);
        float cy = p.y + size.y * 0.5f;
        dl->AddTriangleFilled(ImVec2(sx, cy - tri * 0.52f),
                              ImVec2(sx, cy + tri * 0.52f),
                              ImVec2(sx + tri * 0.9f, cy), txt_col);
    }
    dl->AddText(ImVec2(sx + icon_w, sy), txt_col, label);
    ImGui::SetWindowFontScale(1.0f);
    return pressed;
}

static inline void draw_agrabah_info_chip(ImDrawList* dl, ImVec2 pos, float w, float h,
                                          const char* label, const char* value,
                                          ImU32 accent_col) {
    const ImVec2 mx(pos.x + w, pos.y + h);
    dl->AddRectFilled(pos, mx, IM_COL32(18, 24, 52, 230), px(8.0f));
    dl->AddRect(pos, mx, IM_COL32(214, 165, 58, 70), px(8.0f), 0, px(1.0f));
    dl->AddRectFilled(ImVec2(pos.x, pos.y + px(8.0f)),
                      ImVec2(pos.x + px(3.0f), mx.y - px(8.0f)),
                      accent_col, px(1.5f));
    dl->AddText(ImVec2(pos.x + px(11.0f), pos.y + px(7.0f)),
                IM_COL32(154, 152, 182, 255), label);
    dl->AddText(ImVec2(pos.x + px(11.0f), pos.y + px(25.0f)),
                IM_COL32(244, 240, 230, 255), value);
}

static inline void draw_agrabah_update_modal(LauncherModel* m, const LauncherTheme& th,
                                             const agrabah_upd::Snapshot& upd, bool& open_req) {
    if (open_req) {
        ImGui::OpenPopup("##agr_update_modal");
        open_req = false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                            ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(px(560.0f), 0.0f), ImGuiCond_Appearing);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, col(th.panel));
    ImGui::PushStyleColor(ImGuiCol_Border, col(th.accent));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(px(22.0f), px(20.0f)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, px(14.0f));
    if (ImGui::BeginPopupModal("##agr_update_modal", nullptr,
                               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
                               ImGuiWindowFlags_NoMove)) {
        const float w = px(516.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, col(th.accent));
        ImGui::SetWindowFontScale(1.30f);
        ImGui::TextUnformatted("AgrabahPort Update Available");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        const char* cur = upd.local_version.empty() ? "current" : upd.local_version.c_str();
        ImGui::TextColored(col(th.accent2), "Installed: %s   ->   Latest: %s",
                           cur, upd.rel.tag_name.c_str());
        if (!upd.rel.title.empty() && upd.rel.title != upd.rel.tag_name)
            ImGui::TextColored(col(th.text), "%s", upd.rel.title.c_str());

        ImGui::Dummy(ImVec2(0, px(8.0f)));
        // Data-safety assurance box
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(20, 48, 44, 210));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(70, 227, 155, 160));
        ImGui::BeginChild("##agr_upd_safe", ImVec2(w, px(68.0f)),
                          ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - px(24.0f));
        ImGui::TextColored(col(th.good),
                           "Protected user files: saves/, config.ini, keybinds.ini, ROM and mods "
                           "are NOT touched (a save backup is created in backups/).");
        ImGui::PopTextWrapPos();
        ImGui::EndChild();
        ImGui::PopStyleColor(2);

        if (!upd.rel.notes.empty()) {
            ImGui::Dummy(ImVec2(0, px(8.0f)));
            ImGui::TextColored(col(th.text_muted), "Release notes:");
            ImGui::BeginChild("##agr_upd_notes", ImVec2(w, px(150.0f)), ImGuiChildFlags_Border);
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w - px(28.0f));
            ImGui::TextUnformatted(upd.rel.notes.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndChild();
        }

        ImGui::Dummy(ImVec2(0, px(10.0f)));

        if (upd.state == agrabah_upd::UPD_DOWNLOADING ||
            upd.state == agrabah_upd::UPD_EXTRACTING ||
            upd.state == agrabah_upd::UPD_READY_APPLY) {
            float frac = 0.0f;
            if (upd.state == agrabah_upd::UPD_DOWNLOADING && upd.bytes_total > 0)
                frac = (float)upd.bytes_done / (float)upd.bytes_total;
            else if (upd.state == agrabah_upd::UPD_EXTRACTING)
                frac = 0.90f;
            else if (upd.state == agrabah_upd::UPD_READY_APPLY)
                frac = 1.0f;
            if (frac > 1.0f) frac = 1.0f;

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col(th.accent));
            ImGui::ProgressBar(frac, ImVec2(w, px(22.0f)), "");
            ImGui::PopStyleColor();
            if (upd.state == agrabah_upd::UPD_DOWNLOADING && upd.bytes_total > 0) {
                ImGui::TextColored(col(th.text_muted), "%s  (%.1f / %.1f MB)",
                                   upd.status_text.c_str(),
                                   (double)upd.bytes_done / (1024.0 * 1024.0),
                                   (double)upd.bytes_total / (1024.0 * 1024.0));
            } else {
                ImGui::TextColored(col(th.text_muted), "%s", upd.status_text.c_str());
            }
        } else {
            if (upd.state == agrabah_upd::UPD_ERROR && !upd.error_text.empty()) {
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w);
                ImGui::TextColored(col(th.warn), "%s", upd.error_text.c_str());
                ImGui::PopTextWrapPos();
                ImGui::Dummy(ImVec2(0, px(8.0f)));
            }

            const bool has_zip = !upd.rel.asset_url.empty();
            const float btn_w = (w - px(12.0f)) * 0.5f;
            if (has_zip) {
                const bool go = agrabah_gold_cta("##agr_upd_go",
                                     upd.state == agrabah_upd::UPD_ERROR ? "Retry Update" : "Update Now",
                                     ImVec2(btn_w, px(40.0f)), true, false);
                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere(-1);
                if (go || (ImGui::IsItemFocused() &&
                           (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
                            ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false)))) {
                    agrabah_upd::start_install_async();
                }
            } else if (!upd.rel.html_url.empty()) {
                if (agrabah_gold_cta("##agr_upd_web", "Open GitHub Release",
                                     ImVec2(btn_w, px(40.0f)), true, false)) {
                    agrabah_upd::open_url_in_browser(upd.rel.html_url);
                }
            } else {
                ImGui::Dummy(ImVec2(btn_w, px(40.0f)));
            }

            ImGui::SameLine(0, px(12.0f));
            if (ImGui::Button("Later", ImVec2(btn_w, px(40.0f))))
                ImGui::CloseCurrentPopup();
        }
        (void)m;
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

static inline void draw_agrabah_about_page(LauncherModel* m, const LauncherTheme& th,
                                           const agrabah_upd::Snapshot& upd, bool& open_upd_modal) {
    if (!begin_panel("agr_about", 0, false)) { end_panel(); return; }
    const float availw = ImGui::GetContentRegionAvail().x;
    const char* ver = recomp_launcher_get_game_version();
    ImGui::PushStyleColor(ImGuiCol_Text, col(th.accent));
    ImGui::SetWindowFontScale(1.22f);
    ImGui::TextUnformatted("AgrabahPort — Aladdin (SNES, 1993) Unofficial Port");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    if (ver && ver[0])
        ImGui::TextColored(col(th.text_muted), "Version %s  •  Native C Static Recompilation", ver);

    ImGui::Dummy(ImVec2(0, px(8.0f)));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, px(8.0f)));

    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + availw);
    ImGui::TextUnformatted(
        "AgrabahPort is a native PC port of Disney's Aladdin for SNES, built with static "
        "recompilation (snesrecomp) and dedicated widescreen and timing enhancements.");
    ImGui::Dummy(ImVec2(0, px(8.0f)));
    ImGui::TextColored(col(th.accent2), "Core Features");
    ImGui::BulletText("Adaptive 16:9 widescreen across all stages, magic carpet escape, and bosses");
    ImGui::BulletText("60 FPS hybrid high-resolution frame pacing and fluid Turbo mode (Tab)");
    ImGui::BulletText("Pixel / CRT shaders, Save States (F11), Rewind (F12), and full gamepad support");
    ImGui::Dummy(ImVec2(0, px(8.0f)));
    ImGui::TextColored(col(th.text_muted),
                       "No copyrighted ROM files are included. Supply your own verified US Aladdin (.sfc) dump.");
    ImGui::PopTextWrapPos();

    ImGui::Dummy(ImVec2(0, px(12.0f)));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, px(10.0f)));
    ImGui::TextColored(col(th.accent2), "Updates  (GitHub: testsenvergonha-sys/AgrabahPort)");
    ImGui::TextColored(upd.state == agrabah_upd::UPD_AVAILABLE ? col(th.accent)
                       : upd.state == agrabah_upd::UPD_ERROR   ? col(th.warn)
                                                               : col(th.text_muted),
                       "%s", upd.status_text.empty() ? "Ready to check GitHub Releases." : upd.status_text.c_str());
    ImGui::Dummy(ImVec2(0, px(6.0f)));

    if (upd.state == agrabah_upd::UPD_AVAILABLE) {
        char ub[64];
        std::snprintf(ub, sizeof(ub), "Install %s", upd.rel.tag_name.c_str());
        if (ImGui::Button(ub, ImVec2(px(160.0f), px(34.0f))))
            open_upd_modal = true;
        ImGui::SameLine(0, px(10.0f));
    }
    const bool checking = (upd.state == agrabah_upd::UPD_CHECKING);
    ImGui::BeginDisabled(checking || m->in_session);
    if (ImGui::Button(checking ? "Checking..." : "Check for Updates", ImVec2(px(180.0f), px(34.0f))))
        agrabah_upd::check_async((ver && ver[0]) ? ver : "1.0.0", true);
    ImGui::EndDisabled();

#if defined(RECOMP_UI_LIVE_LOG)
    ImGui::SameLine(0, px(10.0f));
    char log_btn[48];
    std::snprintf(log_btn, sizeof(log_btn), g_live_log_open ? "Close Live Log" : "Open Live Log (%zu)",
                  recomp_live_log_line_count());
    if (ImGui::Button(log_btn, ImVec2(px(210.0f), px(34.0f))))
        g_live_log_open = !g_live_log_open;
#endif
    (void)m;
    end_panel();
}

static inline void draw_agrabah_shell(LauncherModel* m, const LauncherTheme& th,
                                      int logical_w, int logical_h) {
    static int s_tab = AGR_TAB_GAME;
    static LngView s_last_model_view = LNG_VIEW_DASHBOARD;
    static bool s_open_upd_modal = false;

    const char* ver_str = recomp_launcher_get_game_version();
    if (!m->in_session)
        agrabah_upd::check_once_on_boot((ver_str && ver_str[0]) ? ver_str : "1.0.0");
    const agrabah_upd::Snapshot upd = agrabah_upd::snapshot();
    if (upd.should_open_modal && !m->in_session)
        s_open_upd_modal = true;
    if (upd.should_quit_to_apply)
        m->action = LNG_ACTION_QUIT;

    if (m->view != s_last_model_view) {
        s_last_model_view = m->view;
        switch (m->view) {
            case LNG_VIEW_DASHBOARD:  s_tab = AGR_TAB_GAME; break;
            case LNG_VIEW_SETTINGS:   s_tab = AGR_TAB_SETTINGS; break;
            case LNG_VIEW_CONTROLLER: s_tab = AGR_TAB_CONTROLS; break;
            case LNG_VIEW_MODS:       s_tab = AGR_TAB_MODS; break;
            case LNG_VIEW_CREDITS:    s_tab = AGR_TAB_ABOUT; break;
            default: break;
        }
    }

    struct TabDef { AgrabahTab id; const char* label; LngView view; };
    TabDef tabs[AGR_TAB__COUNT];
    int n_tabs = 0;
    tabs[n_tabs++] = { AGR_TAB_GAME,     "Play",     LNG_VIEW_DASHBOARD };
    tabs[n_tabs++] = { AGR_TAB_SETTINGS, "Settings", LNG_VIEW_SETTINGS };
    tabs[n_tabs++] = { AGR_TAB_CONTROLS, "Controls", LNG_VIEW_CONTROLLER };
    if (m->mods && launcher_model_can_open_mods(m))
        tabs[n_tabs++] = { AGR_TAB_MODS, "Mods",     LNG_VIEW_MODS };
    tabs[n_tabs++] = { AGR_TAB_ABOUT,    "About",    LNG_VIEW_CREDITS };

    int cur_idx = 0;
    for (int i = 0; i < n_tabs; ++i) {
        if (tabs[i].id == s_tab) { cur_idx = i; break; }
    }
    s_tab = tabs[cur_idx].id;

    if (!m->capturing && !m->hk_capturing && !ImGui::GetIO().WantTextInput) {
        int step = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false)) step = -1;
        else if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false)) step = +1;
        if (step != 0) {
            cur_idx = (cur_idx + step + n_tabs) % n_tabs;
            s_tab = tabs[cur_idx].id;
            m->view = tabs[cur_idx].view;
            s_last_model_view = m->view;
            g_live_log_open = false;
        }
    }

    const float win_w = (float)logical_w;
    const float t = (float)ImGui::GetTime();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float margin_x = px(92.0f);
    const float content_w = (win_w - margin_x * 2.0f) < px(1080.0f)
                            ? (win_w - margin_x * 2.0f) : px(1080.0f);
    const float left_x = (win_w - content_w) * 0.5f;

    // ---- 1. Authorial Top Navigation Header (Brand Left + Underline Tabs Right) ----
    const float hdr_y = px(20.0f);
    {
        const ImVec2 emblem_c(left_x + px(26.0f), hdr_y + px(26.0f));
        draw_magic_lamp_emblem(dl, emblem_c, px(24.0f), t);

        const float text_x = left_x + px(62.0f);
        ImGui::SetCursorScreenPos(ImVec2(text_x, hdr_y + px(2.0f)));
        ImGui::SetWindowFontScale(1.60f);
        ImGui::TextColored(col(th.accent), "AGRABAHPORT");
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SetCursorScreenPos(ImVec2(text_x, hdr_y + px(32.0f)));
        ImGui::TextColored(col(th.text_muted), "Unofficial PC Port  •  SNES (1993)");

        if (upd.state == agrabah_upd::UPD_AVAILABLE) {
            char badge_txt[64];
            std::snprintf(badge_txt, sizeof(badge_txt), "Update %s", upd.rel.tag_name.c_str());
            const float bw = ImGui::CalcTextSize(badge_txt).x + px(22.0f);
            const float bx = text_x + px(252.0f);
            ImGui::SetCursorScreenPos(ImVec2(bx, hdr_y + px(8.0f)));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(28, 92, 88, 230));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(42, 126, 118, 255));
            ImGui::PushStyleColor(ImGuiCol_Text, col(th.good));
            if (ImGui::Button(badge_txt, ImVec2(bw, px(26.0f))))
                s_open_upd_modal = true;
            ImGui::PopStyleColor(3);
        }

        // Right-aligned underlined Palace Tabs (authorial layout, no pill clutter).
        const float tab_w = px(90.0f);
        const float tab_h = px(38.0f);
        const float tab_gap = px(6.0f);
        const float tabs_total = tab_w * n_tabs + tab_gap * (n_tabs - 1);
        float tx = left_x + content_w - tabs_total;
        const float ty = hdr_y + px(8.0f);

        for (int i = 0; i < n_tabs; ++i) {
            const bool active = (tabs[i].id == s_tab);
            ImGui::SetCursorScreenPos(ImVec2(tx, ty));
            ImVec2 mn(tx, ty), mx(tx + tab_w, ty + tab_h);
            char bid[32];
            std::snprintf(bid, sizeof(bid), "##agr_nav_%d", i);
            bool clicked = ImGui::InvisibleButton(bid, ImVec2(tab_w, tab_h), ImGuiButtonFlags_EnableNav);
            bool hov = ImGui::IsItemHovered();

            if (active) {
                dl->AddRectFilled(mn, mx, IM_COL32(244, 196, 66, 26), px(8.0f));
                dl->AddRectFilled(ImVec2(mn.x + px(12.0f), mx.y - px(3.0f)),
                                  ImVec2(mx.x - px(12.0f), mx.y),
                                  IM_COL32(244, 196, 66, 255), px(1.5f));
            } else if (hov) {
                dl->AddRectFilled(mn, mx, IM_COL32(255, 255, 255, 14), px(8.0f));
            }
            const char* lbl = ui_text(tabs[i].label);
            ImVec2 ls = ImGui::CalcTextSize(lbl);
            dl->AddText(ImVec2(mn.x + (tab_w - ls.x) * 0.5f, mn.y + (tab_h - ls.y) * 0.5f),
                        active ? IM_COL32(248, 208, 86, 255)
                        : hov  ? IM_COL32(244, 240, 230, 255)
                               : IM_COL32(174, 172, 198, 255),
                        lbl);
            if (clicked) {
                s_tab = tabs[i].id;
                m->view = tabs[i].view;
                s_last_model_view = m->view;
                g_live_log_open = false;
            }
            tx += tab_w + tab_gap;
        }
    }

    // Gold ornament hairline beneath header.
    const float rule_y = hdr_y + px(62.0f);
    dl->AddRectFilledMultiColor(
        ImVec2(left_x, rule_y), ImVec2(left_x + content_w, rule_y + px(1.5f)),
        IM_COL32(214, 165, 58, 220), IM_COL32(64, 218, 204, 160),
        IM_COL32(64, 218, 204, 160), IM_COL32(214, 165, 58, 25));

    // ---- 2. Main Body Area ----
    const float bottom_h = px(34.0f);
    const float body_top = rule_y + px(18.0f);
    const float body_h = (float)logical_h - body_top - bottom_h - px(12.0f);

    if (s_tab == AGR_TAB_GAME && !g_live_log_open) {
        // Editorial 2-column Palace Showcase: Arch Hero Left + Clean Control Surface Right.
        const float shell_h = body_h > px(480.0f) ? px(470.0f) : body_h;
        const float top_pad = (body_h - shell_h) * 0.35f;
        const float card_y = body_top + (top_pad > 0.0f ? top_pad : 0.0f);

        ImGui::SetCursorScreenPos(ImVec2(left_x, card_y));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(14, 18, 42, 238));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(214, 165, 58, 115));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, px(18.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(px(22.0f), px(22.0f)));
        ImGui::BeginChild("##agr_play_stage", ImVec2(content_w, shell_h),
                          ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar);

        const float inner_w = ImGui::GetContentRegionAvail().x;
        const float inner_h = ImGui::GetContentRegionAvail().y;
        const float gap_x = px(26.0f);
        const float arch_w = inner_w * 0.45f;
        const float right_w = inner_w - arch_w - gap_x;
        const bool verified = launcher_model_rom_verified(m);

        // Left: Pointed Moorish Arch Scene
        const ImVec2 arch_mn = ImGui::GetCursorScreenPos();
        const ImVec2 arch_mx(arch_mn.x + arch_w, arch_mn.y + inner_h);
        draw_agrabah_arch_showcase(ImGui::GetWindowDrawList(), arch_mn, arch_mx, verified, t);
        ImGui::Dummy(ImVec2(arch_w, inner_h));

        // Right: Game Info, Live Status Chips, Golden CTA, Secondary Actions
        ImGui::SetCursorScreenPos(ImVec2(arch_mn.x + arch_w + gap_x, arch_mn.y + px(6.0f)));
        ImGui::BeginGroup();
        {
            ImDrawList* rdl = ImGui::GetWindowDrawList();
            const float rx = arch_mn.x + arch_w + gap_x;

            ImGui::TextColored(col(th.accent2), "SUPER NINTENDO  •  NATIVE PC RECOMP");
            ImGui::Dummy(ImVec2(0, px(4.0f)));

            ImGui::SetWindowFontScale(1.85f);
            ImGui::TextColored(col(th.text), "Aladdin (SNES, 1993)");
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Dummy(ImVec2(0, px(6.0f)));
            ImGui::PushTextWrapPos(rx + right_w);
            if (verified) {
                ImGui::TextColored(col(th.text_muted),
                                   m->s.widescreen
                                       ? "Verified dump (%s) • 16:9 adaptive widescreen & 60 Hz frame pacing ready."
                                       : "Verified dump (%s) • Classic 4:3 pixel presentation & 60 Hz frame pacing ready.",
                                   m->rom_file[0] ? m->rom_file : "Aladdin.sfc");
            } else if (m->rom_detail[0]) {
                ImGui::TextColored(col(th.warn), "%s", m->rom_detail);
                if (ImGui::SmallButton(ui_text("Copy details"))) {
                    char report[2048];
                    launcher_model_rom_report(m, report, sizeof(report));
                    ImGui::SetClipboardText(report);
                }
            } else {
                ImGui::TextColored(col(th.warn),
                                   "Select your headerless US Aladdin (.sfc) ROM to start playing.");
            }
            if (m->setup_error[0]) {
                ImGui::Dummy(ImVec2(0, px(4.0f)));
                ImGui::TextColored(col(th.warn), "%s", m->setup_error);
            }
            ImGui::PopTextWrapPos();

            ImGui::Dummy(ImVec2(0, px(16.0f)));

            // 3 compact live configuration chips
            const float chip_gap = px(10.0f);
            const float chip_w = (right_w - chip_gap * 2.0f) / 3.0f;
            const float chip_h = px(52.0f);
            const ImVec2 cp = ImGui::GetCursorScreenPos();
            const char* fs_val = m->s.fullscreen == 2 ? "Exclusive"
                               : m->s.fullscreen == 1 ? "Borderless" : "Windowed";
            const char* aspect_val = m->s.widescreen
                ? (m->s.aspect_index == 0 ? "16:9 Wide"
                   : m->s.aspect_index == 1 ? "32:21 Wide" : "4:3 Wide")
                : (m->s.aspect_index == 0 ? "4:3 CRT"
                   : m->s.aspect_index == 1 ? "8:7 Pixel" : "1:1 Frame");
            const char* save_val = "No save";
            if (m->saves_supported) {
                const char* sp = launcher_model_native_save_path(m);
                save_val = launcher_native_save_size(sp ? sp : "") >= 0 ? "Active SRAM" : "Empty";
            }
            draw_agrabah_info_chip(rdl, cp, chip_w, chip_h,
                                   "DISPLAY", fs_val, IM_COL32(244, 196, 66, 230));
            draw_agrabah_info_chip(rdl, ImVec2(cp.x + chip_w + chip_gap, cp.y), chip_w, chip_h,
                                   "ASPECT", aspect_val, IM_COL32(64, 218, 204, 230));
            draw_agrabah_info_chip(rdl, ImVec2(cp.x + (chip_w + chip_gap) * 2.0f, cp.y), chip_w, chip_h,
                                   "SAVE DATA", save_val, IM_COL32(70, 227, 155, 230));
            ImGui::Dummy(ImVec2(right_w, chip_h));

            ImGui::Dummy(ImVec2(0, px(22.0f)));

            // Golden Hero CTA button
            const bool can_play = launcher_model_can_launch(m);
            const bool bios_block = launcher_model_bios_blocks_play(m);
            const bool play_enabled = can_play || bios_block;
            if (rui_nav_rehome_to_play())
                ImGui::SetKeyboardFocusHere();
            if (agrabah_gold_cta("##agr_play",
                                 ui_text(m->in_session ? "RESUME GAME" : "PLAY ALADDIN"),
                                 ImVec2(right_w, px(54.0f)), play_enabled, true)) {
                if (bios_block)
                    launcher_model_bios_play_prompt(m);
                else if (!launcher_model_prepare_rom_patch(m)) {
                    if (m->rom_patch_supported && m->s.rom_patch_enabled) {
                        s_tab = AGR_TAB_MODS;
                        m->view = LNG_VIEW_MODS;
                        s_last_model_view = m->view;
                    }
                } else {
                    request_mod_commit_launch(m);
                }
            }
            ImGui::SetItemDefaultFocus();
            if (!play_enabled && ImGui::IsItemClicked() && m->setup_wizard_supported)
                m->setup_wizard_open = true;

            ImGui::Dummy(ImVec2(0, px(12.0f)));

            // Secondary action row
            const float half_w = (right_w - px(10.0f)) * 0.5f;
            const float sec_h = px(36.0f);
            if (!m->disc_selection_locked) {
                if (ImGui::Button(ui_text(verified ? "Change ROM" : "Select ROM"), ImVec2(half_w, sec_h))) {
                    const SystemProfile* prof = (const SystemProfile*)m->profile;
                    const RomFilterSpec filter = active_rom_filter(m, prof);
                    request_rom_picker(m, "Select Aladdin ROM", filter.patterns,
                                       filter.pattern_count, filter.desc, false);
                }
                ImGui::SameLine(0, px(10.0f));
            }
            if (m->in_session) {
                if (ImGui::Button(ui_text("Quit Game"), ImVec2(m->disc_selection_locked ? right_w : half_w, sec_h)))
                    m->action = LNG_ACTION_QUIT;
            } else if (m->saves_supported) {
                const char* selected_save = launcher_model_native_save_path(m);
                const char* sp = selected_save ? selected_save : "";
                ImGui::BeginDisabled(!sp[0]);
                if (ImGui::Button(ui_text("Import Save"), ImVec2(m->disc_selection_locked ? right_w : half_w, sec_h))) {
                    ui_pick_file(m, "Import SRAM save", {"*.srm", "*.sav"},
                                 "Battery save (.srm .sav)", [m](const char* path) {
                                     if (path) launcher_model_import_sram(m, path);
                                 });
                }
                ImGui::EndDisabled();
            } else if (!m->disc_selection_locked) {
                if (ImGui::Button(ui_text("Settings"), ImVec2(half_w, sec_h))) {
                    s_tab = AGR_TAB_SETTINGS;
                    m->view = LNG_VIEW_SETTINGS;
                    s_last_model_view = m->view;
                }
            }

            ImGui::Dummy(ImVec2(0, px(10.0f)));
            bool skip = m->s.skip_launcher != 0;
            if (ImGui::Checkbox(ui_text("Skip launcher on boot"), &skip))
                launcher_model_request_skip_toggle(m);
        }
        ImGui::EndGroup();

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    } else {
        ImGui::SetCursorScreenPos(ImVec2(left_x, body_top));
        begin_container("##agr_page_body", ImVec2(content_w, body_h));
#if defined(RECOMP_UI_LIVE_LOG)
        if (g_live_log_open) {
            draw_live_log_view(th);
        } else
#endif
        switch (s_tab) {
            case AGR_TAB_SETTINGS:
                draw_settings(m, th);
                if (launcher_model_can_restore_defaults(m)) {
                    ImGui::Dummy(ImVec2(0, px(10.0f)));
                    if (ImGui::Button(ui_text("Restore Defaults"), ImVec2(px(160.0f), px(34.0f))))
                        launcher_model_request_restore_defaults(m);
                }
                break;
            case AGR_TAB_CONTROLS:
                draw_controller(m, th);
                break;
            case AGR_TAB_MODS:
                draw_mods(m, th);
                break;
            case AGR_TAB_ABOUT:
            default:
                draw_agrabah_about_page(m, th, upd, s_open_upd_modal);
                break;
        }
        end_container();
    }

    draw_agrabah_update_modal(m, th, upd, s_open_upd_modal);

    // ---- 3. Minimal Authorial Footer Hint Strip (Super Nintendo Button Badges) ----
    {
        const ImVec2 win_pos = ImGui::GetWindowPos();
        const float bar_y = win_pos.y + (float)logical_h - bottom_h;
        dl->AddRectFilledMultiColor(
            ImVec2(left_x, bar_y), ImVec2(left_x + content_w, bar_y + px(1.0f)),
            IM_COL32(214, 165, 58, 30), IM_COL32(214, 165, 58, 120),
            IM_COL32(214, 165, 58, 120), IM_COL32(214, 165, 58, 30));

        const float cy = bar_y + bottom_h * 0.5f;
        const ImU32 txt_col = IM_COL32(168, 166, 198, 240);
        const ImU32 sep_col = IM_COL32(214, 165, 58, 150);
        float hx = left_x;

        auto draw_snes_shoulder = [&](const char* lbl) {
            const float bw = px(24.0f);
            const float bh = px(17.0f);
            const ImVec2 p0(hx, cy - bh * 0.5f);
            const ImVec2 p1(hx + bw, cy + bh * 0.5f);
            grad_rect(dl, p0, p1, px(4.5f),
                      lng_rgba(0.83f, 0.82f, 0.88f, 0.96f),
                      lng_rgba(0.58f, 0.57f, 0.65f, 0.96f));
            dl->AddRect(p0, p1, IM_COL32(94, 90, 114, 240), px(4.5f), 0, px(1.1f));
            ImVec2 ts = ImGui::CalcTextSize(lbl);
            dl->AddText(ImVec2(hx + (bw - ts.x) * 0.5f, cy - ts.y * 0.5f - px(0.5f)),
                        IM_COL32(28, 24, 46, 255), lbl);
            hx += bw + px(4.0f);
        };

        auto draw_snes_face = [&](const char* lbl) {
            const float r = px(9.2f);
            const ImVec2 c(hx + r, cy);
            dl->AddCircleFilled(c, r, IM_COL32(118, 86, 214, 255), 20);
            dl->AddCircleFilled(ImVec2(c.x - px(1.5f), c.y - px(2.0f)), r * 0.52f,
                                IM_COL32(164, 138, 244, 90), 14);
            dl->AddCircle(c, r, IM_COL32(214, 198, 255, 175), 20, px(1.1f));
            ImVec2 ts = ImGui::CalcTextSize(lbl);
            dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f - px(0.5f)),
                        IM_COL32(250, 248, 255, 255), lbl);
            hx += r * 2.0f + px(5.0f);
        };

        auto draw_keycap = [&](const char* lbl) {
            ImVec2 ts = ImGui::CalcTextSize(lbl);
            const float bw = ts.x + px(11.0f);
            const float bh = px(18.0f);
            const ImVec2 p0(hx, cy - bh * 0.5f);
            const ImVec2 p1(hx + bw, cy + bh * 0.5f);
            dl->AddRectFilled(p0, p1, IM_COL32(24, 30, 62, 235), px(4.0f));
            dl->AddRect(p0, p1, IM_COL32(214, 165, 58, 120), px(4.0f), 0, px(1.0f));
            dl->AddText(ImVec2(hx + (bw - ts.x) * 0.5f, cy - ts.y * 0.5f - px(0.5f)),
                        IM_COL32(236, 232, 224, 245), lbl);
            hx += bw + px(5.0f);
        };

        auto draw_label = [&](const char* txt) {
            ImVec2 ts = ImGui::CalcTextSize(txt);
            dl->AddText(ImVec2(hx, cy - ts.y * 0.5f), txt_col, txt);
            hx += ts.x;
        };

        auto draw_dot = [&]() {
            hx += px(10.0f);
            dl->AddCircleFilled(ImVec2(hx, cy), px(2.0f), sep_col, 8);
            hx += px(10.0f);
        };

        draw_snes_shoulder("L");
        draw_snes_shoulder("R");
        hx += px(2.0f);
        draw_label("Switch tabs");
        draw_dot();
        draw_keycap("Enter");
        draw_label("/ ");
        hx += px(2.0f);
        draw_snes_face("B");
        draw_label("Confirm");
        draw_dot();
        draw_snes_face("A");
        draw_label("Back");
        draw_dot();
        draw_keycap("Esc");
        draw_label("Close");

        char ver_line[96];
        const char* ver = recomp_launcher_get_game_version();
        std::snprintf(ver_line, sizeof(ver_line), "AgrabahPort %s", (ver && ver[0]) ? ver : "");
        ImVec2 vs = ImGui::CalcTextSize(ver_line);
        dl->AddText(ImVec2(left_x + content_w - vs.x, bar_y + (bottom_h - vs.y) * 0.5f),
                    IM_COL32(214, 178, 92, 240), ver_line);
    }
}

#endif // AGRABAH_LAUNCHER_SHELL_H
