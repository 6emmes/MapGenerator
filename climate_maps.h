#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "helper.h"
#include "config.h"
#include "climate_wind.h"

std::vector<std::vector<float>> calculateTemperatureMap(
        int width, int height,
        const std::vector<std::vector<float>>& heightmap);

std::vector<std::vector<float>> calculateHumidityMap2(
        int width,
        int height,
        const std::vector<std::vector<float>>& tempMap,
        const std::vector<std::vector<float>>& waterMap,
        const std::vector<std::vector<float>>& heightMap,
        const std::vector<std::vector<float>>& riverMap);

std::vector<std::vector<float>> calculateFertilityMap(
    std::vector<std::vector<float>>& tempMap, 
    std::vector<std::vector<float>>& humidityMap, 
    std::vector<std::vector<float>>& waterMap);