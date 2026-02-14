#include "ws2812.h"

int main(void) {
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    WS2812_Init();
    WS2812_SetBrightness(255);
    
    while (1) {
        // 测试1: 发送全1（白色）
        WS2812_Set(0, 255, 255, 255);
        WS2812_Update();
        mDelaymS(50);
        
        // 测试2: 发送全0（黑色）
        WS2812_Set(0, 0, 0, 0);
        WS2812_Update();
        mDelaymS(50);
    }
}