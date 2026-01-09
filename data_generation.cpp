#include "data_generation.h"

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
    cx = cx / (TEX_W / 2.0f);
    cy = cy / (TEX_H / 2.0f);
    distance = std::max(std::abs(cx), std::abs(cy));
    if (deadzone > distance) {
        return 0.0f;
    }   else {
        return distance - deadzone;
    }   
}


std::tuple<std::vector<std::vector<float>>, std::vector<std::vector<float>>> generateHeightMap() {
    srand(SEED);
    float bias = -0.0f;
    int randX = rand()%1200;
    int randY = rand()%1200;
    std::vector<std::vector<float>> map(TEX_H, std::vector<float>(TEX_W));
    std::vector<std::vector<float>> water(TEX_H, std::vector<float>(TEX_W, 1024.0));
    float maxVal = -1e10f;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            float nx = (x + randX) * NOISE_SCALE;
            float ny = (y + randY) * NOISE_SCALE;
            float value = MM(nx, ny, 4)+bias;
            value = value - 5 * Chebyshev(x, y, 0.8);
            if (value<0.0) {
                value = 0;
                water[y][x] = 0.0f;
            }
            else if (value > maxVal) maxVal = value;
            map[y][x] = value;
        }
    }
    float norm;
    for (int y = 0; y < TEX_H; ++y) {
        for (int x = 0; x < TEX_W; ++x) {
            if (water[y][x]==0.0f) continue;
            norm = map[y][x];
            norm = norm / maxVal;
            norm = simpleMap(norm);
            map[y][x] = norm;
        }
    }
    
    float value;
    float step = 0.001;
    for (int y = 0; y < TEX_H; ++y) {   //horizontal
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
    for (int x = 0; x < TEX_W; ++x) {   //vertical
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
    return {map,water};
}

std::vector<std::vector<float>> metalDensity(float frequency, float bias, float seed, const std::vector<std::vector<float>>& waterMap) {
    std::vector<std::vector<float>> density(TEX_H, std::vector<float>(TEX_W));
    float dx = (seed+SEED)*123;
    float dy = (seed+SEED)*456;
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
    const TreeClimate palmClimate = {16.0, 40.0};
    const TreeClimate deciduousClimate = {8.0, 20.0}; 
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
    fillValueGreater(heightMap, densityMap, 0.5f, 1.0f, Point{x,y});
    interpolateMissingValues(densityMap, 8);
    return densityMap;
}