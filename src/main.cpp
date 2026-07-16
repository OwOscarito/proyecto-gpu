#include <CImg.h>

#include <argparse/argparse.hpp>
#include <filesystem>
#include <iostream>
#include <string>

#include "ascii/cl_runner.h"

using namespace cimg_library;
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

  try {
    parser.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << "\n";
    return 1;
  }

  fs::path image_path(parser.get<std::string>("image-path"));

  ClRunner cl;
  cl.init();
  cl.setup_kernel("./kernel/mult.cl", "mult");

  const int size = 1024;
  std::vector<float> input(size, 5.0f);
  const float val = 0.5f;

  cl.set_buffer_argument(0,
                         "input",
                         size * sizeof(float),
                         CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                         input.data());
  cl.set_buffer_argument(1, "output", size * sizeof(float), CL_MEM_WRITE_ONLY);

  cl.set_argument(2, val);
  cl.set_argument(3, size);

  cl.run_kernel(size, 256);

  auto output = cl.read_buffer<float>("output");
  std::cout << "output(" + std::to_string(output.size()) + "): [\n";
  for (auto x : output) {
    std::cout << x << " ";
  }
  std::cout << "\n]";
}
