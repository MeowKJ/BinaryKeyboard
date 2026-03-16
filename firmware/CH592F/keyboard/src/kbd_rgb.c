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
#include "kbd_config.h"
#include <string.h>

/** @brief 模块日志标签 */
#define TAG "RGB"

/** @brief RGB 更新间隔 (毫秒) */
#define RGB_UPDATE_INTERVAL_MS 20

/** @brief TMOS 定时事件 */
#define RGB_UPDATE_EVT 0x0001

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
#define LAYER_FLASH_TICKS_PER_100MS (100u / RGB_UPDATE_INTERVAL_MS)

/** @brief 层切换闪烁状态（非阻塞） */
static bool s_layer_flash_active = false;
static uint8_t s_layer_flash_blinks_left = 0;
static bool s_layer_flash_is_on = false;
static uint8_t s_layer_flash_wait_ticks = 0;
static uint8_t s_layer_flash_r = 0, s_layer_flash_g = 0, s_layer_flash_b = 0;
static uint8_t s_layer_flash_layer = 0; /**< 闪烁的目标按键位置 (层号) */

/** @brief 层颜色表 */
static const uint8_t s_layer_colors[5][3] = KBD_LAYER_COLORS;

/** @brief 逻辑按键到物理 LED 映射表 */
static const uint8_t s_logical_to_physical[KBD_NUM_KEYS] = KBD_LOGICAL_TO_PHYSICAL_MAP;

/** @brief TMOS 任务 ID */
static tmosTaskID s_rgb_task_id = TASK_NO_TASK;

/**
 * @brief 将逻辑按键索引转换为 WS2812 LED 索引
 * @param key_idx 逻辑按键索引 (0 ~ KBD_NUM_KEYS-1)
 * @return WS2812 LED 索引 (指示灯偏移 +1)
 */
static inline uint8_t KeyToLed(uint8_t key_idx)
{
    if (key_idx >= KBD_NUM_KEYS) return 0;
    return (WS2812_LED_NUM > 1) ? (s_logical_to_physical[key_idx] + 1) : 0;
}

/*============================================================================*/
/*                          按下效果 (Press Effect)                            */
/*============================================================================*/

/** @brief 按下效果常量 */
#define PRESS_EFFECT_NONE       0
#define PRESS_EFFECT_LIGHT_FADE 1
#define PRESS_EFFECT_DARK_FADE  2

/** @brief 每个按键的按下衰减值 (255=刚按下, 0=无效果) */
static uint8_t s_press_fade[KBD_NUM_KEYS];
static uint8_t s_press_color_r[KBD_NUM_KEYS];
static uint8_t s_press_color_g[KBD_NUM_KEYS];
static uint8_t s_press_color_b[KBD_NUM_KEYS];
static uint8_t s_press_active_count = 0;

/* 前向声明 */
static void ApplyBrightness(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t brightness);
static uint8_t CalcBreathing(uint8_t phase);
static bool CalcBlink(uint8_t phase);
static void SampleModeKeyColor(const kbd_rgb_config_t *cfg,
                               uint8_t key_index,
                               bool allow_fallback,
                               uint8_t *r,
                               uint8_t *g,
                               uint8_t *b);
static void ClearPressEffects(void);

/**
 * @brief 计算每帧衰减步长
 */
static uint8_t PressFadeStep(uint8_t speed)
{
    return (uint8_t)(2u + (speed >> 5));
}

static void ResolvePressFallbackColor(const kbd_rgb_config_t *cfg,
                                      uint8_t *r,
                                      uint8_t *g,
                                      uint8_t *b)
{
    *r = cfg->color_r;
    *g = cfg->color_g;
    *b = cfg->color_b;
    ApplyBrightness(r, g, b, cfg->brightness);

    if (*r == 0 && *g == 0 && *b == 0 && cfg->brightness > 0)
    {
        *r = cfg->brightness;
        *g = cfg->brightness;
        *b = cfg->brightness;
    }
}

static void SampleModeKeyColor(const kbd_rgb_config_t *cfg,
                               uint8_t key_index,
                               bool allow_fallback,
                               uint8_t *r,
                               uint8_t *g,
                               uint8_t *b)
{
    *r = 0;
    *g = 0;
    *b = 0;

    switch (cfg->mode)
    {
    case KBD_RGB_STATIC:
        *r = cfg->color_r;
        *g = cfg->color_g;
        *b = cfg->color_b;
        ApplyBrightness(r, g, b, cfg->brightness);
        break;

    case KBD_RGB_BREATHING:
    {
        uint16_t period_ms = 4000 - ((uint32_t)cfg->speed * 3500 / 255);
        uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;
        uint8_t breath_brightness = CalcBreathing(phase);
        uint8_t floor_val = (uint16_t)cfg->brightness * 64 >> 8;
        uint8_t range = cfg->brightness - floor_val;
        uint8_t final_brightness = floor_val + ((uint16_t)range * breath_brightness >> 8);
        *r = cfg->color_r;
        *g = cfg->color_g;
        *b = cfg->color_b;
        ApplyBrightness(r, g, b, final_brightness);
        break;
    }

    case KBD_RGB_BLINK:
    {
        uint16_t period_ms = 2000 - ((uint32_t)cfg->speed * 1800 / 255);
        uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;
        if (CalcBlink(phase))
        {
            *r = cfg->color_r;
            *g = cfg->color_g;
            *b = cfg->color_b;
            ApplyBrightness(r, g, b, cfg->brightness);
        }
        break;
    }

    case KBD_RGB_RAINBOW:
    {
        uint16_t period_ms = 5000 - ((uint32_t)cfg->speed * 4500 / 255);
        uint16_t hue_base = (s_effect_phase * 360 / period_ms) % 360;
        uint16_t hue = (hue_base + (uint16_t)key_index * 360 / KBD_NUM_KEYS) % 360;
        WS2812_Color color = WS2812_HSVtoRGB(hue, 255, cfg->brightness);
        *r = color.r;
        *g = color.g;
        *b = color.b;
        break;
    }

    default:
        break;
    }

    if (allow_fallback && *r == 0 && *g == 0 && *b == 0)
    {
        ResolvePressFallbackColor(cfg, r, g, b);
    }
}

static void ClearPressEffects(void)
{
    memset(s_press_fade, 0, sizeof(s_press_fade));
    memset(s_press_color_r, 0, sizeof(s_press_color_r));
    memset(s_press_color_g, 0, sizeof(s_press_color_g));
    memset(s_press_color_b, 0, sizeof(s_press_color_b));
    s_press_active_count = 0;
}

/**
 * @brief 处理按下效果：逐键覆写 LED 亮度并衰减
 */
static void ProcessPressEffects(kbd_rgb_config_t *cfg)
{
    uint8_t step = PressFadeStep(cfg->speed);

    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++)
    {
        uint8_t intensity = s_press_fade[i];
        if (intensity == 0)
            continue;

        uint8_t led_idx = KeyToLed(i);

        if (cfg->press_effect == PRESS_EFFECT_LIGHT_FADE)
        {
            uint8_t r = ((uint16_t)s_press_color_r[i] * intensity) >> 8;
            uint8_t g = ((uint16_t)s_press_color_g[i] * intensity) >> 8;
            uint8_t b = ((uint16_t)s_press_color_b[i] * intensity) >> 8;
            WS2812_Set(led_idx, r, g, b);
        }
        else
        {
            uint8_t base_r, base_g, base_b;
            SampleModeKeyColor(cfg, i, false, &base_r, &base_g, &base_b);
            uint8_t factor = 255 - intensity;
            uint8_t r = ((uint16_t)base_r * factor) >> 8;
            uint8_t g = ((uint16_t)base_g * factor) >> 8;
            uint8_t b = ((uint16_t)base_b * factor) >> 8;
            WS2812_Set(led_idx, r, g, b);
        }

        /* 衰减 */
        if (s_press_fade[i] > step)
            s_press_fade[i] -= step;
        else
        {
            s_press_fade[i] = 0;
            if (s_press_active_count > 0)
                s_press_active_count--;
        }
    }
}

/*============================================================================*/
/*                              私有函数                                       */
/*============================================================================*/

/**
 * @brief TMOS 事件处理
 */
static uint16_t KBD_RGB_ProcessEvent(uint8_t task_id, uint16_t events)
{
    (void)task_id;

    if (events & RGB_UPDATE_EVT)
    {
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
    if (phase < 128)
    {
        return phase * 2;
    }
    else
    {
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
    switch (state)
    {
    case KBD_STATE_USB_CONNECTED:
        *r = KBD_IND_USB_CONN_R;
        *g = KBD_IND_USB_CONN_G;
        *b = KBD_IND_USB_CONN_B;
        *effect = 0; /* 常亮 */
        *period_ms = 0;
        break;

    case KBD_STATE_BLE_DISCONNECTED:
        *r = KBD_IND_BLE_DISCONN_R;
        *g = KBD_IND_BLE_DISCONN_G;
        *b = KBD_IND_BLE_DISCONN_B;
        *effect = 1;       /* 呼吸 */
        *period_ms = 2000; /* 2 秒周期 */
        break;

    case KBD_STATE_BLE_ADVERTISING:
        *r = KBD_IND_BLE_ADV_R;
        *g = KBD_IND_BLE_ADV_G;
        *b = KBD_IND_BLE_ADV_B;
        *effect = 1;       /* 呼吸 */
        *period_ms = 1000; /* 1 秒周期 */
        break;

    case KBD_STATE_BLE_CONNECTED:
        *r = KBD_IND_BLE_CONN_R;
        *g = KBD_IND_BLE_CONN_G;
        *b = KBD_IND_BLE_CONN_B;
        *effect = 0; /* 常亮 */
        *period_ms = 0;
        break;

    case KBD_STATE_LOW_BATTERY:
        *r = KBD_IND_LOW_BATT_R;
        *g = KBD_IND_LOW_BATT_G;
        *b = KBD_IND_LOW_BATT_B;
        *effect = 2; /* 快闪 */
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
    if (!cfg->indicator_enabled)
    {
        WS2812_Clear_Indicator();
        return;
    }

    uint8_t r, g, b, effect;
    uint16_t period_ms;
    GetIndicatorParams(s_current_state, &r, &g, &b, &effect, &period_ms);

    uint8_t brightness = cfg->indicator_brightness;

    if (effect == 1 && period_ms > 0)
    {
        /* 呼吸效果 */
        uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;
        brightness = CalcBreathing(phase);
        brightness = ((uint16_t)brightness * cfg->indicator_brightness) >> 8;
    }
    else if (effect == 2 && period_ms > 0)
    {
        /* 闪烁效果 */
        uint8_t phase = (s_effect_phase * 256 / period_ms) & 0xFF;
        if (!CalcBlink(phase))
        {
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
    if (WS2812_LED_NUM > 1)
    {
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
    if (WS2812_LED_NUM > 1)
    {
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

    if (CalcBlink(phase))
    {
        uint8_t r = cfg->color_r;
        uint8_t g = cfg->color_g;
        uint8_t b = cfg->color_b;
        ApplyBrightness(&r, &g, &b, cfg->brightness);
        WS2812_FillKeys(r, g, b);
    }
    else
    {
        WS2812_FillKeys(0, 0, 0);
    }
    /* 多 LED 时，确保指示灯显示系统状态 */
    if (WS2812_LED_NUM > 1)
    {
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
    uint16_t hue_base = (s_effect_phase * 360 / period_ms) % 360;

    /* 逐键色相偏移，形成彩虹铺展 */
    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++)
    {
        uint16_t hue = (hue_base + (uint16_t)i * 360 / KBD_NUM_KEYS) % 360;
        WS2812_Color color = WS2812_HSVtoRGB(hue, 255, cfg->brightness);
        uint8_t led_idx = KeyToLed(i);
        WS2812_Set(led_idx, color.r, color.g, color.b);
    }
    /* 多 LED 时，确保指示灯显示系统状态 */
    if (WS2812_LED_NUM > 1)
    {
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
    if (cfg)
    {
        WS2812_SetBrightness(cfg->brightness);
        WS2812_SetIndicatorBrightness(cfg->indicator_brightness);
    }

    s_effect_phase = 0;
    s_flash_active = false;
    s_layer_flash_active = false;
    ClearPressEffects();

    s_rgb_task_id = TMOS_ProcessEventRegister(KBD_RGB_ProcessEvent);
    tmos_start_task(s_rgb_task_id, RGB_UPDATE_EVT, MS1_TO_SYSTEM_TIME(RGB_UPDATE_INTERVAL_MS));
}

void KBD_RGB_Process(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();

    if (cfg->press_effect == PRESS_EFFECT_NONE && s_press_active_count > 0)
    {
        ClearPressEffects();
    }

    /* 更新相位计数器 */
    s_effect_phase += RGB_UPDATE_INTERVAL_MS;
    if (s_effect_phase > 10000)
    {
        s_effect_phase = s_effect_phase % 10000;
    }

    /* 处理临时闪烁 */
    if (s_flash_active)
    {
        if (s_flash_remain > 0)
        {
            s_flash_remain--;
            WS2812_Set_Indicator(s_flash_r, s_flash_g, s_flash_b);
            WS2812_Update();
            return;
        }
        else
        {
            s_flash_active = false;
        }
    }

    /* 处理层切换闪烁（非阻塞状态机）：全局强制，暂停所有按键灯效果；指示灯独立 */
    if (s_layer_flash_active)
    {
        if (s_layer_flash_wait_ticks > 0)
        {
            s_layer_flash_wait_ticks--;
        }
        else
        {
            if (s_layer_flash_is_on)
            {
                /* 亮 -> 灭（仅按键灯熄灭，指示灯保持独立） */
                s_layer_flash_is_on = false;
                WS2812_FillKeys(0, 0, 0);
                s_layer_flash_wait_ticks = LAYER_FLASH_TICKS_PER_100MS;
            }
            else
            {
                /* 灭 -> 一次闪烁完成 */
                s_layer_flash_blinks_left--;
                if (s_layer_flash_blinks_left == 0)
                {
                    s_layer_flash_active = false;
                    s_effect_phase = 0;
                    /* 不 return，继续执行正常灯效 */
                }
                else
                {
                    s_layer_flash_is_on = true;
                    WS2812_FillKeys(0, 0, 0);
                    WS2812_Set(KeyToLed(s_layer_flash_layer), s_layer_flash_r, s_layer_flash_g, s_layer_flash_b);
                    s_layer_flash_wait_ticks = LAYER_FLASH_TICKS_PER_100MS;
                }
            }
        }
        if (s_layer_flash_active)
        {
            /* 指示灯始终独立运行 */
            ProcessIndicatorMode();
            WS2812_Update();
            return;
        }
    }

    /* 检测按下效果状态 */
    bool has_active_press =
        (cfg->press_effect != PRESS_EFFECT_NONE) && (s_press_active_count > 0);

    /* LIGHT_FADE 激活时，按键灯基底强制熄灭（仅按下的键亮起） */
    bool suppress_base = (cfg->press_effect == PRESS_EFFECT_LIGHT_FADE && has_active_press);

    /* RGB 开关关闭或基底被抑制时，仅关闭按键灯；指示灯与层指示仍保持 */
    if (!cfg->enabled || suppress_base)
    {
        if (WS2812_LED_NUM > 1)
        {
            WS2812_FillKeys(0, 0, 0);
        }
        ProcessIndicatorMode();
    }
    else
    {
        /* 根据模式处理 */
        switch (cfg->mode)
        {
        case KBD_RGB_OFF:
            /* 仅关闭按键灯，指示灯继续显示状态 */
            if (WS2812_LED_NUM > 1)
            {
                WS2812_FillKeys(0, 0, 0);
            }
            ProcessIndicatorMode();
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
            if (WS2812_LED_NUM > 1)
            {
                WS2812_FillKeys(0, 0, 0);
            }
            ProcessIndicatorMode();
            break;

        default:
            break;
        }
    }

    /* 叠加按下效果 */
    if (has_active_press)
    {
        ProcessPressEffects(cfg);
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
    if (mode > KBD_RGB_INDICATOR)
    {
        mode = KBD_RGB_OFF;
    }
    KBD_RGB_SetMode((kbd_rgb_mode_t)mode);
}

void KBD_RGB_PrevMode(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    uint8_t mode = cfg->mode;
    if (mode == KBD_RGB_OFF)
    {
        mode = KBD_RGB_INDICATOR;
    }
    else
    {
        mode--;
    }
    KBD_RGB_SetMode((kbd_rgb_mode_t)mode);
}

void KBD_RGB_Toggle(void)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    cfg->enabled = !cfg->enabled;
    LOG_D(TAG, "toggle=%d", cfg->enabled);

    if (!cfg->enabled)
    {
        if (WS2812_LED_NUM > 1)
        {
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
    if (new_val > 255)
        new_val = 255;
    KBD_RGB_SetBrightness((uint8_t)new_val);
}

void KBD_RGB_BrightnessDown(uint8_t step)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    int16_t new_val = cfg->brightness - step;
    if (new_val < 0)
        new_val = 0;
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
    if (state != s_current_state)
    {
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

void KBD_RGB_FlashLayer(uint8_t layer)
{
    if (layer >= 5)
        layer = 4;

    s_layer_flash_layer = layer;
    s_layer_flash_r = s_layer_colors[layer][0];
    s_layer_flash_g = s_layer_colors[layer][1];
    s_layer_flash_b = s_layer_colors[layer][2];
    s_layer_flash_blinks_left = KBD_LAYER_FLASH_BLINKS; /* 固定 3 次 */
    s_layer_flash_is_on = true;
    s_layer_flash_wait_ticks = LAYER_FLASH_TICKS_PER_100MS;
    s_layer_flash_active = true;

    /* 仅点亮对应层号位置的按键灯，其余熄灭；指示灯保持独立 */
    WS2812_FillKeys(0, 0, 0);
    WS2812_Set(KeyToLed(layer), s_layer_flash_r, s_layer_flash_g, s_layer_flash_b);
    ProcessIndicatorMode();
    WS2812_Update();
}

void KBD_RGB_RegisterKeyPress(uint8_t key_index)
{
    kbd_rgb_config_t *cfg = KBD_GetRgbConfig();
    if (!cfg->enabled || cfg->press_effect == PRESS_EFFECT_NONE ||
        key_index >= KBD_NUM_KEYS)
        return;

    if (cfg->press_effect == PRESS_EFFECT_LIGHT_FADE)
    {
        SampleModeKeyColor(cfg, key_index, true,
                           &s_press_color_r[key_index],
                           &s_press_color_g[key_index],
                           &s_press_color_b[key_index]);
    }

    if (s_press_fade[key_index] == 0)
        s_press_active_count++;

    s_press_fade[key_index] = 255;
}
