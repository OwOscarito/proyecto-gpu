#pragma once

#include "ascii_art.hpp"

AsciiArt run_cpu(const unsigned char *image,
                 int image_width,
                 int image_height,
                 int block_width,
                 int block_height);
