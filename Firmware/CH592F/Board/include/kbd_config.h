#ifndef KBD_CONFIG_H
#define KBD_CONFIG_H

#include "CH59x_common.h"

/*============================================================================*/
/**
 * @defgroup KBD_Type 键盘类型定义
 * @brief 支持多种键盘外形
 * @{
 */

/**
 * @brief 键盘类型枚举
 */
typedef enum {
    KBD_TYPE_BASIC = 0,     /**< 基础款: 4 键 */
    KBD_TYPE_5KEYS = 1,     /**< 五键款: 5 键 */
    KBD_TYPE_KNOB  = 2,     /**< 旋钮款: 4 键 + 1 旋钮 (等效 7 键) */
} kbd_type_t;

/**
 * @brief 各类型键盘的按键数量
 */
#define KBD_KEYS_BASIC      4u      /**< 基础款按键数 */
#define KBD_KEYS_5KEYS      5u      /**< 五键款按键数 */
#define KBD_KEYS_KNOB       7u      /**< 旋钮款按键数 (4 普通键 + 3 旋钮动作) */

/**
 * @brief 旋钮动作索引 (用于旋钮款)
 * @note 旋钮动作映射到虚拟按键索引
 */
#define KBD_KNOB_CW_IDX     4u      /**< 顺时针旋转 */
#define KBD_KNOB_CCW_IDX    5u      /**< 逆时针旋转 */
#define KBD_KNOB_CLICK_IDX  6u      /**< 旋钮按下 */

/** @} */

/*============================================================================*/
/**
 * @defgroup KBD_Layout 键盘布局选择
 * @brief 编译时选择键盘类型 (只能启用一个)
 * @{
 */

// 选择一个键盘布局 (取消注释对应的行)
// #define KBD_LAYOUT_BASIC        /**< 基础款 */
#define KBD_LAYOUT_5KEY         /**< 五键款 */
// #define KBD_LAYOUT_KNOB         /**< 旋钮款 */

/** @} */

/*============================================================================*/
/**
 * @defgroup KBD_GPIO GPIO 类型定义
 * @{
 */

typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
} gpio_port_t;

typedef struct {
    gpio_port_t port;
    uint32_t pin;       /**< 位掩码：GPIO_Pin_x */
} kbd_key_pin_t;

/** @} */

/*============================================================================*/
/**
 * @defgroup KBD_Special 特殊 IO 定义
 * @{
 */

/* WS2812 数据引脚: TMR1 PWM 输出 (PA10) */
#define WS2812_PORT         GPIO_PORT_A
#define WS2812_PIN          GPIO_Pin_10

/* WS2812 使能引脚 (PA9) */
#define WS2812_EN_PORT      GPIO_PORT_A
#define WS2812_EN_PIN       GPIO_Pin_9

/* WS2812 LED 配置：仅指示灯模式 (注释掉则启用按键灯) */
// #define WS2812_INDICATOR_ONLY

/** @} */

/*============================================================================*/
/**
 * @defgroup KBD_FnKeys 功能键定义
 * @brief FN 键只产生下降沿事件，支持短按/长按
 * @{
 */

#define KBD_FN_NUM_KEYS     2u

/* BOOT 按键: PB22 */
#define KBD_FN_BOOT_PORT    GPIO_PORT_B
#define KBD_FN_BOOT_PIN     GPIO_Pin_22

/* FN1: PA4 */
#define KBD_FN1_PORT        GPIO_PORT_A
#define KBD_FN1_PIN         GPIO_Pin_4

/* FN2: PA5 */
#define KBD_FN2_PORT        GPIO_PORT_A
#define KBD_FN2_PIN         GPIO_Pin_5

/** @} */

/*============================================================================*/
/**
 * @defgroup KBD_Keys 普通按键定义
 * @brief 根据键盘类型定义不同的引脚映射
 * @{
 */

#if defined(KBD_LAYOUT_BASIC)
/*---------------------------------------------------------------------------*/
/* 基础款: 4 键, 4 层                                                          */
/*---------------------------------------------------------------------------*/
#define KBD_CURRENT_TYPE    KBD_TYPE_BASIC
#define KBD_NUM_KEYS        KBD_KEYS_BASIC
#define KBD_DEFAULT_LAYERS  4u              /**< 默认层数 = 按键数 */

#define KBD_K1_PORT         GPIO_PORT_B
#define KBD_K1_PIN          GPIO_Pin_4

#define KBD_K2_PORT         GPIO_PORT_B
#define KBD_K2_PIN          GPIO_Pin_7

#define KBD_K3_PORT         GPIO_PORT_B
#define KBD_K3_PIN          GPIO_Pin_12

#define KBD_K4_PORT         GPIO_PORT_A
#define KBD_K4_PIN          GPIO_Pin_8

#elif defined(KBD_LAYOUT_5KEY)
/*---------------------------------------------------------------------------*/
/* 五键款: 5 键, 5 层                                                          */
/*---------------------------------------------------------------------------*/
#define KBD_CURRENT_TYPE    KBD_TYPE_5KEYS
#define KBD_NUM_KEYS        KBD_KEYS_5KEYS
#define KBD_DEFAULT_LAYERS  5u              /**< 默认层数 = 按键数 */

#define KBD_K1_PORT         GPIO_PORT_B
#define KBD_K1_PIN          GPIO_Pin_4

#define KBD_K2_PORT         GPIO_PORT_B
#define KBD_K2_PIN          GPIO_Pin_7

#define KBD_K3_PORT         GPIO_PORT_B
#define KBD_K3_PIN          GPIO_Pin_12

#define KBD_K4_PORT         GPIO_PORT_A
#define KBD_K4_PIN          GPIO_Pin_8

#define KBD_K5_PORT         GPIO_PORT_A
#define KBD_K5_PIN          GPIO_Pin_12

#elif defined(KBD_LAYOUT_KNOB)
/*---------------------------------------------------------------------------*/
/* 旋钮款: 4 普通键 + 1 旋钮, 5 层 (4键 + 旋钮按下)                             */
/*---------------------------------------------------------------------------*/
#define KBD_CURRENT_TYPE    KBD_TYPE_KNOB
#define KBD_NUM_KEYS        4u              /**< 实际物理普通键数量 */
#define KBD_TOTAL_KEYS      KBD_KEYS_KNOB   /**< 总虚拟键数 (含旋钮) */
#define KBD_DEFAULT_LAYERS  5u              /**< 默认层数 = 4键 + 旋钮按下 */
#define KBD_HAS_ENCODER     1               /**< 启用编码器支持 */

/* 普通按键 */
#define KBD_K1_PORT         GPIO_PORT_B
#define KBD_K1_PIN          GPIO_Pin_4

#define KBD_K2_PORT         GPIO_PORT_B
#define KBD_K2_PIN          GPIO_Pin_7

#define KBD_K3_PORT         GPIO_PORT_B
#define KBD_K3_PIN          GPIO_Pin_12

#define KBD_K4_PORT         GPIO_PORT_A
#define KBD_K4_PIN          GPIO_Pin_8

/* 旋钮编码器引脚 */
#define KBD_ENCODER_A_PORT  GPIO_PORT_A
#define KBD_ENCODER_A_PIN   GPIO_Pin_12

#define KBD_ENCODER_B_PORT  GPIO_PORT_A
#define KBD_ENCODER_B_PIN   GPIO_Pin_13

/* 旋钮按键 (可选，复用 FN 或独立) */
#define KBD_ENCODER_BTN_PORT GPIO_PORT_A
#define KBD_ENCODER_BTN_PIN  GPIO_Pin_14

#else
#error "请在 kbd_config.h 中选择一个键盘布局 (KBD_LAYOUT_BASIC / KBD_LAYOUT_5KEY / KBD_LAYOUT_KNOB)"
#endif

/* 统一的虚拟按键总数 (用于按键映射) */
#ifndef KBD_TOTAL_KEYS
#define KBD_TOTAL_KEYS      KBD_NUM_KEYS
#endif

/** @} */

/*============================================================================*/
/**
 * @defgroup KBD_Utils 工具函数
 * @{
 */

/**
 * @brief 获取当前键盘类型
 * @return 键盘类型枚举值
 */
static inline kbd_type_t KBD_GetType(void) {
    return KBD_CURRENT_TYPE;
}

/**
 * @brief 获取当前键盘的总键位数 (用于映射)
 * @return 键位数量
 */
static inline uint8_t KBD_GetTotalKeyCount(void) {
    return KBD_TOTAL_KEYS;
}

/**
 * @brief 获取当前键盘的物理按键数
 * @return 物理按键数量
 */
static inline uint8_t KBD_GetPhysicalKeyCount(void) {
    return KBD_NUM_KEYS;
}

/**
 * @brief 获取默认层数
 * @return 层数 (等于按键数或5)
 */
static inline uint8_t KBD_GetDefaultLayers(void) {
    return KBD_DEFAULT_LAYERS;
}

/** @} */

#endif /* KBD_CONFIG_H */
