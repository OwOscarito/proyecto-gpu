#include "ascii_art.hpp"

std::ostream &operator<<(std::ostream &os, const AsciiArt &ascii) {
  for (int y = 0; y < ascii.height; ++y) {
    for (int x = 0; x < ascii.width; ++x) {
      os << ascii.characters[static_cast<size_t>(y) *
                                 static_cast<size_t>(ascii.width) +
                             static_cast<size_t>(x)];
    }

    os << '\n';
  }
  return os;
}
