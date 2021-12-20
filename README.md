# C++ Log Library
```c++
#include <ex/log.h>
#include <iostream>
#include <thread>

int main() {
  ex::log::init("test.log");
  ex::log::level = ex::log::Level::info;
  ex::log::rotate_max_size = 1024 * 10;
  ex::log::rotate_retain = 5;
  ex::log::print_file_line = false;
  ex::log::rotate_compress = true;
  ex::log::start();

  for (int i = 0; i < 10; ++i) {
    std::thread([]() {
      for (int j = 0;; ++j) {
        Logi(j);
        std::this_thread::sleep_for(10ms);
      }
    }).detach();
  }

  for (int i = 0; i < 500; ++i) {
    std::this_thread::sleep_for(10ms);
    ex::log::flush();
    std::cout << "flush: " << i << std::endl;
  }
  ex::log::close();
  std::this_thread::sleep_for(1s);
  return 0;
}
```