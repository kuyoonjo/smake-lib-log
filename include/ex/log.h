#pragma once

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <ex/safe_queue.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

#define Logt(msg)                                                              \
  if (ex::log::level < 1)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 0, msg)
#define Logd(msg)                                                              \
  if (ex::log::level < 2)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 1, msg)
#define Logi(msg)                                                              \
  if (ex::log::level < 3)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 2, msg)
#define Logw(msg)                                                              \
  if (ex::log::level < 4)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 3, msg)
#define Loge(msg)                                                              \
  if (ex::log::level < 5)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 4, msg)

namespace ex {
class log {
  using sig_handler_t = void (*)(int);

private:
  static inline std::filesystem::directory_entry sm_logfile;
  static inline std::uintmax_t sm_logfile_size;
  static inline std::ofstream sm_os;
  static inline std::map<int, sig_handler_t> sm_old_handlers;
  static inline std::string levels[] = {"TRACE", "DEBUG", "INFO", "WARN",
                                        "ERROR"};

  static inline ex::safe_queue<std::string> sm_queue;

  template <typename... Args>
  static void _replace(std::string &s, const std::string &regex,
                       Args &&...args) {
    s = std::regex_replace(s, std::regex(regex), std::forward<Args>(args)...);
  }

  static std::string _rotate_datetime() {

    auto now = std::chrono::system_clock::now();
    std::time_t dt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[32] = {0};
    std::strftime(buf, sizeof(buf), "%y%m%d.%H%M%S.", std::localtime(&dt));

    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto fraction = now - seconds;
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
    std::ostringstream oss;
    oss << buf << std::setfill('0') << std::setw(3) << milliseconds.count();

    return oss.str();
  }

  static std::set<std::filesystem::path> _rotated_files() {
    auto log_path = sm_logfile.path();
    auto log_ext = log_path.extension().string();
    auto log_dir = log_path.parent_path();
    auto log_stem_reg_str = log_path.stem().string();
    _replace(log_stem_reg_str, "\\.", "\\.");
    _replace(log_ext, "\\.", "\\.");
    std::stringstream log_reg_ss;
    log_reg_ss << log_stem_reg_str << "\\.\\d{6}\\.\\d{6}\\.\\d{3}" << log_ext;
    if (rotate_compress) {
      auto ext = rotate_compress_ext;
      _replace(ext, "\\.", "\\.");
      log_reg_ss << ext;
    }
    std::regex log_reg(log_reg_ss.str());
    std::set<std::filesystem::path> logs;
    for (auto de : std::filesystem::directory_iterator(log_dir)) {
      auto p = de.path();
      if (std::regex_match(p.filename().string(), log_reg))
        logs.insert(p);
    }
    return logs;
  }

  static void _rotate_retain() {
    auto rfs = _rotated_files();
    while (rfs.size() > rotate_retain) {
      auto rf = rfs.begin();
      auto removed = std::filesystem::remove(*rf);
      if (!removed)
        break;
      rfs.erase(rf);
    }
  }

  static void _rotate_compress(std::filesystem::path rotated_path) {
    auto res = rotate_compress_cmd(rotated_path);
    if (res) {
      Logi(rotated_path.string() + " compressed.");
    } else {
      Loge(std::string("Failed to compress " + rotated_path.string()));
    }
  }

  static void _rotate_if_needed(bool force = false) {
    if (force || sm_logfile_size > rotate_max_size) {
      sm_os.close();

      // rename & compress
      auto log_path = sm_logfile.path();
      auto log_ext = log_path.extension();
      std::stringstream rotated_filename_stream;
      rotated_filename_stream << log_path.stem().string() << '.'
                              << _rotate_datetime() << log_ext.string();
      auto rotated_path =
          log_path.parent_path() / rotated_filename_stream.str();
      std::filesystem::rename(log_path, rotated_path);
      sm_os = std::ofstream(sm_logfile.path(), std::ios::app);
      sm_logfile_size = 0;

      _rotate_retain();

      if (rotate_compress)
        std::thread(_rotate_compress, rotated_path).detach();
    }
  }

public:
  static void init(std::filesystem::path logfile_path) {
    sm_logfile = std::filesystem::directory_entry(logfile_path);
    sm_logfile_size =
        std::filesystem::exists(logfile_path) ? sm_logfile.file_size() : 0;
    if (sm_logfile.path().is_relative())
      sm_logfile = std::filesystem::directory_entry(
          std::filesystem::absolute(sm_logfile.path()));
    sm_os = std::ofstream(sm_logfile.path(), std::ios::app);
  }

  static void start() {
    std::thread([] {
      std::stringstream ss;
      for (;;) {
        auto msg = sm_queue.dequeue();
        if (msg == "FLUSH" || msg == "CLOSE") {
          sm_os << ss.str();
          sm_logfile_size += ss.tellp();
          ss.str("");
          sm_os << std::flush;
          if (rotate)
            _rotate_if_needed();
          if (msg == "CLOSE")
            break;
          else
            continue;
        }
        if (msg == "ROTATE") {
          if (rotate)
            _rotate_if_needed(true);
          continue;
        }
        ss << msg;
      }
    }).detach();
  }

  static std::string datetime() {

    auto now = std::chrono::system_clock::now();
    std::time_t dt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    dt += time_diff;

    char buf[32] = {0};
    std::strftime(buf, sizeof(buf), "%F %T.", std::localtime(&dt));
    // return buf;
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto fraction = now - seconds;
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
    std::ostringstream oss;
    oss << buf << std::setfill('0') << std::setw(3) << milliseconds.count();

    return oss.str();
  }

  static void flush() { sm_queue.enqueue("FLUSH"); }
  static void force_rotate() { sm_queue.enqueue("ROTATE"); }

  static void close() { sm_queue.enqueue("CLOSE"); }

  enum Level { trace = 0, debug = 1, info = 2, warn = 3, error = 4 };
  static inline Level level = Level::info;
  static inline std::uintmax_t rotate_max_size = 1024 * 1024 * 1024;
  static inline uint16_t rotate_retain = 30;
  static inline bool rotate_compress = true;
  static inline bool rotate = true;
  static inline bool print_file_line = true;
  static inline std::atomic_int32_t time_diff = 0;

  static inline std::string rotate_compress_ext = ".gz";
  static inline std::function<bool(const std::filesystem::path &)>
      rotate_compress_cmd = [](const std::filesystem::path &log_path) {
        std::stringstream ss;
        ss << "gzip -f " << log_path.string();
        auto errcode = system(ss.str().c_str());
        return !errcode;
      };

  template <typename T>
  static void write_log_with_header(const char *__file__, int __line__,
                                    int __level__, T &&msg) {
    std::stringstream ss;
    ss << '[' << datetime();
    if (print_file_line)
      ss << " " << __file__ << ":" << __line__;
    ss << " " << levels[__level__] << "] " << msg << std::endl;
    sm_queue.enqueue(ss.str());
  }
};
} // namespace ex
