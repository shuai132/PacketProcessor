// 1. 全局控制
// L_O_G_NDEBUG                 关闭DEBUG日志（默认依据NDEBUG自动判断）
// L_O_G_SHOW_DEBUG             强制开启DEBUG日志
// L_O_G_DISABLE_ALL            关闭所有日志
// L_O_G_DISABLE_COLOR          禁用颜色显示
// L_O_G_LINE_END_CRLF          默认是\n结尾 添加此宏将以\r\n结尾
// L_O_G_FOR_MCU                更适用于MCU环境
// L_O_G_NOT_EXIT_ON_FATAL      FATAL默认退出程序 添加此宏将不退出
// L_O_G_SHOW_FULL_PATH         显示文件绝对路径
//
// c++11环境默认打开以下内容
// L_O_G_ENABLE_THREAD_SAFE     线程安全
// L_O_G_ENABLE_THREAD_ID       显示线程ID
// L_O_G_ENABLE_DATE_TIME       显示日期
// 分别可通过下列禁用
// L_O_G_DISABLE_THREAD_SAFE
// L_O_G_DISABLE_THREAD_ID
// L_O_G_DISABLE_DATE_TIME
// 可通过`L_O_G_GET_TID_CUSTOM`自定义获取线程ID的实现
//
// 2. 自定义实现
// L_O_G_PRINTF_CUSTOM          自定义输出实现
// 并添加实现`int L_O_G_PRINTF_CUSTOM(const char *fmt, ...)`
//
// 3. 在库中使用时
// 3.1. 替换`PacketProcessor_LOG`为库名
// 3.2. 定义`PacketProcessor_LOG_IN_LIB`
// 3.3. 可配置项
// PacketProcessor_LOG_SHOW_DEBUG               开启PacketProcessor_LOGD的输出
// PacketProcessor_LOG_SHOW_VERBOSE             显示PacketProcessor_LOGV的输出
// PacketProcessor_LOG_DISABLE_ALL              关闭所有日志

#pragma once

// clang-format off

//#define PacketProcessor_LOG_IN_LIB

#if defined(PacketProcessor_LOG_DISABLE_ALL) || defined(L_O_G_DISABLE_ALL)

#define PacketProcessor_LOG(fmt, ...)           ((void)0)
#define PacketProcessor_LOGT(tag, fmt, ...)     ((void)0)
#define PacketProcessor_LOGI(fmt, ...)          ((void)0)
#define PacketProcessor_LOGW(fmt, ...)          ((void)0)
#define PacketProcessor_LOGE(fmt, ...)          ((void)0)
#define PacketProcessor_LOGF(fmt, ...)          ((void)0)
#define PacketProcessor_LOGD(fmt, ...)          ((void)0)
#define PacketProcessor_LOGV(fmt, ...)          ((void)0)

#else

#ifdef __cplusplus
#include <cstring>
#include <cstdlib>
#if __cplusplus >= 201103L

#if !defined(L_O_G_DISABLE_THREAD_SAFE) && !defined(L_O_G_ENABLE_THREAD_SAFE)
#define L_O_G_ENABLE_THREAD_SAFE
#endif

#if !defined(L_O_G_DISABLE_THREAD_ID) && !defined(L_O_G_ENABLE_THREAD_ID)
#define L_O_G_ENABLE_THREAD_ID
#endif

#if !defined(L_O_G_DISABLE_DATE_TIME) && !defined(L_O_G_ENABLE_DATE_TIME)
#define L_O_G_ENABLE_DATE_TIME
#endif

#endif
#else
#include <string.h>
#include <stdlib.h>
#endif

#ifdef  L_O_G_LINE_END_CRLF
#define PacketProcessor_LOG_LINE_END            "\r\n"
#else
#define PacketProcessor_LOG_LINE_END            "\n"
#endif

#ifdef L_O_G_NOT_EXIT_ON_FATAL
#define PacketProcessor_LOG_EXIT_PROGRAM()
#else
#ifdef L_O_G_FOR_MCU
#define PacketProcessor_LOG_EXIT_PROGRAM()      do{ for(;;); } while(0)
#else
#define PacketProcessor_LOG_EXIT_PROGRAM()      exit(EXIT_FAILURE)
#endif
#endif

#ifdef L_O_G_SHOW_FULL_PATH
#define PacketProcessor_LOG_BASE_FILENAME       (__FILE__)
#else
#ifdef __FILE_NAME__
#define PacketProcessor_LOG_BASE_FILENAME       (__FILE_NAME__)
#else
#define PacketProcessor_LOG_BASE_FILENAME       (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#endif
#endif

#define PacketProcessor_LOG_WITH_COLOR

#if defined(_WIN32) || (defined(__ANDROID__) && !defined(ANDROID_STANDALONE)) || defined(L_O_G_FOR_MCU)
#undef PacketProcessor_LOG_WITH_COLOR
#endif

#ifdef L_O_G_DISABLE_COLOR
#undef PacketProcessor_LOG_WITH_COLOR
#endif

#ifdef PacketProcessor_LOG_WITH_COLOR
#define PacketProcessor_LOG_COLOR_RED           "\033[31m"
#define PacketProcessor_LOG_COLOR_GREEN         "\033[32m"
#define PacketProcessor_LOG_COLOR_YELLOW        "\033[33m"
#define PacketProcessor_LOG_COLOR_BLUE          "\033[34m"
#define PacketProcessor_LOG_COLOR_CARMINE       "\033[35m"
#define PacketProcessor_LOG_COLOR_CYAN          "\033[36m"
#define PacketProcessor_LOG_COLOR_DEFAULT
#define PacketProcessor_LOG_COLOR_END           "\033[m"
#else
#define PacketProcessor_LOG_COLOR_RED
#define PacketProcessor_LOG_COLOR_GREEN
#define PacketProcessor_LOG_COLOR_YELLOW
#define PacketProcessor_LOG_COLOR_BLUE
#define PacketProcessor_LOG_COLOR_CARMINE
#define PacketProcessor_LOG_COLOR_CYAN
#define PacketProcessor_LOG_COLOR_DEFAULT
#define PacketProcessor_LOG_COLOR_END
#endif

#define PacketProcessor_LOG_END                 PacketProcessor_LOG_COLOR_END PacketProcessor_LOG_LINE_END

#ifndef L_O_G_PRINTF
#ifndef PacketProcessor_LOG_PRINTF_DEFAULT
#if defined(__ANDROID__) && !defined(ANDROID_STANDALONE)
#include <android/log.h>
#define PacketProcessor_LOG_PRINTF_DEFAULT(...) __android_log_print(ANDROID_L##OG_DEBUG, "PacketProcessor_LOG", __VA_ARGS__)
#else
#define PacketProcessor_LOG_PRINTF_DEFAULT(...) printf(__VA_ARGS__)
#endif
#endif

#ifndef L_O_G_PRINTF_CUSTOM
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif
#ifdef L_O_G_ENABLE_THREAD_SAFE
#ifndef L_O_G_NS_MUTEX
#define L_O_G_NS_MUTEX L_O_G_NS_MUTEX
#include <mutex>
// 1. struct instead of namespace, ensure single instance
struct L_O_G_NS_MUTEX {
static std::mutex& mutex() {
  // 2. never delete, avoid destroy before user log
  // 3. static memory, avoid memory fragmentation
  static char memory[sizeof(std::mutex)];
  static std::mutex& mutex = *(new (memory) std::mutex());
  return mutex;
}
};
#endif
#define L_O_G_PRINTF(...) { \
  std::lock_guard<std::mutex> lock(L_O_G_NS_MUTEX::mutex()); \
  PacketProcessor_LOG_PRINTF_DEFAULT(__VA_ARGS__); \
}
#else
#define L_O_G_PRINTF(...)       PacketProcessor_LOG_PRINTF_DEFAULT(__VA_ARGS__)
#endif
#else
extern int L_O_G_PRINTF_CUSTOM(const char *fmt, ...);
#define L_O_G_PRINTF(...)       L_O_G_PRINTF_CUSTOM(__VA_ARGS__)
#endif
#endif

#ifdef L_O_G_ENABLE_THREAD_ID
#ifndef L_O_G_NS_GET_TID
#define L_O_G_NS_GET_TID L_O_G_NS_GET_TID
#include <cstdint>
#ifdef L_O_G_GET_TID_CUSTOM
extern uint32_t L_O_G_GET_TID_CUSTOM();
#elif _WIN32
#include <processthreadsapi.h>
struct L_O_G_NS_GET_TID {
static inline uint32_t get_tid() {
  return GetCurrentThreadId();
}
};
#elif defined(__linux__)
#include <sys/syscall.h>
#include <unistd.h>
struct L_O_G_NS_GET_TID {
static inline uint32_t get_tid() {
  return syscall(SYS_gettid);
}
};
#else /* for mac, bsd.. */
#include <pthread.h>
struct L_O_G_NS_GET_TID {
static inline uint32_t get_tid() {
  uint64_t x;
  pthread_threadid_np(nullptr, &x);
  return (uint32_t)x;
}
};
#endif
#endif
#ifdef L_O_G_GET_TID_CUSTOM
#define PacketProcessor_LOG_THREAD_LABEL "%u "
#define PacketProcessor_LOG_THREAD_VALUE ,L_O_G_GET_TID_CUSTOM()
#else
#define PacketProcessor_LOG_THREAD_LABEL "%u "
#define PacketProcessor_LOG_THREAD_VALUE ,L_O_G_NS_GET_TID::get_tid()
#endif
#else
#define PacketProcessor_LOG_THREAD_LABEL
#define PacketProcessor_LOG_THREAD_VALUE
#endif

#ifdef L_O_G_ENABLE_DATE_TIME
#include <chrono>
#include <sstream>
#include <iomanip> // std::put_time
#ifndef L_O_G_NS_GET_TIME
#define L_O_G_NS_GET_TIME L_O_G_NS_GET_TIME
struct L_O_G_NS_GET_TIME {
static inline std::string get_time() {
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
  std::stringstream ss;
  ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
  return ss.str();
}
};
#endif
#define PacketProcessor_LOG_TIME_LABEL "%s "
#define PacketProcessor_LOG_TIME_VALUE ,L_O_G_NS_GET_TIME::get_time().c_str()
#else
#define PacketProcessor_LOG_TIME_LABEL
#define PacketProcessor_LOG_TIME_VALUE
#endif

#define PacketProcessor_LOG(fmt, ...)           do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_GREEN   PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[*]: %s:%d "       fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define PacketProcessor_LOGT(tag, fmt, ...)     do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_BLUE    PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[" tag "]: %s:%d " fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define PacketProcessor_LOGI(fmt, ...)          do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_YELLOW  PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[I]: %s:%d "       fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#define PacketProcessor_LOGW(fmt, ...)          do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_CARMINE PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[W]: %s:%d [%s] "  fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define PacketProcessor_LOGE(fmt, ...)          do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_RED     PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[E]: %s:%d [%s] "  fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); } while(0)                     // NOLINT(bugprone-lambda-function-name)
#define PacketProcessor_LOGF(fmt, ...)          do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_CYAN    PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[!]: %s:%d [%s] "  fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, __func__, ##__VA_ARGS__); PacketProcessor_LOG_EXIT_PROGRAM(); } while(0) // NOLINT(bugprone-lambda-function-name)

#if defined(PacketProcessor_LOG_IN_LIB) && !defined(PacketProcessor_LOG_SHOW_DEBUG) && !defined(L_O_G_NDEBUG)
#define PacketProcessor_LOG_NDEBUG
#endif

#if defined(L_O_G_NDEBUG) && !defined(PacketProcessor_LOG_NDEBUG)
#define PacketProcessor_LOG_NDEBUG
#endif

#if (defined(NDEBUG) || defined(PacketProcessor_LOG_NDEBUG)) && !defined(L_O_G_SHOW_DEBUG)
#define PacketProcessor_LOGD(fmt, ...)          ((void)0)
#else
#define PacketProcessor_LOGD(fmt, ...)          do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_DEFAULT PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[D]: %s:%d "       fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#endif

#if defined(PacketProcessor_LOG_SHOW_VERBOSE)
#define PacketProcessor_LOGV(fmt, ...)          do{ L_O_G_PRINTF(PacketProcessor_LOG_COLOR_DEFAULT PacketProcessor_LOG_TIME_LABEL PacketProcessor_LOG_THREAD_LABEL "[V]: %s:%d "       fmt PacketProcessor_LOG_END PacketProcessor_LOG_TIME_VALUE PacketProcessor_LOG_THREAD_VALUE, PacketProcessor_LOG_BASE_FILENAME, __LINE__, ##__VA_ARGS__); } while(0)
#else
#define PacketProcessor_LOGV(fmt, ...)          ((void)0)
#endif

#endif
