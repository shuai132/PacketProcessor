/**
 * 统一控制调试信息
 * 为了保证输出顺序 都使用stdout而不是stderr
 *
 * 可配置项（默认都是未定义）
 * PacketProcessor_LOG_LINE_END_CRLF        默认是\n结尾 添加此宏将以\r\n结尾
 * PacketProcessor_LOG_SHOW_DEBUG           开启LOGD的输出
 * PacketProcessor_LOG_SHOW_VERBOSE         显示LOGV的输出
 */

#pragma once

#include <stdio.h>

#ifdef  PacketProcessor_LOG_LINE_END_CRLF
#define PacketProcessor_LOG_LINE_END        "\r\n"
#else
#define PacketProcessor_LOG_LINE_END        "\n"
#endif

#ifdef PacketProcessor_LOG_NOT_EXIT_ON_FATAL
#define PacketProcessor_LOG_EXIT_PROGRAM()
#else
#ifdef PacketProcessor_LOG_FOR_MCU
#define PacketProcessor_LOG_EXIT_PROGRAM()  do{ for(;;); } while(0)
#else
#define PacketProcessor_LOG_EXIT_PROGRAM()  exit(EXIT_FAILURE)
#endif
#endif

#ifdef PacketProcessor_LOG_WITH_COLOR
#define PacketProcessor_LOG_COLOR_RED       "\033[31m"
#define PacketProcessor_LOG_COLOR_GREEN     "\033[32m"
#define PacketProcessor_LOG_COLOR_YELLOW    "\033[33m"
#define PacketProcessor_LOG_COLOR_BLUE      "\033[34m"
#define PacketProcessor_LOG_COLOR_CARMINE   "\033[35m"
#define PacketProcessor_LOG_COLOR_CYAN      "\033[36m"
#define PacketProcessor_LOG_COLOR_END       "\033[m"
#else
#define PacketProcessor_LOG_COLOR_RED
#define PacketProcessor_LOG_COLOR_GREEN
#define PacketProcessor_LOG_COLOR_YELLOW
#define PacketProcessor_LOG_COLOR_BLUE
#define PacketProcessor_LOG_COLOR_CARMINE
#define PacketProcessor_LOG_COLOR_CYAN
#define PacketProcessor_LOG_COLOR_END
#endif

#define PacketProcessor_LOG_END             PacketProcessor_LOG_COLOR_END PacketProcessor_LOG_LINE_END

#ifdef __ANDROID__
#include <android/log.h>
#define PacketProcessor_LOG_PRINTF(...)     __android_log_print(ANDROID_LOG_DEBUG, "LOG", __VA_ARGS__)
#else
#define PacketProcessor_LOG_PRINTF(...)     printf(__VA_ARGS__)
#endif

#if defined(PacketProcessor_LOG_SHOW_VERBOSE)
#define PacketProcessor_LOGV(fmt, ...)      do{ PacketProcessor_LOG_PRINTF("[V]: " fmt PacketProcessor_LOG_LINE_END, ##__VA_ARGS__); } while(0)
#else
#define PacketProcessor_LOGV(fmt, ...)      ((void)0)
#endif

#if defined(PacketProcessor_LOG_SHOW_DEBUG)
#define PacketProcessor_LOGD(fmt, ...)      do{ PacketProcessor_LOG_PRINTF("[D]: " fmt PacketProcessor_LOG_LINE_END, ##__VA_ARGS__); } while(0)
#else
#define PacketProcessor_LOGD(fmt, ...)      ((void)0)
#endif

#define PacketProcessor_LOG(fmt, ...)       do{ PacketProcessor_LOG_PRINTF(PacketProcessor_LOG_COLOR_GREEN fmt PacketProcessor_LOG_END, ##__VA_ARGS__); } while(0)
#define PacketProcessor_LOGT(tag, fmt, ...) do{ PacketProcessor_LOG_PRINTF(PacketProcessor_LOG_COLOR_BLUE "[" tag "]: " fmt PacketProcessor_LOG_END, ##__VA_ARGS__); } while(0)
#define PacketProcessor_LOGI(fmt, ...)      do{ PacketProcessor_LOG_PRINTF(PacketProcessor_LOG_COLOR_YELLOW "[I]: " fmt PacketProcessor_LOG_END, ##__VA_ARGS__); } while(0)
#define PacketProcessor_LOGW(fmt, ...)      do{ PacketProcessor_LOG_PRINTF(PacketProcessor_LOG_COLOR_CARMINE "[W]: %s: %d: " fmt PacketProcessor_LOG_END, __func__, __LINE__, ##__VA_ARGS__); } while(0)
#define PacketProcessor_LOGE(fmt, ...)      do{ PacketProcessor_LOG_PRINTF(PacketProcessor_LOG_COLOR_RED "[E]: %s: %d: " fmt PacketProcessor_LOG_END, __func__, __LINE__, ##__VA_ARGS__); } while(0)
