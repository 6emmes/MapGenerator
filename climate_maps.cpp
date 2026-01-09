#include "climate_maps.h"

// Compute incoming solar energy from latitude in degrees
static inline double incomingSolarEnergy(double latitudeDeg) {
    double POLARPOWER = 150;   //  W/m2 real value is 180
    double EQUATORPOWER = 350; //  W/m2 real value is 180
    const double deg2rad = 3.14159265358979323846 / 180.0;
    return POLARPOWER + (EQUATORPOWER - POLARPOWER) * std::cos(latitudeDeg * deg2rad);
}

// Temperature in 260 - 320 kelvin range
std::vector<std::vector<float>> calculateTemperatureMap(int width, int height,
                                                            const std::vector<std::vector<float>>& heightmap) {
    // Constants
    constexpr float STEFAN = 5.67e-8;
    constexpr float EPSILON = 0.93;
    constexpr float GREENHOUSEFACTOR = 0.4;
    float maxtemp = 0; //debug
    std::vector<std::vector<float>> tempMap(height, std::vector<float>(width, 0.0));
    float latDelta = (height > 1) ? (LAT_TOP - LAT_BOTTOM) / float(height - 1) : 0.0;
    for (int y = 0; y < height; ++y) {
        float lat = LAT_BOTTOM + latDelta * float(y);
        float power = incomingSolarEnergy(lat); // W/m^2
        float baseTemperatureK = power / (EPSILON / (1+GREENHOUSEFACTOR));
        baseTemperatureK = baseTemperatureK / STEFAN;
        baseTemperatureK = std::pow(baseTemperatureK,0.25);
        for (int x = 0; x < width; ++x) {
            float altitudeMeters = 0.0;
            if (y < static_cast<int>(heightmap.size()) &&
                x < static_cast<int>(heightmap[0].size())) {
                altitudeMeters = heightmap[y][x] * MAXALTITUDE; // meters
            }
            float tempShiftK = 6.5 * (altitudeMeters / 1000.0); // K
            float tempK = (280 + baseTemperatureK - tempShiftK)/2;

            tempMap[y][x] = (tempK-260)/60;
            if (tempK > maxtemp) maxtemp = tempK; // debug
        }
    }
    std::cout << "Debug: Max temperature in map: " << maxtemp << " K\n";
    return tempMap;
}

static constexpr float OCEAN_REL_HUM = 0.85f; // ocean saturation fraction static
constexpr float WIND_SCALE = 0.85f; // how "wet" incoming ocean air is
static constexpr float DIFFUSION_FACTOR = 0.5f; // 0..1, probability / strength of diffusion 

inline double altitudePenalty(double altitudeDelta, double scale) {
    if (altitudeDelta <= 0.0) return 1.0; // no penalty when going downhill or flat
    return 1.0 / (1.0 + altitudeDelta * altitudeDelta * scale);
}

inline double saturatedAbsoluteHumidity(double temperatureK) {
    // Tetens constants
    const double e0  = 0.61078;   // kPa at 0 °C
    const double T1  = 273.15;    // K reference point (0 °C)
    const double T2  = 35.86;     // Tetens constant
    const double b   = 7.5;       // Tetens b coefficient (typical value)
    const double R   = 0.000461;  // Specific gas constant (kPa·kmol/(K·kg))
    temperatureK = temperatureK*60+260;
    double e = e0 * std::exp(b * (temperatureK - T1) / (temperatureK - T2));
    return e / (R * temperatureK); // kg/m^3
}

std::vector<std::vector<float>> calculateHumidityMap2(
        int width,
        int height,
        const std::vector<std::vector<float>>& tempMap,
        const std::vector<std::vector<float>>& waterMap,
        const std::vector<std::vector<float>>& heightMap,
        const std::vector<std::vector<float>>& riverMap){
    const double someScale  = 128.0;
    const double subScaleSq = double(SUB_SCALE) * double(SUB_SCALE);

    int mapW = width;
    int mapH = height;

    // Layout: humidity[y][x]
    std::vector<std::vector<float>> humidity(mapH, std::vector<float>(mapW, 0.0f));

    // Wind per latitude row
    std::vector<std::tuple<double,double,double>> windDir(mapH);

    auto latitudeAt = [&](int y) {
        double t = double(y) / double(mapH - 1);
        return LAT_TOP + t * (LAT_BOTTOM - LAT_TOP);
    };

    // -----------------------------------------------------------------------
    // FIRST PASS: X wind + Y south wind
    // -----------------------------------------------------------------------
    for (int y = 0; y < mapH; y += SUB_SCALE)
    {
        double lat = latitudeAt(y);
        windDir[y] = getWindDirection(lat);
        auto [windX, windY, windZ] = windDir[y];

        for (int x = 0; x < mapW; x += SUB_SCALE)
        {
            double cellTemp = 0.0;
            double cellAlt  = 0.0;
            float    waterCount = 0;

            // Aggregate block data over [y,y+SUB_SCALE) x [x,x+SUB_SCALE)
            for (int dy = 0; dy < SUB_SCALE; dy++)
            for (int dx = 0; dx < SUB_SCALE; dx++)
            {
                int yy = y + dy;
                int xx = x + dx;
                if (yy >= mapH || xx >= mapW) continue;

                float alt = heightMap[yy][xx];
                float t   = tempMap[yy][xx];
                float w   = waterMap[yy][xx];

                if (w == 0.0f)
                    waterCount++;
                else{
                    cellAlt += alt;
                    waterCount += riverMap[yy][xx]*RIVER_EVAPORATION;
                }

                cellTemp += t;
            }

            cellTemp /= subScaleSq;
            cellAlt  /= subScaleSq;

            double waterRatio  = double(waterCount) / subScaleSq;
            double airCapacity = saturatedAbsoluteHumidity(cellTemp);

            // Base evaporation
            if (waterCount == subScaleSq)
                humidity[y][x] += float(airCapacity * 0.85);
            else
                humidity[y][x] += float(airCapacity * waterRatio);

            // -------------------------
            // X wind transport
            // -------------------------
            if (windX != 0.0)
            {
                int fromX = x + SUB_SCALE * (windX > 0.0 ? -1 : +1);

                if (fromX >= 0 && fromX < mapW)
                {
                    double fromAlt = 0.0;

                    for (int dy = 0; dy < SUB_SCALE; dy++)
                    for (int dx = 0; dx < SUB_SCALE; dx++)
                    {
                        int yy = y + dy;
                        int xx = fromX + dx;
                        if (yy >= mapH || xx >= mapW) continue;

                        fromAlt = std::max(fromAlt, double(heightMap[yy][xx]));
                    }

                    double altDelta = cellAlt - fromAlt;
                    if (altDelta < 0.0) altDelta = 0.0;  // only uphill blocks

                    double dry    = altitudePenalty(altDelta, someScale);
                    double fromHum = humidity[y][fromX];
                    double inc    = fromHum * std::abs(windX) * (1.0 + windZ) * dry;

                    // accumulate, not overwrite
                    humidity[y][x] += float(inc);
                }
            }

            // -------------------------
            // Y wind (south, windY < 0)
            // -------------------------
            if (windY < 0.0)
            {
                int fromY = y - SUB_SCALE;

                if (fromY >= 0 && fromY < mapH)
                {
                    double fromAlt = 0.0;

                    for (int dy = 0; dy < SUB_SCALE; dy++)
                    for (int dx = 0; dx < SUB_SCALE; dx++)
                    {
                        int yy = fromY + dy;
                        int xx = x + dx;
                        if (yy >= mapH || xx >= mapW) continue;

                        fromAlt = std::max(fromAlt, double(heightMap[yy][xx]));
                    }

                    double altDelta = cellAlt - fromAlt;
                    if (altDelta < 0.0) altDelta = 0.0;

                    double dry    = altitudePenalty(altDelta, someScale);
                    double fromHum = humidity[fromY][x];
                    double inc    = fromHum * std::abs(windY) * (1.0 + windZ) * dry;

                    humidity[y][x] += float(inc);
                }
            }

            // Clamp to local capacity (still in g/m³)
            if (humidity[y][x] > airCapacity)
                humidity[y][x] = float(airCapacity);
        }
    }

    // -----------------------------------------------------------------------
    // SECOND PASS: Y north wind (windY > 0)
    // -----------------------------------------------------------------------
    for (int y = mapH - SUB_SCALE; y >= 0; y -= SUB_SCALE)
    {
        auto [windX, windY, windZ] = windDir[y];

        if (windY > 0.0)
        {
            for (int x = 0; x < mapW; x += SUB_SCALE)
            {
                double cellTemp = 0.0;
                double cellAlt  = 0.0;
                int    waterCount = 0;

                for (int dy = 0; dy < SUB_SCALE; dy++)
                for (int dx = 0; dx < SUB_SCALE; dx++)
                {
                    int yy = y + dy;
                    int xx = x + dx;
                    if (yy >= mapH || xx >= mapW) continue;

                    float alt = heightMap[yy][xx];
                    float t   = tempMap[yy][xx];
                    float w   = waterMap[yy][xx];

                    if (w == 0.0f)
                        waterCount++;
                    else{
                        cellAlt += alt;
                        waterCount += riverMap[yy][xx]*RIVER_EVAPORATION;
                    }

                    cellTemp += t;
                }

                cellTemp /= subScaleSq;
                cellAlt  /= subScaleSq;

                double airCapacity = saturatedAbsoluteHumidity(cellTemp);

                int fromY = y + SUB_SCALE;

                if (fromY >= 0 && fromY < mapH)
                {
                    double fromAlt = 0.0;

                    for (int dy = 0; dy < SUB_SCALE; dy++)
                    for (int dx = 0; dx < SUB_SCALE; dx++)
                    {
                        int yy = fromY + dy;
                        int xx = x + dx;
                        if (yy >= mapH || xx >= mapW) continue;

                        fromAlt = std::max(fromAlt, double(heightMap[yy][xx]));
                    }

                    double altDelta = cellAlt - fromAlt;
                    if (altDelta < 0.0) altDelta = 0.0;

                    double dry    = altitudePenalty(altDelta, someScale);
                    double fromHum = humidity[fromY][x];
                    double inc    = fromHum * windY * (1.0 + windZ) * dry;

                    humidity[y][x] += float(inc);
                }

                if (humidity[y][x] > airCapacity)
                    humidity[y][x] = float(airCapacity);
            }
        }
    }

    // -----------------------------------------------------------------------
    // NORMALIZE TO ~0–1 RANGE FOR DISPLAY (30 g/m³ max)
    // -----------------------------------------------------------------------
    for (int y = 0; y < mapH; y++)
    for (int x = 0; x < mapW; x++)
        humidity[y][x] /= 30.0f;

    interpolateMissingValues(humidity, SUB_SCALE);

    return humidity;
}


//White, Mottershead and Harrison Net Primary Productivity formula for rainfall
inline float WMH_humidity(float humidity){
    float CONST_A = 2909;
    float CONST_B = -0.000688;
    float out;
    out = 1-exp(CONST_B  * humidity);
    out = out * CONST_A;
    return out;
}

//White, Mottershead and Harrison Net Primary Productivity formula for temperature
inline float WMH_temperature(float temperature){
    float CONST_A = 2914;
    float CONST_B = -0.128;
    float CONST_C = 3.64;
    float out;
    out = 1 + CONST_C * exp(CONST_B  * temperature);
    out = CONST_A / out;
    return out;
}

// White, Mottershead and Harrison Net Primary Productivity in g/m2 /year
// Ranges from 0 to ~3000
std::vector<std::vector<float>> calculateFertilityMap(
    std::vector<std::vector<float>>& tempMap, 
    std::vector<std::vector<float>>& humidityMap, 
    std::vector<std::vector<float>>& waterMap) {
    
    int width = tempMap.size();
    int height = tempMap[0].size();
    std::vector<std::vector<float>> fert(
        height, std::vector<float>(width, 0.0));
    if (height % SUB_SCALE != 0 || width % SUB_SCALE != 0) {
        std::cerr << "Error: width and height must be multiples of "
                << SUB_SCALE << std::endl;
        return humidityMap; // early exit with zeros
    }
    fert.resize(width, std::vector<float>(height, 0.0f));
    for (int x = 0; x < width; x += SUB_SCALE) {
        for (int y = 0; y < height; y += SUB_SCALE) {
            float temp = tempMap[x][y];
            float humidity = humidityMap[x][y];
            //fert[x][y] = WMH_temperature(temp2celsius(temp))/3000;
            //fert[x][y] = WMH_humidity(humid2rainfall(humidity))/3000;
            fert[x][y] = std::min(WMH_humidity(humid2rainfall(humidity)), WMH_temperature(temp2celsius(temp)))/3000;
        }
    }
    interpolateMissingValues(fert, SUB_SCALE);
    return fert;
}