#include "textures_maps.h"

TextureData heightTexture(const std::vector<std::vector<float>>& map) {
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float h = map[y][x];
            uint32_t c = getTerrainPixel(h);
            data.pixels[y * TEX_W + x] = c;
        }
    }
    return data;
}

TextureData heightTextureFromHeightMap(const std::vector<std::vector<float>>& map) {
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float h = map[y][x];
            uint8_t val = static_cast<uint8_t>(std::clamp(((h - 0.1f) / 0.9f) * 255.f, 0.f, 255.f));
            uint32_t pixel = (val << 24) | (val << 16) | (val << 8) | 0xFF;
            data.pixels[y * TEX_W + x] = pixel;
        }
    }
    return data;
}

TextureData shadingTexture(int w, int h, const std::vector<std::vector<float>>& heightmap,
    std::vector<std::vector<float>> landMap) {
    TextureData shade;
    shade.width = w;
    shade.height = h;
    shade.pixels.resize(w * h);

    const float inv_two = 1.0f / 1.414213562f;
    float h_left;
    float h_right;
    float h_up;
    float h_down;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (landMap[x][y] == 0){
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

TextureData climateTexture(const std::vector<std::vector<float>>& tempMap,
                          const std::vector<std::vector<float>>& humidityMap) {
    TextureData data;
    data.width = static_cast<int>(tempMap[0].size());
    data.height = static_cast<int>(tempMap.size());
    data.pixels.resize(data.width * data.height);
    uint8_t r = 0;
            uint8_t b = 0;
    for (int y = 0; y < data.height; ++y) {
        for (int x = 0; x < data.width; ++x) {
            float tempK = tempMap[y][x];
            // Temperature red channel: map 280K (~7°C) to 0 and 320K (~47°C) to 255.
            //r = static_cast<uint8_t>(std::clamp(tempK * 255.0, 0.0, 255.0));

            // Humidity blue channel: Scale 0-30 to 0-255.
            float hum = humidityMap[y][x];
            b = static_cast<uint8_t>(std::clamp((hum) * 255.0, 0.0, 255.0));

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


TextureData waterTexture(const std::vector<std::vector<float>>& waterMap) {
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float v = waterMap[y][x];
            uint8_t val = static_cast<uint8_t>(std::clamp(v * 255.f, 0.f, 255.f));
            uint32_t pixel = (0 << 24) | (0 << 16) | (val << 8) | val;
            data.pixels[y * TEX_W + x] = pixel;
        }
    }
    return data;
}

TextureData idTexture(const std::vector<std::vector<float>>& waterMap) {
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    uint8_t gray = 1;
    int a =0;
    int fillcolor;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float v = waterMap[y][x];
            if (data.pixels[y * TEX_W + x] == 0){
                if (v==0) fillcolor = (gray << 24) | (gray << 16) | (gray << 8) | 255;
                else fillcolor = ((255-gray) << 24) | ((255-gray) << 16) | ((255-gray) << 8) | 255;
                gray++;
                fillTexture(waterMap, data, v==0, fillcolor, {x,y});
            }
        }
    }
    return data;
}

TextureData calculateColorMap(const std::vector<std::vector<float>>&tempMap, const std::vector<std::vector<float>>&humidityMap, const std::vector<std::vector<float>>&waterMap){
    TextureData data;
    data.width = TEX_W;
    data.height = TEX_H;
    data.pixels.resize(TEX_W * TEX_H);
    int WHITE = (255 << 24) | (255 << 16) | (255 << 8) | 255;
    int BLU = (255 << 8) |  255;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            if (waterMap[y][x] == 0) data.pixels[y * TEX_W + x] = BLU;
            else if (tempMap[y][x] < 0) data.pixels[y * TEX_W + x] = WHITE;
            else data.pixels[y * TEX_W + x] = getColorTexture(tempMap[y][x], humidityMap[y][x]);
        }
    }
    return data;
}