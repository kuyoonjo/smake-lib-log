#include <ex/log.h>
#include <iostream>

int main() {
  ex::log::init("test.log");
  ex::log::level = ex::log::Level::info;
  ex::log::rotate_max_size = 1024 * 1024 * 10;
  ex::log::rotate_retain = 5;

  for (int i = 0; i < 100; ++i) {
    std::thread([]() {
      ex::log::init();
      for (int j = 0;; ++j) {
        Logi(j);
        std::this_thread::sleep_for(1ms);
      }
    }).detach();
  }

  for (int i = 0; i < 500; ++i) {
    std::this_thread::sleep_for(10ms);
    ex::log::poll();
    std::cout << "poll" << i << std::endl;
  }
  ex::log::close();
  return 0;
}