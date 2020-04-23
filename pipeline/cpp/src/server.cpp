
#include <png++/png.hpp>
#include "zmqpp/zmqpp.hpp"

/**
 * This program expects a number in milliseconds, denoting a rate at which
 * images will be produced.
 */

int main(int argc, char *argv[]) {
  png::image<png::rgb_pixel> image{"344.png"};
  uint32_t width = image.get_width();
  uint32_t height = image.get_height();

  zmqpp::context context{};
  zmqpp::socket publisher{context, zqmpp::socket_type::publish};
  publisher.bind("tcp://*:9000");

}

