#pragma once
#include <tuple>
#include <vector>
#include <algorithm>

std::tuple<float, float, float> getWindDirection(float latitudeDeg);
float calculateWind(float WindComponent, float FromHumidity, float AltitudeDelta, float FohenScale, float WindScale);
float calculateWindSimple(float WindComponent, float FromHumidity, float WindScale, float localClamp);