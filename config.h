#pragma once
#include <fstream>
#include <string>
#include <ctime>

static int OCTAVES = 9;
static int TEX_W = 1024;
static int TEX_H = 1024;
static double MAXALTITUDE = 4000.0;
static int SEED = 17;
static float NOISE_SCALE = 0.0025;

static int RIVER_COUNT = 1000;

static float WINDCLAMP = 0.0;
static double WIND_SCALE_DOWNWARD = 1.0;
static float RIVER_EVAPORATION = 0.5f;
static int SUB_SCALE = 8;
static double LAT_BOTTOM = 40.0;
static double LAT_TOP = 50.0;

void loadConfig(const std::string &path);