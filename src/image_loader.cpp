#include "image_loader.hpp"

#include <CImg.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

using cimg_library::CImg;

Image loadImage(const std::string& path) {
    CImg<unsigned char> source(path.c_str());

    if (source.is_empty()) {
        throw std::runtime_error("Could not load image: " + path);
    }

    Image result;
    result.width = source.width();
    result.height = source.height();
    result.channels = 3;

    const std::size_t pixelCount =
        static_cast<std::size_t>(result.width) *
        static_cast<std::size_t>(result.height);

    result.pixels.resize(pixelCount * 3);

    const int sourceChannels = source.spectrum();

    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            const std::size_t index =
                (static_cast<std::size_t>(y) * result.width + x) * 3;

            if (sourceChannels == 1) {
                const auto value = source(x, y, 0, 0);

                result.pixels[index + 0] = value;
                result.pixels[index + 1] = value;
                result.pixels[index + 2] = value;
            } else {
                result.pixels[index + 0] = source(x, y, 0, 0);
                result.pixels[index + 1] = source(x, y, 0, 1);
                result.pixels[index + 2] = source(x, y, 0, 2);
            }
        }
    }

    return result;
}
