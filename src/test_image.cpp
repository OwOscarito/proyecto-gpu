#include "image_loader.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        Image image = loadImage(CAT_IMAGE_PATH);

        std::cout << "Image loaded successfully\n";
        std::cout << "Width: " << image.width << '\n';
        std::cout << "Height: " << image.height << '\n';
        std::cout << "Bytes: " << image.pixels.size() << '\n';

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error loading image: "
                  << error.what() << '\n';

        return 1;
    }
}
