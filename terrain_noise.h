#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "config.h"

float perlin2D(float x, float y);
float fbm(float x, float y);
float MM(float x, float y, int hetero);
float simpleMapScale();
float simpleMap(float val, float scale);