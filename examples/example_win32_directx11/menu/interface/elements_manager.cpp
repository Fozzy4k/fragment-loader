#include "elements_manager.h"

void e_manager::add_game(const char* label, const char* desc, bool u, void* bytes, SIZE_T b_size)
{
    games.push_back(game_t(label, desc, u, bytes, b_size, g_pd3dDevice));
}
