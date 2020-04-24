
#include <memory>

#include <unistd.h>
#include <png++/png.hpp>
#include "zmqpp/zmqpp.hpp"


/**
 * This program expects a number in milliseconds, denoting a rate at which
 * images will be produced.
 */

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "USAGE: " << argv[0] << " rate_ms" << std::endl;
    return -1;
  }

  const auto rate_ms = std::atoi(argv[1]);
  if (rate_ms == 0) {
    std::cerr << "Could not convert argument to int" << std::endl;
    std::cout << "USAGE: " << argv[0] << " rate_ms" << std::endl;
    return -1;
  }

  png::image<png::rgb_pixel> image{"344.png"};
  const uint32_t width = image.get_width();
  const uint32_t height = image.get_height();

  std::cout << "width: " << width << std::endl
    << "height: " << height << std::endl;

  zmqpp::context context{};
  zmqpp::socket publisher{context, zmqpp::socket_type::publish};
  publisher.bind("tcp://*:9000");

  // Only worry about RGB (drop the A)
  std::vector<uint8_t> pixbuf{};
  pixbuf.reserve(width * height * 3);

  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      uint32_t i = y * width + x;
      pixbuf.push_back(image[y][x].red);
      pixbuf.push_back(image[y][x].blue);
      pixbuf.push_back(image[y][x].green);
    }
  }

  std::cout << "pixbuf size: " << pixbuf.size() << std::endl;

  // zmqpp::message image_msg{};
  // image_msg << pixbuf.data();

  for (int i = 0; i < 300; i++) {
    zmqpp::message image_msg{};
    image_msg << "PNG ";
    image_msg.add_raw(pixbuf.data(), pixbuf.size());
    publisher.send(image_msg);
    usleep(rate_ms * 1000);
  }
}

