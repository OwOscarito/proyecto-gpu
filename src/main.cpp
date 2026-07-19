#define STB_IMAGE_IMPLEMENTATION

#include <CL/cl.h>
#include <stb_image.h>

#include <argparse/argparse.hpp>
#include <cstddef>
#include <iostream>
#include <string>

#include "ascii/cl_runner.hpp"
#include "ascii/cpu_runner.hpp"

int main(int argc, char *argv[]) {
  argparse::ArgumentParser parser(
      "ascii", "OpenCL or CPU image to ASCII transformation");

  parser.add_argument("image-path")
      .help("Path to load image")
      .default_value(std::string("image.jpg"));

  parser.add_argument("-r", "--runner")
      .help("Runner to process image: OpenCL or CPU")
      .default_value(std::string("OpenCL"));

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
    std::cerr << err.what() << '\n';
    std::cerr << parser;
    return 1;
  }

  const std::string image_path = parser.get<std::string>("image-path");

  const std::string runner = parser.get<std::string>("runner");

  const int block_width = parser.get<int>("block-width");
  const int block_height = parser.get<int>("block-height");

  if (block_width <= 0 || block_height <= 0) {
    std::cerr << "Block width and height must be greater than zero\n";
    return 1;
  }

  int image_width = 0;
  int image_height = 0;

  unsigned char *image =
      stbi_load(image_path.c_str(), &image_width, &image_height, nullptr, 3);

  if (!image) {
    std::cerr << "Failed to load image: " << image_path << '\n';
    return 1;
  }

  /*
   * OpenCL implementation.
   */
  if (runner == "OpenCL" || runner == "opencl") {
    size_t image_size = static_cast<size_t>(image_width) *
                        static_cast<size_t>(image_height) * 3 *
                        sizeof(cl_uchar);

    int zones_width = (image_width + block_width - 1) / block_width;
    int zones_height = (image_height + block_height - 1) / block_height;
    size_t zones_size =
        static_cast<size_t>(zones_width) * static_cast<size_t>(zones_height);

    ClRunner cl;
    cl.init();

    auto [ascii_kernel] = cl.load_program("./kernel/ascii-gray2.cl", "ascii");

    cl_mem image_buffer = cl.create_buffer(
        image_size, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image);
    cl_mem ascii_buffer =
        cl.create_buffer(zones_size * sizeof(cl_uchar), CL_MEM_WRITE_ONLY);

    cl.set_argument(ascii_kernel, 0, image_buffer);
    cl.set_argument(ascii_kernel, 1, ascii_buffer);
    cl.set_argument(ascii_kernel, 2, image_width);
    cl.set_argument(ascii_kernel, 3, image_height);
    cl.set_argument(ascii_kernel, 4, block_width);
    cl.set_argument(ascii_kernel, 5, block_height);
    cl.set_argument(ascii_kernel, 6, zones_width);
    cl.set_argument(ascii_kernel, 7, zones_height);

    size_t local_w = 16;
    size_t local_h = 16;
    size_t global_w =
        (static_cast<size_t>(zones_width) + local_w - 1) / local_w * local_w;
    size_t global_h =
        (static_cast<size_t>(zones_height) + local_h - 1) / local_h * local_h;
    size_t local_2d[2] = {local_w, local_h};
    size_t global_2d[2] = {global_w, global_h};

    cl.run_kernel(ascii_kernel, global_2d, local_2d, 2);

    auto ascii = cl.read_buffer<char>(ascii_buffer, zones_size * sizeof(char));

    for (int y = 0; y < zones_height; ++y) {
      for (int x = 0; x < zones_width; ++x) {
        std::cout
            << ascii[static_cast<size_t>(y) * static_cast<size_t>(zones_width) +
                     static_cast<size_t>(x)];
      }

      std::cout << '\n';
    }
  }

  /*
   * Sequential CPU implementation.
   */
  else if (runner == "CPU" || runner == "cpu") {
    CpuAsciiResult result =
        run_cpu(image, image_width, image_height, block_width, block_height);

    for (int y = 0; y < result.height; ++y) {
      for (int x = 0; x < result.width; ++x) {
        std::cout << result.characters[static_cast<size_t>(y) *
                                           static_cast<size_t>(result.width) +
                                       static_cast<size_t>(x)];
      }

      std::cout << '\n';
    }
  }

  else {
    std::cerr << "Invalid runner: " << runner << '\n' << "Use OpenCL or CPU\n";

    stbi_image_free(image);
    return 1;
  }

  stbi_image_free(image);

  return 0;
}
