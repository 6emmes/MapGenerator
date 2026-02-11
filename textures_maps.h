#pragma once
#include <algorithm>
#include "textures_core.h"

TextureData heightTexture(const std::vector<std::vector<float>>& map);
TextureData heightTextureFromHeightMap(const std::vector<std::vector<float>>& map);
TextureData shadingTexture(int w, int h, const std::vector<std::vector<float>>& heightmap,
    std::vector<std::vector<float>> landMap);
TextureData shadingTexture(int w, int h, std::vector<std::vector<float>>& shadingdata);
TextureData climateTexture(const std::vector<std::vector<float>>& tempMap,
                        const std::vector<std::vector<float>>& humidityMap);
TextureData riverTexture(const std::vector<std::vector<float>>& riverMap);
TextureData waterTexture(const std::vector<std::vector<float>>& waterMap);
TextureData idTexture(const std::vector<std::vector<float>>& waterMap);
TextureData calculateColorMap(const std::vector<std::vector<float>>&tempMap,
                        const std::vector<std::vector<float>>&humidityMap,
                        const std::vector<std::vector<float>>&waterMap);