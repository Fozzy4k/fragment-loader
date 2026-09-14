#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "helpers.h"
#include "elastic.h"

using namespace ImGui;

inline void ShadeVertsLinearGradY(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImColor col0, ImColor col1)
{
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    float ystal = gradient_p1.y - gradient_p0.y;

    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        if (vert->col != IM_COL32_WHITE)
        {
            continue;
        }
        float d = (vert->pos.y - gradient_p0.y) / ystal;
        float t = ImClamp(d, 0.0f, 1.0f);
        vert->col = ImColor(ImLerp(col0.Value, col1.Value, t));
    }
}
inline void ShadeVertsLinearGradX(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImColor col0, ImColor col1)
{
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    float ystal = gradient_p1.x - gradient_p0.x;

    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        if (vert->col != IM_COL32_WHITE)
        {
            continue;
        }
        float d = (vert->pos.x - gradient_p0.x) / ystal;
        float t = ImClamp(d, 0.0f, 1.0f);
        vert->col = ImColor(ImLerp(col0.Value, col1.Value, t));
    }
}
inline void ShadeVertsLinearGrad(ImDrawList* draw_list, int vert_start_idx, int vert_end_idx, ImVec2 gradient_p0, ImVec2 gradient_p1, ImColor col0, ImColor col1)
{
    ImDrawVert* vert_start = draw_list->VtxBuffer.Data + vert_start_idx;
    ImDrawVert* vert_end = draw_list->VtxBuffer.Data + vert_end_idx;
    ImVec2 ystal = gradient_p1 - gradient_p0;

    for (ImDrawVert* vert = vert_start; vert < vert_end; vert++)
    {
        if (vert->col != IM_COL32_WHITE)
        {
            continue;
        }
        ImVec2 d = ((vert->pos - gradient_p0) / ystal);
        float t = ImClamp(sqrtf(d.x * d.x + d.y * d.y), 0.0f, 1.0f);
        vert->col = ImColor(ImLerp(col0.Value, col1.Value, t));
    }
}
inline ImColor lerp(ImColor col1, ImColor col2, float t, bool a = false, float extra_alpha = 1.f)
{
    col1.Value.w *= extra_alpha;
    col2.Value.w *= extra_alpha;
    if (a)
    {
        col1.Value.w *= t;
        col2.Value.w *= t;
    }
    return ImLerp(col1.Value, col2.Value, t);
}
inline ImColor a(ImColor col, float t)
{
    col.Value.w *= t;
    return col;
}

inline void circle(const ImVec2& center, float radius, ImU32 col, float angle, float angle2, float thic)
{
    auto draw = ::GetWindowDrawList();

    draw->PathClear();
    draw->PathArcTo(center, radius, helpers::deg2rad(angle), helpers::deg2rad(angle + angle2), 40.f);
    draw->PathStroke(col, 0, thic);
}
inline void seg_c(const ImVec2& center, float radius, ImU32 col, int segm, float s = 0.f, float t = 1.f)
{
    float step = (360.f / (float(segm))) / 2.f;
    float start = s + step / 2.f;
    for (int i = 0; i < segm * 2; i++)
    {
        if (i % 2 == 0)
            continue;
        circle(center, radius, col, start + (step * i), step, t);
    }
}
inline float get_y_elements(float total_y, float& wheel)
{
    static float MouseWheel = ::GetIO().MouseWheel;
    MouseWheel = ImLerp(MouseWheel, ::GetIO().MouseWheel, 0.4f);
    wheel += MouseWheel * 33.f;
    float y = 16.f + wheel;
    float delta = y + total_y;
    if (total_y > 322.f)
    {
        if (delta < 322.f)
        {
            wheel -= (y - 16.f) * 0.1f;
        }
    }
    else
    {
        if (y < 16.f)
        {
            wheel -= (y - 16.f) * 0.1f;
        }
    }
    y = 16.f + wheel;
    if (y > 16.f)
    {
        wheel -= (y - 16.f) * 0.1f;
    }
    y = 16.f + wheel;
    return y;
}

inline int rotation_start_index;
inline void rot_start() {
    rotation_start_index = ::GetWindowDrawList()->VtxBuffer.Size;
}
inline ImVec2 rot_center() {
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

    const auto& buf = ::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++) {
        l = ImMin(l, buf[i].pos);
        u = ImMax(u, buf[i].pos);
    }

    return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); // _ClipRectStack
}
inline ImVec2 rot_botr() {
    ImVec2 u(-FLT_MAX, -FLT_MAX);

    const auto& buf = ::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
    {
        u = ImMax(u, buf[i].pos);
    }

    return u; // _ClipRectStack
}
inline ImVec2 rotates(const ImVec2& v, float s, float c) {
    return ImVec2(v.x * c - v.y * s, v.x * s + v.y * c);
}
inline void rot_end(float deg, ImVec2 center = rot_center()) {
    float rad = helpers::deg2rad(deg);
    float s = sinf(rad), c = cosf(rad);
    center = rotates(center, s, c) - center;

    auto& buf = ::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++) {
        buf[i].pos = rotates(buf[i].pos, s, c) - center;
    }
}
