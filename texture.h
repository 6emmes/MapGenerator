#pragma once

// Header declaring the texture generation function used by sdl_3.cpp.
// The function returns a struct containing width, height, and pixel data.
// Implementation lives in texture.cpp.
#include <vector>
#include <cstdint>

struct TextureData {
    int width;
    int height;
    std::vector<uint32_t> pixels;
};

TextureData generateTextureRGBA();
TextureData calculateShadingTexture(const TextureData& heightmap);
