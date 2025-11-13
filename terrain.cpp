#include "terrain.h"
#include <cmath>
#include <algorithm>
#include <vector>

//--- Noise constants ---
constexpr int TEX_W = 1024;
constexpr int TEX_H = 1024;
constexpr float NOISE_SCALE = 0.005f;
constexpr int OCTAVES = 9;
constexpr float PERSISTENCE = 0.5f;

//--- Permutation table for Perlin noise
const int PERM[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190, 6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,
    134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,
    230,220,105,92,41,55,46,245,40,244,102,143,54, 65,25,63,
    161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,135,
    130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,
    250,124,123, 5,202,38,147,118,126,255,82,85,212,207,206,59,
    227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,
    2,44,154,163, 70,221,153,101,155,167, 43,172, 9,129,22,39,
    253,19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,
    228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,
    235,249,14, 239,107, 49,192,214, 31,181,199,106,157,184, 84,
    204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,
    67,29,24,72,243,141,128,195,78,110, 96,62, 27, 58, 15, 1,
    140, 73, 33, 27, 91, 45, 18, 56, 49, 12, 23, 58, 73, 39, 93, 13,
    61, 74, 20, 55, 31, 27, 30, 14, 71, 60, 30, 44, 35, 20, 12, 12,
    53, 39, 17, 20, 30, 14, 58, 45, 23, 20, 56, 44, 30, 14, 60, 23
};

// Gradient vectors for 2D Perlin noise
const float GRADIENTS[8][2] = {
    {-1,1}, {1,1}, {1,-1}, {-1,-1},
    {1,0}, {0,-1}, {0,1}, {-1,0}
   // {1,1}, {0,1}, {1,0}, {0,0},
   // {1,0.5}, {0,0.5}, {0.5,1}, {0.5,0}
};

//--- Helper functions -------------------------------------------------------
inline float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

inline float grad(int hash) {
    //return ((hash & 1) ? x : -x) + ((hash & 2) ?  2.0f * y : -2.0f * y);
    int h = hash % 8;
    float u = GRADIENTS[h][0];
    float v = GRADIENTS[h][1];
    return u + 2.0f * v;
    //return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

//--- Perlin noise ------------------------------------------------------------
float perlin2D(float x, float y) {
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float u = fade(xf);
    float v = fade(yf);
    int aa = PERM[PERM[xi] + yi];
    int ab = PERM[PERM[xi] + yi + 1];
    int ba = PERM[PERM[xi + 1] + yi];
    int bb = PERM[PERM[xi + 1] + yi + 1];
    float x1 = lerp(u, grad(aa), grad(ba));
    float x2 = lerp(u, grad(ab), grad(bb));
    return lerp(v, x1, x2);
}

float fbm(float x, float y) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    for (int i = 0; i < OCTAVES; ++i) {
        total += perlin2D(x * frequency, y * frequency) * amplitude;
        frequency *= 2.0f;
        amplitude *= PERSISTENCE;
    }
    return total;
}

float MM(float x, float y) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 3.0f;
    total = perlin2D(x * frequency, y * frequency) * amplitude;
    frequency *= 2.0f;
    amplitude *= PERSISTENCE;
    for (int i = 1; i < OCTAVES; ++i) {
        total += std::clamp(total,0.1f,1.0f) * perlin2D(x * frequency, y * frequency) * amplitude;
        frequency *= 2.0f;
        amplitude *= PERSISTENCE;
    }
    return total;
}

float simpleMap(float val) {
    const float breakpoint = 0.75f;
    const float slope1 = 0.2f;   // can be chosen
    const float slope2 = (1.0f - slope1 * breakpoint) / (1.0f - breakpoint);

    if (val <= breakpoint) {
        return slope1 * val;
    } else {
        float intercept = slope1 * breakpoint;
        return intercept + slope2 * (val - breakpoint);
    }
}

//--- Terrain color mapping ----------------------------------------------------
Color getTerrainPixel(float height) {
    height = std::max(0.0f, std::min(1.0f, height));
    std::vector<std::pair<float, Color>> ramp = {
        {0.0f,   {0, 0, 255}},      // Blue
        {0.099f, {0, 0, 255}},      // Blue
        {0.1f,   {0, 77, 0}},       // Dark Green
        {0.35f,  {0, 204, 0}},     // Green
        {0.5f,   {255, 255, 0}},    // Yellow
        {0.65f,  {255, 128, 0}},   // Orange
        {0.8f,   {255, 0, 0}},      // Red
        {0.9f,   {128, 0, 0}},      // Maroon
        {1.0f,   {255, 255, 255}}   // White
    };
    for (size_t i = 1; i < ramp.size(); ++i) {
        if (height <= ramp[i].first) {
            float t = (height - ramp[i-1].first) / (ramp[i].first - ramp[i-1].first);
            Color c = { static_cast<uint8_t>(ramp[i-1].second.r + (ramp[i].second.r - ramp[i-1].second.r) * t),
                        static_cast<uint8_t>(ramp[i-1].second.g + (ramp[i].second.g - ramp[i-1].second.g) * t),
                        static_cast<uint8_t>(ramp[i-1].second.b + (ramp[i].second.b - ramp[i-1].second.b) * t)};
            return c;
        }
    }
    return ramp.back().second;
}
