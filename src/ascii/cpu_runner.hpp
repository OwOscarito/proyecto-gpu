#pragma once

#include <vector>

struct CpuAsciiResult {
  int width;
  int height;
  std::vector<char> characters;
};

CpuAsciiResult run_cpu(const unsigned char *image,
                       int image_width,
                       int image_height,
                       int block_width,
                       int block_height);
