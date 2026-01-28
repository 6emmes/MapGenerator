#include "climate_wind.h"


//Wind direction is {West, North, Up} 
std::tuple<float, float, float> getWindDirection(float latitudeDeg) {
    float ROOT2 = 1;//0.70710678118;
    float SCALE = 2;
    float CLAMP = 0.98;
    //return std::make_tuple(0.97, 0.97, 0.0);
    latitudeDeg = std::max(-90.0f, std::min(90.0f, latitudeDeg));
    std::vector<std::pair<float, std::tuple<float, float, float>>> ramp = {
        {-90.0f,   {ROOT2, ROOT2, 0}},      // Polar Easterlies
        {-60.0f, {0, 0, 1}},                // Subpolar Low
        {-45.0f,   {-ROOT2, -ROOT2, 0}},    // Westerlies
        {-30.0f,  {0, 0, -1}},              // Subtropical High
        {-15.0f,   {ROOT2, ROOT2, 0}},      // SE Trade
        {0.0f,   {1, 0, 1}},                // ITCZ
        {15.0f,   {ROOT2, -ROOT2, 0}},      // NE Trade
        {30.0f,  {0, 0, -1}},               // Subtropical High
        {45.0f,   {-ROOT2, ROOT2, 0}},      // Westerlies
        {60.0f, {0, 0, 1}},                 // Subpolar Low
        {90.0f,   {ROOT2, -ROOT2, 0}},      // Polar Easterlies
    };
    for (size_t i = 1; i < ramp.size(); ++i) {
        if (latitudeDeg <= ramp[i].first) {
            float t = (latitudeDeg - ramp[i-1].first) / (ramp[i].first - ramp[i-1].first);
            float a = SCALE *(std::get<0>(ramp[i-1].second) + (std::get<0>(ramp[i].second) - std::get<0>(ramp[i-1].second)) * t);
            float b = SCALE *(std::get<1>(ramp[i-1].second) + (std::get<1>(ramp[i].second) - std::get<1>(ramp[i-1].second)) * t);
            float c = SCALE *(std::get<2>(ramp[i-1].second) + (std::get<2>(ramp[i].second) - std::get<2>(ramp[i-1].second)) * t);

            a = std::clamp(a, -CLAMP, CLAMP);
            b = std::clamp(b, -CLAMP, CLAMP);
            c = std::clamp(c, -CLAMP, CLAMP);
            std::tuple out = {a, b, c};
            return out;
        }
    }
    return ramp.back().second;
}


float calculateWind(float WindComponent, float FromHumidity, float AltitudeDelta, float FohenScale, float WindScale){
    float FohenFactor;
	if (AltitudeDelta < 0) AltitudeDelta = 0;
	else if (std::abs(AltitudeDelta) > 0.5) AltitudeDelta = 0;
	FohenFactor = 1 / (1 + AltitudeDelta * AltitudeDelta * FohenScale);
	FohenFactor = std::clamp(FohenFactor, 0.0f, 2.0f);
    return  WindComponent * FromHumidity * FohenFactor * WindScale;
}

float calculateWindSimple(float WindComponent, float FromHumidity, float WindScale, float localClamp){
    //WindComponent = std::clamp(WindComponent * WindScale, 0.0f, WINDCLAMP);
    return  FromHumidity * std::clamp(WindComponent * WindScale, 0.0f, localClamp);
}