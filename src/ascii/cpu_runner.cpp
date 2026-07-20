#include "cpu_runner.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "ascii_art.hpp"

namespace {

// Debe ser exactamente la misma escala utilizada en el kernel.
constexpr char ASCII_RAMP[] = " .:-=+*#%@";

constexpr int ASCII_RAMP_SIZE = static_cast<int>(sizeof(ASCII_RAMP) - 1);

float calculate_luminance(unsigned char red,
                          unsigned char green,
                          unsigned char blue) {
  return 0.299f * static_cast<float>(red) + 0.587f * static_cast<float>(green) +
         0.114f * static_cast<float>(blue);
}

}  // namespace

AsciiArt run_cpu(const unsigned char *image,
                 int image_width,
                 int image_height,
                 int block_width,
                 int block_height) {
  if (image == nullptr) {
    throw std::invalid_argument("Image pointer cannot be null");
  }

  if (image_width <= 0 || image_height <= 0) {
    throw std::invalid_argument("Image dimensions must be positive");
  }

  if (block_width <= 0 || block_height <= 0) {
    throw std::invalid_argument("Block dimensions must be positive");
  }

  AsciiArt result;

  result.width = (image_width + block_width - 1) / block_width;
  result.height = (image_height + block_height - 1) / block_height;

  const std::size_t zones_size = static_cast<std::size_t>(result.width) *
                                 static_cast<std::size_t>(result.height);

  /*
   * Este vector representa el mismo buffer intermedio
   * zones_buffer que utilizas en OpenCL.
   */
  std::vector<float> zones(zones_size, 0.0f);

  /*
   * Primera etapa:
   * calcular la luminosidad promedio de cada bloque.
   */
  for (int zone_y = 0; zone_y < result.height; ++zone_y) {
    for (int zone_x = 0; zone_x < result.width; ++zone_x) {
      float luminance_sum = 0.0f;
      int pixel_count = 0;

      const int start_x = zone_x * block_width;
      const int start_y = zone_y * block_height;

      for (int offset_y = 0; offset_y < block_height; ++offset_y) {
        const int pixel_y = start_y + offset_y;

        if (pixel_y >= image_height) {
          break;
        }

        for (int offset_x = 0; offset_x < block_width; ++offset_x) {
          const int pixel_x = start_x + offset_x;

          if (pixel_x >= image_width) {
            break;
          }

          const std::size_t pixel_index =
              (static_cast<std::size_t>(pixel_y) *
                   static_cast<std::size_t>(image_width) +
               static_cast<std::size_t>(pixel_x)) *
              3;

          const unsigned char red = image[pixel_index];
          const unsigned char green = image[pixel_index + 1];
          const unsigned char blue = image[pixel_index + 2];

          luminance_sum += calculate_luminance(red, green, blue);

          ++pixel_count;
        }
      }

      const std::size_t zone_index =
          static_cast<std::size_t>(zone_y) *
              static_cast<std::size_t>(result.width) +
          static_cast<std::size_t>(zone_x);

      if (pixel_count > 0) {
        zones[zone_index] = luminance_sum / static_cast<float>(pixel_count);
      }
    }
  }

  result.characters.resize(zones_size);

  for (std::size_t index = 0; index < zones_size; ++index) {
    const float brightness = zones[index];

    int character_index = static_cast<int>(
        brightness / 255.0f * static_cast<float>(ASCII_RAMP_SIZE - 1));

    character_index = std::clamp(character_index, 0, ASCII_RAMP_SIZE - 1);

    result.characters[index] = ASCII_RAMP[character_index];
  }

  return result;
}
