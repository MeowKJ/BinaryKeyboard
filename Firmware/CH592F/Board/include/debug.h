#ifndef DEBUG_H_
#define DEBUG_H_

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ======================= 配置 ======================= */

#define DEBUG_UART1_BAUDRATE    115200

/* 日志级别 */
#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4

/* 当前日志级别 (编译时配置) */
#ifndef LOG_LEVEL
#define LOG_LEVEL         LOG_LEVEL_DEBUG
#endif

/* ======================= API ======================= */

/**
 * @brief  初始化调试串口 (UART1, PB13)
 */
void Debug_Init(void);

/**
 * @brief  底层日志输出
 */
void Log_Output(const char *level, const char *tag, const char *fmt, ...);

/* ======================= 日志宏 ======================= */

#ifdef DEBUG

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_E(tag, fmt, ...)  Log_Output("E", tag, fmt, ##__VA_ARGS__)
#else
#define LOG_E(tag, fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_W(tag, fmt, ...)  Log_Output("W", tag, fmt, ##__VA_ARGS__)
#else
#define LOG_W(tag, fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(tag, fmt, ...)  Log_Output("I", tag, fmt, ##__VA_ARGS__)
#else
#define LOG_I(tag, fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(tag, fmt, ...)  Log_Output("D", tag, fmt, ##__VA_ARGS__)
#else
#define LOG_D(tag, fmt, ...)
#endif

#else /* !DEBUG */

#define LOG_E(tag, fmt, ...)
#define LOG_W(tag, fmt, ...)
#define LOG_I(tag, fmt, ...)
#define LOG_D(tag, fmt, ...)

#endif /* DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H_ */
