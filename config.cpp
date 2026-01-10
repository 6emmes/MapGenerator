#include "config.h"


double LAT_TOP = 50.0;
int OCTAVES = 9;
int TEX_W = 1024;
int TEX_H = 1024;
double MAXALTITUDE = 4000.0;
int SEED = 17;
float NOISE_SCALE = 0.0025;

int RIVER_COUNT = 1000;

float WINDCLAMP = 0.0;
double WIND_SCALE_DOWNWARD = 1.0;
float RIVER_EVAPORATION = 0.5f;
int SUB_SCALE = 8;
double LAT_BOTTOM = 40.0;


inline int initRandSeed(int seed) {
    if (seed != -1) return seed;
    return static_cast<int>(std::time(nullptr));
}

void loadConfig(const std::string &path) {
    std::ifstream fin(path);
    if (!fin.is_open()) return;
    int read_seed = -1;
    std::string line;
    while (std::getline(fin, line)) {
        // Ignore comments and empty lines
        auto comment = line.find('#');
        if (comment != std::string::npos) line.erase(comment);
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = line.find_last_not_of(" \t\r\n");
        line = line.substr(start, end - start + 1);
        if (line.empty()) continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        // Trim key/value
        key.erase(key.find_last_not_of(" \t") + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        try {
            if (key == "lat_bottom") LAT_BOTTOM = std::stod(val);
            else if (key == "lat_top") LAT_TOP = std::stod(val);
            else if (key == "maxaltitude") MAXALTITUDE = std::stod(val);
            else if (key == "sub_scale") SUB_SCALE = std::stoi(val);
            else if (key == "river_count") RIVER_COUNT = std::stoi(val);
            else if (key == "wind_clamp") WINDCLAMP = float(std::stoi(val))/1000;
            else if (key == "wind_scale_downward") WIND_SCALE_DOWNWARD = std::stoi(val);
            else if (key == "tex_w") TEX_W = std::stoi(val);
            else if (key == "tex_h") TEX_H = std::stoi(val);
            else if (key == "seed") read_seed = std::stoi(val);
            else if (key == "noise_scale") NOISE_SCALE = std::stof(val);
            else if (key == "river_evaporation") RIVER_EVAPORATION = std::stof(val); 
        } catch (...) {}
    }
    SEED = initRandSeed(read_seed);
}