#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <examples/example_win32_directx11/menu/helpers/anim/anim.h>
#include <examples/example_win32_directx11/menu/helpers/elastic.h>
#include <examples/example_win32_directx11/menu/helpers/render.h>
#include <examples/example_win32_directx11/menu/defs/colors.h>
#include <time.h>
#include <imgui.h>
#include <imgui_internal.h>

template <class _Ty>
_Ty random(_Ty min, _Ty max)
{
    return min + rand() % (max - min);
}

struct rain_p
{
    int x{};
    float min_y{};
    float start_y{};
    float timer_r{};
    float alpha{};
    elastic_point point{};
    anim_t* timer = nullptr;
};

class Rain
{
public:
    Rain()
    {
        srand(time(0));
        for (int i = 0; i < 10; i++)
        {
            particles[i] = particle(i);
        }
    };
    rain_p particle(int idx)
    {
        rain_p p{};
        p.x = (32 * idx) + random(8, 35);
        p.min_y = -random(5, 20);
        p.start_y = 16 + random(1, 72);
        p.timer_r = 0.8f - float(random(1, 30)) / 100.f;
        p.alpha = 1.f;
        p.point = elastic_point(p.min_y, 0.1f, 1.1f, 6.f, p.start_y);
        p.timer = anim::animation((std::string("timer##") + std::to_string(idx)), anim_t(clamp_out, 0.1f));
        return p;
    }
    void update()
    {
        for (int i = 0; i < 10; i++)
        {
            auto& p = particles[i];
            p.timer = anim::animation((std::string("timer##") + std::to_string(i)), anim_t(clamp_out, 0.1f));//update timer
            if (p.point.get_pos() > p.start_y)
            {
                if (p.timer->val >= p.timer_r)
                {
                    p.timer->reset();
                    p.point.set_target(p.point.get_target() + 140);
                }
                else
                {
                    p.timer->active = true;
                }
            }
            if (p.point.get_pos() > 110)
            {
                p.alpha = 1.f - (p.point.get_pos() - 110) / 30.f;
                if (p.alpha <= 0.f)
                {
                    p.timer->reset();//reset timer bc we dont need to create new
                    p = particle(i);//create new particle
                }
            }
            p.point.update(0.14f);
        }
    }
    void render(ImVec2 min, ImDrawList* draw)
    {
        for (int i = 0; i < 10; i++)
        {
            auto& p = particles[i];
            float render_y = p.point.get_pos();
            float vel_y = p.point.get_v() * 1.25f;
            if (vel_y < 0.f)//anim: collects it back into one pixel (drop)
                vel_y *= -10.f;

            ImVec2 particle = min + ImVec2(p.x, render_y);

            ImRect bbLine = ImRect(particle + ImVec2(0.f, -max(vel_y, 6.f)),
                particle + ImVec2(0.f, vel_y));

            float modd = float(abs(p.x - 160)) / 160.f;

            ImColor cl = a(c::white72, max(1.f - modd, 0.1f) * p.alpha);

            int vert_start_idx = draw->VtxBuffer.Size;
            draw->AddLine(bbLine.Min, bbLine.Max, IM_COL32_WHITE);
            int vert_end_idx = draw->VtxBuffer.Size;
            ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
                bbLine.Min, bbLine.Max, c::bg0, cl);
        }
    }

private:
    rain_p particles[10]{};
};
