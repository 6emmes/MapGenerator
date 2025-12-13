#pragma once

// Header declaring the texture generation function used by sdl_3.cpp.
// The function returns a struct containing width, height, and pixel data.
// Implementation lives in texture.cpp.
#include <vector>
#include <string>
#include <cstdint>

struct TextureData {
    int width;
    int height;
    std::vector<uint32_t> pixels;
};

// Generates a color texture from a height map
TextureData heightTexture(const std::vector<std::vector<float>>& map);
// Generates a grayscale height texture for shading calculations
TextureData heightTextureFromHeightMap(const std::vector<std::vector<float>>& map);
// Generates a 2D array of height values in the range [0,1].
std::tuple<std::vector<std::vector<float>>, std::vector<std::vector<float>>> generateHeightMap();
// Calculates a shading texture from a height texture
TextureData shadingTexture(int w, int h, const std::vector<std::vector<float>>& heightmap, std::vector<std::vector<float>> landMap);
// Saves the given heightmap to a 24‑bit BMP file. Returns true on success.
bool saveHeightMapBMP(const std::vector<std::vector<float>>& map, const char *filename);
// New signature: accepts a vector of pointers to 3‑D float layers.
void saveTiff32(const std::vector<std::vector<std::vector<float>>*>& layers, std::vector<std::string> layerNames, const char *filename);
void saveTiff8(const std::vector<std::vector<std::vector<float>>*>& layers, std::vector<std::string> layerNames, const char *filename);
// New: generate a climate texture where the red channel encodes temperature.
TextureData climateTexture(const std::vector<std::vector<float>>& tempMap, const std::vector<std::vector<float>>& humidityMap);
TextureData riverTexture(const std::vector<std::vector<float>> &riverMap);
TextureData waterTexture(const std::vector<std::vector<float>>& waterMap);
TextureData idTexture(const std::vector<std::vector<float>>& waterMap);
std::vector<std::vector<float>> idData(const std::vector<std::vector<float>>& waterMap);
// Generates a 2D metal‑density map using fbm noise.
std::vector<std::vector<float>> metalDensity();
