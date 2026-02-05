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

void Log_Output(const char *level, const char *tag, const char *fmt, ...)
{
    static char buf[80];
    va_list args;

    // 格式: [L/TAG] message\n
    int len = snprintf(buf, sizeof(buf), "[%s/%s] ", level, tag);
    
    va_start(args, fmt);
    len += vsnprintf(buf + len, sizeof(buf) - len, fmt, args);
    va_end(args);
    
    // 添加换行
    if (len < (int)sizeof(buf) - 1) {
        buf[len++] = '\n';
        buf[len] = '\0';
    }
    
    // 直接输出到 UART1
    for (int i = 0; i < len && buf[i]; i++) {
        while (R8_UART1_TFC == UART_FIFO_SIZE);
        R8_UART1_THR = buf[i];
    }
}
