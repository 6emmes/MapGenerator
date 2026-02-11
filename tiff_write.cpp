#include <tiffio.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <climits>
#include <fstream>
#include "textures_core.h"


void saveTiff32(const std::vector<std::vector<std::vector<float>>*>& layers, std::vector<std::string> layerNames, const char *filename) {
    if (layers.empty()) {
        throw std::runtime_error("No layers to save.");
    }

    // Assume all layers have the same dimensions
    // Dereference the first pointer to get dimensions.  All layers
    // are assumed to be the same size.
    int height = (*layers[0]).size();
    int width  = (*layers[0])[0].size();
    int * buffer = new int[width];
    char * boolbuffer = new char[width];
    
    TIFF* tif = TIFFOpen(filename, "w");
    if (!tif) {
        throw std::runtime_error("Failed to open TIFF file for writing.");
    }

    for (int layer = 0; layer < layers.size(); ++layer) {
        // Start a new directory (layer/page)
        if (layer > 0) {
            TIFFCreateDirectory(tif);
            // if (!TIFFCreateDirectory(tif)) {
            //     TIFFClose(tif);
            //     throw std::runtime_error("Failed to create new TIFF directory.");
            // }
        }
        
        TIFFSetField(tif, TIFFTAG_PAGENAME, layerNames[layer].c_str());
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32); // float32
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE); // uncompressed

        // Write scanlines, dereferencing the pointer for each layer
        for (int row = 0; row < height; ++row) {
            for (int x = 0; x < width; ++x) {
                buffer[x] = int((*layers[layer])[row][x]*INT_MAX);
            }

            if (TIFFWriteScanline(tif, (void*)buffer, row, 0) < 0) {
                TIFFClose(tif);
                throw std::runtime_error("Failed to write scanline.");
            }
        }

        TIFFWriteDirectory(tif); // finalize this layer
    }
    delete boolbuffer;
    delete buffer;
    TIFFClose(tif);
}

void saveTiff8(int lat_top, int lat_bot, const std::vector<std::vector<std::vector<float>>*>& layers, std::vector<std::string> layerNames, const char *filename) {
    if (layers.empty()) {
        throw std::runtime_error("No layers to save.");
    }

    // Assume all layers have the same dimensions
    // Dereference the first pointer to get dimensions.  All layers
    // are assumed to be the same size.
    int height = (*layers[0]).size();
    int width  = (*layers[0])[0].size();
    uint8_t UNIT8_MAX = 255;
    uint8_t* buffer = new uint8_t[width];
    
    TIFF* tif = TIFFOpen(filename, "w");
    if (!tif) {
        throw std::runtime_error("Failed to open TIFF file for writing.");
    }

    for (int layer = 0; layer < layers.size(); ++layer) {
        // Start a new directory (layer/page)
        if (layer > 0) {
            TIFFCreateDirectory(tif);
            // if (!TIFFCreateDirectory(tif)) {
            //     TIFFClose(tif);
            //     throw std::runtime_error("Failed to create new TIFF directory.");
            // }
        }
        
        TIFFSetField(tif, TIFFTAG_PAGENAME, layerNames[layer].c_str());
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE); // uncompressed

        // Write scanlines, dereferencing the pointer for each layer
        for (int row = 0; row < height; ++row) {
            for (int x = 0; x < width; ++x) {
                buffer[x] = uint8_t((*layers[layer])[row][x]*UINT8_MAX);
            }

            if (TIFFWriteScanline(tif, (void*)buffer, row, 0) < 0) {
                TIFFClose(tif);
                throw std::runtime_error("Failed to write scanline.");
            }
        }

        TIFFWriteDirectory(tif); // finalize this layer
    }
    delete buffer;
    TIFFClose(tif);
    
    std::ofstream ofs(filename, std::ios::binary | std::ios::app);
    if (!ofs) {
        throw std::runtime_error("Failed to open file for appending byte.");
    }
    ofs.put(static_cast<char>(lat_top));
    ofs.put(static_cast<char>(lat_bot));
    ofs.close();
}

