/**
 * @file    kbd_rgb.h
 * @brief   MeowKeyboard RGB 灯效引擎接口
 * @author  MeowKJ
 * @version V1.0.0
 * @date    2024-11-07
 *
 * @details
 * 本模块提供 RGB 灯效控制功能：
 * - 多种灯效模式：静态、呼吸、闪烁、彩虹
 * - 状态指示：根据键盘连接状态自动变换颜色
 * - 亮度与速度调节
 * - 临时闪烁反馈
 *
 * @note    基于 WS2812 驱动实现
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#ifndef __KBD_RGB_H
#define __KBD_RGB_H

#include "kbd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/**
 * @defgroup KBD_RGB_Colors 状态指示颜色定义
 * @brief 预定义的状态指示颜色 (RGB 各 0-255)
 * @{
 */

/** @name 蓝牙未连接 - 红色 */
#define KBD_IND_BLE_DISCONN_R   255
#define KBD_IND_BLE_DISCONN_G   0
#define KBD_IND_BLE_DISCONN_B   0

/** @name 蓝牙广播中 - 蓝色 */
#define KBD_IND_BLE_ADV_R       0
#define KBD_IND_BLE_ADV_G       0
#define KBD_IND_BLE_ADV_B       255

/** @name 蓝牙已连接 - 绿色 */
#define KBD_IND_BLE_CONN_R      0
#define KBD_IND_BLE_CONN_G      255
#define KBD_IND_BLE_CONN_B      0

/** @name USB 已连接 - 白色 */
#define KBD_IND_USB_CONN_R      255
#define KBD_IND_USB_CONN_G      255
#define KBD_IND_USB_CONN_B      255

/** @name 低电量 - 红色 */
#define KBD_IND_LOW_BATT_R      255
#define KBD_IND_LOW_BATT_G      0
#define KBD_IND_LOW_BATT_B      0

/** @} */ /* end of KBD_RGB_Colors */

/*============================================================================*/
/**
 * @defgroup KBD_RGB_API RGB 控制接口
 * @{
 */

/**
 * @brief 初始化 RGB 灯效引擎
 *
 * 初始化 WS2812 驱动并加载 RGB 配置
 */
void KBD_RGB_Init(void);

/**
 * @brief RGB 效果处理函数
 *
 * 根据当前模式更新 LED 状态
 *
 * @note 应在主循环中定期调用，建议间隔 10-20ms
 */
void KBD_RGB_Process(void);

/**
 * @brief 设置 RGB 模式
 *
 * @param[in] mode 目标模式 @ref kbd_rgb_mode_t
 */
void KBD_RGB_SetMode(kbd_rgb_mode_t mode);

/**
 * @brief 获取当前 RGB 模式
 *
 * @return 当前模式 @ref kbd_rgb_mode_t
 */
kbd_rgb_mode_t KBD_RGB_GetMode(void);

/**
 * @brief 切换到下一个模式
 */
void KBD_RGB_NextMode(void);

/**
 * @brief 切换到上一个模式
 */
void KBD_RGB_PrevMode(void);

/**
 * @brief 开关 RGB
 *
 * 切换 RGB 总开关状态
 */
void KBD_RGB_Toggle(void);

/**
 * @brief 设置亮度
 *
 * @param[in] brightness 亮度值 (0-255)
 */
void KBD_RGB_SetBrightness(uint8_t brightness);

/**
 * @brief 增加亮度
 *
 * @param[in] step 增量 (建议 16)
 */
void KBD_RGB_BrightnessUp(uint8_t step);

/**
 * @brief 降低亮度
 *
 * @param[in] step 减量 (建议 16)
 */
void KBD_RGB_BrightnessDown(uint8_t step);

/**
 * @brief 设置静态颜色
 *
 * @param[in] r 红色分量 (0-255)
 * @param[in] g 绿色分量 (0-255)
 * @param[in] b 蓝色分量 (0-255)
 */
void KBD_RGB_SetColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置动画速度
 *
 * @param[in] speed 速度值 (0=最慢, 255=最快)
 */
void KBD_RGB_SetSpeed(uint8_t speed);

/**
 * @brief 设置状态指示
 *
 * 根据键盘当前状态设置指示灯颜色和效果
 *
 * @param[in] state 当前状态 @ref kbd_state_t
 */
void KBD_RGB_SetState(kbd_state_t state);

/**
 * @brief 启用/禁用状态指示
 *
 * @param[in] enable true=启用, false=禁用
 */
void KBD_RGB_EnableIndicator(bool enable);

/**
 * @brief 临时闪烁指示
 *
 * 用于按键反馈或提示，闪烁后自动恢复原状态
 *
 * @param[in] r           红色分量
 * @param[in] g           绿色分量
 * @param[in] b           蓝色分量
 * @param[in] duration_ms 闪烁持续时间 (毫秒)
 */
void KBD_RGB_Flash(uint8_t r, uint8_t g, uint8_t b, uint16_t duration_ms);

/**
 * @brief 层切换指示闪烁
 *
 * 切换层时闪烁对应颜色:
 * - 层1: 蓝色 (闪1次)
 * - 层2: 绿色 (闪2次)
 * - 层3: 黄色 (闪3次)
 * - 层4: 紫色 (闪4次)
 * - 层5: 红色 (闪5次)
 *
 * @param[in] layer 层号 (0-4)
 */
void KBD_RGB_FlashLayer(uint8_t layer);

/** @} */ /* end of KBD_RGB_API */

#ifdef __cplusplus
}
#endif

#endif /* __KBD_RGB_H */
