#include "terrain_noise.h"

inline int coordHash(int xi, int yi) {
    uint64_t h = static_cast<uint64_t>(xi) * 0x5DEECE66Dull +
                 static_cast<uint64_t>(yi) * 0xB;
    h ^= static_cast<uint64_t>(SEED);
    h = (h ^ (h >> 33)) * 0xFF51AFD7ED558CCDull;
    h ^= h >> 33;
    return static_cast<int>(h & 0xFFFFFFFFu);
}

const float GRADIENTS[8][2] = {
    {-1,1}, {1,1}, {1,-1}, {-1,-1},
    {1,0}, {0,-1}, {0,1}, {-1,0}
};

inline float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline float lerp(float t, float a, float b) {
    return a + t * (b - a);
}

inline float grad(int hash, float x, float y) {
    int h = hash & 7;
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

float perlin2D(float x, float y) {
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
        amplitude *= 0.5f;
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
        amplitude *= 0.5f;
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