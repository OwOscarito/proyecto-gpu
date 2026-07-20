#pragma once
#include <ostream>
#include <vector>

class AsciiArt {
 public:
  std::vector<char> characters;
  int width;
  int height;
  friend std::ostream &operator<<(std::ostream &os, const AsciiArt &ascii);
};

class ColorAsciiArt {
 public:
  std::vector<char> characters;
  std::vector<char> colors;
  int width;
  int height;
  friend std::ostream &operator<<(std::ostream &os, const ColorAsciiArt &ascii);
};
