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

// Latitude bounds read from configuration
static double LAT_BOTTOM = 0.0;
static double LAT_TOP = 30.0;
// Default maximum altitude for a height value of 1.0 in the heightmap.
// Can be overridden by a configuration entry `maxaltitude`.
static double MAXALTITUDE = 4000.0; // meters

// Sub-scale factor for temperature map resolution.
// Can be overridden by a configuration entry `sub_scale`.
static int SUB_SCALE = 4;

static int RIVER_LENGTH = 762;
static int RIVER_COUNT = 4000;

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
            else if (key == "river_length") RIVER_LENGTH = std::stoi(val);
            else if (key == "river_count") RIVER_COUNT = std::stoi(val);
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

int createRiver(int width, int height, const std::pair<int,int> &originPoint, std::vector<std::vector<float>> &riverMap,
    std::vector<std::vector<float>> &heightmap, std::vector<std::vector<bool>> &landMap) {
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
                return 0;
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
            else return 0;
        }
        if (tmpPathFrontier.size() == 0) {
            return 0;
        }
        currentPoint = tmpPathFrontier.front();
        tmpPathFrontier.pop();
        if (heightmap[currentPoint.second][currentPoint.first] < curheight) {
            curheight = heightmap[currentPoint.second][currentPoint.first];
            curheight += curheight/200;
            minHeightPoint = currentPoint;
        }
        if (landMap[currentPoint.second][currentPoint.first] == false) {
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
    //=====================================================================Saving the path to the map
    std::cout  << "River length: "<< tmpPathSize <<"/"<<riverlen<<std::endl;
    for (int j=0;j<tmpPathSize;j++) {
        currentPoint = tmpPath.top();
        tmpPath.pop();
        if (riverMap[currentPoint.second][currentPoint.first] > 0) {
            riverFound = 1;
            //delta = delta*10;
            break;
        }
        else riverMap[currentPoint.second][currentPoint.first] = value;
        value += delta;
    }
        return 1;
    if (riverFound){
        value = 1;
        float currentValue = riverMap[currentPoint.second][currentPoint.first];
        riverMap[currentPoint.second][currentPoint.first] +=value;
        for (int i =0;i<10;i++){
            riverFound = 0;
            nextstep = currentPoint;
            for (const auto& d : directions) {
                if (riverMap[currentPoint.second + d.second][currentPoint.first + d.first] > currentValue && tmpMap[currentPoint.second + d.second][currentPoint.first + d.first]>0){
                    if  (tmpMap[currentPoint.second + d.second][currentPoint.first + d.first] > tmpMap[nextstep.second][nextstep.first]) {
                        nextstep.first = currentPoint.first + d.first;
                        nextstep.second = currentPoint.second + d.second;
                    }

                }
            }
            std::cout << "a"<<std::endl;
            riverMap[nextstep.second][nextstep.first] = value;
            std::cout << "b"<<std::endl; 
            currentPoint.first = nextstep.first;
            currentPoint.second = nextstep.second;
            float currentValue = tmpMap[currentPoint.second][currentPoint.first];
            if (riverFound==0) break;

        }
    }
    return 1;
}

std::vector<std::vector<float>> generateRiverPoints_main(int width, int height,
    std::vector<std::vector<float>> &heightmap,
    std::vector<std::vector<bool>> &landMap) {
    std::vector<std::vector<float>> riverMap(height, std::vector<float>(width, 0.0f));
    //std::mt19937 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
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
    int a=0;    //debug
    for (const auto &p : points) {
        riversmade += createRiver(width, height, p, riverMap, heightmap, landMap);
        //break;
    }
    std::cout << "Generated " << riversmade <<" rivers out of "<< points.size() << " starting river points in " << RIVER_COUNT << " attempts.\n";
    std::cout << "River map generation complete.\n";
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

    double e = e0 * std::exp(b * (temperatureK - T1) / (temperatureK - T2));
    return e / (R * temperatureK); // kg/m^3
}




// Compute incoming solar energy from latitude in degrees
static inline double incomingSolarEnergy(double latitudeDeg) {
    const double deg2rad = 3.14159265358979323846 / 180.0;
    return (420.0 - 175.0) + 175.0 * std::cos(latitudeDeg * deg2rad);
}

// Generate a temperature map using thermal equilibrium based on the
// incoming solar energy.  The returned values are in kilo‑kelvins.
// Formula:
//   Te = (power*1000 / STEFAN)^(1/4)
//   Temperature = Te * (2/(2 - ε))^(1/4)
//   Result divided by 1000.
//   ε = 0.89, STEFAN = 5.67e-8.
// Heightmap values range from 0.0 (~sea level) to 1.0 (~max altitude).
// Uses a standard lapse rate (6.5°C per km) to adjust temperature.
std::vector<std::vector<float>> calculateTemperatureMap(int width, int height,
                                                            const std::vector<std::vector<float>>& heightmap) {
    // Constants
    constexpr float STEFAN = 5.67e-8;
    constexpr float EPSILON = 0.89;
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
        float Te = std::pow((power) / STEFAN, 0.25);
        float baseTemperatureK = Te * std::pow(2.0 / (2.0 - EPSILON), 0.25); // Kelvin
        for (int x = 0; x < width; ++x) {
            float altitudeMeters = 0.0;
            if (y < static_cast<int>(heightmap.size()) &&
                x < static_cast<int>(heightmap[0].size())) {
                altitudeMeters = heightmap[y][x] * MAXALTITUDE; // meters
            }
            float tempShiftK = 6.5 * (altitudeMeters / 1000.0); // K
            float tempK = baseTemperatureK - tempShiftK;
            tempMap[y][x] = tempK;
            if (tempK > maxtemp) maxtemp = tempK; // debug
        }
    }
    std::cout << "Debug: Max temperature in map: " << maxtemp << " K\n";
    return tempMap;
}


    //Wind direction is {West, North, Up} 
std::tuple<float, float, float> getWindDirection(float latitudeDeg) {
    latitudeDeg = std::max(-90.0f, std::min(90.0f, latitudeDeg));
    std::vector<std::pair<float, std::tuple<float, float, float>>> ramp = {
        {-90.0f,   {1, 1, 0}},      // Polar Easterlies
        {-60.0f, {0, 0, 1}},      // Subpolar Low
        {-45.0f,   {-1, -1, 0}},      // Westerlies
        {-30.0f,  {0, 0, -1}},      // Subtropical High
        {-15.0f,   {1, 1, 0}},      // SE Trade
        {0.0f,   {1, 0, 1}},      // ITCZ
        {15.0f,   {1, -1, 0}},      // NE Trade
        {30.0f,  {0, 0, -1}},      // Subtropical High
        {45.0f,   {-1, 1, 0}},      // Westerlies
        {60.0f, {0, 0, 1}},      // Subpolar Low
        {90.0f,   {1, -1, 0}},      // Polar Easterlies
    };
    for (size_t i = 1; i < ramp.size(); ++i) {
        if (latitudeDeg <= ramp[i].first) {
            float t = (latitudeDeg - ramp[i-1].first) / (ramp[i].first - ramp[i-1].first);
            std::tuple c = { std::get<0>(ramp[i-1].second) + (std::get<0>(ramp[i].second) - std::get<0>(ramp[i-1].second)) * t,
                            std::get<1>(ramp[i-1].second) + (std::get<1>(ramp[i].second) - std::get<1>(ramp[i-1].second)) * t,
                            std::get<2>(ramp[i-1].second) + (std::get<2>(ramp[i].second) - std::get<2>(ramp[i-1].second)) * t};
            return c;
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

float calculateWindSimple(float WindComponent, float FromHumidity, float WindScale){
    return  FromHumidity * std::clamp(WindComponent * WindScale, 0.0f, 1.0f);
}


std::vector<std::vector<float>> calculateHumidityMap(
    int width,
    int height,
    const std::vector<std::vector<float>>& tempMap,
    const std::vector<std::vector<float>>& heightmap,
    const std::vector<std::vector<float>>& rivermap) {
    // Create blank output array filled with zeros
    std::vector<std::vector<float>> humidityMap(
        height, std::vector<float>(width, 0.0));
    if (height % SUB_SCALE != 0 || width % SUB_SCALE != 0) {
        std::cerr << "Error: width and height must be multiples of "
                << SUB_SCALE << std::endl;
        return humidityMap; // early exit with zeros
    }
    // Iterate over every sub_scale positions
    double tmpHumidity;
    int watercells = 0;
    float rivercells = 0;
    float tmpWindHumidity = 0 ;
    float tmpSaturationValue;
    int loopOffset;
    int tmpIndex; //Used to reverse inner loop direction
    float latDelta = (height > 1) ? (LAT_TOP - LAT_BOTTOM) / (height - 1) : 0.0;
    for (int y = 0; y < height; y += SUB_SCALE) {   //north - south
        float lat = LAT_BOTTOM + latDelta * y;

        std::tuple<float, float, float> winddirection = getWindDirection(lat);
        float windW = std::get<0>(winddirection);
        float windN = std::get<1>(winddirection);
        float windU = std::get<2>(winddirection);
        if (windW < 0) loopOffset = width-SUB_SCALE;
        else loopOffset = 0;

        for (int x = 0; x < width; x += SUB_SCALE) {    //east - west
            watercells = 0;
            rivercells = 0;
            tmpHumidity = 0;
            tmpIndex = abs(loopOffset - x);
            tmpSaturationValue = saturatedAbsoluteHumidity(tempMap[y][tmpIndex]);
            for (int dy = 0; dy < SUB_SCALE; ++dy) {
                for (int dx = 0; dx < SUB_SCALE; ++dx) {
                    int yy = y + dy;
                    int xx = tmpIndex + dx;
                    if (yy < height && xx < width) {
                        if (heightmap[yy][xx] < 0.101f) {
                            watercells++;
                        }
                        if (rivermap[yy][xx]>0.0){
                            rivercells += rivermap[yy][xx]/2;
                        }
                    }
                }
            }
            tmpHumidity = tmpSaturationValue*0.85*watercells/(SUB_SCALE*SUB_SCALE); // 85% relative humidity from ocean tiles
            tmpHumidity += rivercells;///(SUB_SCALE*SUB_SCALE);
            if (tmpHumidity<0.1*tmpSaturationValue)tmpHumidity = 0.1*tmpSaturationValue;
            if (y>=SUB_SCALE){   //try wind from north
                if (windN<0){
                    tmpWindHumidity = calculateWindSimple(-1*windN, humidityMap[y-SUB_SCALE][tmpIndex], 1.1);
                    if (tmpWindHumidity>tmpHumidity) tmpHumidity = tmpWindHumidity;
                }
            }
            //East/West wind:
            if(tmpIndex >= SUB_SCALE){
                tmpWindHumidity = calculateWindSimple(std::abs(windW), humidityMap[y][tmpIndex-SUB_SCALE], 0.8);
                if (tmpWindHumidity>tmpHumidity) tmpHumidity = tmpWindHumidity;
            }

            humidityMap[y][tmpIndex] = std::clamp(tmpHumidity, 0.0, double(tmpSaturationValue));
        }
    }
    interpolateMissingValues(humidityMap, SUB_SCALE);
    return humidityMap;
}

