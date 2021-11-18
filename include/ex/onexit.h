#pragma once

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>

namespace ex {
namespace os {
using sig_handler_t = void (*)(int);
class onexit {
private:
  static inline sig_handler_t sm_handler = nullptr;
  static inline std::map<int, sig_handler_t> sm_old_handlers;

  static void on_signal(int sig) {
    std::cout << "SIG: " << sig << std::endl;
    sm_handler(sig);
    std::signal(sig, sm_old_handlers[sig]);
    std::raise(sig);
  }
  static void on_failure(int sig) {
    std::cout << "Failure: " << sig << std::endl;
    sm_handler(sig);
    std::signal(sig, sm_old_handlers[sig]);
    std::raise(sig);
  }

#ifdef _WIN32
#else
  static sig_handler_t signal(int sig, sig_handler_t handler, int flag = 0) {
    struct sigaction sa, old;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    if (flag > 0)
      sa.sa_flags = flag;
    sa.sa_handler = handler;
    int r = sigaction(sig, &sa, &old);
    return r == 0 ? old.sa_handler : SIG_ERR;
  }
#endif
public:
  static void init(sig_handler_t handler) {
    sm_handler = handler;
    std::atexit([]() { std::cout << "atexit" << std::endl; });
    for (auto sig : {SIGINT, SIGTERM})
      sm_old_handlers[sig] = signal(sig, on_signal);

#ifdef _WIN32
#else
    for (auto sig : {SIGQUIT})
      sm_old_handlers[sig] = signal(sig, on_signal);
    constexpr int flag = SA_SIGINFO | SA_ONSTACK | SA_NODEFER | SA_RESETHAND;
    for (auto sig : {
             // Signals for which the default action is "Core".
             SIGABRT, // Abort signal from abort(3)
             SIGBUS,  // Bus error (bad memory access)
             SIGFPE,  // Floating point exception
             SIGILL,  // Illegal Instruction
             SIGIOT,  // IOT trap. A synonym for SIGABRT
             SIGQUIT, // Quit from keyboard
             SIGSEGV, // Invalid memory reference
             SIGSYS,  // Bad argument to routine (SVr4)
             SIGTRAP, // Trace/breakpoint trap
             SIGXCPU, // CPU time limit exceeded (4.2BSD)
             SIGXFSZ, // File size limit exceeded (4.2BSD)
#ifdef __APPLE__
             SIGEMT,  // emulation instruction executed
#endif
         })
      sm_old_handlers[sig] = signal(sig, on_failure, flag);
    signal(SIGPIPE, SIG_IGN);
#endif
  }
};
} // namespace os
} // namespace ex