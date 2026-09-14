#include "texture.h"

// Only the decoders the loader's embedded assets need, and only the in-memory
// entry point (no stdio, no HDR, no linear-space conversions).
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"

#include <cstdint>
#include <limits>

ID3D11ShaderResourceView* create_texture_from_memory(ID3D11Device* device, const void* bytes, size_t size)
{
    if (!device || !bytes || size == 0 || size > static_cast<size_t>((std::numeric_limits<int>::max)()))
    {
        return nullptr;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(
        static_cast<const stbi_uc*>(bytes),
        static_cast<int>(size),
        &width,
        &height,
        &channels,
        4);
    if (!pixels || width <= 0 || height <= 0)
    {
        if (pixels)
        {
            stbi_image_free(pixels);
        }
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC description{};
    description.Width = static_cast<UINT>(width);
    description.Height = static_cast<UINT>(height);
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initial{};
    initial.pSysMem = pixels;
    initial.SysMemPitch = static_cast<UINT>(width) * 4u;

    ID3D11Texture2D* texture = nullptr;
    const HRESULT created = device->CreateTexture2D(&description, &initial, &texture);
    stbi_image_free(pixels);
    if (FAILED(created) || !texture)
    {
        return nullptr;
    }

    ID3D11ShaderResourceView* view = nullptr;
    const HRESULT viewed = device->CreateShaderResourceView(texture, nullptr, &view);
    texture->Release();
    if (FAILED(viewed))
    {
        return nullptr;
    }
    return view;
}
