#include "helper.h"

// 260~320 kelvin to Celsius
float temp2celsius(float temp) {return temp*60-13;}
// gram water vapor to mm rainfall
float humid2rainfall(float humid) {return humid*100*30;} // 100mm per gram of humidity and 30 grams in a map unit of humidity

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


bool checkCondition(float var, bool checkEqual) {
    if (checkEqual) {
        return var == 0;
    } else {
        return var != 0;
    }
}