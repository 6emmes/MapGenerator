#include "textures_core.h"

void fillTexture(const std::vector<std::vector<float>>& waterMap, TextureData& texture, bool targetColor, int fillColor, Point start){
    std::stack<Point> seed_stack;
    Point current_pos = start;
    seed_stack.push(current_pos);
    int left, right;
    int cur_y;
    int topflag, botflag;
    while (seed_stack.size()>0){
        current_pos = seed_stack.top();
        seed_stack.pop();
        cur_y = current_pos.y;
        if (cur_y >= TEX_H || cur_y < 0) continue;
        left = current_pos.x;
        while (left > 0 && checkCondition(waterMap[cur_y][left],targetColor)) left -=1;
        right = current_pos.x;
        while (right < TEX_W && checkCondition(waterMap[cur_y][right],targetColor)) right +=1;
        topflag = 0;
        botflag = 0;
        for (int i=left; i<right; i++){
            texture.pixels[cur_y * TEX_W + i] = fillColor;
            if (cur_y<TEX_H-1){
                if (checkCondition(waterMap[cur_y+1][i], targetColor) && texture.pixels[(cur_y+1) * TEX_W + i] != fillColor){
                    if (botflag==0) {
                        seed_stack.push({i, cur_y+1});
                        botflag = 1;
                    }
                }
                else {
                    botflag = 0;
                }
            }
             if (cur_y>0){
                if (checkCondition(waterMap[cur_y-1][i], targetColor) && texture.pixels[(cur_y-1) * TEX_W + i] != fillColor){
                    if (topflag==0) {
                        seed_stack.push({i, cur_y-1});
                        topflag = 1;
                    }
                }
                else {
                    topflag = 0;
                }
            }
        }
    }
}


uint32_t averageRGBA(uint32_t c1, uint32_t c2) {
    uint32_t r = (((c1 >> 24) & 0xFF) + ((c2 >> 24) & 0xFF)) >> 1;
    uint32_t g = (((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF)) >> 1;
    uint32_t b = (((c1 >> 8)  & 0xFF) + ((c2 >> 8)  & 0xFF)) >> 1;
    uint32_t a = 0xFF;
    return (r << 24) | (g << 16) | (b << 8) | a;
}

uint32_t weightedAverageRGBA(uint32_t c1, uint32_t c2, float dist1, float dist2) {
    // Handle degenerate case: both distances zero → return either color
    float sum = dist1 + dist2;
    if (sum == 0.0f) return c1;

    // Inverse-distance weighting
    float w1 = dist2 / sum;
    float w2 = dist1 / sum;

    uint32_t r1 = (c1 >> 24) & 0xFF;
    uint32_t g1 = (c1 >> 16) & 0xFF;
    uint32_t b1 = (c1 >> 8)  & 0xFF;

    uint32_t r2 = (c2 >> 24) & 0xFF;
    uint32_t g2 = (c2 >> 16) & 0xFF;
    uint32_t b2 = (c2 >> 8)  & 0xFF;

    uint32_t r = (uint32_t)(r1 * w1 + r2 * w2);
    uint32_t g = (uint32_t)(g1 * w1 + g2 * w2);
    uint32_t b = (uint32_t)(b1 * w1 + b2 * w2);

    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}


int getColorPalette(float temp, float humidity, std::vector<TextureSample> cloud){
    double bestDist1 = INFINITY, bestDist2 = INFINITY;
    int bestValue1 = 0, bestValue2 = 0;
    float new_temp = temp*1.5;
    float new_humidity = humidity;
    for (const auto& p: cloud){
        //double dist = std::abs(p.x - localX) + std::abs(p.y - localY);
        double dist = std::max(std::abs(p.x - new_temp) , std::abs(p.y - new_humidity));
        if (dist < bestDist1) {
            bestDist2 = bestDist1;
            bestValue2 = bestValue1;
            bestDist1 = dist;
            bestValue1 = p.value;
        }
         else if (dist < bestDist2) {
            if (p.value != bestValue1){
            bestDist2 = dist;
            bestValue2 = p.value;

            }
        }

    }
    if (bestValue1 == 0xff00ffff) return 0xff00ffff;
    //else if (bestValue2 != 0xff00ffff) return averageRGBA(bestValue1,bestValue2);
    else if (bestValue2 != 0xff00ffff) return weightedAverageRGBA(bestValue1,bestValue2, bestDist1, bestDist2);
    else return bestValue1;
}

uint32_t blend(uint32_t c00, uint32_t c10, uint32_t c01, uint32_t c11,
               float fx, float fy)
{
    uint8_t out[4];

    for (int i = 0; i < 4; i++) {
        float a = (float)((c00 >> (24 - 8*i)) & 0xFF);
        float b = (float)((c10 >> (24 - 8*i)) & 0xFF);
        float c = (float)((c01 >> (24 - 8*i)) & 0xFF);
        float d = (float)((c11 >> (24 - 8*i)) & 0xFF);

        float v0 = a + (b - a) * fx;
        float v1 = c + (d - c) * fx;
        float v  = v0 + (v1 - v0) * fy;

        out[i] = (uint8_t)v;
    }

    return (out[0] << 24) | (out[1] << 16) | (out[2] << 8) | 255;
}

int getColorTexture(float temp, float humidity) {
    float x = temp * 1.5f * (COLOR_TEXTURE_WIDTH  - 1);
    float y = humidity     * (COLOR_TEXTURE_HEIGHT - 1);

    int x0 = (int)x;
    int y0 = (int)y;
    int x1 = (x0 + 1 < COLOR_TEXTURE_WIDTH)  ? x0 + 1 : x0;
    int y1 = (y0 + 1 < COLOR_TEXTURE_HEIGHT) ? y0 + 1 : y0;

    float fx = x - x0;
    float fy = y - y0;
    
    return blend(
        CLIMATETEXTURE[y0][x0],
        CLIMATETEXTURE[y0][x1],
        CLIMATETEXTURE[y1][x0],
        CLIMATETEXTURE[y1][x1],
        fx, fy
    );
}

uint32_t getTerrainPixel(float height) {
    if (height==0) return 0x000099ff; // water color
    height = std::max(0.0f, std::min(1.0f, height));
    int heightIndex = static_cast<int>(height * 256.0f);
    return COLOR_RAMP[heightIndex];
}
