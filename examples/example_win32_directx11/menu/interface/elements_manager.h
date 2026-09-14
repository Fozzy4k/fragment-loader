#pragma once
#include <unordered_map>
#include <string>
#include <functional>
#include <D3D11.h>
#include <cstdint>

#include "../helpers/texture.h"

struct game_t
{
    game_t(std::string l, std::string d, bool u, void* bytes, SIZE_T b_size, ID3D11Device* device)
    {
        label = l; desc = d; updated = u;
        if (img == nullptr && bytes != nullptr && b_size > 0)
            img = create_texture_from_memory(device, bytes, static_cast<size_t>(b_size));
    };
    std::string label{};
    std::string desc{};
    bool updated{};
    ID3D11ShaderResourceView* img = nullptr;
};

enum status_e : byte
{
    E_NONE = 0,
    E_LOADING,
    E_SUCCESS,
    E_ERROR
};

class e_manager
{
public:

    void add_game(const char* label, const char* desc, bool u, void* bytes, SIZE_T b_size);
    std::vector<game_t> games{};

    //load & notify
    std::function<void(int&)> callback{};
    status_e status{};
    std::string loading_module{};
    std::string notify_desc{};
    bool locked{};
    
    //help me
    ID3D11Device* g_pd3dDevice = nullptr;
};

inline e_manager* MGR = new e_manager();
