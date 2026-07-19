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
