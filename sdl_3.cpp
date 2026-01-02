#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include "texture.h"
#include "terrain.h"

#include "climate.h"

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
static Rect rect;
//
static SDL_Texture *terrainTex = nullptr;
static SDL_Texture *shadingTex = nullptr;
static SDL_Texture *climateTex = nullptr;
static SDL_Texture *riverTex = nullptr;
static SDL_Texture *waterTex = nullptr;
static SDL_Texture *colorTex = nullptr;
// Flags to enable/disable rendering of textures
static bool g_showTerrainTex = true;
static bool g_showShadingTex = false;
static bool g_showClimateTex = false;
static bool g_showRiverTex = true;
static bool g_showWaterTex = false;
static bool g_showColorTex = false;
    // Flags already defined above at file scope

void isNormal(std::vector<std::vector<float>> input){
    for (int x=0;x<input.size();x++){
        for(int y=0;y<input[0].size();y++){
            if (input[x][y]>1 || input[x][y]<0) {
                std::cout<<"NOT NORMAL "<<input[x][y]<<std::endl;
                return;
            }
        }
    }
    std::cout<<"NORMAL"<<std::endl;

}

void initMapTextures(){
    loadConfig("mapgen.conf");
    
    std::vector<std::vector<std::vector<float>>*> data;

    std::tuple<std::vector<std::vector<float>>, std::vector<std::vector<float>>> heighTuple = generateHeightMap();
    std::vector<std::vector<float>> heightMap = std::get<0>(heighTuple);
    std::vector<std::vector<float>> waterMap = std::get<1>(heighTuple);
    std::vector<std::vector<float>> riverData = generateRiverPoints_main(TEX_W, TEX_H, heightMap, waterMap);
    std::vector<std::vector<float>> tempMap = calculateTemperatureMap(TEX_W, TEX_H, heightMap);
    std::vector<std::vector<float>> humidityMap = calculateHumidityMap(TEX_W, TEX_H, tempMap, waterMap, riverData);
    std::vector<std::vector<float>> idMap = idData(waterMap);
    std::vector<std::vector<float>> metalMap = metalDensity();
    std::vector<std::vector<float>> fertilityMap = calculateFertilityMap(tempMap, humidityMap, riverData);
    std::vector<std::reference_wrapper<std::vector<std::vector<float>>>> maps = { heightMap, riverData, tempMap,
                                                                                humidityMap, waterMap, idMap,
                                                                                metalMap, fertilityMap };

    for (auto& m : maps) isNormal(m.get());

    for (auto& m : maps) data.push_back(&m.get());

    std::vector<std::string> layerNames = { "height_map", "river_map", "temp_map", 
                                            "humidity_map", "water_map", "id_map", 
                                            "metal_map", "fertility_map" }; 

    //saveTiff32(data, layerNames, "NowaMapa32.tiff");
    saveTiff8(data, layerNames, "NowaMapa8.tiff");

    TextureData texData = heightTexture(heightMap);
    TextureData riverTexData = riverTexture(riverData);
    TextureData climateData = climateTexture(tempMap, humidityMap);
    TextureData shadingData = shadingTexture(TEX_W, TEX_H, heightMap, waterMap);
    TextureData waterData = waterTexture(waterMap);
    TextureData colorData = calculateColorMap(tempMap, humidityMap, waterMap);
    

    auto createTex = [&](SDL_Texture*& tex) {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, TEX_W, TEX_H);
    };

    createTex(terrainTex);
    createTex(riverTex);
    createTex(climateTex);
    createTex(waterTex);
    createTex(shadingTex);
    createTex(colorTex);

    SDL_UpdateTexture(terrainTex, NULL, texData.pixels.data(), TEX_W * 4);
    SDL_UpdateTexture(riverTex, NULL, riverTexData.pixels.data(), TEX_W * 4);
    SDL_UpdateTexture(waterTex, NULL, waterData.pixels.data(), TEX_W * 4);
    SDL_UpdateTexture(climateTex, NULL, climateData.pixels.data(), TEX_W * 4);
    SDL_UpdateTexture(shadingTex, NULL, shadingData.pixels.data(), TEX_W * 4);
    SDL_UpdateTexture(colorTex, NULL, colorData.pixels.data(), TEX_W * 4);


    auto setModes = [&](SDL_Texture* tex) {
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    };

    setModes(terrainTex);
    setModes(riverTex);
    setModes(climateTex);
    setModes(waterTex);
    setModes(shadingTex);
    setModes(colorTex);
}


static int Init()
{   
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL_Init() Error: %s", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow("Rectangle Demo", 800, 800, SDL_WINDOW_HIDDEN);
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
    initMapTextures();
    rect.w = TEX_W;
    rect.h = TEX_H;
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
    if (g_showColorTex && colorTex) {
        SDL_RenderTexture(renderer, colorTex, NULL, &dst);
    }
    // Overlay shading texture on top of rectangle, if enabled
    if (g_showShadingTex && shadingTex) {
        SDL_RenderTexture(renderer, shadingTex, NULL, &dst);
    }
    if (g_showRiverTex && riverTex) {
        SDL_RenderTexture(renderer, riverTex, NULL, &dst);
    }
    if (g_showClimateTex && climateTex) {
        SDL_RenderTexture(renderer, climateTex, NULL, &dst);
    }
    if (g_showWaterTex && waterTex) {
        SDL_RenderTexture(renderer, waterTex, NULL, &dst);
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
                    {SDL_SCANCODE_5, &g_showWaterTex},
                    {SDL_SCANCODE_7, &g_showColorTex},
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
