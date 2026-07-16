#include <CImg.h>

#include <argparse/argparse.hpp>
#include <filesystem>
#include <iostream>
#include <string>

using namespace cimg_library;
namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
  argparse::ArgumentParser parser("ascii",
                                  "OpenCl image to ascii transformation");

  parser.add_argument("image-path").help("Path to load image").required();
  parser.add_argument("-r", "--runner")
      .help("Runner to process image")
      .default_value(std::string("OpenCL"));

  try {
    parser.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << "\n";
    return 1;
  }

  fs::path image_path(parser.get<std::string>("image-name"));
}
