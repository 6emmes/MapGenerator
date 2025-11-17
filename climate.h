// Climate utility header.
// Declares temperature and humidity map generation helpers.
#pragma once
#include <vector>

// Generates a temperature map in kilo‑Kelvin.
std::vector<std::vector<double>> calculateTemperatureMap(int width, int height,
                                                    const std::vector<std::vector<float>>& heightmap);

// Generates a humidity map (0‑30 range) aligned with temperature map.
std::vector<std::vector<double>> calculateHumidityMap(int width, int height,
                                                    const std::vector<std::vector<double>>& tempMap);
// Returns the current subscale value from configuration.
int getSubscale();
