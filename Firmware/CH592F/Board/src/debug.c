#include "debug.h"
#include "CH59x_common.h"
#include <stdio.h>
#include <string.h>

void Debug_Init(void)
{
    // 启用 UART1 引脚重映射: PA8/PA9 -> PB12/PB13
    GPIOPinRemap(ENABLE, RB_PIN_UART1);

    // 配置 PB13 为 UART1 TXD 复用功能
    GPIOB_SetBits(bTXD1_);
    GPIOB_ModeCfg(bTXD1_, GPIO_ModeOut_PP_5mA);

    // 初始化 UART1
    UART1_DefInit();
    UART1_BaudRateCfg(DEBUG_UART1_BAUDRATE);
}

static void Debug_PutChar(char c)
{
    while (R8_UART1_TFC == UART_FIFO_SIZE);
    R8_UART1_THR = c;
}

static void Debug_PutString(const char *str)
{
    while (*str) {
        Debug_PutChar(*str++);
    }
}

void Log_Output(const char *level, const char *tag, const char *fmt, ...)
{
    char buf[128];
    va_list args;
    int len;

    // 格式: [L/TAG] message
    len = snprintf(buf, sizeof(buf), "[%s/%s] ", level, tag);
    if (len > 0) {
        Debug_PutString(buf);
    }

    va_start(args, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0) {
        Debug_PutString(buf);
    }
    Debug_PutChar('\n');
}
