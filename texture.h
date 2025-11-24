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

// Generates a color texture from a height map
TextureData textureFromHeightMap(const std::vector<std::vector<float>>& map);
// Generates a grayscale height texture for shading calculations
TextureData heightTextureFromHeightMap(const std::vector<std::vector<float>>& map);
// Generates a 2D array of height values in the range [0,1].
std::vector<std::vector<float>> generateHeightMap();
// Calculates a shading texture from a height texture
TextureData calculateShadingTexture(int w, int h, const std::vector<std::vector<float>>& heightmap);
// Saves the given heightmap to a 24‑bit BMP file. Returns true on success.
bool saveHeightMapBMP(const std::vector<std::vector<float>>& map, const char *filename);
// New: generate a climate texture where the red channel encodes temperature.
TextureData climateTexture(const std::vector<std::vector<double>>& tempMap, const std::vector<std::vector<double>>& humidityMap);

TextureData riverTexture(const std::vector<std::vector<float>> &riverMap);
