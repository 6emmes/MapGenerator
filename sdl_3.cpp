#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include <iostream>
#include "texture.h"
#include "terrain.h"
#include "climate.h"
// Function to compute temperature map from height map
extern std::vector<std::vector<double>> calculateTemperatureMap(int width, int height, const std::vector<std::vector<float>>& heightmap);

// A simple movable rectangle.
struct Rect
{
    float x, y;           // top‑left
    float w, h;           // width and height
    float speed;          // movement speed (pixels/sec)
    float scale;          // zoom factor
    SDL_Color color;      // rectangle color
};

static SDL_Window *window = nullptr;
static SDL_Renderer *renderer = nullptr;
// Texture representing the terrain surface. Renamed from rectTex for clarity
static SDL_Texture *terrainTex = nullptr; // texture for terrain
static Rect rect;
// 
// Shading texture overlay
static SDL_Texture *shadingTex = nullptr; // shading texture
// Climate texture overlay (red channel temperature)
static SDL_Texture *climateTex = nullptr;
static SDL_Texture *riverTex = nullptr;
// Flags to enable/disable rendering of textures
static bool g_showTerrainTex = true;
static bool g_showShadingTex = false;
static bool g_showClimateTex = false;
static bool g_showRiverTex = true;
    // Flags already defined above at file scope

// No hard‑coded sizes; will be obtained from texture generation.

// <function moved to gradient.cpp>

static int Init()
{   
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init() Error: %s", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("Rectangle Demo", 640, 480, SDL_WINDOW_HIDDEN);
    if (!window) {
        SDL_Log("SDL_CreateWindow() Error: %s", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer() Error: %s", SDL_GetError());
        return -1;
    }

    // initial rectangle state - dimensions to be filled after texture generation
    rect.x = 100.0f;
    rect.y = 100.0f;
    rect.speed = 300.0f; // faster horizontal movement
    rect.scale = 1.0f;
    rect.color.r = 0; rect.color.g = 128; rect.color.b = 255; rect.color.a = 255;
    // Create texture from gradient pixels
    // Generate height map, then convert to texture for rendering
    // Load rendering configuration from file before generating map
    loadConfig("mapgen.conf");
    auto heightMap = generateHeightMap();
    // Save height map to PNG and BMP for debugging.
    if (!saveHeightMapBMP(heightMap, "heightmap.bmp")) {
        SDL_Log("Failed to write heightmap BMP");
    }
    std::cout<<"map dimensioons: "<<TEX_W<<"x"<<TEX_H<<std::endl;
    auto riverData = generateRiverPoints_main(TEX_W, TEX_H, heightMap);
    auto riverTexData = riverTexture(riverData);
    riverTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, riverTexData.width, riverTexData.height);
    SDL_SetTextureScaleMode(riverTex, SDL_SCALEMODE_NEAREST);
    if (!riverTex) {
        SDL_Log("SDL_CreateTexture() River Error: %s", SDL_GetError());
        return -1;
    }
    if (SDL_UpdateTexture(riverTex, NULL, riverTexData.pixels.data(), riverTexData.width * 4) < 0) {
        SDL_Log("SDL_UpdateTexture() River Error: %s", SDL_GetError());
        return -1;
    }
    SDL_SetTextureBlendMode(riverTex, SDL_BLENDMODE_BLEND);
    auto texData = textureFromHeightMap(heightMap);
    // Generate a separate height texture for shading calculations
    auto heightTexData = heightTextureFromHeightMap(heightMap);
    rect.w = texData.width;
    rect.h = texData.height;
    terrainTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, texData.width, texData.height);
    SDL_SetTextureScaleMode(terrainTex, SDL_SCALEMODE_NEAREST);
    if (!terrainTex) {
        SDL_Log("SDL_CreateTexture() Error: %s", SDL_GetError());
        return -1;
    }
    if (SDL_UpdateTexture(terrainTex, NULL, texData.pixels.data(), texData.width * 4) < 0) {
        SDL_Log("SDL_UpdateTexture() Error: %s", SDL_GetError());
        return -1;
    }
    SDL_SetTextureScaleMode(terrainTex, SDL_SCALEMODE_NEAREST);

    // Create climate texture overlay using temperature map
    // Temperature map in kilo-Kelvin values, computed from height map
    auto tempMap = calculateTemperatureMap(TEX_W, TEX_H, heightMap);
    auto humidityMap = calculateHumidityMap(TEX_W, TEX_H, tempMap, heightMap, riverData);
    TextureData climateData = climateTexture(tempMap, humidityMap);
    climateTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, climateData.width, climateData.height);
    SDL_SetTextureScaleMode(climateTex, SDL_SCALEMODE_NEAREST);
    if (!climateTex) {
        SDL_Log("SDL_CreateTexture() Climate Error: %s", SDL_GetError());
        return -1;
    }
    if (SDL_UpdateTexture(climateTex, NULL, climateData.pixels.data(), climateData.width * 4) < 0) {
        SDL_Log("SDL_UpdateTexture() Climate Error: %s", SDL_GetError());
        return -1;
    }
    SDL_SetTextureBlendMode(climateTex, SDL_BLENDMODE_BLEND);

    // Calculate shading texture based on height texture
    TextureData shadingData = calculateShadingTexture(heightTexData);
    shadingTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, shadingData.width, shadingData.height);
    SDL_SetTextureScaleMode(shadingTex, SDL_SCALEMODE_NEAREST);
    if (!shadingTex) {
        SDL_Log("SDL_CreateTexture() Shading Error: %s", SDL_GetError());
        return -1;
    }
    if (SDL_UpdateTexture(shadingTex, NULL, shadingData.pixels.data(), shadingData.width * 4) < 0) {
        SDL_Log("SDL_UpdateTexture() Shading Error: %s", SDL_GetError());
        return -1;
    }
    SDL_SetTextureBlendMode(shadingTex, SDL_BLENDMODE_BLEND);

    return 0;
}




static void Term()
{
    if (terrainTex) SDL_DestroyTexture(terrainTex);
    if (shadingTex) SDL_DestroyTexture(shadingTex);
    if (riverTex) SDL_DestroyTexture(riverTex);
    if (climateTex) SDL_DestroyTexture(climateTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Render the rectangle to the screen.

static void Render()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_FRect dst = { rect.x, rect.y, rect.w * rect.scale, rect.h * rect.scale };
    if (g_showTerrainTex && terrainTex) {
        SDL_SetRenderDrawColor(renderer, rect.color.r, rect.color.g, rect.color.b, rect.color.a);
        SDL_RenderTexture(renderer, terrainTex, NULL, &dst);
    }
    if (g_showRiverTex && riverTex) {
        SDL_RenderTexture(renderer, riverTex, NULL, &dst);
    }
    // Overlay climate texture on top of shading, if enabled
    if (g_showClimateTex && climateTex) {
        SDL_RenderTexture(renderer, climateTex, NULL, &dst);
    }

    // Overlay shading texture on top of rectangle, if enabled
    if (g_showShadingTex && shadingTex) {
        SDL_RenderTexture(renderer, shadingTex, NULL, &dst);
    }
    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[])
{
    if (Init() != 0) return 1;
    SDL_SetWindowSize(window, 640, 480);
    SDL_ShowWindow(window);
    Uint32 last = SDL_GetTicks();
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            // Mousewheel zoom – scale around cursor.
            if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                // Preserve old scale for coordinate recomputation
                float oldScale = rect.scale;
                if (event.wheel.y > 0) {
                    rect.scale *= 1.1f;
                } else if (event.wheel.y < 0) {
                    rect.scale /= 1.1f;
                }
                // Clamp scale to avoid excessive zoom
                if (rect.scale < 0.1f) rect.scale = 0.1f;
                if (rect.scale > 10.0f) rect.scale = 10.0f;

                // Recenter rectangle so the cursor stays over the same point
                float mx, my;
                SDL_GetMouseState(&mx, &my);
                float factor = rect.scale / oldScale; // new / old
                rect.x = mx - (mx - rect.x) * factor;
                rect.y = my - (my - rect.y) * factor;
            }
            // Handle numeric key toggles
            if (event.type == SDL_EVENT_KEY_DOWN) {
                // Table of keys to flag pointers – can be extended.
                static const struct {
                    SDL_Scancode scancode;
                    bool *flag;
                } toggles[] = {
                    {SDL_SCANCODE_1, &g_showTerrainTex},
                    {SDL_SCANCODE_2, &g_showShadingTex},
                    {SDL_SCANCODE_3, &g_showClimateTex},
                    {SDL_SCANCODE_4, &g_showRiverTex},
                };
                for (const auto &t : toggles) {
                if (event.key.scancode == t.scancode) {
                        *(t.flag) = !*(t.flag);
                        break;
                    }
                }
            }
        }
        Uint32 now = SDL_GetTicks();
        float dt = (now - last) / 1000.0f;
        last = now;
        // movement handled via keyboard state
        const bool *keys = SDL_GetKeyboardState(nullptr);
        if (keys[SDL_SCANCODE_LEFT])  rect.x += rect.speed * dt;
        if (keys[SDL_SCANCODE_RIGHT]) rect.x -= rect.speed * dt;
        if (keys[SDL_SCANCODE_UP])    rect.y += rect.speed * dt;
        if (keys[SDL_SCANCODE_DOWN])  rect.y -= rect.speed * dt;
        Render();
        SDL_Delay(16); // cap ~60fps
    }
    Term();
    return 0;
}
