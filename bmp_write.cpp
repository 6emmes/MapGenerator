// Very small BMP writer that writes a 24‑bit uncompressed BMP.
// The function takes a heightmap with values in [0,1] and writes
// an RGB image where the grayscale value is derived from the height.

#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include "texture.h"


#pragma pack(push, 1)
struct BMPDIBHeader {
    uint32_t biSize = 40;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 24;
    uint32_t biCompression = 0; // BI_RGB
    uint32_t biSizeImage = 0;
    int32_t  biXPelsPerMeter = 0;
    int32_t  biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};

struct BMPHeader {
    uint16_t bfType = 0x4D42; // 'BM'
    uint32_t bfSize;
    uint16_t bfReserved1 = 0;
    uint16_t bfReserved2 = 0;
    uint32_t bfOffBits = 14 + 40;
};
#pragma pack(pop)

bool saveHeightMapBMP(const std::vector<std::vector<float>>& map, const char* filename) {
    if (map.empty() || map[0].empty()) return false;
    int width = map[0].size();
    int height = map.size();
    // BMP rows are padded to 4-byte boundaries.
    int rowStride = ((width * 3 + 3) / 4) * 4;
    BMPHeader h;
    BMPDIBHeader d;
    d.biWidth = width;
    d.biHeight = height;
    d.biSizeImage = rowStride * height;
    h.bfSize = h.bfOffBits + d.biSizeImage;

    FILE* f = fopen(filename, "wb");
    if (!f) return false;
    fwrite(&h, sizeof(h), 1, f);
    fwrite(&d, sizeof(d), 1, f);
    std::vector<uint8_t> row(rowStride);
    for (int y = height - 1; y >= 0; --y) { // BMP stores bottom‑up
        for (int x = 0; x < width; ++x) {
            float v = map[y][x];
            v = std::clamp(v, 0.0f, 1.0f);
            uint8_t gray = static_cast<uint8_t>(v * 255.0f + 0.5f);
            row[x * 3 + 0] = gray; // B
            row[x * 3 + 1] = gray; // G
            row[x * 3 + 2] = gray; // R
        }
        fwrite(row.data(), 1, rowStride, f);
    }
    fclose(f);
    return true;
}
