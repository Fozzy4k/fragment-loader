#pragma once

#include <d3d11.h>

// Decodes an in-memory PNG/JPEG into a shader resource view.
// Replaces D3DX11CreateShaderResourceViewFromMemory so the loader does not
// depend on the deprecated DirectX SDK (June 2010).
//
// Returns nullptr on failure; the caller owns the returned view.
ID3D11ShaderResourceView* create_texture_from_memory(ID3D11Device* device, const void* bytes, size_t size);
