#pragma once
#include <vector>

float temp2celsius(float temp);
float humid2rainfall(float humid);
void interpolateMissingValues(std::vector<std::vector<float>>& grid,
                              int subScale);
bool checkCondition(float var, bool checkEqual);