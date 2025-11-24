// Implementation of generateSineGradientRGBA.
// This file is separate from sdl_3.cpp to keep gradient logic isolated.

#include "texture.h"
#include "terrain.h"
#include <cmath>
#include <cstdint>
#include <vector>
#include <cstdlib>
#include <time.h>
#include <iostream> //debug
#include <algorithm>
// Expose seed variable defined in terrain.cpp
extern int SEED;


inline float Chebyshev(float x, float y, float deadzone=0.0f) {
    std::cout<<"cheby"<<std::endl;
    float cx = x - TEX_W / 2.0f;
    float cy = y - TEX_H / 2.0f;
    float distance = 0.0f;
    float ratio = static_cast<float>(TEX_H) / static_cast<float>(TEX_W);
    if (ratio > 1.0f) {
        cx *= ratio;
    } else {
        cy /= ratio;
    }
    distance = std::max(std::abs(cx), std::abs(cy))/ (TEX_W / 2.0f);
    if (deadzone < distance) {
        return 0.0f;
    }   else {
        return distance - deadzone;
    }   
}


std::tuple<std::vector<std::vector<float>>, std::vector<std::vector<bool>>> generateHeightMap() {
    // Seed for reproducibility using SEED from config; if SEED is -1 use current time
    if (SEED == -1) {
        srand(time(NULL));
    } else {
        srand(SEED);
    }
    int randX = rand()%1200;
    int randY = rand()%1200;
    
    std::vector<std::vector<float>> map(TEX_H, std::vector<float>(TEX_W));
    std::vector<std::vector<bool>> land(TEX_H, std::vector<bool>(TEX_W, true));
    float maxVal = -1e10f;
    float minVal = 1e10f;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float nx = (x + randX) * NOISE_SCALE;
            float ny = (y + randY) * NOISE_SCALE;
            float value = MM(nx, ny, 4);
            
            //if (value < 0.f) value = 0.f;
            if (value > maxVal) maxVal = value;
            if (value < minVal) minVal = value;
            map[y][x] = value;
        }
    }
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            // Normalize to [0,1] based on min/max found
            float norm = map[y][x];
            norm = (norm - minVal) / (maxVal - minVal);
            //norm = norm - 1.5 * Chebyshev(x, y, 0.7);
            norm = simpleMap(norm);
            map[y][x] = norm;
            if (norm<0.1) land[y][x] = false;
        }
    }
    return {map,land};
}

// Convert a height map to a color texture using getTerrainPixel.
TextureData textureFromHeightMap(const std::vector<std::vector<float>>& map) {
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float h = map[y][x];
            Color c = getTerrainPixel(h);
            if (h < 0.1f) {
                uint32_t pixel = (c.r << 24) | (c.g << 16) | (c.b << 8) | 127;
                data.pixels[y * TEX_W + x] = pixel;
                continue;
            }
            uint32_t pixel = (c.r << 24) | (c.g << 16) | (c.b << 8) | 0xFF;
            data.pixels[y * TEX_W + x] = pixel;
        }
    }
    return data;
}

// Convert a height map to a grayscale texture suitable for shading.
TextureData heightTextureFromHeightMap(const std::vector<std::vector<float>>& map) {
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float h = map[y][x];
            uint8_t val = static_cast<uint8_t>(std::clamp(((h - 0.1f) / 0.9f) * 255.f, 0.f, 255.f));
            // Store height as greyscale in R,G,B channels
            uint32_t pixel = (val << 24) | (val << 16) | (val << 8) | 0xFF;
            data.pixels[y * TEX_W + x] = pixel;
        }
    }
    return data;
}

//---------------------------------------------------------------------------
//  calculateShadingTexture
//  Generates a greyscale shading texture based on height variations
//  in the supplied heightmap.  The heightmap must be the output of
//  `generateTextureRGBA`, where each pixel's red channel encodes the
//  height.  For each pixel we look at its four orthogonal
//  neighbours (up, down, left, right) to compute a central‑difference
//  gradient.  The gradient direction is compared to the unit vector
//  pointing toward the top‑left corner.
//  Pixels where the gradient points toward the top‑left are made
//  white (intensity 255); where it points toward the bottom‑right
//  they are dark (intensity 0).  Intermediate directions are mapped
//  linearly to the full greyscale range.  The resulting texture is
//  75 % transparent (alpha = 192).
//--------------------------------------------------------------------------
TextureData calculateShadingTexture(int w, int h, const std::vector<std::vector<float>>& heightmap,
    std::vector<std::vector<bool>> landMap) {
    TextureData shade;
    shade.width = w;
    shade.height = h;
    shade.pixels.resize(w * h);

    const float inv_two = 1.0f / 1.414213562f; // 1/sqrt(2) for top‑left unit vector
    float h_left;
    float h_right;
    float h_up;
    float h_down;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (landMap[x][y] == false){
                shade.pixels[x * w + y] = (0 << 24) | (0 << 16) | (0 << 8) | 0;
                continue;
            }
            if (x-1>=0) h_left  = heightmap[x - 1][ y];
            else h_left = heightmap[x][y];
            if (x+1<w) h_right = heightmap[x + 1] [y];
            else h_right = heightmap[x] [y];
            if (y-1>=0) h_up = heightmap[x][y - 1];
            else h_up = heightmap[x][y];
            if (y+1<h) h_down  = heightmap[x][y + 1];
            else h_down  = heightmap[x][y];

            // Central differences
            float gx = h_right - h_left;
            float gy = h_down - h_up;
            float nx = -gx;
            float ny = -gy;
            float nz = 0.005f; // vertical component
            float len = std::sqrt(nx*nx + ny*ny + nz*nz);
            nx /= len;
            ny /= len;
            nz /= len;

            // Example light vector pointing diagonally down-left
            float lx = -1.0f, ly = -1.0f, lz = 1.0f;
            float llen = std::sqrt(lx*lx + ly*ly + lz*lz);
            lx /= llen; ly /= llen; lz /= llen;

            float dot = nx*lx + ny*ly + nz*lz;
            dot = std::clamp(dot, -1.0f, 1.0f);
            uint8_t intensity = static_cast<uint8_t>((dot + 1.0f) * 127.5f);
            const uint8_t alpha = static_cast<uint8_t>(64); // 75% opacity
            shade.pixels[x * w + y] = (intensity << 24) | (intensity << 16) | (intensity << 8) | alpha;
        }
    }
    return shade;
}

// duplicate removed

//-----
//  climateTextureFromTemperature
//-----
// Generate a texture where each pixel's red channel encodes temperature
// supplied as a matrix of kilo‑Kelvin values. The result is RGBA8888
// with red channel varying, green/blue zero, alpha opaque.
TextureData climateTexture(const std::vector<std::vector<float>>& tempMap,
                          const std::vector<std::vector<float>>& humidityMap) {
    TextureData data;
    data.width = static_cast<int>(tempMap[0].size());
    data.height = static_cast<int>(tempMap.size());
    data.pixels.resize(data.width * data.height);
    for (int y = 0; y < data.height; ++y) {
        for (int x = 0; x < data.width; ++x) {
            float tempK = tempMap[y][x];
            // Temperature red channel: map 280K (~7°C) to 0 and 320K (~47°C) to 255.
            uint8_t r = static_cast<uint8_t>(std::clamp((tempK - 280.0) / 40.0 * 255.0, 0.0, 255.0));

            // Humidity blue channel: Scale 0-30 to 0-255.
            uint8_t b = 0;
            float hum = humidityMap[y][x];
            b = static_cast<uint8_t>(std::clamp((hum) / 30.0 * 255.0, 0.0, 255.0));

            uint32_t pixel = (r << 24) | (0 << 16) | (b << 8) | 0xFF;
            data.pixels[y * data.width + x] = pixel;
        }
    }
    return data;
}

TextureData riverTexture(const std::vector<std::vector<float>>& riverMap) {
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float v = riverMap[y][x];
            uint8_t val = static_cast<uint8_t>(std::clamp(v * 255.f, 0.f, 255.f));
            uint32_t pixel = (0 << 24) | (0 << 16) | (val << 8) | val;
            data.pixels[y * TEX_W + x] = pixel;
        }
    }
    return data;
}
