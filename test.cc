#include <ex/log.h>
#include <stdexcept>
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

int main() {
  ex::log::init();
  // ex::log::nocolors = false;
  // ex::log::console = false;
  // ex::log::i << "info" << std::endl;
  // while (true) {
  // std::this_thread::sleep_for(1s);
  // std::cout << "tick" << std::endl;
  // }
  throw std::runtime_error("");
    return 0;
}