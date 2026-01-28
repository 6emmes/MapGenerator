#pragma once
#include <fstream>
#include <string>
#include <ctime>

extern int OCTAVES;
extern int TEX_W;
extern int TEX_H;
extern double MAXALTITUDE;
extern float PEAKRATIO;
extern int SEED;
extern float NOISE_SCALE;

extern int RIVER_COUNT;

extern float WINDCLAMP;
extern double WIND_SCALE_DOWNWARD;
extern float RIVER_EVAPORATION;
extern int SUB_SCALE;
extern double LAT_BOTTOM;
extern double LAT_TOP;

void loadConfig(const std::string &path);