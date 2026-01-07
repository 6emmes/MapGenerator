#include "terrain.h"
#include "colorramp.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdint>
#include <iostream>

//--- Noise constants ---
// The following variables are defined with default values.  They
// may be overridden by a configuration file loaded at runtime.
int TEX_W = 1024;
int TEX_H = 1024;
float NOISE_SCALE = 0.002;
float RIVER_EVAPORATION = 0.5;
int OCTAVES = 9;
constexpr float PERSISTENCE = 0.5f;
// Initialize randSeed based on SEED; if SEED is -1, use current time.
int initRandSeed(int seed) {
    if (seed != -1) return seed;
    return static_cast<int>(std::time(nullptr));
}
int SEED = 0;


void loadConfig(const std::string &path) {
    int read_seed = -1;
    std::ifstream fin(path);
    if (!fin.is_open()) return; // silently ignore missing file
    std::string line;
    while (std::getline(fin, line)) {
        // Remove comments and trim whitespace
        auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        // Trim leading/trailing whitespace
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        if (line.empty()) continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // Trim key/value
        auto kstart = key.find_first_not_of(" \t");
        auto kend = key.find_last_not_of(" \t");
        key = key.substr(kstart, kend - kstart + 1);
        auto vstart = val.find_first_not_of(" \t");
        auto vend = val.find_last_not_of(" \t");
        val = val.substr(vstart, vend - vstart + 1);
        try {
        if (key == "octaves") OCTAVES = std::stoi(val);
        else if (key == "tex_w") TEX_W = std::stoi(val);
        else if (key == "tex_h") TEX_H = std::stoi(val);
        else if (key == "seed") read_seed = std::stoi(val);
        else if (key == "noise_scale") NOISE_SCALE = std::stof(val);
        else if (key == "river_evaporation") RIVER_EVAPORATION = std::stof(val); 
        } catch (...) {
            // ignore malformed integers
        }
    }
    
    SEED = initRandSeed(read_seed);
}


inline int coordHash(int xi, int yi) {
    uint64_t h = static_cast<uint64_t>(xi) * 0x5DEECE66Dull +
                 static_cast<uint64_t>(yi) * 0xB;
    h ^= static_cast<uint64_t>(SEED);
    h = (h ^ (h >> 33)) * 0xFF51AFD7ED558CCDull;
    h ^= h >> 33;
    return static_cast<int>(h & 0xFFFFFFFFu);
}

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

inline float grad(int hash, float x, float y) {
    int h = hash & 7; // 8 directions
    //  h ^= h >> 13;
    //  h *= 0xC2B2AE35;
    //  h ^= h >> 16;
    //  h = h & 7;
    float u = GRADIENTS[h][0];
    float v = GRADIENTS[h][1];
    return u * x + v * y; // dot product with offset
}

inline float gradFast(uint32_t h, float x, float y) {
    // 8 gradients encoded in 3 bits
    switch (h & 7) {
        case 0: return  x + y;
        case 1: return  x - y;
        case 2: return -x + y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        default: return -y;
    }
}


//--- Perlin noise ------------------------------------------------------------
float perlin2D(float x, float y) {
    //int xi = static_cast<int>(std::floor(x)) & 255;
    //int yi = static_cast<int>(std::floor(y)) & 255;
    int xi = static_cast<int>(std::floor(x));
    int yi = static_cast<int>(std::floor(y));
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float u = fade(xf);
    float v = fade(yf);
    int aa = coordHash(xi, yi);
    int ab = coordHash(xi, yi + 1);
    int ba = coordHash(xi + 1, yi);
    int bb = coordHash(xi + 1, yi + 1);
    float x1 = lerp(u, gradFast(aa, xf, yf), gradFast(ba, xf-1, yf));
    float x2 = lerp(u, gradFast(ab, xf, yf-1), gradFast(bb, xf-1, yf-1));
    return lerp(v, x1, x2);
}

float fbm(float x, float y) {
    float total = 0.0f;
    float frequency = 1;
    float amplitude = 1.0f;
    for (int i = 0; i < OCTAVES; ++i) {
        total += perlin2D(x * frequency, y * frequency) * amplitude;
        frequency *= 2.0f;
        amplitude *= PERSISTENCE;
    }
    return total;
}

float MM(float x, float y, int hetero) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    for (int i = 0; i < OCTAVES; ++i) {
        if (i < hetero) total += perlin2D(x * frequency, y * frequency) * amplitude;
        else total += std::clamp(total,0.5f,1.0f) * perlin2D(x * frequency, y * frequency) * amplitude;
        frequency *= 2.0f;
        amplitude *= PERSISTENCE;
    }

    return total;
}

float simpleMap(float val) {
    const float breakpoint = 0.7f;
    const float slope1 = 0.3f;   // can be chosen
    const float slope2 = (1.0f - slope1 * breakpoint) / (1.0f - breakpoint);

    float intercept = slope1 * breakpoint;
    float y1 = slope1 * val;
    float y2 = intercept + slope2 * (val - breakpoint);

    // Logistic blend around breakpoint
    float sharpness = 20.0f; // higher = sharper transition
    float t = 1.0f / (1.0f + exp(-sharpness * (val - breakpoint)));

    return (1.0f - t) * y1 + t * y2;
}

//--- Terrain color mapping ----------------------------------------------------
uint32_t getTerrainPixel(float height) {
    if (height==0) return 0x000099ff; // water color
    height = std::max(0.0f, std::min(1.0f, height));
    int heightIndex = static_cast<int>(height * 256.0f);
    return COLOR_RAMP[heightIndex];
}
