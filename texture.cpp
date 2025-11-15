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


inline float Chebyshev(float x, float y, float deadzone=0.0f) {
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


std::vector<std::vector<float>> generateHeightMap() {
    //srand(time(NULL)); // Seed for reproducibility
    srand(0);
    int randX = rand()%1000;
    int randY = rand()%1000;
    
    std::vector<std::vector<float>> map(TEX_H, std::vector<float>(TEX_W));
    float maxVal = -1e10f;
    float minVal = 1e10f;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float nx = (x + randX) * NOISE_SCALE;
            float ny = (y + randY) * NOISE_SCALE;
            float value = MM(nx, ny)+10;
            
            if (value < 0.f) value = 0.f;
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
        }
    }
    return map;
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
TextureData calculateShadingTexture(const TextureData& heightmap) {
    const int w = heightmap.width;
    const int h = heightmap.height;
    TextureData shade;
    shade.width = w;
    shade.height = h;
    shade.pixels.resize(w * h);

    const float inv_two = 1.0f / 1.414213562f; // 1/sqrt(2) for top‑left unit vector

    auto getHeight = [&](int x, int y) -> float {
        // Clamp to borders; we use nearest pixel value at edges.
        x = std::max(0, std::min(x, w - 1));
        y = std::max(0, std::min(y, h - 1));
        uint32_t pixel = heightmap.pixels[y * w + x];
        uint8_t r = (pixel >> 24) & 0xFF; // red channel as height
        return static_cast<float>(r);
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float h_left  = getHeight(x - 1, y);
            float h_right = getHeight(x + 1, y);
            float h_up    = getHeight(x, y - 1);
            float h_down  = getHeight(x, y + 1);

            // Central differences
            float gx = h_right - h_left;
            float gy = h_down - h_up;
            float len = std::sqrt(gx * gx + gy * gy);
            uint8_t intensity;
            if (len == 0.0f) {
                intensity = 128;
            } else {
                float nx = gx / len;
                float ny = gy / len;
                // Dot with vector pointing to top-left (-1,-1)
                float dot = -(nx + ny) * inv_two; // cos(theta)
                // Map [-1,1] to [0,255]
                intensity = static_cast<uint8_t>(std::clamp((dot + 1.0f) * 127.5f, 0.0f, 255.0f));
            }
            const uint8_t alpha = static_cast<uint8_t>(128); // 75% opacity
            shade.pixels[y * w + x] = (intensity << 24) | (intensity << 16) | (intensity << 8) | alpha;
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
TextureData climateTexture(const std::vector<std::vector<double>>& tempMap) {
    TextureData data;
    data.width = static_cast<int>(tempMap[0].size());
    data.height = static_cast<int>(tempMap.size());
    data.pixels.resize(data.width * data.height);
    for (int y = 0; y < data.height; ++y) {
        for (int x = 0; x < data.width; ++x) {
            double tempK = tempMap[y][x];
            // Simple linear mapping: 200K -> 0, 400K -> 255
            //uint8_t r = static_cast<uint8_t>(std::clamp((tempK - 200.0) / 200.0 * 255.0, 0.0, 255.0));
            uint8_t r = static_cast<uint8_t>(std::clamp((tempK - 200.0) / 200.0 * 255.0, 0.0, 255.0));
            uint32_t pixel = (r << 24) | (0 << 16) | (0 << 8) | 0xFF;
            data.pixels[y * data.width + x] = pixel;
        }
    }
    return data;
}
