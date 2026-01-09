#include <cmath>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <stack>
#include <iostream>
#include <random>
#include <chrono>
#include <utility>
#include <tuple>
#include <algorithm>
#include "helper.h"

// Latitude bounds read from configuration
static double LAT_BOTTOM = 0.0;
static double LAT_TOP = 30.0;
// Default maximum altitude for a height value of 1.0 in the heightmap.
// Can be overridden by a configuration entry `maxaltitude`.
static double MAXALTITUDE = 4000.0; // meters

// Sub-scale factor for temperature map resolution.
// Can be overridden by a configuration entry `sub_scale`.
static int SUB_SCALE = 4;

static int RIVER_COUNT = 4000;
static float WINDCLAMP = 0.0;
static double WIND_SCALE_DOWNWARD = 1.0;

extern float RIVER_EVAPORATION;

static void loadclimateconfig(const std::string &path) {
    std::ifstream fin(path);
    if (!fin.is_open()) return;
    std::string line;
    while (std::getline(fin, line)) {
        // Ignore comments and empty lines
        auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
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
        key.erase(key.find_last_not_of(" \t") + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        try {
            if (key == "lat_bottom") LAT_BOTTOM = std::stod(val);
            else if (key == "lat_top") LAT_TOP = std::stod(val);
            else if (key == "maxaltitude") MAXALTITUDE = std::stod(val);
            else if (key == "sub_scale") SUB_SCALE = std::stoi(val);
            else if (key == "river_count") RIVER_COUNT = std::stoi(val);
            else if (key == "wind_clamp") WINDCLAMP = float(std::stoi(val))/1000;
            else if (key == "wind_scale_downward") WIND_SCALE_DOWNWARD = std::stoi(val);
        } catch (...) {}
    }
}

void interpolateMissingValues(std::vector<std::vector<float>>& grid,
                              int subScale) {
    size_t height = grid.size();
    if (height == 0) return;
    size_t width = grid[0].size();

    // Clamp subScale to avoid division by zero
    if (subScale <= 0) subScale = 1;

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            // Skip known points: coordinates that are multiples
            if ((x % subScale == 0) && (y % subScale == 0)) {
                continue;
            }
            size_t x0 = (x / subScale) * subScale;
            size_t x1 = std::min(x0 + static_cast<size_t>(subScale), width - 1);
            size_t y0 = (y / subScale) * subScale;
            size_t y1 = std::min(y0 + static_cast<size_t>(subScale), height - 1);

            float v00 = grid[y0][x0];
            float v10 = grid[y0][x1];
            float v01 = grid[y1][x0];
            float v11 = grid[y1][x1];

            float tx = (x1 == x0) ? 0.0 : static_cast<float>(x - x0) / (x1 - x0);
            float ty = (y1 == y0) ? 0.0 : static_cast<float>(y - y0) / (y1 - y0);

            // Bilinear interpolation
            float interpolated = (1 - tx) * (1 - ty) * v00
                                 + tx * (1 - ty) * v10
                                 + (1 - tx) * ty * v01
                                 + tx * ty * v11;
            grid[y][x] = interpolated;
        }
    }
}

bool outOfBounds(int x, int y, int width, int height, int margin=0) {
    return (x < margin || x >= width - margin || y < margin || y >= height - margin);
}

std::pair<int, float> createRiver(int width, int height, const std::pair<int,int> &originPoint, std::vector<std::vector<float>> &riverMap,
    std::vector<std::vector<float>> &heightmap, std::vector<std::vector<float>> &waterMap) {
    std::pair<int,int> nextstep;
    std::pair<int,int> currentPoint = originPoint;
    std::vector<std::pair<int,int>> directions = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    float curheight = 0.0;
    //=========================================================move closer to the peak
    for(int loo=0;loo<40;loo++){
        if (outOfBounds(currentPoint.first, currentPoint.second, width, height, 1)) break;
        curheight = heightmap[currentPoint.second][currentPoint.first];
        float nextheight = 0;
        for(int i=-1;i<=1;i++){
            for(int j=-1;j<=1;j++){
                if(i==0 && j==0) continue;
                if(i==j || i==-j) continue;
                if (outOfBounds(currentPoint.first + i, currentPoint.second + j, width, height, 1)) continue;
                float nh = heightmap[currentPoint.second + j][currentPoint.first + i];
                if (nh > nextheight) {
                    nextheight = nh;
                    nextstep = std::make_pair(i,j);
                }
            }
        }
        if (nextheight <= curheight) break; // reached local
        currentPoint.first += nextstep.first;
        currentPoint.second += nextstep.second;
    }
    currentPoint.first = (originPoint.first+currentPoint.first*2)/3;
    currentPoint.second = (originPoint.second+currentPoint.second*2)/3;
    for (int i=-10;i<=10;i++){
        for (int j=-10;j<=10;j++){
            if (outOfBounds(currentPoint.first + i, currentPoint.second + j, width, height)) continue;
            if (riverMap[currentPoint.second + j][currentPoint.first + i] > 0.2 && 
                (currentPoint.second + j!=originPoint.second || currentPoint.first + i!=originPoint.first)) {
                //std::cout << "River killed" << std::endl;
                return {0,0};
            }
        }
    }
    std::queue<std::pair<int,int>> tmpPathFrontier;
    std::vector<std::vector<float>> tmpMap(height, std::vector<float>(width, 0.0f));
    std::pair<int,int> minHeightPoint = currentPoint;
    std::pair<int,int> loopStart = originPoint;
    std::pair<int,int> riverEnd;
    float value = 0.0f;
    float delta = 0.001f;
    tmpMap[currentPoint.second][currentPoint.first] = value;
    tmpPathFrontier.emplace(currentPoint);
    curheight = heightmap[currentPoint.second][currentPoint.first]+0.001;
    int riverlen = 0;
    //=====================================================================Forward search
    for (int i = 0;i<5000;i++){
        if (i == 4999){
            if (std::max(abs(minHeightPoint.first - loopStart.first), abs(minHeightPoint.second - loopStart.second)) > 20) {
                i = 0;
                loopStart = minHeightPoint;
            }
            else return {0,0};
        }
        if (tmpPathFrontier.size() == 0) {
            return {0,0};
        }
        currentPoint = tmpPathFrontier.front();
        tmpPathFrontier.pop();
        if (heightmap[currentPoint.second][currentPoint.first] < curheight) {
            curheight = heightmap[currentPoint.second][currentPoint.first];
            curheight += curheight/200;
            minHeightPoint = currentPoint;
        }
        if (waterMap[currentPoint.second][currentPoint.first] == 0) {
            break;
        }
        if (outOfBounds(currentPoint.second, currentPoint.first, width, height, 1)) continue;
        value = tmpMap[currentPoint.second][currentPoint.first];
        for (const auto& d : directions) {
            if (tmpMap[currentPoint.second + d.second][currentPoint.first + d.first] > 0)   continue;
            if (heightmap[currentPoint.second + d.second][currentPoint.first + d.first] < curheight){
                tmpPathFrontier.emplace(currentPoint.first + d.first, currentPoint.second + d.second);
                tmpMap[currentPoint.second + d.second][currentPoint.first + d.first] = value+curheight-heightmap[currentPoint.second + d.second][currentPoint.first + d.first];
                riverlen++;
            }
        }
    }

    std::vector<std::pair<int,int>> neighbors;
    uint16_t index = 0;
    std::stack<std::pair<int,int>> tmpPath;
    //=====================================================================Reverse trace
    for (;;){
        tmpPath.emplace(currentPoint);
        if (tmpMap[currentPoint.second][currentPoint.first] <= delta) break;
        //if (riverMap[currentPoint.second][currentPoint.first] > 0) break;
        float curscore = tmpMap[currentPoint.second][currentPoint.first];
        neighbors = {};
        for (const auto& d : directions) {
            if (tmpMap[currentPoint.second + d.second][currentPoint.first + d.first] <= curscore && tmpMap[currentPoint.second + d.second][currentPoint.first + d.first]>0) {
                neighbors.emplace_back(currentPoint.first + d.first, currentPoint.second + d.second);
            }
        }
        if (neighbors.size() == 0) break;
        index = index * 37 + 13;
        currentPoint = neighbors[index%neighbors.size()];
    }
    value = 0.02;
    int tmpPathSize = tmpPath.size();
    int riverFound = 0;
    float riverHash = 0;
    //=====================================================================Saving the path to the map
    for (int j=0;j<tmpPathSize;j++) {
        currentPoint = tmpPath.top();
        tmpPath.pop();
        if (riverMap[currentPoint.second][currentPoint.first] > 0) {
            riverFound = 1;
            break;
        }
        else riverMap[currentPoint.second][currentPoint.first] = value;
        riverHash += value;
        value += delta;
    }
    return {1, riverHash};
}

std::vector<std::vector<float>> generateRiverPoints_main(int width, int height,
    std::vector<std::vector<float>> &heightmap,
    std::vector<std::vector<float>> &landMap) {
    std::vector<std::vector<float>> riverMap(height, std::vector<float>(width, 0.0f));
    std::mt19937 rng(3);
    std::uniform_int_distribution<int> distX(0, static_cast<int>(width - 1));
    std::uniform_int_distribution<int> distY(0, static_cast<int>(height - 1));

    std::vector<std::pair<int,int>> points;
    
    loadclimateconfig("mapgen.conf");
    points.reserve(static_cast<size_t>(RIVER_COUNT));
    for (int i = 0; i < RIVER_COUNT; ++i) {
        int x = distX(rng);
        int y = distY(rng);
        // Filter points whose height > 0.5
        if (heightmap[y][x] > 0.4f) {
            points.emplace_back(x, y);
        }
    }
    int riversmade = 0;
    std::pair<int, float> tmp;
    int riverHash = 0;
    for (const auto &p : points) {
        tmp = createRiver(width, height, p, riverMap, heightmap, landMap);
        riversmade += tmp.first;
        riverHash += int(tmp.second*100);
        //break;
    }
    std::cout << "Generated " << riversmade <<" rivers out of "<< points.size() << " starting river points in " << RIVER_COUNT << " attempts.\n";
    std::cout <<"River hash: "<< riverHash <<std::endl;
    return riverMap;
}

// ------------------------------------------------------------
// Helper: calculate saturated absolute humidity from temperature
// ------------------------------------------------------------
// Computes absolute humidity (kg/m^3) based on the Tetens
// formula for saturation vapor pressure, then applies the ideal
// gas law.
double saturatedAbsoluteHumidity(double temperatureK) {
    // Tetens constants
    const double e0  = 0.61078;   // kPa at 0 °C
    const double T1  = 273.15;    // K reference point (0 °C)
    const double T2  = 35.86;     // Tetens constant
    const double b   = 7.5;       // Tetens b coefficient (typical value)
    const double R   = 0.000461;  // Specific gas constant (kPa·kmol/(K·kg))
    temperatureK = temperatureK*60+260;
    double e = e0 * std::exp(b * (temperatureK - T1) / (temperatureK - T2));
    return e / (R * temperatureK); // kg/m^3
}




// Compute incoming solar energy from latitude in degrees
static inline double incomingSolarEnergy(double latitudeDeg) {
    double POLARPOWER = 150;   //  W/m2 real value is 180
    double EQUATORPOWER = 350; //  W/m2 real value is 180
    const double deg2rad = 3.14159265358979323846 / 180.0;
    return POLARPOWER + (EQUATORPOWER - POLARPOWER) * std::cos(latitudeDeg * deg2rad);
}

// Temperature in 260 - 320 kelvin range
std::vector<std::vector<float>> calculateTemperatureMap(int width, int height,
                                                            const std::vector<std::vector<float>>& heightmap) {
    // Constants
    constexpr float STEFAN = 5.67e-8;
    constexpr float EPSILON = 0.93;
    constexpr float GREENHOUSEFACTOR = 0.4;
    static bool loaded = false;
    if (!loaded) {
        loadclimateconfig("mapgen.conf");
        loaded = true;
    }
    float maxtemp = 0; //debug
    std::vector<std::vector<float>> tempMap(height, std::vector<float>(width, 0.0));
    float latDelta = (height > 1) ? (LAT_TOP - LAT_BOTTOM) / float(height - 1) : 0.0;
    for (int y = 0; y < height; ++y) {
        float lat = LAT_BOTTOM + latDelta * float(y);
        float power = incomingSolarEnergy(lat); // W/m^2
        float baseTemperatureK = power / (EPSILON / (1+GREENHOUSEFACTOR));
        baseTemperatureK = baseTemperatureK / STEFAN;
        baseTemperatureK = std::pow(baseTemperatureK,0.25);
        for (int x = 0; x < width; ++x) {
            float altitudeMeters = 0.0;
            if (y < static_cast<int>(heightmap.size()) &&
                x < static_cast<int>(heightmap[0].size())) {
                altitudeMeters = heightmap[y][x] * MAXALTITUDE; // meters
            }
            float tempShiftK = 6.5 * (altitudeMeters / 1000.0); // K
            float tempK = (280 + baseTemperatureK - tempShiftK)/2;

            tempMap[y][x] = (tempK-260)/60;
            if (tempK > maxtemp) maxtemp = tempK; // debug
        }
    }
    std::cout << "Debug: Max temperature in map: " << maxtemp << " K\n";
    return tempMap;
}


    //Wind direction is {West, North, Up} 
std::tuple<float, float, float> getWindDirection(float latitudeDeg) {
    if (latitudeDeg < 0){
        return  {0.0,0.0,0.0};
    }
    float ROOT2 = 1;//0.70710678118;
    float SCALE = 2;
    float CLAMP = 0.98;
    //return std::make_tuple(0.97, 0.97, 0.0);
    latitudeDeg = std::max(-90.0f, std::min(90.0f, latitudeDeg));
    std::vector<std::pair<float, std::tuple<float, float, float>>> ramp = {
        {-90.0f,   {ROOT2, ROOT2, 0}},      // Polar Easterlies
        {-60.0f, {0, 0, 1}},                // Subpolar Low
        {-45.0f,   {-ROOT2, -ROOT2, 0}},    // Westerlies
        {-30.0f,  {0, 0, -1}},              // Subtropical High
        {-15.0f,   {ROOT2, ROOT2, 0}},      // SE Trade
        {0.0f,   {1, 0, 1}},                // ITCZ
        {15.0f,   {ROOT2, -ROOT2, 0}},      // NE Trade
        {30.0f,  {0, 0, -1}},               // Subtropical High
        {45.0f,   {-ROOT2, ROOT2, 0}},      // Westerlies
        {60.0f, {0, 0, 1}},                 // Subpolar Low
        {90.0f,   {ROOT2, -ROOT2, 0}},      // Polar Easterlies
    };
    for (size_t i = 1; i < ramp.size(); ++i) {
        if (latitudeDeg <= ramp[i].first) {
            float t = (latitudeDeg - ramp[i-1].first) / (ramp[i].first - ramp[i-1].first);
            float a = SCALE *(std::get<0>(ramp[i-1].second) + (std::get<0>(ramp[i].second) - std::get<0>(ramp[i-1].second)) * t);
            float b = SCALE *(std::get<1>(ramp[i-1].second) + (std::get<1>(ramp[i].second) - std::get<1>(ramp[i-1].second)) * t);
            float c = SCALE *(std::get<2>(ramp[i-1].second) + (std::get<2>(ramp[i].second) - std::get<2>(ramp[i-1].second)) * t);

            a = std::clamp(a, -CLAMP, CLAMP);
            b = std::clamp(b, -CLAMP, CLAMP);
            c = std::clamp(c, -CLAMP, CLAMP);
            std::tuple out = {a, b, c};
            return out;
        }
    }
    return ramp.back().second;
}

float calculateWind(float WindComponent, float FromHumidity, float AltitudeDelta, float FohenScale, float WindScale){
    float FohenFactor;
	if (AltitudeDelta < 0) AltitudeDelta = 0;
	else if (std::abs(AltitudeDelta) > 0.5) AltitudeDelta = 0;
	FohenFactor = 1 / (1 + AltitudeDelta * AltitudeDelta * FohenScale);
	FohenFactor = std::clamp(FohenFactor, 0.0f, 2.0f);
    return  WindComponent * FromHumidity * FohenFactor * WindScale;
}

float calculateWindSimple(float WindComponent, float FromHumidity, float WindScale, float localClamp){
    //WindComponent = std::clamp(WindComponent * WindScale, 0.0f, WINDCLAMP);
    return  FromHumidity * std::clamp(WindComponent * WindScale, 0.0f, localClamp);
}

static constexpr float OCEAN_REL_HUM = 0.85f; // ocean saturation fraction static
constexpr float WIND_SCALE = 0.85f; // how "wet" incoming ocean air is
static constexpr float DIFFUSION_FACTOR = 0.5f; // 0..1, probability / strength of diffusion 

inline double altitudePenalty(double altitudeDelta, double scale) {
    if (altitudeDelta <= 0.0) return 1.0; // no penalty when going downhill or flat
    return 1.0 / (1.0 + altitudeDelta * altitudeDelta * scale);
}

std::vector<std::vector<float>> calculateHumidityMap2(
        int width,
        int height,
        const std::vector<std::vector<float>>& tempMap,
        const std::vector<std::vector<float>>& waterMap,   // 0 = water
        const std::vector<std::vector<float>>& heightMap,
        const std::vector<std::vector<float>>& riverMap)
{
    const double someScale  = 128.0;
    const double subScaleSq = double(SUB_SCALE) * double(SUB_SCALE);

    int mapW = width;
    int mapH = height;

    // Layout: humidity[y][x]
    std::vector<std::vector<float>> humidity(mapH, std::vector<float>(mapW, 0.0f));

    // Wind per latitude row
    std::vector<std::tuple<double,double,double>> windDir(mapH);

    auto latitudeAt = [&](int y) {
        double t = double(y) / double(mapH - 1);
        return LAT_TOP + t * (LAT_BOTTOM - LAT_TOP);
    };

    // -----------------------------------------------------------------------
    // FIRST PASS: X wind + Y south wind
    // -----------------------------------------------------------------------
    for (int y = 0; y < mapH; y += SUB_SCALE)
    {
        double lat = latitudeAt(y);
        windDir[y] = getWindDirection(lat);
        auto [windX, windY, windZ] = windDir[y];

        for (int x = 0; x < mapW; x += SUB_SCALE)
        {
            double cellTemp = 0.0;
            double cellAlt  = 0.0;
            float    waterCount = 0;

            // Aggregate block data over [y,y+SUB_SCALE) x [x,x+SUB_SCALE)
            for (int dy = 0; dy < SUB_SCALE; dy++)
            for (int dx = 0; dx < SUB_SCALE; dx++)
            {
                int yy = y + dy;
                int xx = x + dx;
                if (yy >= mapH || xx >= mapW) continue;

                float alt = heightMap[yy][xx];
                float t   = tempMap[yy][xx];
                float w   = waterMap[yy][xx];

                if (w == 0.0f)
                    waterCount++;
                else{
                    cellAlt += alt;
                    waterCount += riverMap[yy][xx]*RIVER_EVAPORATION;
                }

                cellTemp += t;
            }

            cellTemp /= subScaleSq;
            cellAlt  /= subScaleSq;

            double waterRatio  = double(waterCount) / subScaleSq;
            double airCapacity = saturatedAbsoluteHumidity(cellTemp);

            // Base evaporation
            if (waterCount == subScaleSq)
                humidity[y][x] += float(airCapacity * 0.85);
            else
                humidity[y][x] += float(airCapacity * waterRatio);

            // -------------------------
            // X wind transport
            // -------------------------
            if (windX != 0.0)
            {
                int fromX = x + SUB_SCALE * (windX > 0.0 ? -1 : +1);

                if (fromX >= 0 && fromX < mapW)
                {
                    double fromAlt = 0.0;

                    for (int dy = 0; dy < SUB_SCALE; dy++)
                    for (int dx = 0; dx < SUB_SCALE; dx++)
                    {
                        int yy = y + dy;
                        int xx = fromX + dx;
                        if (yy >= mapH || xx >= mapW) continue;

                        fromAlt = std::max(fromAlt, double(heightMap[yy][xx]));
                    }

                    double altDelta = cellAlt - fromAlt;
                    if (altDelta < 0.0) altDelta = 0.0;  // only uphill blocks

                    double dry    = altitudePenalty(altDelta, someScale);
                    double fromHum = humidity[y][fromX];
                    double inc    = fromHum * std::abs(windX) * (1.0 + windZ) * dry;

                    // accumulate, not overwrite
                    humidity[y][x] += float(inc);
                }
            }

            // -------------------------
            // Y wind (south, windY < 0)
            // -------------------------
            if (windY < 0.0)
            {
                int fromY = y - SUB_SCALE;

                if (fromY >= 0 && fromY < mapH)
                {
                    double fromAlt = 0.0;

                    for (int dy = 0; dy < SUB_SCALE; dy++)
                    for (int dx = 0; dx < SUB_SCALE; dx++)
                    {
                        int yy = fromY + dy;
                        int xx = x + dx;
                        if (yy >= mapH || xx >= mapW) continue;

                        fromAlt = std::max(fromAlt, double(heightMap[yy][xx]));
                    }

                    double altDelta = cellAlt - fromAlt;
                    if (altDelta < 0.0) altDelta = 0.0;

                    double dry    = altitudePenalty(altDelta, someScale);
                    double fromHum = humidity[fromY][x];
                    double inc    = fromHum * std::abs(windY) * (1.0 + windZ) * dry;

                    humidity[y][x] += float(inc);
                }
            }

            // Clamp to local capacity (still in g/m³)
            if (humidity[y][x] > airCapacity)
                humidity[y][x] = float(airCapacity);
        }
    }

    // -----------------------------------------------------------------------
    // SECOND PASS: Y north wind (windY > 0)
    // -----------------------------------------------------------------------
    for (int y = mapH - SUB_SCALE; y >= 0; y -= SUB_SCALE)
    {
        auto [windX, windY, windZ] = windDir[y];

        if (windY > 0.0)
        {
            for (int x = 0; x < mapW; x += SUB_SCALE)
            {
                double cellTemp = 0.0;
                double cellAlt  = 0.0;
                int    waterCount = 0;

                for (int dy = 0; dy < SUB_SCALE; dy++)
                for (int dx = 0; dx < SUB_SCALE; dx++)
                {
                    int yy = y + dy;
                    int xx = x + dx;
                    if (yy >= mapH || xx >= mapW) continue;

                    float alt = heightMap[yy][xx];
                    float t   = tempMap[yy][xx];
                    float w   = waterMap[yy][xx];

                    if (w == 0.0f)
                        waterCount++;
                    else{
                        cellAlt += alt;
                        waterCount += riverMap[yy][xx]*RIVER_EVAPORATION;
                    }

                    cellTemp += t;
                }

                cellTemp /= subScaleSq;
                cellAlt  /= subScaleSq;

                double airCapacity = saturatedAbsoluteHumidity(cellTemp);

                int fromY = y + SUB_SCALE;

                if (fromY >= 0 && fromY < mapH)
                {
                    double fromAlt = 0.0;

                    for (int dy = 0; dy < SUB_SCALE; dy++)
                    for (int dx = 0; dx < SUB_SCALE; dx++)
                    {
                        int yy = fromY + dy;
                        int xx = x + dx;
                        if (yy >= mapH || xx >= mapW) continue;

                        fromAlt = std::max(fromAlt, double(heightMap[yy][xx]));
                    }

                    double altDelta = cellAlt - fromAlt;
                    if (altDelta < 0.0) altDelta = 0.0;

                    double dry    = altitudePenalty(altDelta, someScale);
                    double fromHum = humidity[fromY][x];
                    double inc    = fromHum * windY * (1.0 + windZ) * dry;

                    humidity[y][x] += float(inc);
                }

                if (humidity[y][x] > airCapacity)
                    humidity[y][x] = float(airCapacity);
            }
        }
    }

    // -----------------------------------------------------------------------
    // NORMALIZE TO ~0–1 RANGE FOR DISPLAY (30 g/m³ max)
    // -----------------------------------------------------------------------
    for (int y = 0; y < mapH; y++)
    for (int x = 0; x < mapW; x++)
        humidity[y][x] /= 30.0f;

    interpolateMissingValues(humidity, SUB_SCALE);

    return humidity;
}



//White, Mottershead and Harrison Net Primary Productivity formula for rainfall
float WMH_humidity(float humidity){
    float CONST_A = 2909;
    float CONST_B = -0.000688;
    float out;
    out = 1-exp(CONST_B  * humidity);
    out = out * CONST_A;
    return out;
}

//White, Mottershead and Harrison Net Primary Productivity formula for temperature
float WMH_temperature(float temperature){
    float CONST_A = 2914;
    float CONST_B = -0.128;
    float CONST_C = 3.64;
    float out;
    out = 1 + CONST_C * exp(CONST_B  * temperature);
    out = CONST_A / out;
    return out;
}

// White, Mottershead and Harrison Net Primary Productivity in g/m2 /year
// Ranges from 0 to ~3000
std::vector<std::vector<float>> calculateFertilityMap(
    std::vector<std::vector<float>>& tempMap, 
    std::vector<std::vector<float>>& humidityMap, 
    std::vector<std::vector<float>>& waterMap) {
    
    int width = tempMap.size();
    int height = tempMap[0].size();
    std::vector<std::vector<float>> fert(
        height, std::vector<float>(width, 0.0));
    if (height % SUB_SCALE != 0 || width % SUB_SCALE != 0) {
        std::cerr << "Error: width and height must be multiples of "
                << SUB_SCALE << std::endl;
        return humidityMap; // early exit with zeros
    }
    fert.resize(width, std::vector<float>(height, 0.0f));
    for (int x = 0; x < width; x += SUB_SCALE) {
        for (int y = 0; y < height; y += SUB_SCALE) {
            float temp = tempMap[x][y];
            float humidity = humidityMap[x][y];
            if (x==752 && y==752){
                //debug
                std::cout << "Debug fertility calc at (" << x << "," << y << "): temp=" << temp2celsius(temp) << "C, humidity=" << humid2rainfall(humidity) << "mm\n";
                std::cout << "  WMH temp=" << WMH_temperature(temp2celsius(temp)) << ", WMH humid=" << WMH_humidity(humid2rainfall(humidity)) << "\n";
            }
            //fert[x][y] = WMH_temperature(temp2celsius(temp))/3000;
            //fert[x][y] = WMH_humidity(humid2rainfall(humidity))/3000;
            fert[x][y] = std::min(WMH_humidity(humid2rainfall(humidity)), WMH_temperature(temp2celsius(temp)))/3000;
        }
    }
    interpolateMissingValues(fert, SUB_SCALE);
    return fert;
}

