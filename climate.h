// Climate utility header.
// Declares temperature and humidity map generation helpers.
//#pragma once
#include <vector>

// Generates river map by marking points in the height map.
// Returns a reference to the modified height map.
std::vector<std::vector<float>> generateRiverPoints_main(int width, int height, std::vector<std::vector<float>>& heightmap);
// Generates a temperature map in kilo‑Kelvin.
std::vector<std::vector<double>> calculateTemperatureMap(int width, int height,
                                                    const std::vector<std::vector<float>>& heightmap);

// Generates a humidity map (0‑30 range) aligned with temperature map.
std::vector<std::vector<double>> calculateHumidityMap(int width, int height,
                                                    const std::vector<std::vector<double>>& tempMap,
                                                    const std::vector<std::vector<float>>& heightmap,
                                                    const std::vector<std::vector<float>>& rivermap);
// Returns the current subscale value from configuration.
int getSubscale();
// Computes saturated absolute humidity (kg/m^3) from temperature in Kelvin.
double saturatedAbsoluteHumidity(double temperatureKelvin);
void interpolateMissingValues(std::vector<std::vector<double>>& grid, int subScale);
