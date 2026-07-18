#define STB_IMAGE_IMPLEMENTATION

#include <CL/cl.h>
#include <stb_image.h>

#include <argparse/argparse.hpp>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "ascii/cl_runner.hpp"

namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  argparse::ArgumentParser parser("ascii",
                                  "OpenCl image to ascii transformation");

  parser.add_argument("image-path")
      .help("Path to load image")
      .default_value(std::string("image.jpg"));
  parser.add_argument("-r", "--runner")
      .help("Runner to process image")
      .default_value(std::string("OpenCL"));
  //parser.add_argument("-w", "--width")
  //    .help("Width of the output ascii")
  //    .default_value(0);
  //parser.add_argument("-h", "--height")
  //    .help("Height of the output ascii")
  //    .default_value(0);
  parser.add_argument("-W", "--block-width")
      .help("Width in pixels of each image block")
      .scan<'i', int>()
      .default_value(4);
  parser.add_argument("-H", "--block-height")
      .help("Height in pixels of each image block")
      .scan<'i', int>()
      .default_value(4);

  try {
    parser.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << "\n";
    return 1;
  }

  std::string image_path_string = parser.get<std::string>("image-path");

  //int output_width = parser.get<int>("width");
  //int output_height = parser.get<int>("height");

  const int BLOCK_W = parser.get<int>("block-width");
  const int BLOCK_H = parser.get<int>("block-height");

  fs::path image_path(image_path_string);
  int image_width, image_height;
  unsigned char *image =
      stbi_load(image_path.c_str(), &image_width, &image_height, nullptr, 3);

  if (!image) throw std::runtime_error("Failed to load image");

  size_t image_size = image_width * image_height * 3 * sizeof(cl_uchar);

  int zones_width = (image_width + BLOCK_W - 1) / BLOCK_W;
  int zones_height = (image_height + BLOCK_H - 1) / BLOCK_H;
  size_t zones_size = zones_width * zones_height;

  ClRunner cl;
  cl.init();

  auto [divide_kernel, ascii_kernel] =
      cl.load_program("./kernel/ascii-gray.cl", "zones", "ascii");

  cl_mem image_buffer = cl.create_buffer(
      image_size, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image);

stbi_image_free(image);
  cl_mem zones_buffer =
      cl.create_buffer(zones_size * sizeof(float), CL_MEM_READ_WRITE);

  cl_mem ascii_buffer =
      cl.create_buffer(zones_size * sizeof(cl_uchar), CL_MEM_WRITE_ONLY);

  cl.set_argument(divide_kernel, 0, image_buffer);
  cl.set_argument(divide_kernel, 1, zones_buffer);
  cl.set_argument(divide_kernel, 2, image_width);
  cl.set_argument(divide_kernel, 3, image_height);
  cl.set_argument(divide_kernel, 4, BLOCK_W);
  cl.set_argument(divide_kernel, 5, BLOCK_H);

  cl.set_argument(ascii_kernel, 0, zones_buffer);
  cl.set_argument(ascii_kernel, 1, ascii_buffer);
  cl.set_argument(ascii_kernel, 2, (int)zones_size);

  size_t local_w = 16;
  size_t local_h = 16;
  size_t global_w = ((zones_width + local_w - 1) / local_w) * local_w;
  size_t global_h = ((zones_height + local_h - 1) / local_h) * local_h;

  size_t local_2d[2] = {local_w, local_h};
  size_t global_2d[2] = {global_w, global_h};

  size_t local_1d = 256;
  // Round up to nearest multiple of 256
  size_t global_1d = ((zones_size + local_1d - 1) / local_1d) * local_1d;

  cl.run_kernel(divide_kernel, global_2d, local_2d, 2);
  cl.run_kernel(ascii_kernel, &global_1d, &local_1d);

  auto ascii = cl.read_buffer<char>(ascii_buffer, zones_size * sizeof(char));

  // Print as a grid
  for (int y = 0; y < zones_height; ++y) {
    for (int x = 0; x < zones_width; ++x) {
      std::cout << ascii[y * zones_width + x];
    }
    std::cout << "\n";
  }
}
