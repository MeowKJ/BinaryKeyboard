#ifndef KBD_BATTERY_H
#define KBD_BATTERY_H

#include "CH59x_common.h"

/**
 * @file    kbd_battery.h
 * @brief   电池电压 ADC 采样 + TP4054 充电状态检测
 *
 * 硬件:
 * - PA14 (AIN4): VBAT 经 100K+100K 分压 (1/2) 后的 ADC 输入
 * - PA15: VBAT_AD_EN, 低电平启动分压电路 (PMOS, 默认下拉)
 * - PA13: TP4054 CHRG 引脚 (开漏输出)
 *   - 低电平: 正在充电
 *   - 高阻态: 未充电 (内部上拉读高)
 */

/*============================================================================*/
/**
 * @defgroup BAT_ChargeState 充电状态
 * @{
 */

typedef enum {
  BAT_CHG_NONE = 0,     /**< 未充电 (CHRG 高阻 → 上拉读高) */
  BAT_CHG_CHARGING = 1, /**< 充电中 (CHRG 拉低) */
} kbd_charge_state_t;

/** @} */

/*============================================================================*/
/**
 * @defgroup BAT_API 电池接口
 * @{
 */

/**
 * @brief 初始化电池检测 (ADC + 充电引脚)
 * @note 在 KBD_Storage_Init 之后调用
 */
void KBD_Battery_Init(void);

/**
 * @brief 获取电池电量百分比
 * @return 0-100
 */
uint8_t KBD_Battery_GetLevel(void);

/**
 * @brief 获取电池电压 (毫伏)
 * @return 电压 mV, 如 4200 = 4.2V
 */
uint16_t KBD_Battery_GetVoltage_mV(void);

/**
 * @brief 获取充电状态
 * @return kbd_charge_state_t
 */
kbd_charge_state_t KBD_Battery_GetChargeState(void);

/**
 * @brief 获取电压 (0.1V 单位, 用于 HID 协议)
 * @return 如 42 = 4.2V
 */
uint8_t KBD_Battery_GetVoltage_dV(void);

/** @} */

#endif /* KBD_BATTERY_H */
