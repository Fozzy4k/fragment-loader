#pragma once
#include <imgui.h>

// fragment palette. Window/border values mirror source/gui/colors/colors.h so the
// loader matches the overlay; the loader's near-black background is kept.
namespace c
{
    inline ImColor bg = ImColor(14, 15, 17, 255);
    inline ImColor bg48 = ImColor(14, 15, 17, 122);
    inline ImColor bg0 = ImColor(14, 15, 17, 0);
    inline ImColor black = ImColor(0, 0, 0, 255);
    inline ImColor black48 = ImColor(0, 0, 0, 122);
    inline ImColor black24 = ImColor(0, 0, 0, 61);
    inline ImColor primary = ImColor(50, 120, 200, 255);      // #3278C8 - fragment accent
    inline ImColor primary48 = ImColor(50, 120, 200, 122);
    inline ImColor primary24 = ImColor(50, 120, 200, 61);
    inline ImColor primary12 = ImColor(50, 120, 200, 30);
    inline ImColor primary6 = ImColor(50, 120, 200, 15);
    inline ImColor primary0 = ImColor(50, 120, 200, 0);
    inline ImColor secondary = ImColor(88, 152, 220, 255);    // lighter blue for gradient tops
    inline ImColor secondary12 = ImColor(88, 152, 220, 30);
    inline ImColor secondary2 = ImColor(88, 152, 220, 5);
    inline ImColor secondary0 = ImColor(88, 152, 220, 0);
    inline ImColor highlight = ImColor(130, 185, 240, 255);
    inline ImColor white = ImColor(255, 255, 255, 255);
    inline ImColor white72 = ImColor(255, 255, 255, 182);
    inline ImColor white48 = ImColor(255, 255, 255, 122);
    inline ImColor white36 = ImColor(255, 255, 255, 91);
    inline ImColor white24 = ImColor(255, 255, 255, 61);
    inline ImColor white12 = ImColor(255, 255, 255, 30);
    inline ImColor white6 = ImColor(255, 255, 255, 15);
    inline ImColor white4 = ImColor(255, 255, 255, 10);
    inline ImColor white3 = ImColor(255, 255, 255, 7);
    inline ImColor white2 = ImColor(255, 255, 255, 5);
    inline ImColor white1 = ImColor(255, 255, 255, 3);
    inline ImColor white0 = ImColor(255, 255, 255, 0);

    inline ImColor gray = ImColor(173, 173, 173, 255);
    inline ImColor gray2 = ImColor(118, 118, 118, 255);

    inline struct grad_t
    {
        ImColor f = secondary;
        ImColor s = primary;
    } grad;
};
