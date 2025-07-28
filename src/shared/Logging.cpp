#include "Logging.h"
#include <vector>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef __ANDROID__
#include <spdlog/sinks/android_sink.h>
#endif
#include <iostream>
#include <unordered_map>

namespace MRDesk {
static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers;

class LoggerBuf : public std::streambuf {
 public:
  LoggerBuf(
      std::shared_ptr<spdlog::logger> logger, spdlog::level::level_enum lvl)
      : m_logger(std::move(logger)), m_level(lvl) {}

 protected:
  int overflow(int c) override {
    if (c == traits_type::eof()) {
      return traits_type::eof();
    }
    if (c == '\n') {
      flush();
    } else {
      m_buffer += static_cast<char>(c);
    }
    return c;
  }

  int sync() override {
    flush();
    return 0;
  }

 private:
  void flush() {
    if (!m_buffer.empty()) {
      m_logger->log(m_level, m_buffer);
      m_buffer.clear();
    }
  }

  std::string m_buffer;
  std::shared_ptr<spdlog::logger> m_logger;
  spdlog::level::level_enum m_level;
};

static std::shared_ptr<spdlog::logger> createLogger(const char* tag) {
#ifdef __ANDROID__
  auto sink = std::make_shared<spdlog::sinks::android_sink_mt>(tag);
  auto logger = std::make_shared<spdlog::logger>(tag, sink);
#else
  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      "mrdesktop.log", 1048576 * 5, 3);
  std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
  auto logger =
      std::make_shared<spdlog::logger>(tag, sinks.begin(), sinks.end());
#endif
  logger->set_level(spdlog::level::debug);
  logger->set_pattern("[%H:%M:%S] [%n] [%^%l%$] %v");
  spdlog::register_logger(logger);
  return logger;
}

std::shared_ptr<spdlog::logger> GetLogger(const char* tag) {
  auto it = loggers.find(tag);
  if (it != loggers.end()) {
    return it->second;
  }
  auto logger = createLogger(tag);
  loggers[tag] = logger;
  return logger;
}

void InitLogging() {
  auto outLogger = GetLogger("MRDesk.Stdout");
  auto errLogger = GetLogger("MRDesk.Stderr");
  static LoggerBuf outBuf(outLogger, spdlog::level::info);
  static LoggerBuf errBuf(errLogger, spdlog::level::err);
  std::cout.rdbuf(&outBuf);
  std::cerr.rdbuf(&errBuf);
}
} // namespace MRDesk
