/**
 * @file    kbd_rgb.c
 * @brief   MeowKeyboard RGB 灯效引擎实现
 * @author  MeowKJ
 * @version V1.0.0
 * @date    2024-11-07
 *
 * @details
 * 实现多种 RGB 灯效：
 * - 静态：固定颜色常亮
 * - 呼吸：亮度渐变循环
 * - 闪烁：周期性开关
 * - 彩虹：色相渐变
 * - 状态指示：根据连接状态自动变换
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#include "kbd_rgb.h"
#include "kbd_storage.h"
#include "ws2812.h"
#include "debug.h"
#include "CH59x_common.h"
#include "ble_config.h"

/** @brief 模块日志标签 */
#define TAG "RGB"

/** @brief RGB 更新间隔 (毫秒) */
#define RGB_UPDATE_INTERVAL_MS  20

/** @brief TMOS 定时事件 */
#define RGB_UPDATE_EVT  0x0001

/*============================================================================*/
/*                              私有变量                                       */
/*============================================================================*/

/** @brief 当前状态指示类型 */
static kbd_state_t s_current_state = KBD_STATE_USB_CONNECTED;

/** @brief 效果相位计数器 */
static uint32_t s_effect_phase = 0;

/** @brief 临时闪烁激活标志 */
static bool s_flash_active = false;

/** @brief 临时闪烁颜色 */
static uint8_t s_flash_r, s_flash_g, s_flash_b;

/** @brief 临时闪烁剩余计数 */
static uint16_t s_flash_remain = 0;

/** @brief 层切换闪烁：100ms 亮/灭 所需的 Process 调用次数 (约 20ms/次) */
#define LAYER_FLASH_TICKS_PER_100MS  (100u / RGB_UPDATE_INTERVAL_MS)

/** @brief 层切换闪烁状态（非阻塞） */
static bool s_layer_flash_active = false;
static uint8_t s_layer_flash_blinks_left = 0;
static bool s_layer_flash_is_on = false;
static uint8_t s_layer_flash_wait_ticks = 0;
static uint8_t s_layer_flash_r = 0, s_layer_flash_g = 0, s_layer_flash_b = 0;
static uint8_t s_layer_flash_led_index = 0;

/** @brief TMOS 任务 ID */
static tmosTaskID s_rgb_task_id = TASK_NO_TASK;

/*============================================================================*/
/*                              私有函数                                       */
/*============================================================================*/

/**
 * @brief TMOS 事件处理
 */
static uint16_t KBD_RGB_ProcessEvent(uint8_t task_id, uint16_t events)
{
    (void)task_id;

    if (events & RGB_UPDATE_EVT) {
        KBD_RGB_Process();
        tmos_start_task(s_rgb_task_id, RGB_UPDATE_EVT, MS1_TO_SYSTEM_TIME(RGB_UPDATE_INTERVAL_MS));
        return (events ^ RGB_UPDATE_EVT);
    }
    return 0;
}

/**
 * @brief 应用亮度到颜色值
 *
 * @param[in,out] r          红色分量指针
 * @param[in,out] g          绿色分量指针
 * @param[in,out] b          蓝色分量指针
 * @param[in]     brightness 亮度值 (0-255)
 */
static void ApplyBrightness(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t brightness)
{
    *r = ((uint16_t)*r * brightness) >> 8;
    *g = ((uint16_t)*g * brightness) >> 8;
    *b = ((uint16_t)*b * brightness) >> 8;
}

/**
 * @brief 计算呼吸效果亮度
 *
 * @param[in] phase 相位 (0-255)
 * @return 亮度值 (0-255)
 */
static uint8_t CalcBreathing(uint8_t phase)
{
    /* 三角波：0->255->0 */
    if (phase < 128) {
        return phase * 2;
    } else {
        return (255 - phase) * 2;
    }
}

/**
 * @brief 计算闪烁效果状态
 *
 * @param[in] phase 相位 (0-255)
 * @return true=亮, false=灭
 */
static bool CalcBlink(uint8_t phase)
{
    return (phase < 128);
}

/**
 * @brief 获取状态指示参数
 *
 * @param[in]  state     当前状态
 * @param[out] r         红色分量
 * @param[out] g         绿色分量
 * @param[out] b         蓝色分量
 * @param[out] effect    效果类型 (0=常亮, 1=呼吸, 2=闪烁)
 * @param[out] period_ms 动画周期 (毫秒)
 */
static void GetIndicatorParams(kbd_state_t state, uint8_t *r, uint8_t *g, uint8_t *b,
                                uint8_t *effect, uint16_t *period_ms)
{
    switch (state) {
        case KBD_STATE_USB_CONNECTED:
            *r = KBD_IND_USB_CONN_R;
            *g = KBD_IND_USB_CONN_G;
            *b = KBD_IND_USB_CONN_B;
            *effect = 0;        /* 常亮 */
            *period_ms = 0;
            break;

        case KBD_STATE_BLE_DISCONNECTED:
            *r = KBD_IND_BLE_DISCONN_R;
            *g = KBD_IND_BLE_DISCONN_G;
            *b = KBD_IND_BLE_DISCONN_B;
            *effect = 1;        /* 呼吸 */
            *period_ms = 2000;  /* 2 秒周期 */
            break;

        case KBD_STATE_BLE_ADVERTISING:
            *r = KBD_IND_BLE_ADV_R;
            *g = KBD_IND_BLE_ADV_G;
            *b = KBD_IND_BLE_ADV_B;
            *effect = 1;        /* 呼吸 */
            *period_ms = 1000;  /* 1 秒周期 */
            break;

        case KBD_STATE_BLE_CONNECTED:
            *r = KBD_IND_BLE_CONN_R;
            *g = KBD_IND_BLE_CONN_G;
            *b = KBD_IND_BLE_CONN_B;
            *effect = 0;        /* 常亮 */
            *period_ms = 0;
            break;

        case KBD_STATE_LOW_BATTERY:
            *r = KBD_IND_LOW_BATT_R;
            *g = KBD_IND_LOW_BATT_G;
            *b = KBD_IND_LOW_BATT_B;
            *effect = 2;        /* 快闪 */
            *period_ms = 500;
            break;

        default:
            *r = *g = *b = 0;
            *effect = 0;
            *period_ms = 0;
            break;
    }
}

/**
 * @brief 处理状态指示模式
 */
static void ProcessIndicatorMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    if (!cfg->indicator_enabled) {
        WS2812_Clear_Indicator();
        return;
    }

    uint8_t r, g, b, effect;
    uint16_t period_ms;
    GetIndicatorParams(s_current_state, &r, &g, &b, &effect, &period_ms);

    uint8_t brightness = cfg->indicator_brightness;

    if (effect == 1 && period_ms > 0) {
        /* 呼吸效果 */
        uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;
        brightness = CalcBreathing(phase);
        brightness = ((uint16_t)brightness * cfg->indicator_brightness) >> 8;
    } else if (effect == 2 && period_ms > 0) {
        /* 闪烁效果 */
        uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;
        if (!CalcBlink(phase)) {
            brightness = 0;
        }
    }

    ApplyBrightness(&r, &g, &b, brightness);
    WS2812_Set_Indicator(r, g, b);
}

/**
 * @brief 处理静态模式
 */
static void ProcessStaticMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    uint8_t r = cfg->color_r;
    uint8_t g = cfg->color_g;
    uint8_t b = cfg->color_b;
    ApplyBrightness(&r, &g, &b, cfg->brightness);
    WS2812_FillKeys(r, g, b);
    /* 多 LED 时，确保指示灯显示系统状态 */
    if (WS2812_LED_NUM > 1) {
        ProcessIndicatorMode();
    }
}

/**
 * @brief 处理呼吸模式
 * @note 低亮度下保证可见：呼吸范围映射为 [brightness*25%, brightness]
 *       避免 0~brightness 时低亮度几乎不亮
 */
static void ProcessBreathingMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();

    /* 根据速度计算周期 */
    uint16_t period_ms = 4000 - ((uint32_t)cfg->speed * 3500 / 255);
    uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;
    uint8_t breath_brightness = CalcBreathing(phase);

    uint8_t r = cfg->color_r;
    uint8_t g = cfg->color_g;
    uint8_t b = cfg->color_b;

    /* 映射 breath(0~255) 到 [brightness*25%, brightness]，低亮度下仍有明显起伏 */
    uint8_t floor_val = (uint16_t)cfg->brightness * 64 >> 8;
    uint8_t range = cfg->brightness - floor_val;
    uint8_t final_brightness = floor_val + ((uint16_t)range * breath_brightness >> 8);
    ApplyBrightness(&r, &g, &b, final_brightness);
    WS2812_FillKeys(r, g, b);
    /* 多 LED 时，确保指示灯显示系统状态 */
    if (WS2812_LED_NUM > 1) {
        ProcessIndicatorMode();
    }
}

/**
 * @brief 处理闪烁模式
 */
static void ProcessBlinkMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();

    uint16_t period_ms = 2000 - ((uint32_t)cfg->speed * 1800 / 255);
    uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;

    if (CalcBlink(phase)) {
        uint8_t r = cfg->color_r;
        uint8_t g = cfg->color_g;
        uint8_t b = cfg->color_b;
        ApplyBrightness(&r, &g, &b, cfg->brightness);
        WS2812_FillKeys(r, g, b);
    } else {
        WS2812_FillKeys(0, 0, 0);
    }
    /* 多 LED 时，确保指示灯显示系统状态 */
    if (WS2812_LED_NUM > 1) {
        ProcessIndicatorMode();
    }
}

/**
 * @brief 处理彩虹模式
 */
static void ProcessRainbowMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();

    /* 色相旋转 */
    uint16_t period_ms = 5000 - ((uint32_t)cfg->speed * 4500 / 255);
    uint16_t hue = (s_effect_phase * 360 / period_ms) % 360;

    WS2812_Color color = WS2812_HSVtoRGB(hue, 255, cfg->brightness);
    WS2812_FillKeys(color.r, color.g, color.b);
    /* 多 LED 时，确保指示灯显示系统状态 */
    if (WS2812_LED_NUM > 1) {
        ProcessIndicatorMode();
    }
}

/*============================================================================*/
/*                              公共函数实现                                   */
/*============================================================================*/

void KBD_RGB_Init(void)
{
    LOG_I(TAG, "RGB init");

    WS2812_Init();

    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    if (cfg) {
        WS2812_SetBrightness(cfg->brightness);
        WS2812_SetIndicatorBrightness(cfg->indicator_brightness);
    }

    s_effect_phase = 0;
    s_flash_active = false;
    s_layer_flash_active = false;

    s_rgb_task_id = TMOS_ProcessEventRegister(KBD_RGB_ProcessEvent);
    tmos_start_task(s_rgb_task_id, RGB_UPDATE_EVT, MS1_TO_SYSTEM_TIME(RGB_UPDATE_INTERVAL_MS));
}

void KBD_RGB_Process(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();

    /* 更新相位计数器 */
    s_effect_phase += RGB_UPDATE_INTERVAL_MS;
    if (s_effect_phase > 10000) {
        s_effect_phase = s_effect_phase % 10000;
    }

    /* 处理临时闪烁 */
    if (s_flash_active) {
        if (s_flash_remain > 0) {
            s_flash_remain--;
            WS2812_Set_Indicator(s_flash_r, s_flash_g, s_flash_b);
            WS2812_Update();
            return;
        } else {
            s_flash_active = false;
        }
    }

    /* 处理层切换闪烁（非阻塞状态机） */
    if (s_layer_flash_active) {
        if (s_layer_flash_wait_ticks > 0) {
            s_layer_flash_wait_ticks--;
        } else {
            if (s_layer_flash_is_on) {
                /* 亮 -> 灭 */
                s_layer_flash_is_on = false;
                WS2812_Set(s_layer_flash_led_index, 0, 0, 0);
                s_layer_flash_wait_ticks = LAYER_FLASH_TICKS_PER_100MS;
            } else {
                /* 灭 -> 一次闪烁完成 */
                s_layer_flash_blinks_left--;
                if (s_layer_flash_blinks_left == 0) {
                    s_layer_flash_active = false;
                    s_effect_phase = 0;
                    /* 不 return，继续执行正常灯效 */
                } else {
                    s_layer_flash_is_on = true;
                    WS2812_Set(s_layer_flash_led_index, s_layer_flash_r, s_layer_flash_g, s_layer_flash_b);
                    s_layer_flash_wait_ticks = LAYER_FLASH_TICKS_PER_100MS;
                }
            }
        }
        if (s_layer_flash_active) {
            WS2812_Update();
            return;
        }
    }

    /* RGB 开关关闭时，仅关闭按键灯；指示灯仍由状态指示逻辑控制 */
    if (!cfg->enabled) {
        if (WS2812_LED_NUM > 1) {
            WS2812_FillKeys(0, 0, 0);
        }
        ProcessIndicatorMode();
        WS2812_Update();
        return;
    }

    /* 根据模式处理 */
    switch (cfg->mode) {
        case KBD_RGB_OFF:
            WS2812_Fill(0, 0, 0);
            break;

        case KBD_RGB_STATIC:
            ProcessStaticMode();
            break;

        case KBD_RGB_BREATHING:
            ProcessBreathingMode();
            break;

        case KBD_RGB_BLINK:
            ProcessBlinkMode();
            break;

        case KBD_RGB_RAINBOW:
            ProcessRainbowMode();
            break;

        case KBD_RGB_INDICATOR:
            /* 仅指示灯模式：按键灯全部熄灭 */
            if (WS2812_LED_NUM > 1) {
                WS2812_FillKeys(0, 0, 0);
            }
            ProcessIndicatorMode();
            break;

        default:
            break;
    }

    WS2812_Update();
}

void KBD_RGB_SetMode(kbd_rgb_mode_t mode)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->mode = mode;
    s_effect_phase = 0;
    LOG_D(TAG, "mode=%d", mode);
}

kbd_rgb_mode_t KBD_RGB_GetMode(void)
{
    return (kbd_rgb_mode_t)KBD_GetRgbConfig()->mode;
}

void KBD_RGB_NextMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    uint8_t mode = cfg->mode + 1;
    if (mode > KBD_RGB_INDICATOR) {
        mode = KBD_RGB_OFF;
    }
    KBD_RGB_SetMode((kbd_rgb_mode_t)mode);
}

void KBD_RGB_PrevMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    uint8_t mode = cfg->mode;
    if (mode == KBD_RGB_OFF) {
        mode = KBD_RGB_INDICATOR;
    } else {
        mode--;
    }
    KBD_RGB_SetMode((kbd_rgb_mode_t)mode);
}

void KBD_RGB_Toggle(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->enabled = !cfg->enabled;
    LOG_D(TAG, "toggle=%d", cfg->enabled);

    if (!cfg->enabled) {
        if (WS2812_LED_NUM > 1) {
            WS2812_FillKeys(0, 0, 0);
        }
        ProcessIndicatorMode();
        WS2812_Update();
    }
}

void KBD_RGB_SetBrightness(uint8_t brightness)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->brightness = brightness;
    WS2812_SetBrightness(brightness);
}

void KBD_RGB_SetIndicatorBrightness(uint8_t brightness)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->indicator_brightness = brightness;
    WS2812_SetIndicatorBrightness(brightness);
}

void KBD_RGB_BrightnessUp(uint8_t step)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    uint16_t new_val = cfg->brightness + step;
    if (new_val > 255) new_val = 255;
    KBD_RGB_SetBrightness((uint8_t)new_val);
}

void KBD_RGB_BrightnessDown(uint8_t step)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    int16_t new_val = cfg->brightness - step;
    if (new_val < 0) new_val = 0;
    KBD_RGB_SetBrightness((uint8_t)new_val);
}

void KBD_RGB_SetColor(uint8_t r, uint8_t g, uint8_t b)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->color_r = r;
    cfg->color_g = g;
    cfg->color_b = b;
}

void KBD_RGB_SetSpeed(uint8_t speed)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->speed = speed;
}

void KBD_RGB_SetState(kbd_state_t state)
{
    if (state != s_current_state) {
        s_current_state = state;
        s_effect_phase = 0;
        LOG_D(TAG, "state=%d", state);
    }
}

void KBD_RGB_EnableIndicator(bool enable)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->indicator_enabled = enable ? 1 : 0;
}

void KBD_RGB_Flash(uint8_t r, uint8_t g, uint8_t b, uint16_t duration_ms)
{
    s_flash_active = true;
    s_flash_r = r;
    s_flash_g = g;
    s_flash_b = b;
    s_flash_remain = duration_ms / RGB_UPDATE_INTERVAL_MS;

    /* 立即点亮指示灯（用于启动动画等场景） */
    WS2812_Set_Indicator(r, g, b);
    WS2812_Update();
}

/**
 * @brief 获取层对应的RGB LED索引
 * @param layer 层号 (0-4)
 * @return WS2812 LED索引 (1-5, 0为指示灯)
 * @note 映射关系定义在 kbd_config.h 中的 KBD_LAYER_TO_LED_MAP
 */
static uint8_t GetLayerLedIndex(uint8_t layer)
{
    /* 从配置文件读取映射表 */
    static const uint8_t layer_to_led_map[] = KBD_LAYER_TO_LED_MAP;

    if (layer < KBD_LAYER_TO_LED_MAP_SIZE) {
        return layer_to_led_map[layer];
    }

    /* 超出范围时返回第一个LED (作为fallback) */
    return 1;
}

void KBD_RGB_FlashLayer(uint8_t layer)
{
    /* 从配置文件读取层颜色 */
    static const uint8_t layer_colors[5][3] = KBD_LAYER_COLORS;

    /* 限制层号范围 */
    if (layer >= 5) layer = 4;

    /* 设置闪烁参数 */
    s_layer_flash_r = layer_colors[layer][0];
    s_layer_flash_g = layer_colors[layer][1];
    s_layer_flash_b = layer_colors[layer][2];
    s_layer_flash_led_index = GetLayerLedIndex(layer);
    s_layer_flash_blinks_left = KBD_LAYER_FLASH_BLINKS;
    s_layer_flash_is_on = true;
    s_layer_flash_wait_ticks = LAYER_FLASH_TICKS_PER_100MS;
    s_layer_flash_active = true;

    /* 立即显示第一拍亮，后续由 Process 非阻塞推进 */
    WS2812_Set(s_layer_flash_led_index, s_layer_flash_r, s_layer_flash_g, s_layer_flash_b);
    WS2812_Update();
}
