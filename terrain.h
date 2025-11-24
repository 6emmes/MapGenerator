#pragma once
// Header exposing noise and terrain related constants and functions.
// These are moved from texture.cpp to separate terrain generation.

#include <cstdint>
#include <string>

// Basic color structure used for terrain mapping.
struct Color { uint8_t r, g, b; };

// --- Constants -----------------------------------------------------------
// Texture dimensions (used by both texture generation and noise
// calculations). Defining here keeps the values in a single place.
// Runtime configuration variables. They are set by the configuration
// loader in terrain.cpp and can be changed before any texture
// generation starts.  The defaults are defined in terrain.cpp.
extern int TEX_W;          // Texture width
extern int TEX_H;          // Texture height
extern float NOISE_SCALE;          // Controls frequency
extern int OCTAVES;                   // Number of fractal layers
extern const float PERSISTENCE;          // Amplitude decay

// Load configuration from a file.  The function is defined in terrain.cpp.
void loadConfig(const std::string &path);

// Classic Perlin permutation table.
extern const int PERM[512];

// Gradient vectors for 2D noise.
extern const float GRADIENTS[8][2];

// --- Functions ----------------------------------------------------------
float perlin2D(float x, float y);
float fbm(float x, float y);
float MM(float x, float y, int hetero);
float simpleMap(float val);
Color getTerrainPixel(float height);
