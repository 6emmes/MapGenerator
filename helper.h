#pragma once
// 260~320 kelvin to Celsius
inline float temp2celsius(float temp) {return temp*60-13;}

// gram water vapor to mm rainfall
inline float humid2rainfall(float humid) {return humid*100*30;} // 100mm per gram of humidity and 30 grams in a map unit of humidity