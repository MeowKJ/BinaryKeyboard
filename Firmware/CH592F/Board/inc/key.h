#ifndef KBD_KEY_H_
#define KBD_KEY_H_

#include <stdint.h>
#include "kbd_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 消抖配置参数 ====================
 * KEY_LOCKOUT_MS: 锁定窗口时长（单位：ms）
 *   - 机械轴：推荐 8~12ms
 *   - 薄膜开关：推荐 15~20ms
 * 说明：边沿触发后，在此时间内屏蔽该键的后续中断，避免抖动误触发
 */
#ifndef KEY_LOCKOUT_MS
#define KEY_LOCKOUT_MS 10
#endif

/* KEY_ENABLE_RELEASE_EVENT: 是否产生释放事件
 *   - 键盘应用：必须为 1（需要同时检测按下/松开）
 *   - 简单按钮：可设为 0（仅关心按下事件）
 */
#ifndef KEY_ENABLE_RELEASE_EVENT
#define KEY_ENABLE_RELEASE_EVENT 1
#endif

/* KEY_QUEUE_SIZE: 事件队列大小（必须为 2 的幂次方，便于位运算优化）
 *   - 计算公式：(键数 * 2) * 余量系数(2~4)
 *   - 5键键盘：5*2*2=20，取32（2^5）
 */
#ifndef KEY_QUEUE_SIZE
#define KEY_QUEUE_SIZE 32
#endif

#ifndef FNKEY_QUEUE_SIZE
#define FNKEY_QUEUE_SIZE 16
#endif

/* 中断优先级配置（CH592支持0~7，数值越小优先级越高）
 *   - 定时器优先级应高于GPIO，确保锁定计时准确
 */
#ifndef KEY_IRQ_PRIORITY
#define KEY_IRQ_PRIORITY 3
#endif

#ifndef TIMER_IRQ_PRIORITY
#define TIMER_IRQ_PRIORITY 2
#endif

/* ==================== 事件类型定义 ==================== */

typedef enum {
    KEY_EVT_PRESS   = 1,
    KEY_EVT_RELEASE = 2,
} key_evt_type_t;

typedef struct {
    uint8_t  key;       // 键索引：0 ~ (KBD_NUM_KEYS-1)
    uint8_t  type;      // 事件类型：KEY_EVT_PRESS / KEY_EVT_RELEASE
    uint32_t tick_ms;   // 事件时间戳（ms），未启用时为 0
} key_event_t;

typedef enum {
    FNKEY_BOOT = 0,
    FNKEY_1    = 1,
    FNKEY_2    = 2,
} fnkey_id_t;

typedef struct {
    uint8_t  id;
    uint32_t tick_ms;
} fnkey_event_t;

/* ==================== 对外 API 函数 ==================== */

/**
 * @brief  初始化键盘驱动
 * @note   必须在使用其他 API 前调用
 */
void Key_Init(void);

/**
 * @brief  获取一个普通键事件
 * @param  evt: 输出参数，存放事件内容
 * @return 1=成功获取事件，0=队列为空
 * @note   非阻塞函数，应在主循环中轮询调用
 */
uint8_t Key_GetEvent(key_event_t *evt);

/**
 * @brief  获取一个功能键事件
 * @param  evt: 输出参数，存放事件内容
 * @return 1=成功获取事件，0=队列为空
 */
uint8_t FnKey_GetEvent(fnkey_event_t *evt);

/**
 * @brief  查询某个普通键的逻辑状态
 * @param  key_index: 键索引 (0 ~ KBD_NUM_KEYS-1)
 * @return 1=按下，0=释放，-1=非法索引
 */
int8_t Key_IsDown(uint8_t key_index);

/**
 * @brief  进入睡眠前的准备工作
 */
void Key_EnterSleep(void);

/**
 * @brief  唤醒后的恢复工作
 */
void Key_ExitSleep(void);

#ifdef __cplusplus
}
#endif

#endif /* KBD_KEY_H_ */