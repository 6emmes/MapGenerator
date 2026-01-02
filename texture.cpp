// Implementation of generateSineGradientRGBA.
// This file is separate from sdl_3.cpp to keep gradient logic isolated.

#include "texture.h"
#include "terrain.h"
#include "colorpalette.h"
#include <cmath>
#include <cstdint>
#include <vector>
#include <stack>
#include <tuple>
#include <cstdlib>
#include <time.h>
#include <iostream> //debug
#include <algorithm>
// Expose seed variable defined in terrain.cpp
extern int SEED;

struct Point {
    int x;
    int y;
};

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


std::tuple<std::vector<std::vector<float>>, std::vector<std::vector<float>>> generateHeightMap() {
    // Seed for reproducibility using SEED from config; if SEED is -1 use current time
    if (SEED == -1) {
        srand(time(NULL));
    } else {
        srand(SEED);
    }
    int randX = rand()%1200;
    int randY = rand()%1200;
    long long hash = 0;
    std::vector<std::pair<int,int>> directions = { {0,1}, {0,-1}};   //{1,0}, {-1,0},
    std::vector<std::vector<float>> map(TEX_H, std::vector<float>(TEX_W));
    std::vector<std::vector<float>> water(TEX_H, std::vector<float>(TEX_W, 1024.0));
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
    float norm;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            // Normalize to [0,1] based on min/max found
            norm = map[y][x];
            norm = (norm - minVal) / (maxVal - minVal);
            //norm = norm - 1.5 * Chebyshev(x, y, 0.7);
            norm = simpleMap(norm);
            if (norm < 0) norm=0;
            if (norm > 1) norm=1;
            map[y][x] = norm;
            hash += int(norm*100);
            if (norm<0.1) water[y][x] = 0.0;
        }
    }
    
    int changed = 0;
    float value;
    float step = 0.001;//float(1/TEX_H);
    for (int y = 0; y < TEX_H; ++y) {   //poziomo
        for (int x = 1; x < TEX_W; ++x) {
            value = water[y][x];
            if (value == 0) continue;
            if (water[y][x-1]+step<water[y][x]) water[y][x] = water[y][x-1]+step;
        }
        for (int x = TEX_W-2; x >=0; x--) {
            value = water[y][x];
            if (value == 0) continue;
            if (water[y][x+1]+step<water[y][x]) water[y][x] = water[y][x+1]+step;
        }
    }
    for (int x = 0; x < TEX_W; ++x) {   //poinowo
        for (int y = 1; y < TEX_H; ++y) {
            value = water[y][x];
            if (value == 0) continue;
            if (water[y-1][x]+step<water[y][x]) water[y][x] = water[y-1][x]+step;
        }
        for (int y = TEX_H-2; y >=0; y--) {
            value = water[y][x];
            if (value == 0) continue;
            if (water[y+1][x]+step<water[y][x]) water[y][x] = water[y+1][x]+step;
        }
    }
    std::cout <<"Heightmap hash: "<< hash <<std::endl;
    return {map,water};
}

// Convert a height map to a color texture using getTerrainPixel.
TextureData heightTexture(const std::vector<std::vector<float>>& map) {
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

TextureData shadingTexture(int w, int h, const std::vector<std::vector<float>>& heightmap,
    std::vector<std::vector<float>> landMap) {
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


std::vector<std::vector<float>> metalDensity() {
    std::vector<std::vector<float>> density(TEX_H, std::vector<float>(TEX_W));
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            // Scale coordinates to control frequency.
            float nx = static_cast<float>(x) * NOISE_SCALE;
            float ny = static_cast<float>(y) * NOISE_SCALE;
            float v = (fbm(nx, ny)- 0.25) *0.571;
            // Clamp into 0‑1.  The fbm range is roughly [-2,2] for
            // the current settings, so clamping keeps it stable.
            density[y][x] = std::clamp(v, 0.0f, 1.0f);
        }
    }
    return density;
}


bool checkCondition(float var, bool checkEqual) {
    if (checkEqual) {
        return var == 0;
    } else {
        return var != 0;
    }
}

void fillTexture(const std::vector<std::vector<float>>& waterMap, TextureData& texture, bool targetColor, int fillColor, Point start){
    std::stack<Point> seed_stack;
    Point current_pos = start;
    seed_stack.push(current_pos);
    int left, right;
    int cur_y;
    int topflag, botflag;
    while (seed_stack.size()>0){
        current_pos = seed_stack.top();
        seed_stack.pop();
        cur_y = current_pos.y;
        if (cur_y >= TEX_H || cur_y < 0) continue;
        left = current_pos.x;
        while (left > 0 && checkCondition(waterMap[cur_y][left],targetColor)) left -=1;
        right = current_pos.x;
        while (right < TEX_W && checkCondition(waterMap[cur_y][right],targetColor)) right +=1;
        //std::cout<<left<<"_"<<right<<std::endl;
        topflag = 0;
        botflag = 0;
        for (int i=left; i<right; i++){
            texture.pixels[cur_y * TEX_W + i] = fillColor;
            if (cur_y<TEX_H-1){
                if (checkCondition(waterMap[cur_y+1][i], targetColor) && texture.pixels[(cur_y+1) * TEX_W + i] != fillColor){
                    if (botflag==0) {
                        seed_stack.push({i, cur_y+1});
                        botflag = 1;
                    }
                }
                else {
                    botflag = 0;
                    //std::cout<<i<<std::endl;
                }
            }
             if (cur_y>0){
                if (checkCondition(waterMap[cur_y-1][i], targetColor) && texture.pixels[(cur_y-1) * TEX_W + i] != fillColor){
                    if (topflag==0) {
                        seed_stack.push({i, cur_y-1});
                        topflag = 1;
                    }
                }
                else {
                    topflag = 0;
                    //std::cout<<i<<std::endl;
                }
            }
        }

    }
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

void fillValue(const std::vector<std::vector<float>>& waterMap, std::vector<std::vector<float>>& data, bool targetColor, float fillColor, Point start){
    std::stack<Point> seed_stack;
    Point current_pos = start;
    seed_stack.push(current_pos);
    int left, right;
    int cur_y;
    int topflag, botflag;
    float delta;
    while (seed_stack.size()>0){
        current_pos = seed_stack.top();
        seed_stack.pop();
        cur_y = current_pos.y;
        if (cur_y >= TEX_H || cur_y < 0) continue;
        left = current_pos.x;
        while (left > 0 && checkCondition(waterMap[cur_y][left],targetColor)) left -=1;
        right = current_pos.x;
        while (right < TEX_W && checkCondition(waterMap[cur_y][right],targetColor)) right +=1;
        topflag = 0;
        botflag = 0;
        for (int i=left; i<right; i++){
            data[cur_y][i] = fillColor;
            if (cur_y<TEX_H-1){
                //delta = data[(cur_y+1)][i] - fillColor;
                if (checkCondition(waterMap[cur_y+1][i], targetColor) && data[(cur_y+1)][i] != fillColor){
                    if (botflag==0) {
                        seed_stack.push({i, cur_y+1});
                        botflag = 1;
                    }
                }
                else {
                    botflag = 0;
                }
            }
             if (cur_y>0){
                //delta = data[(cur_y-1)][ i] - fillColor;
                if (checkCondition(waterMap[cur_y-1][i], targetColor) && data[(cur_y-1)][ i] != fillColor){
                    if (topflag==0) {
                        seed_stack.push({i, cur_y-1});
                        topflag = 1;
                    }
                }
                else {
                    topflag = 0;
                }
            }
        }

    }
}


std::vector<std::vector<float>> idData(const std::vector<std::vector<float>>& waterMap) {
    std::vector<std::vector<float>> data(TEX_H, std::vector<float>(TEX_W, 0.0));
    int gray = 1;
    float fillvalue;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float v = waterMap[y][x];
            if (data[y][x] == 0.0){
                if (v==0) fillvalue = float(gray)/256;
                else fillvalue = float(255-gray)/256;
                gray++;
                if (gray > 120) throw std::domain_error("Too many terrain islands");
                fillValue(waterMap, data, v==0, fillvalue, {x,y});
            }
        }
    }
    return data;
}

uint32_t averageRGBA(uint32_t c1, uint32_t c2) {
    uint32_t r = (((c1 >> 24) & 0xFF) + ((c2 >> 24) & 0xFF)) >> 1;
    uint32_t g = (((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF)) >> 1;
    uint32_t b = (((c1 >> 8)  & 0xFF) + ((c2 >> 8)  & 0xFF)) >> 1;
    uint32_t a = 0xFF;
    return (r << 24) | (g << 16) | (b << 8) | a;
}

uint32_t weightedAverageRGBA(uint32_t c1, uint32_t c2, float dist1, float dist2) {
    // Handle degenerate case: both distances zero → return either color
    float sum = dist1 + dist2;
    if (sum == 0.0f) return c1;

    // Inverse-distance weighting
    float w1 = dist2 / sum;
    float w2 = dist1 / sum;

    uint32_t r1 = (c1 >> 24) & 0xFF;
    uint32_t g1 = (c1 >> 16) & 0xFF;
    uint32_t b1 = (c1 >> 8)  & 0xFF;

    uint32_t r2 = (c2 >> 24) & 0xFF;
    uint32_t g2 = (c2 >> 16) & 0xFF;
    uint32_t b2 = (c2 >> 8)  & 0xFF;

    uint32_t r = (uint32_t)(r1 * w1 + r2 * w2);
    uint32_t g = (uint32_t)(g1 * w1 + g2 * w2);
    uint32_t b = (uint32_t)(b1 * w1 + b2 * w2);

    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}


int getColor(float temp, float humidity, std::vector<TextureSample> cloud){
    double bestDist1 = INFINITY, bestDist2 = INFINITY;
    int bestValue1 = 0, bestValue2 = 0;
    float new_temp = temp*1.5;
    float new_humidity = humidity/30;
    for (const auto& p: cloud){
        //double dist = std::abs(p.x - localX) + std::abs(p.y - localY);
        double dist = std::max(std::abs(p.x - new_temp) , std::abs(p.y - new_humidity));
        if (dist < bestDist1) {
            bestDist2 = bestDist1;
            bestValue2 = bestValue1;
            bestDist1 = dist;
            bestValue1 = p.value;
        }
         else if (dist < bestDist2) {
            if (p.value != bestValue1){
            bestDist2 = dist;
            bestValue2 = p.value;

            }
        }

    }
    if (bestValue1 == 0xff00ffff) return 0xff00ffff;
    //else if (bestValue2 != 0xff00ffff) return averageRGBA(bestValue1,bestValue2);
    else if (bestValue2 != 0xff00ffff) return weightedAverageRGBA(bestValue1,bestValue2, bestDist1, bestDist2);
    else return bestValue1;
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
            else data.pixels[y * TEX_W + x] = getColor(tempMap[y][x], humidityMap[y][x], CLIMATEPALETTE);
        }
        //std::cout<<" "<<tempMap[y][3]*1.5<<" ";
    }
    //std::cout<<std::endl;
    return data;
}
