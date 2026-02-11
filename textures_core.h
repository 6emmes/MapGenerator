#pragma once
#include <vector>
#include <stack>
#include <cstdint>
#include <cmath>
#include "colorpalette.h"
#include "colortexture.h"
#include "colorramp.h"
#include "config.h"
#include "helper.h"

struct TextureData {
    int width;
    int height;
    std::vector<uint32_t> pixels;
};

struct Point {
    int x;
    int y;
};

void fillTexture(const std::vector<std::vector<float>>& waterMap, TextureData& texture, bool targetColor, int fillColor, Point start);
uint32_t averageRGBA(uint32_t c1, uint32_t c2);
uint32_t weightedAverageRGBA(uint32_t c1, uint32_t c2, float dist1, float dist2);
uint32_t blend(uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11,
               float fx, float fy);
int getColorPalette(float temp, float humidity, std::vector<TextureSample> cloud);
int getColorTexture(float temp, float humidity);
uint32_t getTerrainPixel(float height);
void saveTiff8(int lat_top, int lat_bot, const std::vector<std::vector<std::vector<float>>*>& layers, std::vector<std::string> layerNames, const char *filename);