#include "data_generation.h"
#include <iostream>
#include <algorithm>
#include <cmath>
// Calculate per‑pixel shading intensity map based on heightmap and land mask.
// Returns a 2D vector [x][y] with intensity values 0‑255.
std::vector<std::vector<float>> calculateShadingMap(int w, int h,
    const std::vector<std::vector<float>>& heightmap) {
    std::vector<std::vector<float>> intensityMap(w, std::vector<float>(h, 0.0f));
    const float inv_two = 1.0f / 1.414213562f; // unused, keep to mirror original
    float h_left, h_right, h_up, h_down;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (heightmap[x][y] == 0) {
                continue;
            }
            if (x - 1 >= 0)
                h_left = heightmap[x - 1][y];
            else
                h_left = heightmap[x][y];
            if (x + 1 < w)
                h_right = heightmap[x + 1][y];
            else
                h_right = heightmap[x][y];
            if (y - 1 >= 0)
                h_up = heightmap[x][y - 1];
            else
                h_up = heightmap[x][y];
            if (y + 1 < h)
                h_down = heightmap[x][y + 1];
            else
                h_down = heightmap[x][y];
            float gx = h_right - h_left;
            float gy = h_down - h_up;
            float nx = -gx;
            float ny = -gy;
            float nz = 0.02f;
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            nx /= len;
            ny /= len;
            nz /= len;
            // Light vector pointing diagonally down-left
            float lx = -1.0f, ly = -1.0f, lz = 1.0f;
            float llen = std::sqrt(lx * lx + ly * ly + lz * lz);
            lx /= llen;
            ly /= llen;
            lz /= llen;
            float dot = nx * lx + ny * ly + nz * lz;
            dot = std::clamp(dot, -1.0f, 1.0f);
            //float intensity = (dot + 1.0f) * 127.5f; // 0‑255
            float intensity = (dot + 1.0f) * 0.5f; // 0‑1
            intensityMap[x][y] = intensity;
        }
    }
    return intensityMap;
}

inline float Chebyshev(float x, float y, float deadzone=0.0f, int size_x=TEX_W, int size_y=TEX_H) {

    float cx = x - size_x / 2.0f;
    float cy = y - size_y / 2.0f;
    float distance = 0.0f;
    float ratio = static_cast<float>(size_y) / static_cast<float>(size_x);
    if (ratio > 1.0f) {
        cx *= ratio;
    } else {
        cy /= ratio;
    }
    cx = cx / (size_x / 2.0f);
    cy = cy / (size_y / 2.0f);
    distance = std::max(std::abs(cx), std::abs(cy));
    if (deadzone > distance) {
        return 0.0f;
    }   else {
        return distance - deadzone;
    }   
}

void maskCutter(int x, int y, std::vector<std::vector<float>>& map, std::vector<std::vector<float>>& mask, int MASK_SIZE) {
    int cur_x, cur_y;
    float value;
    for (int mask_y = 0; mask_y < MASK_SIZE; ++mask_y) {
        for (int mask_x = 0; mask_x < MASK_SIZE; ++mask_x) {
            cur_x = x + mask_x - MASK_SIZE / 2;
            cur_y = y + mask_y - MASK_SIZE / 2;
            if (cur_x >= 0 && cur_x < TEX_W && cur_y >= 0 && cur_y < TEX_H) {
                if (y*x%2==0) value = mask[mask_y][mask_x];
                else value = mask[mask_x][mask_y];
                value = map[cur_y][cur_x] - value*0.2;
                if (value < 0.0f) value = 0.0f;
                map[cur_y][cur_x] = value;
            }
        }
    }
}

std::vector<std::vector<float>> generateHeightMap() {
    srand(SEED);
    float bias = -0.0f;
    int randX = rand()%1200;
    int randY = rand()%1200;
    int MASK_SIZE = SUB_SCALE*6;
    std::vector<std::vector<float>> map(TEX_H, std::vector<float>(TEX_W));
    std::vector<std::vector<float>> mask_1(MASK_SIZE, std::vector<float>(MASK_SIZE));
    std::vector<std::vector<float>> mask_2(MASK_SIZE, std::vector<float>(MASK_SIZE));
    float maxVal = -1e10f;
    float value;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float nx = (x + randX) * NOISE_SCALE;
            float ny = (y + randY) * NOISE_SCALE;
            value = MM(nx, ny, 4)+bias;
            if (value<0.0) {
                value = 0;
            }
            else if (value > maxVal) maxVal = value;
            map[y][x] = value;
        }
    }
    
    float norm;
    float mapScale = 1/simpleMapScale();
    std::cout<<"mapScale: "<<mapScale<<std::endl;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            if (map[y][x]==0.0f) continue;
            norm = map[y][x];
            norm = norm / maxVal;
            norm = simpleMap(norm, mapScale);
            if (norm < 0.004) norm = 0.004;
            map[y][x] = norm;
        }
    }
    randX = (randX*31)%1200;
    randY = (randY*17)%1200;
    maxVal = -10.0;
    float minVal = 10.0;
    for (int y = 0; y < MASK_SIZE; ++y) {
        for (int x = 0; x < MASK_SIZE; ++x) {
            float nx = (x + randX) * NOISE_SCALE;
            float ny = (y + randY) * NOISE_SCALE;
            value = fbm(nx, ny)+0.5;
            float cheby = 1 - Chebyshev(x, y, 0.0, MASK_SIZE, MASK_SIZE)+0.1;
            value =  cheby * value;
            if (value > maxVal) maxVal = value;
            if (value < minVal) minVal = value;
            mask_1[y][x] = value;
        }
    }
    std::cout<<"min: "<<minVal<<std::endl;
    std::cout<<"max: "<<maxVal<<std::endl;
    for (int y = 0; y < MASK_SIZE; ++y) {
        for (int x = 0; x < MASK_SIZE; ++x) {
            value = (mask_1[y][x]-minVal) / (maxVal - minVal)-0.2;
            if (value < 0.0f) value = 0.0f;
            mask_1[y][x] = value;
        }
    }

    randX = (randX*31)%1200;
    randY = (randY*17)%1200;
    maxVal = -10.0;
    minVal = 10.0;
    for (int y = 0; y < MASK_SIZE; ++y) {
        for (int x = 0; x < MASK_SIZE; ++x) {
            float nx = (x + randX) * NOISE_SCALE;
            float ny = (y + randY) * NOISE_SCALE;
            value = fbm(nx, ny)+0.5;
            float cheby = 1 - Chebyshev(x, y, 0.0, MASK_SIZE, MASK_SIZE)+0.1;
            value =  cheby * value;
            if (value > maxVal) maxVal = value;
            if (value < minVal) minVal = value;
            mask_2[y][x] = value;
        }
    }
    std::cout<<"min: "<<minVal<<std::endl;
    std::cout<<"max: "<<maxVal<<std::endl;
    for (int y = 0; y < MASK_SIZE; ++y) {
        for (int x = 0; x < MASK_SIZE; ++x) {
            value = (mask_2[y][x]-minVal) / (maxVal - minVal)-0.2;
            if (value < 0.0f) value = 0.0f;
            mask_2[y][x] = value;
        }
    }

    std::vector<int> toSample = {0, SUB_SCALE, TEX_H-1, TEX_H-1-SUB_SCALE};
    int cur_y, cur_x;

    for (int y : toSample) {
        for (int x = 0; x < TEX_W; ++x) {
            if (map[y][x] > 0.0f) {
                if (x+y%2==0) maskCutter(x, y, map, mask_1, MASK_SIZE);
                else maskCutter(x, y, map, mask_2, MASK_SIZE);
            }
        }
    }

    for (int y = 0; y < TEX_H; ++y) {
        for (int x : toSample) {
            if (map[y][x] > 0.0f) {
                if (x+y%2==0) maskCutter(x, y, map, mask_1, MASK_SIZE);
                else maskCutter(x, y, map, mask_2, MASK_SIZE);
            }
        }
    }
    return map;
}

std::vector<std::vector<float>> generateWaterMap(const std::vector<std::vector<float>> &heightMap){
    std::vector<std::vector<float>> water(TEX_H, std::vector<float>(TEX_W, 1024.0));
    float value;
    float step = 0.004;
    for (int y = 0; y < TEX_H; ++y) {   //horizontal
        for (int x = 1; x < TEX_W; ++x) {
            value = heightMap[y][x];
            if (value == 0) {
                water[y][x] = 0;
                continue;
            }
            if (water[y][x-1]+step<water[y][x]) water[y][x] = water[y][x-1]+step;
            //if (water[y][x] > 1.0) water[y][x] = 1.0;
        }
        for (int x = TEX_W-2; x >=0; x--) {
            value = heightMap[y][x];
            if (value == 0) {
                water[y][x] = 0;
                continue;
            }
            if (water[y][x+1]+step<water[y][x]) water[y][x] = water[y][x+1]+step;
            //if (water[y][x] > 1.0) water[y][x] = 1.0;
        }
    }
    for (int x = 0; x < TEX_W; ++x) {   //vertical
        for (int y = 1; y < TEX_H; ++y) {
            value = heightMap[y][x];
            if (value == 0) {
                water[y][x] = 0;
                continue;
            }
            if (water[y-1][x]+step<water[y][x]) water[y][x] = water[y-1][x]+step;
            //if (water[y][x] > 1.0) water[y][x] = 1.0;
        }
        for (int y = TEX_H-2; y >=0; y--) {
            value = heightMap[y][x];
            if (value == 0) {
                water[y][x] = 0;
                continue;
            }
            if (water[y+1][x]+step<water[y][x]) water[y][x] = water[y+1][x]+step;
            //if (water[y][x] > 1.0) water[y][x] = 1.0;
        }
    }
    return water;
}

std::vector<std::vector<float>> metalDensity(float frequency, float bias, float seed, const std::vector<std::vector<float>>& waterMap) {
    std::vector<std::vector<float>> density(TEX_H, std::vector<float>(TEX_W));
    float dx = seed*13+SEED%4096;
    float dy = seed*37+SEED%4096;
    float maxv = -1e10f;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            if (waterMap[y][x] == 0){
                density[y][x] = 0.0f;
                continue;
            }
            float nx = static_cast<float>(x+dx) * frequency;
            float ny = static_cast<float>(y+dy) * frequency;
            float v = (perlin2D(nx, ny) - bias);
            if (v < 0) v = 0;
            if (v > maxv) maxv = v;
            density[y][x] = v;
        }
    }
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            density[y][x] /= maxv;
        }
    }
    return density;
}

//Tree mass in range 0-30 kt/km2
struct TreeClimate {
    float minTemp;
    float maxTemp;
};

inline float temperatureFactor(float tempK, const TreeClimate& climate) {
    if (tempK < climate.minTemp || tempK > climate.maxTemp) {
        return 0.0f;
    }
    float midTemp = (climate.minTemp + climate.maxTemp) / 2.0f;
    float range = (climate.maxTemp - climate.minTemp) / 2.0f;
    return 1.0f - std::abs(tempK - midTemp) / range;
}

std::tuple<std::vector<std::vector<float>>, std::vector<std::vector<float>>, std::vector<std::vector<float>>> calculateTreeDensity(
    float frequency, float seed, const std::vector<std::vector<float>>& waterMap, const std::vector<std::vector<float>>& tempMap,
    const std::vector<std::vector<float>>& fertilityMap) {
    std::vector<std::vector<float>> densityPalm(TEX_H, std::vector<float>(TEX_W));
    std::vector<std::vector<float>> densityDeciduous(TEX_H, std::vector<float>(TEX_W));
    std::vector<std::vector<float>> densityConiferous(TEX_H, std::vector<float>(TEX_W));

    float dx = (seed+SEED)*234;
    float dy = (seed+SEED)*567;
    const TreeClimate palmClimate = {14.0, 40.0};
    const TreeClimate deciduousClimate = {8.0, 18.0}; 
    const TreeClimate coniferousClimate = {0.0, 12.0};

    float PalmTempFactor, DeciduousTempFactor, ConiferousTempFactor;
    float TotalFactor;

    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            if (waterMap[y][x] == 0){
                densityPalm[y][x] = 0.0f;
                densityDeciduous[y][x] = 0.0f;
                densityConiferous[y][x] = 0.0f;
                continue;
            }
            float capacity = 20 * fertilityMap[y][x];
            float nx = static_cast<float>(x+dx) * frequency;
            float ny = static_cast<float>(y+dy) * frequency;
            float noise = (perlin2D(nx, ny)/2)+0.7f;
            if (noise < 0) noise = 0;
            float tempC = temp2celsius(tempMap[y][x]);

            PalmTempFactor = temperatureFactor(tempC, palmClimate);
            DeciduousTempFactor = temperatureFactor(tempC, deciduousClimate);
            ConiferousTempFactor = temperatureFactor(tempC, coniferousClimate);
            TotalFactor = PalmTempFactor + DeciduousTempFactor + ConiferousTempFactor;

            densityPalm[y][x] = noise * capacity * (PalmTempFactor/TotalFactor) / 50.0f;
            densityDeciduous[y][x] = noise * capacity * (DeciduousTempFactor/TotalFactor) / 50.0f;
            densityConiferous[y][x] = noise * capacity * (ConiferousTempFactor/TotalFactor) / 50.0f;
        }
    }
    return {densityPalm, densityDeciduous, densityConiferous};
}

template <typename T>
void fillValue(const std::vector<std::vector<float>>& waterMap, std::vector<std::vector<T>>& data, bool targetColor, T fillColor, Point start){
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

template <typename T>
void fillValueGreater(const std::vector<std::vector<float>>& dataMap, std::vector<std::vector<T>>& data, float targetColor, T fillColor, Point start){
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
        while (left > 0 && dataMap[cur_y][left] > targetColor) left -=1;
        right = current_pos.x;
        while (right < TEX_W && dataMap[cur_y][right] > targetColor) right +=1;
        topflag = 0;
        botflag = 0;
        for (int i=left; i<right; i++){
            data[cur_y][i] = fillColor;
            if (cur_y<TEX_H-1){
                if (dataMap[cur_y+1][i] > targetColor && data[(cur_y+1)][i] != fillColor){
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
                if (dataMap[cur_y-1][i] > targetColor && data[(cur_y-1)][ i] != fillColor){
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

std::vector<std::vector<float>> marbleDensity(std::vector<std::vector<float>> &heightMap, float seed){
    std::vector<std::vector<float>> densityMap(TEX_W, std::vector<float>(TEX_H, 0.0f));
    //if (MAXALTITUDE<2000) return densityMap;  //TODO -> marble only in high altitude
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> distX(0, static_cast<int>(TEX_W - 1));
    std::uniform_int_distribution<int> distY(0, static_cast<int>(TEX_H - 1));
    std::vector<std::pair<int,int>> directions = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    std::pair<int,int> bestDir;
    int x=0;
    int y=0;
    int end=0;
    int tryCount=20;
    for (int i =0; i<tryCount; i++){
        x = distX(rng);
        y = distY(rng);
        float heightValue = heightMap[y][x];
        if (heightValue == 0.0f) continue;
        for (int path=0;path<TEX_W;path++){
            for (const auto& d : directions) {
                int newX = x + d.first;
                int newY = y + d.second;
                if (newX >= 0 && newX < TEX_W && newY >= 0 && newY < TEX_H) {
                    float currentHeight = heightMap[newY][newX];
                    if (currentHeight>heightValue){
                        heightValue = currentHeight;
                        bestDir = d;
                    }
                }
            }
            x += bestDir.first;
            y += bestDir.second;
            if (x < 0 || x >= TEX_W || y < 0 || y >= TEX_H) break;
            if (heightMap[y][x] > 0.75) {
                i = tryCount;
                break;
            }
        }
    }
    fillValueGreater(heightMap, densityMap, 0.6f, 1.0f, Point{x,y});
    interpolateMissingValues(densityMap, 8);
    return densityMap;
}
