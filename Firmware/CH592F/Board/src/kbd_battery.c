/**
 * @file    kbd_battery.c
 * @brief   电池电压 ADC 采样 + TP4054 充电状态检测
 *
 * 采样电路 (VDD = 2.5V):
 * - PA15 (VBAT_AD_EN): 低电平启动分压电路 (PMOS, 默认下拉=使能)
 * - PA14 (AIN4): VBAT 经两个 100K 电阻分压 (1/2) 后进入 ADC
 * - ADC 配置: 外部通道 CH_EXTIN_4, PGA = -6dB (1/2x)
 * - Vref = 内部 1.05V 带隙基准 (与 VDD 无关)
 * - 公式: VBAT_mV = ADC_val * 4200 / 2048
 *   (1/2 分压 × 1/2 PGA = 1/4, Vref ≈ 1.05V, 满量程对应 4.2V)
 * - ADC 输入最大 2.1V < VDD 2.5V, 安全
 *
 * 充电检测:
 * - PA13 (TP4054 CHRG): 开漏输出, 内部上拉
 *   读高 = 未充电, 读低 = 充电中
 */

#include "kbd_battery.h"
#include "kbd_config.h"
#include "debug.h"

#define TAG "BAT"

/*============================================================================*/
/*                              私有变量                                      */
/*============================================================================*/

/** ADC 粗调校准偏移量 */
static int16_t s_adc_calib = 0;

/*============================================================================*/
/*                              LiPo 电压 → 百分比                            */
/*============================================================================*/

/**
 * @brief 分段线性映射 (LiPo 3.0V - 4.2V)
 */
static const struct {
  uint16_t mv;
  uint8_t pct;
} lipo_curve[] = {
    {3000, 0},  {3300, 5},   {3600, 20}, {3700, 40},
    {3800, 60}, {3950, 80},  {4100, 95}, {4200, 100},
};
#define CURVE_LEN (sizeof(lipo_curve) / sizeof(lipo_curve[0]))

static uint8_t voltage_to_percent(uint16_t mv) {
  if (mv <= lipo_curve[0].mv)
    return 0;
  if (mv >= lipo_curve[CURVE_LEN - 1].mv)
    return 100;

  for (uint8_t i = 1; i < CURVE_LEN; i++) {
    if (mv <= lipo_curve[i].mv) {
      uint16_t dv = lipo_curve[i].mv - lipo_curve[i - 1].mv;
      uint8_t dp = lipo_curve[i].pct - lipo_curve[i - 1].pct;
      return lipo_curve[i - 1].pct +
             (uint8_t)((uint32_t)(mv - lipo_curve[i - 1].mv) * dp / dv);
    }
  }
  return 100;
}

/*============================================================================*/
/*                              ADC 采样                                       */
/*============================================================================*/

/** 初始化外部 ADC 通道用于 VBAT 采样 */
static void adc_vbat_init(void) {
  /* 外部通道初始化: 采样时钟 3.2MHz, PGA = -6dB (1/2x) */
  ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_1_2);
  /* 选择 AIN4 通道 (PA14) */
  ADC_ChannelCfg(KBD_VBAT_ADC_CH);
}

/**
 * @brief 使能分压电路, 多次采样取平均, 然后关闭分压
 * @return 校准后的 ADC 值
 */
static uint16_t adc_sample_avg(void) {
  uint32_t sum = 0;

  /* 使能分压电路 (PA15 拉低) */
  GPIOA_ResetBits(KBD_VBAT_EN_PIN);

  /* 初始化 ADC 通道 */
  adc_vbat_init();

  /* 丢弃首次采样 (等待分压电路稳定) */
  ADC_ExcutSingleConver();

  for (uint8_t i = 0; i < 8; i++) {
    sum += ADC_ExcutSingleConver();
  }

  /* 关闭分压电路 (PA15 拉高, 省电) */
  GPIOA_SetBits(KBD_VBAT_EN_PIN);

  int32_t avg = (int32_t)(sum >> 3) + s_adc_calib;
  if (avg < 0)
    avg = 0;
  if (avg > 2047)
    avg = 2047;

  return (uint16_t)avg;
}

/*============================================================================*/
/*                              公共接口                                       */
/*============================================================================*/

void KBD_Battery_Init(void) {
  /* PA15: 分压使能, 推挽输出, 默认高 (关闭分压省电) */
  GPIOA_SetBits(KBD_VBAT_EN_PIN);
  GPIOA_ModeCfg(KBD_VBAT_EN_PIN, GPIO_ModeOut_PP_5mA);

  /* PA14: ADC 输入, 浮空 */
  GPIOA_ModeCfg(KBD_VBAT_ADC_PIN, GPIO_ModeIN_Floating);

  /* 初始化 ADC 并获取校准值 */
  adc_vbat_init();
  s_adc_calib = ADC_DataCalib_Rough();

#if KBD_HAS_CHARGE_DET
  /* PA13: 充电引脚, 上拉输入 (检测 TP4054 开漏输出) */
  GPIOA_ModeCfg(KBD_CHG_PIN, GPIO_ModeIN_PU);
#endif

  /* 初始采样验证 */
  uint16_t mv = KBD_Battery_GetVoltage_mV();
  LOG_I(TAG, "Battery init: %dmV calib=%d", mv, s_adc_calib);
}

uint16_t KBD_Battery_GetVoltage_mV(void) {
  uint16_t adc = adc_sample_avg();
  /*
   * 1/2 外部分压 × 1/2 PGA(-6dB) = 总衰减 1/4
   * Vref ≈ 1.05V, ADC 满量程 2048
   * VBAT_mV = adc * (1.05 * 4 * 1000) / 2048 = adc * 4200 / 2048
   */
  return (uint16_t)((uint32_t)adc * 4200 / 2048);
}

uint8_t KBD_Battery_GetLevel(void) {
  return voltage_to_percent(KBD_Battery_GetVoltage_mV());
}

kbd_charge_state_t KBD_Battery_GetChargeState(void) {
#if KBD_HAS_CHARGE_DET
  return (GPIOA_ReadPortPin(KBD_CHG_PIN) != 0) ? BAT_CHG_NONE
                                                : BAT_CHG_CHARGING;
#else
  return BAT_CHG_NONE;
#endif
}

uint8_t KBD_Battery_GetVoltage_dV(void) {
  return (uint8_t)(KBD_Battery_GetVoltage_mV() / 100);
}
