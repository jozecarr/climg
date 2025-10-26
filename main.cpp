#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <clocale>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>    // strtof
#include "console_size.h"

#if defined(_WIN32)
  #include <windows.h>
#endif

struct pixel {
    int r, g, b;
};

const char* palette[] = { " ", u8"░", u8"▒", u8"▓", u8"█" };
constexpr int palLen = sizeof(palette) / sizeof(palette[0]);

static inline float srgbToLinear(float c) {
    return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

float calculateLuminance(pixel p) {
    float r = srgbToLinear(p.r / 255.0f);
    float g = srgbToLinear(p.g / 255.0f);
    float b = srgbToLinear(p.b / 255.0f);
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

pixel getPixel(int x, int y, const unsigned char* data, int width, int height, int stride) {
    pixel p{0,0,0};
    if (x < 0 || y < 0 || x >= width || y >= height) return p;

    const int idx = (y * width + x) * stride; // stride == 3
    p.r = data[idx + 0];
    p.g = data[idx + 1];
    p.b = data[idx + 2];
    return p;
}

int main(int argc, char** argv) {
    // args
    if (argc < 2) {
        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "climg") 
            << " <image_path> [exposure]\n"
            << "    exposure: optional float (default = 2.5)\n";
        return 2;
    }
    const char* image_path = argv[1];

    float exposure = 3.0f;
    if(argc >= 3) {
        char* end = nullptr;
        float v = std::strtof(argv[2], &end);
        if (end != argv[2] && v > 0.0f) {
            exposure = v;
        } else {std::cout << "Exposure must be float > 0 (e.g. \"1.2\", \"4\", default is 3)" << "\n";} 
    }
    
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, ".UTF-8");

    int width, height, actualChannels;
    const int requiredChannels = 3;
    unsigned char* data = stbi_load(image_path, &width, &height, &actualChannels, requiredChannels);

    if (!data) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }

    //console setup
    std::cout << "\n";
    ConsoleSize sz = get_console_size(); // sz.cols, sz.rows
    float aspect = 1.0f; // console characters aren't square (played around with this, works best as 1???)
    std::pair<float, float> scaling = {
        static_cast<float>(width)  / static_cast<float>(sz.cols),
        (static_cast<float>(height) / static_cast<float>(sz.rows)) * aspect 
    };


    //precompute
    std::vector<int> colStart(sz.cols), colEnd(sz.cols);
    std::vector<int> rowStart(sz.rows), rowEnd(sz.rows);

    for (int x = 0; x < sz.cols; ++x) {
        int x0 = (x * width)  / sz.cols;
        int x1 = ((x+1) * width)  / sz.cols;
        if (x1 <= x0) x1 = x0 + 1;
        colStart[x] = x0; colEnd[x] = x1;
    }
    int rowsAdj = sz.rows;
    for (int y = 0; y < sz.rows; ++y) {
        int y0 = (y * height) / rowsAdj;
        int y1 = ((y+1) * height) / rowsAdj;
        if (y1 <= y0) y1 = y0 + 1;
        rowStart[y] = y0; rowEnd[y] = y1;
    }

    std::vector<std::vector<float>> avgLums(sz.rows, std::vector<float>(sz.cols, 0.0f));
    
    //float baseInvArea = 1.0f / (scaling.first * scaling.second);

    // for each cell
    for (int y = 0; y < sz.rows; ++y) {
        const int y0 = rowStart[y], y1 = rowEnd[y];
        for (int x = 0; x < sz.cols; ++x) {
            const int x0 = colStart[x], x1 = colEnd[x];
            float avgLum = 0;

            // for each pixel within cell
            for (int iy = y0; iy < y1; ++iy) {
                for (int ix = x0; ix < x1; ++ix) {
                    avgLum += calculateLuminance(getPixel(ix, iy, data, width, height, requiredChannels));
                }
            }

            avgLum /= (y1-y0) * (x1-x0); 
            avgLums[y][x] = avgLum;
        }
    }    

    std::string out = "";
    out.reserve(sz.rows * sz.cols * 3); // pre-size

    // load output
    for (int y = 0; y < sz.rows; ++y) {
        for (int x = 0; x < sz.cols; ++x) {
            float lum = std::clamp(avgLums[y][x] * exposure, 0.0f, 1.0f);
            int idx = static_cast<int>(lum * (palLen - 1));
            out += palette[idx];
        }
    }

    //output
    std::cout << out;

    stbi_image_free(data);
    return 0;
}