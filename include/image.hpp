// include/image.hpp

#pragma once

#include <cstdint>
#include <vector>

struct Image {
    int width = 0;
    int height = 0;
    int channels = 3;

    // R G B R G B R G B
    std::vector<std::uint8_t> pixels;
};
