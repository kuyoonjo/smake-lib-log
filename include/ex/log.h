#pragma once

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

namespace ex {
class log {
private:
  static inline std::shared_ptr<log> s_log = nullptr;

public:
  static void init() {
    if (s_log)
      return;
    s_log = std::make_shared<log>();
    std::atexit([]() { std::cout << "atexit" << std::endl; });
    std::set_terminate([]() { std::cout << "set_terminate" << std::endl; });
  }
};
} // namespace ex