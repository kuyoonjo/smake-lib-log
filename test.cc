#include <ex/log.h>
#include <istream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <vector>
using namespace std::chrono_literals;

struct A {
  virtual ~A() {
    std::cout << "a gone." << std::endl;
  }
};
int main() {
  ex::log::init();
  // ex::log::nocolors = false;
  // ex::log::console = false;
  // ex::log::i << "info" << std::endl;
  std::vector<int> *p;
  A a;

    // std::stringstream ss;
    // ss << "123";
    // std::cout << ss.rdbuf() << std::endl;
    // ss << "456";
    // std::cout << ss.rdbuf() << std::endl;
    // std::cout << ss.rdbuf() << std::endl;
  while (true) {
  std::this_thread::sleep_for(1s);
  std::cout << "tick 1" << std::endl;
  // throw std::runtime_error("");
  std::cout << p->size() << std::endl;
  }
    return 0;
}