#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>

#include "render.h"
#include "../helpers/anim/anim.h"
#include "../helpers/restore.h"
#include "../defs/fonts.h"
#include "../defs/textures.h"
#include "../defs/colors.h"
#include "../interface/elements_manager.h"

using namespace ImGui;

inline float global_a = 0.f;

inline bool text_button(ImVec2 pos, const char* text, ImColor col = c::white12, float f_sz = 13.f, ImFont* font = f::icons13)
{
    auto draw = ::GetWindowDrawList();
    auto curr = anim::animation((std::string(text) + "##ibutton").c_str(), anim_t(clamp_out, 0.1f));

    draw->AddText(font, f_sz, pos, lerp(col, c::primary, curr->val), text);

    ::GetCurrentWindow()->DC.CursorPos = pos;
    ImVec2 text_size = font->CalcTextSizeA(f_sz, FLT_MAX, -1.f, text, 0, NULL);
    bool ret = ::InvisibleButton((std::string(text) + "##ibutton").c_str(), ImVec2(text_size.x, f_sz));
    curr->active = ::IsItemHovered();

    return ret;
}

inline void text_center(ImVec2 pos, const char* text, ImColor col = c::white, float f_sz = 14.f, ImFont* font = f::medium12, float offset = 0.f)
{
    auto draw = ::GetWindowDrawList();
    ImVec2 text_size = font->CalcTextSizeA(f_sz, FLT_MAX, -1.f, text, 0, NULL);
    ImVec2 t_pos = ImVec2(pos + ImVec2(-text_size.x / 2.f + offset, 0.f));
    draw->AddText(font, f_sz, t_pos, col, text);
}
inline void input(ImDrawList* draw, char* buf, ImVec2 p0, const char* icon, const char* text, float ex_a = 1.f, ImVec2 size = ImVec2(256.f, 30.f))
{
    auto curr = anim::animation((std::string("input##") + text).c_str(), anim_t(clamp_out, 0.1f));
    const auto& p = ::GetWindowPos() + ImVec2(1.f, 1.f);

    ImRect bb = ImRect(p0, p0 + size);
    const float text_y = (size.y - 14.f) * 0.5f;

    if (std::string(buf).empty())
    {
        draw->AddText(f::medium12, 14.f, ImVec2(p0.x + 10.f, p0.y + text_y), a(c::white48, curr->inverse() * ex_a), text);
    }

    //bg & border
    draw->AddRectFilled(bb.Min, bb.Max, a(lerp(c::white3, c::white6, curr->val), ex_a), 8.f);
    draw->AddRect(bb.Min, bb.Max, a(lerp(c::white1, c::white4, curr->val), ex_a), 8.f);

    //icon cell, sized to the field height
    {
        const float cell = size.y;
        ImColor icon_bg1 = a(lerp(c::white2, c::grad.f, curr->val), ex_a);
        ImColor icon_bg2 = a(lerp(c::white2, c::grad.s, curr->val), ex_a);
        int vert_start_idx = draw->VtxBuffer.Size;
        draw->AddRectFilled(bb.Max - ImVec2(cell, cell), bb.Max, IM_COL32_WHITE, 7.f);
        int vert_end_idx = draw->VtxBuffer.Size;
        ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
            bb.Max - ImVec2(cell, cell), bb.Max, icon_bg1, icon_bg2);

        ImColor icon_br1 = a(lerp(c::primary0, c::highlight, curr->val), ex_a);
        ImColor icon_br2 = a(lerp(c::primary0, c::grad.s, curr->val), ex_a);
        vert_start_idx = draw->VtxBuffer.Size;
        draw->AddRect(bb.Max - ImVec2(cell, cell), bb.Max, IM_COL32_WHITE, 7.f);
        vert_end_idx = draw->VtxBuffer.Size;
        ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
            bb.Max - ImVec2(cell, cell), bb.Max, icon_br1, icon_br2);

        text_center(bb.Max + ImVec2(-cell / 2.f, -(cell + 12.f) * 0.5f), icon, a(lerp(c::white48, c::white, curr->val), ex_a), 12.f, f::icons12, icon == "d" ? 1.f : 0.f);//fixing a disproportionate icon
    }

    ImGui::SetCursorPos(p0 - p + ImVec2(10.f, text_y));
    ::SetNextItemWidth(size.x - size.y - 12.f);
    ::PushFont(f::medium12);
    ::PushStyleColor(ImGuiCol_Text, a(c::white, ex_a).Value);
    ::PushStyleColor(ImGuiCol_FrameBg, ImVec4(255.f / 255.f, 255.f / 255.f, 255.f / 255.f, 0.f));
    ::PushStyleColor(ImGuiCol_TextSelectedBg, a(c::primary, ex_a).Value);
    ::InputText((std::string("##") + text).c_str(), buf, 96);
    ::PopStyleColor(3);
    ::PopFont();

    curr->active = ::IsItemActive();
}
inline bool button_ex(const char* label, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, pos + size_arg);
    ItemSize(size_arg, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);
    return pressed;
}
inline bool button(ImVec2 pos, const char* text, bool active, float off, const char* icon, float ex_a = 1.f, const char* custom_id = NULL, ImVec2& out_size = ImVec2(0, 0), ImVec2 size2 = ImVec2(0, 0))
{
    auto draw = ::GetWindowDrawList();
    ImGuiWindow* window = ::GetCurrentWindow();
    std::string buf = custom_id == NULL ? text : custom_id;
    std::string id_char = (std::string("button##") + buf);
    const auto id = CONST_HASH(id_char.c_str());
    ImVec2 text_size = f::medium12->CalcTextSizeA(14.f, FLT_MAX, -1.f, text, 0, NULL);
    float icon_size = 0.f;
    if (icon)
    {
        icon_size = f::icons10->CalcTextSizeA(10.f, FLT_MAX, -1.f, icon, 0, NULL).x + 8.f;
    }
    ImVec2 size = ImVec2(off * 2.f + text_size.x + icon_size, 30.f);
    if (size2 != ImVec2(0, 0))
    {
        size = size2;
    }

    // A fixed-size button centres its icon+label as a pair. Callers were passing a
    // guessed left padding, which never lined up for both labels.
    const bool fixed_width = (size2.x != 0.f);
    const float content_x = fixed_width ? (size.x - (icon_size + text_size.x)) * 0.5f : off;


    auto curr = anim::animation(id_char, anim_t(clamp_out, 0.1f));
    static std::map<hash32_t, elastic_point> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, elastic_point(size.x, 0.1f, 1.1f, 5.f) });
        it_anim = anim.find(id);
    }
    ImRect bb = ImRect(pos, pos + ImVec2(it_anim->second.get_pos(), size.y));

    it_anim->second.set_target(size.x);
    it_anim->second.update(0.3f);

    //bg
    ImColor bg1 = lerp(c::white6, c::grad.f, curr->val);
    ImColor bg2 = lerp(c::white6, c::grad.s, curr->val);
    int vert_start_idx = draw->VtxBuffer.Size;
    draw->AddRectFilled(bb.Min, bb.Max, IM_COL32_WHITE, 8.f);
    int vert_end_idx = draw->VtxBuffer.Size;
    ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
        bb.Min, bb.Max, a(bg1, ex_a), a(bg2, ex_a));

    //border
    ImColor br1 = lerp(c::white6, c::highlight, curr->val);
    ImColor br2 = lerp(c::white6, c::grad.s, curr->val);
    vert_start_idx = draw->VtxBuffer.Size;
    draw->AddRect(bb.Min, bb.Max, IM_COL32_WHITE, 8.f);
    vert_end_idx = draw->VtxBuffer.Size;
    ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
        bb.Min, bb.Max, a(br1, ex_a), a(br2, ex_a));

    // Vertical placement is derived from the resolved height so callers can pass
    // any size2 without the label drifting off-centre.
    const float icon_y = (size.y - 12.f) * 0.5f;
    const float text_y = (size.y - 16.f) * 0.5f;

    if (icon_size > 0.f)
        draw->AddText(f::icons10, 10.f, bb.Min + ImVec2(content_x, icon_y), lerp(c::white48, c::white, curr->val), icon);

    draw->AddText(f::medium12, 14.f, bb.Min + ImVec2(content_x + icon_size, text_y), lerp(c::white48, c::white, curr->val), text);

    curr->active = active;

    out_size = ImVec2(it_anim->second.get_pos(), size.y);

    window->DC.CursorPos = pos;
    return button_ex(id_char.c_str(), size);
}
inline bool game(float y, game_t& game, bool active, bool can_use, float ex_a)
{
    std::string id_char = (std::string("game##") + game.label);
    auto curr = anim::animation(id_char.c_str(), anim_t(clamp_out, 0.1f));
    auto draw = ::GetWindowDrawList();
    ImGuiWindow* window = ::GetCurrentWindow();
    const auto& p = ::GetWindowPos() + ImVec2(1.f, 1.f);
    ImVec2 size = ImVec2(386.f, 116.f);
    ImVec2 pos = p + ImVec2(16.f, y);
    ImRect bb = ImRect(pos, pos + size);

    draw->AddRectFilled(bb.Min, bb.Max, a(lerp(c::white2, c::white6, curr->val), ex_a), 10.f);
    draw->AddRect(bb.Min, bb.Max, a(lerp(c::white1, c::white4, curr->val), ex_a), 10.f);

    ImRect bbImg = ImRect(bb.Min + ImVec2(16.f, 16.f), bb.Min + ImVec2(16.f, 16.f) + ImVec2(128.f, 84.f));
    draw->AddImageRounded(ImTextureID(game.img), bbImg.Min, bbImg.Max, ImVec2(0, 0), ImVec2(1, 1), a(c::white, max(curr->val, 0.48f) * ex_a), 6.f);
    draw->AddRect(bbImg.Min, bbImg.Max, a(lerp(c::white12, c::white24, curr->val), ex_a), 6.f);

    const float text_x = 16.f + 128.f + 18.f;
    draw->AddText(f::medium12, 16.f, bb.Min + ImVec2(text_x, 22.f), a(lerp(c::white48, c::white, curr->val), ex_a), game.label.c_str());
    draw->AddText(f::regular10, 13.f, bb.Min + ImVec2(text_x, 48.f), a(lerp(c::white12, c::white48, curr->val), ex_a), game.desc.c_str());

    if (game.updated)
    {
        ImRect bbUpd = ImRect(bb.Min + ImVec2(306.f, 16.f), bb.Min + ImVec2(306.f, 16.f) + ImVec2(64.f, 20.f));
        //bg
        ImColor bg1 = lerp(c::white6, c::grad.f, curr->val);
        ImColor bg2 = lerp(c::white6, c::grad.s, curr->val);
        int vert_start_idx = draw->VtxBuffer.Size;
        draw->AddRectFilled(bbUpd.Min, bbUpd.Max, IM_COL32_WHITE, 16.f);
        int vert_end_idx = draw->VtxBuffer.Size;
        ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
            bbUpd.Min, bbUpd.Max, a(bg1, ex_a), a(bg2, ex_a));

        //border
        ImColor br1 = lerp(c::white6, c::highlight, curr->val);
        ImColor br2 = lerp(c::white6, c::grad.s, curr->val);
        vert_start_idx = draw->VtxBuffer.Size;
        draw->AddRect(bbUpd.Min, bbUpd.Max, IM_COL32_WHITE, 16.f);
        vert_end_idx = draw->VtxBuffer.Size;
        ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
            bbUpd.Min, bbUpd.Max, a(br1, ex_a), a(br2, ex_a));

        draw->AddText(f::medium8, 11.f, bbUpd.Min + ImVec2(10.f, 4.f), a(lerp(c::white48, c::white, curr->val), ex_a), "Updated");
    }

    curr->active = active;
    window->DC.CursorPos = pos;
    return can_use ? button_ex(id_char.c_str(), size) : false;
}
