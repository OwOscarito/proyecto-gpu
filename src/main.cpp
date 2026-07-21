#define STB_IMAGE_IMPLEMENTATION

#include <CL/cl.h>
#include <stb_image.h>

#include <argparse/argparse.hpp>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "ascii/ascii_art.hpp"
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

  std::string runner = parser.get<std::string>("runner");
  for (auto &c : runner) {
    c = std::tolower(static_cast<unsigned char>(c));
  }

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
  if (runner == "opencl") {
    size_t image_size = static_cast<size_t>(image_width) *
                        static_cast<size_t>(image_height) * 3 *
                        sizeof(cl_uchar);

    int zones_width = (image_width + block_width - 1) / block_width;
    int zones_height = (image_height + block_height - 1) / block_height;
    size_t zones_size =
        static_cast<size_t>(zones_width) * static_cast<size_t>(zones_height);

    const auto init_start = std::chrono::steady_clock::now();
    ClRunner cl;
    cl.init();
    const auto init_end = std::chrono::steady_clock::now();

    const auto kernel_start = std::chrono::steady_clock::now();

    auto [ascii_kernel] = cl.load_program("./kernel/ascii-gray.cl", "ascii");

    const auto kernel_end = std::chrono::steady_clock::now();
    const auto processing_start = std::chrono::steady_clock::now();

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

    AsciiArt ascii;
    ascii.characters =
        cl.read_buffer<char>(ascii_buffer, zones_size * sizeof(char));
    ascii.width = zones_width;
    ascii.height = zones_height;

    const auto processing_end = std::chrono::steady_clock::now();
    const double init_ms =
        std::chrono::duration<double, std::milli>(init_end - init_start)
            .count();
    const double kernel_ms =
        std::chrono::duration<double, std::milli>(kernel_end - kernel_start)
            .count();
    const double processing_ms = std::chrono::duration<double, std::milli>(
                                     processing_end - processing_start)
                                     .count();
    const double total_ms = init_ms + kernel_ms + processing_ms;

    std::cout << ascii;

    std::cerr << "OpenCL init time: " << init_ms << " ms\n";
    std::cerr << "OpenCL setup time: " << kernel_ms << " ms\n";
    std::cerr << "OpenCL processing time: " << processing_ms << " ms\n";
    std::cerr << "OpenCL total time: " << total_ms << " ms\n";
  }

  /*
   * Sequential CPU implementation.
   */
  else if (runner == "CPU" || runner == "cpu") {
    const auto start = std::chrono::steady_clock::now();

    AsciiArt ascii =
        run_cpu(image, image_width, image_height, block_width, block_height);

    const auto end = std::chrono::steady_clock::now();
    const double elapsed_ms =
        std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << ascii;

    std::cerr << "CPU processing time: " << elapsed_ms << " ms\n";
  }

  else {
    std::cerr << "Invalid runner: " << runner << '\n' << "Use OpenCL or CPU\n";

    stbi_image_free(image);
    return 1;
  }

  stbi_image_free(image);

  return 0;
}
