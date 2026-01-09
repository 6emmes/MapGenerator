#pragma once
#include <vector>
#include <random>
#include <queue>
#include <stack>
#include <iostream>
#include "config.h"


std::pair<int, float> createRiver(
            int width, int height, const std::pair<int,int> &originPoint,
            std::vector<std::vector<float>> &riverMap,
            std::vector<std::vector<float>> &heightmap,
            std::vector<std::vector<float>> &waterMap);

std::vector<std::vector<float>> generateRiverPoints_main(
    int width, int height,
    std::vector<std::vector<float>> &heightmap,
    std::vector<std::vector<float>> &landMap);