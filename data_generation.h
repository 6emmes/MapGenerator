#pragma once
#include <vector>
#include <random>
#include <stdio.h>
#include "config.h"
#include "terrain_noise.h"
#include "helper.h"
#include "textures_core.h"

std::vector<std::vector<float>> generateHeightMap();
std::vector<std::vector<float>> generateWaterMap(const std::vector<std::vector<float>>& heightMap);
std::vector<std::vector<float>> metalDensity(float frequency, float bias,
                                            float seed, const std::vector<std::vector<float>>& waterMap);
std::tuple<std::vector<std::vector<float>>,std::vector<std::vector<float>>, std::vector<std::vector<float>>> calculateTreeDensity(
                        float frequency, float seed,
                        const std::vector<std::vector<float>>& waterMap,
                        const std::vector<std::vector<float>>& tempMap,
                        const std::vector<std::vector<float>>& fertilityMap);
template <typename T>
void fillValue(const std::vector<std::vector<float>>& waterMap,
                    std::vector<std::vector<T>>& data,
                    bool targetColor, T fillColor, Point start);
template <typename T>
void fillValueGreater(const std::vector<std::vector<float>>& dataMap,
                    std::vector<std::vector<T>>& data,
                    float targetColor, T fillColor, Point start);
std::vector<std::vector<float>> idData(const std::vector<std::vector<float>>& waterMap);
std::vector<std::vector<float>> marbleDensity(std::vector<std::vector<float>> &heightMap, float seed);
// Returns a per‑pixel shading intensity map (0‑255 float) based on height and land masks.
std::vector<std::vector<float>> calculateShadingMap(int w, int h,
    const std::vector<std::vector<float>>& heightmap);
