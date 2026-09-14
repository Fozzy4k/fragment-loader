#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include "menu_i.h"
#include "../helpers/anim/anim.h"
#include "../helpers/restore.h"
#include "../helpers/render.h"
#include "../helpers/texture.h"
#include "../helpers/widgets.h"
#include "../defs/fonts.h"
#include "../defs/textures.h"
#include "../defs/colors.h"
#include "../bytes/bg.h"
#include "../bytes/loadbg.h"
#include "../bytes/medium.h"
#include "../bytes/regular.h"
#include "../bytes/logo.h"
#include "../bytes/icon.h"
#include "../bytes/noise.h"
#include "elements_manager.h"
#include "rain.hpp"

using namespace ImGui;

//need for anim imgui AddText
float alpha_text = 1.f;

class _c_menu : public c_menu
{
public:
    void setup_data(ID3D11Device* device) override
    {
        MGR->g_pd3dDevice = g_pd3dDevice = device;
        if (tex::login1_bg == nullptr)
            tex::login1_bg = create_texture_from_memory(g_pd3dDevice, background_1_, sizeof(background_1_));
        if (tex::login2_bg == nullptr)
            tex::login2_bg = create_texture_from_memory(g_pd3dDevice, background_2_, sizeof(background_2_));
        if (tex::login3_bg == nullptr)
            tex::login3_bg = create_texture_from_memory(g_pd3dDevice, background_3_, sizeof(background_3_));
        if (tex::main_logo == nullptr)
            tex::main_logo = create_texture_from_memory(g_pd3dDevice, logo, sizeof(logo));
        if (tex::noise == nullptr)
            tex::noise = create_texture_from_memory(g_pd3dDevice, noise, sizeof(noise));

        ImGuiIO& io = ImGui::GetIO();
        f::regular10 = io.Fonts->AddFontFromMemoryTTF(&regular_ttf, sizeof regular_ttf, 12.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::medium12 = io.Fonts->AddFontFromMemoryTTF(&medium_ttf, sizeof medium_ttf, 14.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::medium8 = io.Fonts->AddFontFromMemoryTTF(&medium_ttf, sizeof medium_ttf, 10.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::icons60 = io.Fonts->AddFontFromMemoryTTF(&icons_ttf, sizeof icons_ttf, 60.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::icons30 = io.Fonts->AddFontFromMemoryTTF(&icons_ttf, sizeof icons_ttf, 30.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::icons20 = io.Fonts->AddFontFromMemoryTTF(&icons_ttf, sizeof icons_ttf, 20.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::icons13 = io.Fonts->AddFontFromMemoryTTF(&icons_ttf, sizeof icons_ttf, 13.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::icons12 = io.Fonts->AddFontFromMemoryTTF(&icons_ttf, sizeof icons_ttf, 12.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        f::icons10 = io.Fonts->AddFontFromMemoryTTF(&icons_ttf, sizeof icons_ttf, 10.f, NULL, io.Fonts->GetGlyphRangesCyrillic());
        rain = new Rain();
    }
    void setup_imgui() override
    {
        ImGuiStyle* style = &::GetStyle();
        ImVec4* colors = style->Colors;

        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 255);
        colors[ImGuiCol_ChildBg] = ImColor(19, 18, 23, 255);
        colors[ImGuiCol_PopupBg] = ImColor(7, 8, 18, 127);
        colors[ImGuiCol_Border] = ImColor(0, 0, 0, 0);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = c::white4;
        colors[ImGuiCol_ScrollbarGrab] = c::white12;
        colors[ImGuiCol_ScrollbarGrabHovered] = c::white12;
        colors[ImGuiCol_ScrollbarGrabActive] = c::white12;
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
        colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
        colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
        colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        colors[ImGuiCol_WindowShadow] = ImVec4(0.08f, 0.08f, 0.08f, 0.35f);

        style->ChildRounding = 0.f;
        style->WindowRounding = 16.f;
        style->WindowPadding = ImVec2(0, 0);
        style->ItemSpacing.y = 8.f;
        style->ScrollbarSize = 5.f;
        style->ScrollbarRounding = 512.f;
    }
    void loading_screen()
    {
        auto draw = ::GetWindowDrawList();
        const auto& p = ::GetWindowPos() + ImVec2(1.f, 1.f);
        const ImVec2& region = ::GetContentRegionMax() - ImVec2(2.f, 2.f);
        ImVec2 center = ImVec2(p.x + region.x/2.f, p.y + 185.f);
        auto switch_screen_a = anim::get_anim_obj(CONST_HASH("switch_screen"));
        auto circle_rot_a = anim::animation("circle_rot", anim_t(clamp_out, 0.01f));
        auto circle_timer_a = anim::animation("circle_timer", anim_t(clamp_out, 0.1f));
        auto loading_screen_a = anim::animation("loading_screen", anim_t(clamp_out, 0.05f));
        loading_screen_a->active = true;

        //notify
        auto notify_a = anim::animation("notify", anim_t(clamp_out, 0.1f));
        auto notify_extra_a = anim::animation("notify_extra", anim_t(clamp_out, 0.1f));
        auto notify_timer_a = anim::animation("notify_timer", anim_t(clamp_out, 0.005f));
        
        float anim = switch_screen_a->inverse() * loading_screen_a->val * notify_a->inverse();
        float anim2 = notify_a->extra();

        if (anim2 >= 1.f)
        {
            notify_extra_a->active = true;
            notify_timer_a->active = true;
        }

        if (MGR->status == E_NONE)
            anim2 *= switch_screen_a->inverse();

        float rot = 360.f * circle_rot_a->extra() - 360.f * anim2;
        static float circles[6] = { rot, 0.f, 0.f, 0.f, 0.f, 0.f };
        static bool resett = false;

        //load circle
        {
            if (circle_timer_a->val <= 0.f && loading_screen_a->val > 0.8f)
            {
                circle_rot_a->active = true;
            }
            if (circle_timer_a->val >= 1.f)
            {
                circle_timer_a->reset();
            }
            if (circles[0] >= 360.f)
            {
                resett = true;
            }
            if (resett)
            {
                circles[0] -= 5.f;
                rot = circles[0] - 20.f;
                if (circles[0] <= 0.f)
                {
                    rot = 0.f; circle_rot_a->reset(); circle_timer_a->active = true; resett = false;
                }
            }
            int vert_start_idx = 0;
            int vert_end_idx = 0;
            float rad = 0.f;
            float alph = 1.f / 6.f;
            float alph_mod = circles[5] / 360.f;
            for (int i = 0; i < 6; i++)
            {
                if (i == 0)
                    circles[i] = ImLerp(circles[i], rot, 0.5f);
                else
                    circles[i] = ImLerp(circles[i], circles[i - 1], 0.4f);

                float alpha = fabsf(alph_mod - std::clamp((alph * float(i)) - 0.10f, 0.06f, 1.f)) * anim;
                rad = 3.f + (9.f * i);
                vert_start_idx = draw->VtxBuffer.Size;
                seg_c(center, rad + 130.f * anim2, IM_COL32_WHITE, max(4 * i, 4), circles[i], i == 5 ? 2.f : 1.f);
                vert_end_idx = draw->VtxBuffer.Size;
                ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
                    center - ImVec2(rad, rad), center + ImVec2(rad, rad), a(c::grad.f, alpha), a(c::grad.s, alpha));
            }
        }

        text_center(center + ImVec2(0.f, 54.f + 10.f * loading_screen_a->extra()), "Initializing... This won't take long.", a(c::white, anim));

        MGR->callback(s_game);

        text_center(center + ImVec2(0.f, 72.f + 10.f * loading_screen_a->extra()), MGR->loading_module.c_str(), a(c::white48, anim), 12.f, f::regular10);

        static status_e status = MGR->status;
        static std::string notify_desc = MGR->notify_desc;
        if (MGR->status == E_NONE)
        {
            next_screen = 2; switch_screen_a->active = true;
        }
        else
        {
            //notify
            if (MGR->status == E_SUCCESS || MGR->status == E_ERROR)
            {
                status = MGR->status;
                notify_desc = MGR->notify_desc;
                MGR->locked = true;
                notify_a->active = true;
                if (notify_timer_a->val >= 1.f)
                    MGR->status = E_NONE;
            }
        }

        //render notify
        if (anim2 > 0.f && s_game > -1)
        {
            auto& game = MGR->games.at(s_game);
            ImRect bbImg = ImRect(center - ImVec2(88.f * anim2, 49.5f * anim2), center + ImVec2(88.f * anim2, 49.5f * anim2));
            draw->AddImageRounded(ImTextureID(game.img), bbImg.Min, bbImg.Max, ImVec2(0, 0), ImVec2(1, 1), a(c::white, anim2), 8.f);
            draw->AddRect(bbImg.Min, bbImg.Max, a(c::white24, anim2), 8.f);
            if (notify_extra_a->val > 0.f)
            {
                float anim3 = switch_screen_a->inverse() * notify_extra_a->extra();
                //text notify
                {
                    draw->PushClipRect(center - ImVec2(region.x / 2.f * anim3, region.y / 2.f * anim2),
                        center + ImVec2(region.x / 2.f * anim3, region.y / 2.f * anim2));

                    if (status == E_SUCCESS)
                    {
                        text_center(center + ImVec2(0.f, 66.f), (std::string("Success! ") + game.label + " Loaded.").c_str(), a(c::white, anim3));
                        text_center(center + ImVec2(0.f, 88.f), (game.label + " will start automatically in a moment.").c_str(), a(c::white48, anim3), 12.f, f::regular10);
                    }
                    else if (status == E_ERROR)
                    {
                        text_center(center + ImVec2(0.f, 66.f), (std::string("Error! ") + game.label + " failed to load.").c_str(), a(c::white, anim3));
                        text_center(center + ImVec2(0.f, 88.f), "Please try again in a few moments.", a(c::white48, anim3), 12.f, f::regular10);
                    }

                    draw->PopClipRect();
                }


                ImRect bbNotify = ImRect(p + ImVec2(64.f, 532.f - 200 * anim3), p + ImVec2(256.f, 584.f - 200 * anim3));

                ImColor col_new = c::primary;
                ImColor col_new2 = c::primary;
                ImColor col_icon = c::primary;
                std::string icon = "a";
                std::string text = "No Info";
                switch (status)
                {
                case E_SUCCESS:
                    col_new = ImColor(33, 47, 31, 255);
                    col_new2 = ImColor(25, 31, 26, 255);
                    col_icon = ImColor(147, 255, 108, 255);
                    icon = "C";
                    text = "Success!";
                    break;
                case E_ERROR:
                    col_new = ImColor(49, 31, 33, 255);
                    col_new2 = ImColor(30, 24, 26, 255);
                    col_icon = ImColor(255, 108, 110, 255);
                    icon = "D";
                    text = "Error!";
                    break;
                default: col_new = c::primary;
                    break;
                }
                //bg
                {
                    //a crutch because of imgui
                    {
                        //left
                        /*int vert_start_idx = draw->VtxBuffer.Size;
                        draw->AddRectFilled(bbNotify.Min, ImVec2(bbNotify.Min.x + 24.f, bbNotify.Max.y), IM_COL32_WHITE, 16.f, ImDrawFlags_RoundCornersLeft);
                        int vert_end_idx = draw->VtxBuffer.Size;
                        ShadeVertsLinearGradX(draw, vert_start_idx, vert_end_idx,
                            bbNotify.Min, ImVec2(bbNotify.Min.x + 24.f, bbNotify.Max.y),
                            col_new2, col_new);*/

                        //center
                        //draw->AddRectFilled(ImVec2(bbNotify.Min.x + 24.f, bbNotify.Min.y), ImVec2(bbNotify.Max.x - 24.f, bbNotify.Max.y), col_new);
                        draw->AddRectFilled(bbNotify.Min, bbNotify.Max, col_new, 16.f);

                        //right
                        /*vert_start_idx = draw->VtxBuffer.Size;
                        draw->AddRectFilled(ImVec2(bbNotify.Max.x - 24.f, bbNotify.Min.y), bbNotify.Max, IM_COL32_WHITE, 16.f, ImDrawFlags_RoundCornersRight);
                        vert_end_idx = draw->VtxBuffer.Size;
                        ShadeVertsLinearGradX(draw, vert_start_idx, vert_end_idx,
                            ImVec2(bbNotify.Max.x - 24.f, bbNotify.Min.y), bbNotify.Max,
                            col_new, col_new2);*/
                    }
                    //draw->AddRect(bbNotify.Min - ImVec2(1.f, 1.f), bbNotify.Max + ImVec2(1.f, 1.f), c::black48, 16.f);
                    draw->AddRectFilled(bbNotify.Min, bbNotify.Max, c::bg48, 16.f);
                    draw->AddRect(bbNotify.Min, bbNotify.Max, c::white6, 16.f);

                    //rain
                    draw->PushClipRect(bbNotify.Min, bbNotify.Max - ImVec2(16.f, 0.f), true);
                    rain->render(bbNotify.Min, draw);
                    draw->PopClipRect();
                }

                draw->AddText(f::icons20, 20.f, bbNotify.Min + ImVec2(14.f, 16.f), col_icon, icon.c_str());
                draw->AddText(f::medium12, 14.f, bbNotify.Min + ImVec2(44.f, 12.f), c::white, text.c_str());
                draw->AddText(f::regular10, 12.f, bbNotify.Min + ImVec2(44.f, 28.f), c::white48, notify_desc.c_str());
            }
        }

        //reset
        if (switch_screen_a->val >= 1.f)
        {
            loading_screen_a->reset(); notify_a->reset(); resett = false; MGR->locked = false;
            circle_rot_a->reset(); circle_timer_a->reset(); notify_timer_a->reset(); s_game = -1;
            notify_extra_a->reset(); status = MGR->status;

            for (auto& circle : circles)
                circle = 0.f;
        }
    }
    void select_screen()
    {
        auto switch_screen_a = anim::get_anim_obj(CONST_HASH("switch_screen"));
        auto button_a = anim::animation("button_a", anim_t(clamp_out, 0.1f));
        auto select_screen_a = anim::animation("select_screen", anim_t(clamp_out, 0.05f));
        select_screen_a->active = true;
        float anim = switch_screen_a->inverse() * select_screen_a->val;
        if (!MGR->games.empty())
        {
            float total_y = 76.f * int(MGR->games.size()) + 6.f * (int(MGR->games.size()) - 1);
            static float wheel_s = 0.f;
            active_product_y = ImLerp(active_product_y, get_y_elements(total_y, wheel_s), 0.4f);
            float y = active_product_y + (400.f) * select_screen_a->extra(i_inverse | i_inv_bounce) + (400.f) * switch_screen_a->val;
            int curr_game = 0;
            int old_game = s_game;
            for (; curr_game < MGR->games.size(); curr_game++)
            {
                auto& game_e = MGR->games.at(curr_game);
                if (game(y, game_e, s_game == curr_game, (y + 76) < 369.f, anim))
                {
                    s_game = s_game != curr_game ? curr_game : -1;
                }
                y += 76.f + 6.f;
            }
            if (old_game != s_game && (old_game == -1 || s_game == -1))
            {
                button_a->active = true;
            }
            auto draw = ::GetWindowDrawList();
            const auto& p = ::GetWindowPos() + ImVec2(1.f, 1.f);
            const ImVec2& region = ::GetContentRegionMax() - ImVec2(2.f, 2.f);
            ImRect bb = ImRect(ImVec2(p.x, p.y + 369.f), ImVec2(p.x + region.x, p.y + region.y));
            draw->AddRectFilled(bb.Min, bb.Max, a(c::bg, switch_screen_a->inverse()), 16.f, ImDrawFlags_RoundCornersBottom);

            ImRect bbShadow = ImRect(ImVec2(p.x, p.y + 320.f), ImVec2(p.x + region.x, p.y + 369.f));
            int vert_start_idx = draw->VtxBuffer.Size;
            draw->AddRectFilled(bbShadow.Min, bbShadow.Max, IM_COL32_WHITE);
            int vert_end_idx = draw->VtxBuffer.Size;
            ShadeVertsLinearGradY(draw, vert_start_idx, vert_end_idx,
                bbShadow.Min, bbShadow.Max, c::bg0, a(c::bg, switch_screen_a->inverse()));


            static float offset = 102.f;
            static std::string button_t = "Select Game";
            static std::string button_i = "";
            static bool switch_name = false;
            //anim blinking text
            {
                float inverse_a = button_a->inverse();
                if (inverse_a >= 0.65f)
                {
                    alpha_text = (inverse_a - 0.65f) * 2.87f;
                }
                else if (inverse_a < 0.65f && inverse_a > 0.25f)
                {
                    alpha_text = 0.f; switch_name = true;
                }
                else if (inverse_a <= 0.25f)
                {
                    alpha_text = (button_a->val - 0.75f) * 4.f;
                    if (inverse_a <= 0.f)
                    {
                        button_a->reset();
                    }
                }
                if (switch_name)
                {
                    switch_name = false;
                    if (s_game >= 0)
                    {
                        button_t = "Load"; button_i = "B"; offset = 121.f;
                    }
                    else
                    {
                        button_t = "Select Game"; button_i = ""; offset = 102.f;
                    }
                }
            }

            if (button(bb.Min + ImVec2(16.f, 1.f), button_t.c_str(), s_game >= 0, offset,
                button_i.c_str(), anim, "gamesel", ImVec2(), ImVec2(288.f, 30.f)))
            {
                next_screen = 3; switch_screen_a->active = true;
            }
            alpha_text = 1.f;

            //reset
            if (switch_screen_a->val >= 1.f)
            {
                select_screen_a->reset(); button_a->reset(); button_t = "Select Game"; button_i = ""; offset = 102.f;
            }
        }
    }
    void login_screen()
    {
        auto draw = ::GetWindowDrawList();
        const auto& p = ::GetWindowPos() + ImVec2(1.f, 1.f);
        const ImVec2& region = ::GetContentRegionMax() - ImVec2(2.f, 2.f);
        auto switch_screen_a = anim::get_anim_obj(CONST_HASH("switch_screen"));
        auto login_size = anim::animation("login_size", anim_t(clamp_out, 0.06f));
        auto login_y = anim::animation("login_y", anim_t(clamp_out, 0.1f));
        auto pulse = anim::animation("pulse", anim_t(clamp_out, 0.015f));
        if (pulse->val <= 0.1f)
            pulse->active = true;
        if (pulse->val >= 0.9f)
            pulse->active = false;

        float login_y_ = 19.f * login_y->val;
        float login_alpha = switch_screen_a->inverse();
        ImRect bb = ImRect(ImVec2(p.x, p.y), ImVec2(p.x + region.x, p.y + region.y));
        float center = bb.GetWidth() / 2.f;

        //glow
        draw->AddShadowCircle(bb.Min + ImVec2(center, 52.f), 18.f, a(c::primary, login_alpha), 70.f + 180.f * pulse->extra(), ImVec2(0, 0));

        //anim logo
        float logo_y = switch_screen_a->val * 108.f;

        //logo bg
        draw->AddImage(ImTextureID(tex::main_logo), bb.Min + ImVec2(center - 36.f, -logo_y), bb.Min + ImVec2(center + 36.f, 104.f - logo_y));//def

        //logo icon
        ImVec2 i_pos = ImVec2(bb.Min + ImVec2(center - 10.f, 42.f - logo_y));
        draw->AddText(f::icons20, 20.f, i_pos, c::white, "a");

        //text
        float extra_a = max((login_y->inverse() - 0.5f) * 2.f, 0.f);
        if (extra_a > 0.f)
            text_center(bb.Min + ImVec2(center, 128.f - login_y_), "Hello Again!", a(c::white, extra_a * login_alpha));

        extra_a = max((login_y->val - 0.5f) * 2.f, 0.f);
        if (extra_a > 0.f)
            text_center(bb.Min + ImVec2(center, 128.f - login_y_), "Welcome!", a(c::white, extra_a * login_alpha));

        //desc
        extra_a = max((login_y->inverse() - 0.5f) * 2.f, 0.f);
        if (extra_a > 0.f)
            text_center(bb.Min + ImVec2(center, 148.f - login_y_), "We're glad you're back. Please enter your login details.", a(c::white48, extra_a * login_alpha), 12.f, f::regular10);

        extra_a = max((login_y->val - 0.5f) * 2.f, 0.f);
        if (extra_a > 0.f)
            text_center(bb.Min + ImVec2(center, 148.f - login_y_), "Ready to join us? Let's create your account.", a(c::white48, extra_a * login_alpha), 12.f, f::regular10);

        //widgets
        {
            //inputs
            float extra_a2 = max((login_y->val - 0.5f) * 2.f, 0.f);
            if (extra_a2 > 0.f)
                input(draw, email, bb.Min + ImVec2(32.f, 174.f - login_y_), "f", "email", extra_a2 * login_alpha);

            input(draw, login, bb.Min + ImVec2(32.f, 174.f + login_y_), "d", "login", login_alpha);
            input(draw, password, bb.Min + ImVec2(32.f, 212.f + login_y_), "e", "password", login_alpha);

            //anim blinking text
            ImVec2 out_size = ImVec2(0, 0);
            static bool switch_name = false;
            float inverse_a = login_size->inverse();
            if (inverse_a >= 0.65f)
            {
                alpha_text = (inverse_a - 0.65f) * 2.87f;
            }
            else if (inverse_a < 0.65f && inverse_a > 0.20f)
            {
                alpha_text = 0.f; switch_name = true;
            }
            else if (inverse_a <= 0.20f)
            {
                alpha_text = (login_size->val - 0.80f) * 5.f;
                if (inverse_a <= 0.f)
                {
                    login_size->reset();
                }
            }
            alpha_text *= login_alpha;
            static std::string login = "Login";
            static std::string regist = "Register";
            static std::string loginI = "g";
            static std::string registI = "A";

            if (switch_name)
            {
                switch_name = false;
                if (screen != 0)
                {
                    login = "Register";
                    regist = "Login";
                    loginI = "A";
                    registI = "g";
                }
                if (screen == 0)
                {
                    login = "Login";
                    regist = "Register";
                    loginI = "g";
                    registI = "A";
                }
            }

            if (button(bb.Min + ImVec2(32.f, 258.f + login_y_), login.c_str(), true, 56.f,
                loginI.c_str(), login_alpha, "first", out_size))
            {
                next_screen = 2;
                switch_screen_a->active = true;
            }

            if (button(bb.Min + ImVec2(32.f + out_size.x + 8.f, 258.f + login_y_), regist.c_str(), false, 10.f,
                registI.c_str(), login_alpha, "second") && !switch_screen_a->active)
            {
                screen = !screen;
                login_size->active = true;
                login_y->active = !login_y->active;
            }

            alpha_text = 1.f;//restore

            alpha_text *= login_alpha;

            //socials
            text_button(bb.Min + ImVec2(141.f, 370.f), "b");//tg
            text_button(bb.Min + ImVec2(165.f, 371.f), "c");//discord

            alpha_text = 1.f;//restore
        }
    }
    void draw() override
    {
        anim::update_animation_speed();
        auto switch_screen_a = anim::animation("switch_screen", anim_t(clamp_out, 0.1f));
        
        begin("General");
        {
            background();

            if (screen < 2)
                login_screen();
            else if (screen == 2)
                select_screen();
            else
                loading_screen();
        }
        if (switch_screen_a->val >= 1.f)
        {
            screen = next_screen;
            switch_screen_a->reset();
        }
        //call end() in render loop
    }
    bool begin(const char* name) override
    {
        //bg on example window
        ::GetBackgroundDrawList()->AddImage(ImTextureID(tex::bg), ImVec2(0, 0), ImVec2(1920, 1080), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

        ::SetNextWindowSize(ImVec2(320, 416));
        ImGui::SetNextWindowPos({ 0, 0 });
        bool ret = ::Begin(name, nullptr, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus);



        return ret;
    }
    void background() override
    {
        auto draw = ::GetWindowDrawList();
        const auto& p = ::GetWindowPos() + ImVec2(1.f, 1.f);
        const ImVec2& region = ::GetContentRegionMax() - ImVec2(2.f, 2.f);

        ImRect bb = ImRect(ImVec2(p.x, p.y), ImVec2(p.x + region.x, p.y + region.y));

        //bg
        draw->AddRectFilled(bb.Min, bb.Max, c::bg, 16.f);

        //bg img
        {
            static ImColor col = c::primary;
            ImColor col_new = c::primary;
            switch (MGR->status)
            {
            case E_NONE: col_new = c::primary;
                break;
            case E_SUCCESS: col_new = ImColor(147, 255, 108, 255);
                break;
            case E_ERROR: col_new = ImColor(255, 108, 110, 255);
                break;
            default: col_new = c::primary;
                break;
            }

            col = lerp(col, col_new, 0.03f);

            draw->AddImage(ImTextureID(tex::login1_bg), bb.Min, bb.Max, ImVec2(0, 0), ImVec2(1, 1), col);
            draw->AddImage(ImTextureID(tex::login2_bg), bb.Min, bb.Max, ImVec2(0, 0), ImVec2(1, 1), col);
            draw->AddImage(ImTextureID(tex::login3_bg), bb.Min, bb.Max);//def
        }

        rain->update();
        rain->render(bb.Min, draw);
    }
    void end() override
    {
        auto draw = ::GetWindowDrawList();
        const auto& p = ::GetWindowPos() + ImVec2(1.f, 1.f);
        const ImVec2& region = ::GetContentRegionMax() - ImVec2(2.f, 2.f);
        ImRect bb = ImRect(ImVec2(p.x, p.y), ImVec2(p.x + region.x, p.y + region.y));

        //noise
        draw->AddImage(ImTextureID(tex::noise), bb.Min, bb.Max, ImVec2(0, 0), ImVec2(1, 1), c::white12);//def

        //border
        draw->AddRect(bb.Min, bb.Max, c::white4, 16.f);

        move_window();

        ::End();
    }
    void destroy() override
    {
        delete this;
    }

    std::string get_login() override
    {
        return login;
    }
    std::string get_password() override
    {
        return password;
    }
    std::string get_email() override
    {
        return email;
    }
private:
    int screen = 0;
    int next_screen = 0;
    int s_game = -1;
    float active_product_y = 16.f;
    char login[64]{};
    char password[64]{};
    char email[64]{};
    Rain* rain;
    ID3D11Device* g_pd3dDevice = NULL;
};

c_menu* create_menu()
{
    return new _c_menu();
}
