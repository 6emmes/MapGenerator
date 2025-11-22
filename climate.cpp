#include <cmath>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <utility>

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

void interpolateMissingValues(std::vector<std::vector<double>>& grid,
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

            double v00 = grid[y0][x0];
            double v10 = grid[y0][x1];
            double v01 = grid[y1][x0];
            double v11 = grid[y1][x1];

            double tx = (x1 == x0) ? 0.0 : static_cast<double>(x - x0) / (x1 - x0);
            double ty = (y1 == y0) ? 0.0 : static_cast<double>(y - y0) / (y1 - y0);

            // Bilinear interpolation
            double interpolated = (1 - tx) * (1 - ty) * v00
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

int createRiver(int width, int height, const std::pair<int,int> &originPoint, std::vector<std::vector<float>> &riverMap, std::vector<std::vector<float>> &heightmap) {
    std::pair<int,int> nextstep;
    std::pair<int,int> currentPoint = originPoint;
    std::pair<float, float> momentum;
    std::vector<std::pair<int,int>> directions = {{1,0}, {-1,0}, {0,1}, {0,-1}};//, {0,-2}, {0,2}, {-2,0}, {2,0}};
    float energy = 1.0f;
    
    for(int loo=0;loo<40;loo++){
    
        if (outOfBounds(currentPoint.first, currentPoint.second, width, height, 1)) break;
        float curheight = heightmap[currentPoint.second][currentPoint.first];
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
        momentum.first = -1*nextstep.first;
        momentum.second = -1*nextstep.second;
        currentPoint.first += nextstep.first;
        currentPoint.second += nextstep.second;
    }
    currentPoint.first = (originPoint.first+currentPoint.first*2)/3;
    currentPoint.second = (originPoint.second+currentPoint.second*2)/3;
    for (int i=-10;i<=10;i++){
        for (int j=-10;j<=10;j++){
            if (outOfBounds(currentPoint.first + i, currentPoint.second + j, width, height)) continue;
            if (riverMap[currentPoint.second + j][currentPoint.first + i] > 0.6 && 
                (currentPoint.second + j!=originPoint.second || currentPoint.first + i!=originPoint.first)) {
                //std::cout << "River killed" << std::endl;
                return 0;
            }
        }
    }
    std::vector<std::pair<int,int>> path;
    path.push_back(currentPoint);
    float curheight;
    for (int loo=0;loo<RIVER_LENGTH;loo++){
        if (loo==RIVER_LENGTH-1) {
            std::cout<<"river reached max length"<<std::endl;
            break;
        }
        if (outOfBounds(currentPoint.first, currentPoint.second, width, height, 2)) {
            path = {};
            std::cout << "River oob" << std::endl;
            break;
        }

        curheight = heightmap[currentPoint.second][currentPoint.first];
        float nextheight = curheight;
        for(auto [i,j] : directions){
                float nh = heightmap[currentPoint.second + j][currentPoint.first + i];
                if (nh < nextheight) {
                    nextheight = nh;
                    nextstep = std::make_pair(i,j);
                }
        }

        if ( currentPoint.first + nextstep.first == path[path.size() - 2].first && currentPoint.second + nextstep.second == path[path.size() - 2].second){
            nextstep.first = round(momentum.first);
            nextstep.second = round(momentum.second);
        }

        if (nextheight >= curheight) {
            if (energy > 0.0f) {
                energy=energy-1;
            }
            else{
                if (curheight > 0.099){
                    path = {};
                    std::cout<<"River dried up before reaching sea a"<<std::endl;
                }
                else std::cout << "River dried up, height: "<<curheight << std::endl;
                break;
            }
        }
        else energy += (curheight - nextheight)*128.0f;
        currentPoint.first += nextstep.first;//int((nextstep.first + 2*momentum.first)/3);
        currentPoint.second += nextstep.second;//int((nextstep.second + 2*momentum.second)/3);
        momentum.first = nextstep.first;//(nextstep.first + momentum.first)/2;
        momentum.second = nextstep.second;//(nextstep.second + momentum.second)/2;
        if (path.back() == currentPoint) {
                std::cout << "River reversed height: "<<curheight <<"->"<<nextheight << "  last step: "<< nextstep.first << "," << nextstep.second << std::endl;
            break;
        }
        if(curheight <= 0.099){
            std::cout << "River reached sea level" << std::endl;
            break;
        }
        path.push_back(currentPoint);
    }
    if (curheight > 0.099){
        std::cout<<"River dried up before reaching sea b"<<std::endl;
        return 0;
    }
    if (path.size() < 20) {
        std::cout<<"River too short"<<std::endl;
        return 0;
    } 
    std::cout<<"River created with length: "<<path.size()<<std::endl;
    float value = 1.0;
    for (const auto &p : path) {
        riverMap[p.second][p.first] = value;
        value -= 1.0f/512.0f;
    }
    return 1;
}

std::vector<std::vector<float>> generateRiverPoints_main(int width, int height,
    std::vector<std::vector<float>> &heightmap) {
    std::cout << "Generating river points of size " << width << "x" << height << " n:" << RIVER_COUNT << std::endl;
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

    for (const auto &p : points) {
        riversmade += createRiver(width, height, p, riverMap, heightmap);
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
std::vector<std::vector<double>> calculateTemperatureMap(int width, int height,
                                                            const std::vector<std::vector<float>>& heightmap) {
    // Constants
    constexpr double STEFAN = 5.67e-8;
    constexpr double EPSILON = 0.89;
    static bool loaded = false;
    if (!loaded) {
        loadclimateconfig("mapgen.conf");
        loaded = true;
    }
    double maxtemp = 0; //debug
    std::vector<std::vector<double>> tempMap(height, std::vector<double>(width, 0.0));
    double latDelta = (height > 1) ? (LAT_TOP - LAT_BOTTOM) / double(height - 1) : 0.0;
    for (int y = 0; y < height; ++y) {
        double lat = LAT_BOTTOM + latDelta * double(y);
        double power = incomingSolarEnergy(lat); // W/m^2
        double Te = std::pow((power) / STEFAN, 0.25);
        double baseTemperatureK = Te * std::pow(2.0 / (2.0 - EPSILON), 0.25); // Kelvin
        for (int x = 0; x < width; ++x) {
            double altitudeMeters = 0.0;
            if (y < static_cast<int>(heightmap.size()) &&
                x < static_cast<int>(heightmap[0].size())) {
                altitudeMeters = heightmap[y][x] * MAXALTITUDE; // meters
            }
            double tempShiftK = 6.5 * (altitudeMeters / 1000.0); // K
            double tempK = baseTemperatureK - tempShiftK;
            tempMap[y][x] = tempK;
            if (tempK > maxtemp) maxtemp = tempK; // debug
        }
    }
    std::cout << "Debug: Max temperature in map: " << maxtemp << " K\n";
    return tempMap;
}

// Calculate a humidity map based purely on temperature.
// The map is the same dimensions as the input temperature map.
// Each cell will initially be 0. The function walks the array
// stepping `SUB_SCALE` cells at a time and marks the visited
// cells with a value of 1.0.
// This is a placeholder, but gives the structure for later
// humidity modeling.
std::vector<std::vector<double>> calculateHumidityMap(
    int width,
    int height,
    const std::vector<std::vector<double>>& tempMap,
    const std::vector<std::vector<float>>& heightmap) {
    // Create blank output array filled with zeros
    std::vector<std::vector<double>> humidityMap(
        height, std::vector<double>(width, 0.0));
    if (height % SUB_SCALE != 0 || width % SUB_SCALE != 0) {
        std::cerr << "Error: width and height must be multiples of "
                << SUB_SCALE << std::endl;
        return humidityMap; // early exit with zeros
    }
    // Iterate over every sub_scale positions
    for (int y = 0; y < height; y += SUB_SCALE) {
        for (int x = 0; x < width; x += SUB_SCALE) {
            int watercells = 0;
            for (int dy = 0; dy < SUB_SCALE; ++dy) {
                for (int dx = 0; dx < SUB_SCALE; ++dx) {
                    int yy = y + dy;
                    int xx = x + dx;
                    if (yy < height && xx < width) {
                        if (heightmap[yy][xx] < 0.099f) { // arbitrary water threshold
                            watercells++;
                        }
                    }
                }
            }

            humidityMap[y][x] = saturatedAbsoluteHumidity(tempMap[y][x])*0.85*watercells/(SUB_SCALE*SUB_SCALE); // 85% relative humidity from ocean tiles
        }
    }
    interpolateMissingValues(humidityMap, SUB_SCALE);
    return humidityMap;
}
