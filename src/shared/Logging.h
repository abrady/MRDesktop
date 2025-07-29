#pragma once
#include <memory>
#include <sstream>
#include <spdlog/spdlog.h>

namespace MRDesk {
std::shared_ptr<spdlog::logger> GetLogger(const char* tag);
void InitLogging();
} // namespace MRDesk

#ifndef LOG_TAG
#define LOG_TAG "MRDesk"
#endif

#define LOGI(...) MRDesk::GetLogger(LOG_TAG)->info(__VA_ARGS__)
#define LOGE(...) MRDesk::GetLogger(LOG_TAG)->error(__VA_ARGS__)
#define LOGW(...) MRDesk::GetLogger(LOG_TAG)->warn(__VA_ARGS__)
#define LOGD(...) MRDesk::GetLogger(LOG_TAG)->debug(__VA_ARGS__)

#define LOG_IS(MSG)                              \
  do {                                           \
    std::ostringstream _os;                      \
    _os << MSG;                                  \
    MRDesk::GetLogger(LOG_TAG)->info(_os.str()); \
  } while (0)
#define LOG_ES(MSG)                               \
  do {                                            \
    std::ostringstream _os;                       \
    _os << MSG;                                   \
    MRDesk::GetLogger(LOG_TAG)->error(_os.str()); \
  } while (0)
#define LOG_WS(MSG)                              \
  do {                                           \
    std::ostringstream _os;                      \
    _os << MSG;                                  \
    MRDesk::GetLogger(LOG_TAG)->warn(_os.str()); \
  } while (0)
#define LOG_DS(MSG)                               \
  do {                                            \
    std::ostringstream _os;                       \
    _os << MSG;                                   \
    MRDesk::GetLogger(LOG_TAG)->debug(_os.str()); \
  } while (0)
