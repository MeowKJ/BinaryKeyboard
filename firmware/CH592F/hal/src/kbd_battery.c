/**
 * @file    kbd_battery.c
 * @brief   电池电压 ADC 采样 + TP4054 充电状态检测
 *
 * 采样电路 (VDD = 2.5V):
 * - PA15 (VBAT_AD_EN): 高电平启动分压电路, 低电平关闭
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

/* TMOS 事件 */
#define BAT_SAMPLE_EVT 0x0001
#define BAT_PERIODIC_EVT 0x0002

/* 采样时序 */
#define BAT_SETTLE_MS 5u
#define BAT_PERIODIC_MS (30u * 1000u)

/*============================================================================*/
/*                              私有变量                                      */
/*============================================================================*/

/** ADC 粗调校准偏移量 */
static int16_t s_adc_calib = 0;
static tmosTaskID s_task_id = TASK_NO_TASK;
static uint16_t s_cached_voltage_mv = 3700;
static uint8_t s_cache_ready = FALSE;
static uint8_t s_sample_pending = FALSE;

/*============================================================================*/
/*                              LiPo 电压 → 百分比                            */
/*============================================================================*/

/**
 * @brief 分段线性映射 (LiPo 3.0V - 4.2V)
 * 注: 实际充电时电压通常在4.15V-4.18V即停止，故调整最后一段使其更容易达到100%
 */
static const struct {
  uint16_t mv;
  uint8_t pct;
} lipo_curve[] = {
    {3000, 0},  {3300, 5},   {3600, 20}, {3700, 40},
    {3800, 60}, {3950, 80},  {4100, 95}, {4150, 100},
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

static uint16_t KBD_Battery_ProcessEvent(uint8_t task_id, uint16_t events);

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
 * @brief 在分压已稳定后执行多次 ADC 采样并求平均
 * @return 校准后的 ADC 值
 */
static uint16_t adc_sample_avg(void) {
  uint32_t sum = 0;

  for (uint8_t i = 0; i < 8; i++) {
    sum += ADC_ExcutSingleConver();
  }

  int32_t avg = (int32_t)(sum >> 3) + s_adc_calib;
  if (avg < 0)
    avg = 0;
  if (avg > 2047)
    avg = 2047;

  return (uint16_t)avg;
}

static void battery_schedule_periodic_refresh(void) {
  if (s_task_id != TASK_NO_TASK) {
    tmos_start_task(s_task_id, BAT_PERIODIC_EVT, MS1_TO_SYSTEM_TIME(BAT_PERIODIC_MS));
  }
}

static void battery_start_sample(void) {
  if (s_task_id == TASK_NO_TASK || s_sample_pending) {
    return;
  }

  /* 使能分压后交给 TMOS 延时，避免阻塞主循环。 */
  GPIOA_SetBits(KBD_VBAT_EN_PIN);
  adc_vbat_init();
  s_sample_pending = TRUE;
  tmos_start_task(s_task_id, BAT_SAMPLE_EVT, MS1_TO_SYSTEM_TIME(BAT_SETTLE_MS));
}

static void battery_finish_sample(void) {
  uint16_t adc;

  if (!s_sample_pending) {
    return;
  }

  /* 丢弃首次采样，等待模拟前端稳定。 */
  ADC_ExcutSingleConver();
  adc = adc_sample_avg();
  GPIOA_ResetBits(KBD_VBAT_EN_PIN);

  s_cached_voltage_mv = (uint16_t)((uint32_t)adc * KBD_VBAT_FULL_SCALE_MV / 2048);
  s_cache_ready = TRUE;
  s_sample_pending = FALSE;
}

/*============================================================================*/
/*                              公共接口                                       */
/*============================================================================*/

void KBD_Battery_Init(void) {
  /* PA15: 分压使能, 推挽输出, 默认低 (关闭分压省电) */
  GPIOA_ResetBits(KBD_VBAT_EN_PIN);
  GPIOA_ModeCfg(KBD_VBAT_EN_PIN, GPIO_ModeOut_PP_5mA);

  /* PA14: ADC 输入, 浮空 */
  GPIOA_ModeCfg(KBD_VBAT_ADC_PIN, GPIO_ModeIN_Floating);

  /* 初始化 ADC 并获取校准值 */
  adc_vbat_init();
  s_adc_calib = ADC_DataCalib_Rough();

  s_task_id = TMOS_ProcessEventRegister(KBD_Battery_ProcessEvent);
  if (s_task_id == TASK_NO_TASK) {
    LOG_W(TAG, "Battery TMOS task register failed");
  }

#if KBD_HAS_CHARGE_DET
  /* PA13: 充电引脚, 上拉输入 (检测 TP4054 开漏输出) */
  GPIOA_ModeCfg(KBD_CHG_PIN, GPIO_ModeIN_PU);
#endif

  KBD_Battery_RequestRefresh();
  battery_schedule_periodic_refresh();
  LOG_I(TAG, "Battery init: cached=%dmV calib=%d", s_cached_voltage_mv,
        s_adc_calib);
}

void KBD_Battery_RequestRefresh(void) {
  battery_start_sample();
}

uint16_t KBD_Battery_GetVoltage_mV(void) {
  if (!s_cache_ready || !s_sample_pending) {
    KBD_Battery_RequestRefresh();
  }
  return s_cached_voltage_mv;
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

static uint16_t KBD_Battery_ProcessEvent(uint8_t task_id, uint16_t events) {
  (void)task_id;

  if (events & BAT_SAMPLE_EVT) {
    battery_finish_sample();
    return (events ^ BAT_SAMPLE_EVT);
  }

  if (events & BAT_PERIODIC_EVT) {
    if (!s_sample_pending) {
      battery_start_sample();
    }
    battery_schedule_periodic_refresh();
    return (events ^ BAT_PERIODIC_EVT);
  }

  return 0;
}
