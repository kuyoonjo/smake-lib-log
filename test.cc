#include <chrono>
#include <ex/log.h>
#include <ex/onexit.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

namespace ex {
class log {
  using sig_handler_t = void (*)(int);

private:
  static inline std::filesystem::directory_entry sm_logfile;
  static inline std::ofstream sm_os;
  static inline std::uintmax_t sm_max_size;
  static inline std::map<int, sig_handler_t> sm_old_handlers;
  static inline std::map<std::thread::id, std::ostringstream> sm_streams;
  static inline std::map<std::thread::id, uint16_t> sm_short_thread_ids;
  static inline std::string levels[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

  static void rotate_if_needed() {
    if (sm_logfile.file_size() > sm_max_size) {
      sm_os.close();
      // rename & compress
      sm_os = std::ofstream(sm_logfile.path(), std::ios::app);
    }
  }

public:
  static void init(std::filesystem::path logfile_path) {
    sm_logfile = std::filesystem::directory_entry(logfile_path);
    if (sm_logfile.path().is_relative())
      sm_logfile = std::filesystem::directory_entry(
          std::filesystem::absolute(sm_logfile.path()));
          std::cout << "Log path: " << sm_logfile.path() << std::endl;
    sm_os = std::ofstream(sm_logfile.path(), std::ios::app);
  }

  static uint16_t short_thread_id() {
      auto id = std::this_thread::get_id();
    auto ex = sm_short_thread_ids.find(id);
    if(ex == sm_short_thread_ids.end()) {
      auto res = sm_short_thread_ids.emplace(id, sm_short_thread_ids.size());
      ex = res.first;
    }
    return ex->second;
  }

  static std::string datetime()
{



    auto now = std::chrono::system_clock::now();
    std::time_t dt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[32] = {0};
    std::strftime(buf, sizeof(buf), " %F %T.", std::localtime(&dt));
    // return buf;
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto fraction = now - seconds;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
    std::ostringstream oss;
    oss << buf << std::setfill('0') << std::setw(3) << milliseconds.count() << " ";

    return oss.str();
}

  static void poll() {
    for(auto& [key, value] : sm_streams) {
      sm_os << value.str();
      value.str("");
    }
    sm_os << std::flush;
    rotate_if_needed();
  }

  static void close() {
    poll();
    sm_os.close();
  }

  static std::ostringstream& stream() {
      auto id = std::this_thread::get_id();
      auto ex = sm_streams.find(id);
      if(ex == sm_streams.end()) {
        auto res = sm_streams.emplace(id, std::ostringstream());
        ex = res.first;
      }
      return ex->second;
  }

  enum Level {
    debug = 0,
    info = 1,
    warn = 2,
    error = 3,
    fatal = 4
  };
  static inline Level level = Level::warn;

  static std::ostringstream& generate_log_header(const char* __file__, int __line__, int __level__) {
    auto& s = stream();
    s << '[' << levels[__level__] << datetime() <<std::setfill('0') << std::setw(2) <<  short_thread_id() << " " << __FILE__ << ":" << __LINE__  << "] ";
    return s;
  }
};
} // namespace ex

#define Logd if (ex::log::level < 1) ex::log::generate_log_header(__FILE__, __LINE__, 0)
#define Logi if (ex::log::level < 2) ex::log::generate_log_header(__FILE__, __LINE__, 1)
#define Logw if (ex::log::level < 3) ex::log::generate_log_header(__FILE__, __LINE__, 2)
#define Loge if (ex::log::level < 4) ex::log::generate_log_header(__FILE__, __LINE__, 3)
#define Logf if (ex::log::level < 5) ex::log::generate_log_header(__FILE__, __LINE__, 4)

int main() {
  ex::log::init("test.log");
  ex::log::level = ex::log::Level::info;
  // ex::log::nocolors = false;
  // ex::log::console = false;

  for (int i = 0; i < 10; ++i) {
  Logi << i << std::endl;
  ex::log::poll();
  std::this_thread::sleep_for(1s);
  }
  ex::log::close();
  return 0;
}