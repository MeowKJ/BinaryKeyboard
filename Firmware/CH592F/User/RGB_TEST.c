#include "ws2812.h"

// 测试模式枚举
typedef enum {
    MODE_BREATHING, // 呼吸灯（蓝色）
    MODE_RAINBOW,   // 彩虹循环
    MODE_CHASER,    // 跑马灯
    MODE_MAX
} TestMode_t;

/**
 * @brief 执行综合测试流程
 */
void WS2812_Run_Demo(void) {
    static uint32_t tick = 0;       // 时间轴计数器
    static TestMode_t mode = MODE_BREATHING;
    static uint32_t mode_tick = 0;  // 当前模式持续时间记录

    // --- 逻辑处理 ---
    mode_tick++;
    tick++;

    // 每隔 500 次循环（约 10 秒）切换一次模式
    if (mode_tick > 500) {
        mode = (mode + 1) % MODE_MAX;
        mode_tick = 0;
        WS2812_Fill(0, 0, 0); // 切换模式时清空
    }

    switch (mode) {
        case MODE_BREATHING: {
            // --- 呼吸灯模式 ---
            // 使用三角波计算亮度：0 -> 255 -> 0
            uint8_t breath_v;
            uint16_t temp = tick % 512; 
            if (temp < 256) breath_v = temp;         // 渐亮
            else breath_v = 511 - temp;             // 渐暗
            
            // 使用 HSV：色调(Hue) 240 是蓝色，饱和度(S) 255，明度(V) 就是 breath_v
            WS2812_Color c = WS2812_HSVtoRGB(240, 255, breath_v);
            WS2812_Fill(c.r, c.g, c.b);
            break;
        }

        case MODE_RAINBOW: {
            // --- 彩虹流水模式 ---
            for (uint8_t i = 0; i < WS2812_LED_NUM; i++) {
                // 每个灯珠的颜色偏移一点，形成流动感
                WS2812_Color c = WS2812_Wheel((tick + i * 10) & 255);
                WS2812_Set(i, c.r, c.g, c.b);
            }
            break;
        }

        case MODE_CHASER: {
            // --- 跑马灯模式 ---
            WS2812_Fill(0, 0, 0); // 先关灯
            uint8_t active_led = (tick / 5) % WS2812_LED_NUM; // 控制移动速度
            WS2812_Set(active_led, 255, 255, 255); // 白光闪过
            break;
        }
        default: break;
    }

    // --- 发送显示 ---
    WS2812_Update();
    mDelaymS(20); // 控制全局刷新率约为 50Hz
}

int main(void) {
    // 1. 系统初始化
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    
    // 2. 驱动初始化
    WS2812_Init();
    WS2812_SetBrightness(100); // 初始亮度不要太刺眼（0-255）

    // 3. 主循环
    while (1) {
        WS2812_Run_Demo();
    }
}