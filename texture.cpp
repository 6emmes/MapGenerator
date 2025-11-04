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

// Constants moved to terrain.h. Only texture generation remains.

// Chebyshev distance helper used for texture distortion
inline float Chebyshev(float x, float y) {
    // Center coordinates around the texture origin
    float cx = x - TEX_W / 2.0f;
    float cy = y - TEX_H / 2.0f;
    // Compute aspect ratio to stretch distance in the longer axis.
    // If the texture is wider than it is tall, distances in X are scaled
    // up by the width/height ratio. Vice versa for taller textures.
    float ratio = static_cast<float>(TEX_H) / static_cast<float>(TEX_W);
    if (ratio > 1.0f) {
        // Width dominates: stretch in X
        cx *= ratio;
    } else {
        // Height dominates: stretch in Y
        cy /= ratio; // effectively stretch in Y by ratio = H/W >1
    }
    return std::max(std::abs(cx), std::abs(cy));
}

// The noise and terrain mapping logic has been moved to terrain.cpp.

TextureData generateTextureRGBA() {
    srand(time(NULL)); // Seed for reproducibility
    int randX = rand()%1000;
    int randY = rand()%1000;
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    float maxVal = -1e10f;
    float minVal = 1e10f;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float nx = (x + randX) * NOISE_SCALE;
            float ny = (y + randY) * NOISE_SCALE;
            float value = fbm(nx, ny); // [-1,1]
            // Map to [0,1]
            float norm = (value+3)/9;
            norm = norm - 0.5*Chebyshev(x, y)/512;
            if (norm < 0 ) norm = 0.0;
            //float norm = value;
            if (norm > maxVal) maxVal = norm;
            if (norm < minVal) minVal = norm;
            //uint8_t intensity = static_cast<uint8_t>(norm * 255.0f);
            Color c = getTerrainPixel(norm);
            uint8_t r = c.r;
            uint8_t g = c.g;
            uint8_t b = c.b;
            uint8_t a = 255;
            data.pixels[y * TEX_W + x] = (r << 24) | (g << 16) | (b << 8) | a;
        }
    }
    std::cout << "Max Value: " << maxVal << std::endl; //debug
    std::cout << "Min Value: " << minVal << std::endl; //debug
    return data;
}
