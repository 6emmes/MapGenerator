// climate.cpp
// Placeholder for climate-related map generation logic.
// Currently minimal implementation.

// Example: function to simulate temperature based on height.
// Adjusted for potential future integration.
double temperatureFromHeight(double height) {
    // Simple linear mapping: 0m -> 30°C, 1000m -> 0°C
    return 30.0 - 0.03 * height;
}

// If desired, expose additional functions here.
// This file is currently not compiled by Makefile.
// To include it, add it to SRCS in Makefile.

// *** New solar energy calculation helpers ***
// Include necessary headers
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

// Latitude bounds read from configuration
static double LAT_BOTTOM = 0.0;
static double LAT_TOP = 30.0;

// Load latitude bounds from mapgen.conf
static void loadLatitudeConfig(const std::string &path) {
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
        } catch (...) {}
    }
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
std::vector<std::vector<double>> calculateTemperatureMap(int width, int height) {
    // Constants
    constexpr double STEFAN = 5.67e-8;
    constexpr double EPSILON = 0.89;
    static bool loaded = false;
    if (!loaded) {
        loadLatitudeConfig("mapgen.conf");
        loaded = true;
    }
    std::vector<std::vector<double>> tempMap(height, std::vector<double>(width, 0.0));
    double latDelta = (height > 1) ? (LAT_TOP - LAT_BOTTOM) / double(height - 1) : 0.0;
    for (int y = 0; y < height; ++y) {
        double lat = LAT_BOTTOM + latDelta * double(y);
        double power = incomingSolarEnergy(lat); // W/m^2
        double Te = std::pow((power * 1000.0) / STEFAN, 0.25);
        double Temperature = Te * std::pow(2.0 / (2.0 - EPSILON), 0.25);
        double kiloKelvin = Temperature / 1000.0;
        for (int x = 0; x < width; ++x) {
            tempMap[y][x] = kiloKelvin;
        }
    }
    return tempMap;
}
