#include "ascii_art.hpp"

#include <cstddef>

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
