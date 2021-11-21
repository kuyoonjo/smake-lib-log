#pragma once

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <regex>
#include <set>
#include <map>
#include <string>
#include <thread>
#include <functional>
#include <ctime>
#include <sstream>

using namespace std::chrono_literals;

#define Logd(msg)                                                              \
  if (ex::log::level < 1)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 0, msg)
#define Logi(msg)                                                              \
  if (ex::log::level < 2)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 1, msg)
#define Logw(msg)                                                              \
  if (ex::log::level < 3)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 2, msg)
#define Loge(msg)                                                              \
  if (ex::log::level < 4)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 3, msg)
#define Logf(msg)                                                              \
  if (ex::log::level < 5)                                                      \
  ex::log::write_log_with_header(__FILE__, __LINE__, 4, msg)

namespace ex {
class log {
  using sig_handler_t = void (*)(int);

private:
  static inline std::filesystem::directory_entry sm_logfile;
  static inline std::ofstream sm_os;
  static inline std::map<int, sig_handler_t> sm_old_handlers;
  static inline std::string levels[] = {"DEBUG", "INFO", "WARN", "ERROR",
                                        "FATAL"};

  static inline std::mutex sm_rotate_compress_mtx;
  static inline bool sm_rotate_complete = true;

  struct thread {
    uint16_t short_id;
    std::ostringstream stream;
    std::mutex mutex;
    std::atomic_bool deleted = false;
  };
  static inline std::map<std::thread::id, std::shared_ptr<thread>> sm_threads;
  static inline std::mutex sm_main_mtx;

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
    auto log_ext = log_path.extension();
    auto log_dir = log_path.parent_path();
    auto log_stem_reg_str = log_path.stem().string();
    _replace(log_stem_reg_str, "\\.", ".");
    std::stringstream log_reg_ss;
    log_reg_ss << log_stem_reg_str << "\\.\\d{6}\\.\\d{6}\\.\\d{3}\\"
               << log_ext.string();
    if (rotate_compress)
      log_reg_ss << rotate_compress_ext;
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
    while (rfs.size() >= rotate_retain) {
      auto rf = rfs.begin();
      auto removed = std::filesystem::remove(*rf);
      if (!removed)
        break;
      rfs.erase(rf);
    }
  }

  static void _rotate_compress(std::filesystem::path rotated_path) {
    auto res = rotate_compress_cmd(rotated_path);
    init();
    if (res) {
      Logi(rotated_path.string() + " compressed.");
    } else {
      Loge(std::string("Failed to compress " + rotated_path.string()));
    }
    deinit();
  }

  static void _rotate_if_needed() {
    if (sm_logfile.file_size() > rotate_max_size) {
      sm_os.close();

      // rename & compress
      auto log_path = sm_logfile.path();
      auto log_ext = log_path.extension();
      std::stringstream rotated_filename_stream;
      rotated_filename_stream << log_path.stem().c_str() << '.'
                              << _rotate_datetime() << log_ext.c_str();
      auto rotated_path =
          log_path.parent_path() / rotated_filename_stream.str();
      std::filesystem::rename(log_path, rotated_path);
      sm_os = std::ofstream(sm_logfile.path(), std::ios::app);

      if (rotate_compress) {
        sm_rotate_complete = false;
        std::thread([rotated_path]() {
          std::lock_guard<std::mutex> lg(sm_rotate_compress_mtx);
          _rotate_compress(rotated_path);
          _rotate_retain();
          sm_rotate_complete = true;
        }).detach();
      } else
        _rotate_retain();
    }
  }

public:
  static void init() {
    std::lock_guard<std::mutex> lg(sm_main_mtx);
    auto id = std::this_thread::get_id();
    auto t = std::make_shared<thread>();
    t->short_id = sm_threads.size();
    sm_threads.emplace(id, t);
  }
  static void init(std::filesystem::path logfile_path) {
    sm_logfile = std::filesystem::directory_entry(logfile_path);
    if (sm_logfile.path().is_relative())
      sm_logfile = std::filesystem::directory_entry(
          std::filesystem::absolute(sm_logfile.path()));
    sm_os = std::ofstream(sm_logfile.path(), std::ios::app);
    init();
  }

  static void deinit() {
    std::lock_guard<std::mutex> lg(sm_main_mtx);
    auto id = std::this_thread::get_id();
    auto &t = sm_threads[id];
    sm_os << t->stream.str();
    t->stream.str("");
    t->deleted = true;
  }

  static std::string datetime() {

    auto now = std::chrono::system_clock::now();
    std::time_t dt =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[32] = {0};
    std::strftime(buf, sizeof(buf), " %F %T.", std::localtime(&dt));
    // return buf;
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    auto fraction = now - seconds;
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
    std::ostringstream oss;
    oss << buf << std::setfill('0') << std::setw(3) << milliseconds.count()
        << " ";

    return oss.str();
  }

  static void poll() {
    std::lock_guard<std::mutex> lg(sm_main_mtx);
    std::vector<std::thread::id> to_remove;
    for (auto &[id, t] : sm_threads) {
      if (t->deleted) {
        to_remove.push_back(id);
        continue;
      }
      std::lock_guard<std::mutex> lg(t->mutex);
      sm_os << t->stream.str();
      t->stream.str("");
    }
    for (auto id : to_remove)
      sm_threads.erase(id);
    sm_os << std::flush;
    if (rotate)
      _rotate_if_needed();
  }

  static void close() {
    sm_os.close();
    std::condition_variable cv;
    std::unique_lock<std::mutex> ul(sm_rotate_compress_mtx);
    cv.wait_for(ul, 5s, []() { return sm_rotate_complete; });
  }

  enum Level { debug = 0, info = 1, warn = 2, error = 3, fatal = 4 };
  static inline Level level = Level::warn;
  static inline std::uintmax_t rotate_max_size = 1024 * 1024 * 1024;
  static inline uint16_t rotate_retain = 30;
  static inline bool rotate_compress = true;
  static inline bool rotate = true;
  static inline bool print_file_line = true;

#ifdef _WIN32
  static inline std::string rotate_compress_ext = ".zip";
  static inline std::function<bool(const std::filesystem::path &)>
      rotate_compress_cmd = [](const std::filesystem::path &log_path) {
        std::stringstream ss;
        ss << "powershell \"Compress-Archive " << log_path.string() << " "
           << log_path.stem() << ".zip\"";
        auto errcode = system(ss.str().c_str());
        std::filesystem::remove(log_path);
        return !errcode;
      };
#else
  static inline std::string rotate_compress_ext = ".gz";
  static inline std::function<bool(const std::filesystem::path &)>
      rotate_compress_cmd = [](const std::filesystem::path &log_path) {
        std::stringstream ss;
        ss << "gzip -f " << log_path.string();
        auto errcode = system(ss.str().c_str());
        return !errcode;
      };
#endif

  template <typename T>
  static void write_log_with_header(const char *__file__, int __line__,
                                    int __level__, T &&msg) {
    auto id = std::this_thread::get_id();
    auto &t = sm_threads[id];
    std::lock_guard<std::mutex> lg(t->mutex);
    t->stream << '[' << levels[__level__] << datetime() << std::setfill('0')
              << std::setw(4) << std::hex << t->short_id << std::dec;
    if (print_file_line)
      t->stream << " " << __file__ << ":" << __line__;
    t->stream << "] " << msg << std::endl;
  }
};
} // namespace ex
