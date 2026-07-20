#include "ascii_art.hpp"

#include <cstddef>

std::ostream &operator<<(std::ostream &os, const ColorAsciiArt &ascii) {
  for (int y = 0; y < ascii.height; y++) {
    for (int x = 0; x < ascii.width; x++) {
      size_t zi = y * ascii.width + x;
      unsigned char r = ascii.colors[zi * 3];
      unsigned char g = ascii.colors[zi * 3 + 1];
      unsigned char b = ascii.colors[zi * 3 + 2];
      os << "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" +
                std::to_string(b) + "m";
      os << ascii.characters[zi];
    }
    os << "\033[0m\n";  // reset at end of each line
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const AsciiArt &ascii) {
  for (int y = 0; y < ascii.height; ++y) {
    for (int x = 0; x < ascii.width; ++x) {
      size_t zi = y * ascii.width + x;
      os << ascii.characters[zi];
    }
    os << '\n';
  }
  return os;
}
