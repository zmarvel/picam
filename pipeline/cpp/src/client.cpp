

#include "zmqpp/zmqpp.hpp"

int main(int argc, char *argv[]) {
  zmqpp::context context{};

  zmqpp::socket subscriber{context, zmqpp::socket_type::subscribe};
  subscriber.connect("tcp://localhost:9000");
  subscriber.subscribe("PNG ");

  while (true) {
    zmqpp::message rcvd;
    subscriber.receive(rcvd);
    std::string filter, data;
    rcvd >> filter;
    rcvd >> data;
    std::cout << "got " << rcvd.parts() << " parts: " <<
      filter.size() << "/" << data.size() << std::endl;
    std::cout << filter << std::endl;
  }
}
