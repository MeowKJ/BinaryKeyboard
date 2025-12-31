#include "key.h"
#include "kbd_config.h"
#include <string.h>

/* ==================== 编译期检查 ==================== */
/* 队列大小必须是2的幂，以便用位运算优化取模：idx % 32 → idx & 31 */
#define STATIC_ASSERT(cond, msg) typedef char static_assert_##msg[(cond)?1:-1]
STATIC_ASSERT((KEY_QUEUE_SIZE & (KEY_QUEUE_SIZE-1)) == 0, key_queue_must_be_power_of_2);
STATIC_ASSERT((FNKEY_QUEUE_SIZE & (FNKEY_QUEUE_SIZE-1)) == 0, fn_queue_must_be_power_of_2);

/* ==================== 引脚映射表 ==================== */
#ifdef KBD_LAYOUT_5KEY
static const kbd_key_pin_t g_key_pins[KBD_NUM_KEYS] = {
    {KBD_K1_PORT, KBD_K1_PIN},
    {KBD_K2_PORT, KBD_K2_PIN},
    {KBD_K3_PORT, KBD_K3_PIN},
    {KBD_K4_PORT, KBD_K4_PIN},
    {KBD_K5_PORT, KBD_K5_PIN},
};
#endif

static const kbd_key_pin_t g_fnkey_pins[KBD_FN_NUM_KEYS] = {
    {KBD_FN_BOOT_PORT, KBD_FN_BOOT_PIN},
    {KBD_FN1_PORT,     KBD_FN1_PIN    },
    {KBD_FN2_PORT,     KBD_FN2_PIN    },
};

/* ==================== 事件队列（环形缓冲区）==================== */
static volatile key_event_t s_key_queue[KEY_QUEUE_SIZE];
static volatile uint8_t s_key_wr = 0;  // 写指针（ISR更新）
static volatile uint8_t s_key_rd = 0;  // 读指针（主循环更新）

static volatile fnkey_event_t s_fnkey_queue[FNKEY_QUEUE_SIZE];
static volatile uint8_t s_fnkey_wr = 0;
static volatile uint8_t s_fnkey_rd = 0;

/* ==================== 按键状态上下文 ==================== */
typedef struct {
    volatile uint16_t lock_ms;   // 锁定倒计时（ms），>0表示锁定中
    volatile uint8_t  is_down;   // 逻辑状态：1=按下，0=释放
    volatile uint8_t  expect;    // 期待的边沿：0=按下(下降沿)，1=释放(上升沿)
} key_ctx_t;

typedef struct {
    volatile uint16_t lock_ms;
} fnkey_ctx_t;

static volatile key_ctx_t s_key_ctx[KBD_NUM_KEYS];
static volatile fnkey_ctx_t s_fnkey_ctx[KBD_FN_NUM_KEYS];
static volatile uint8_t s_timer_active = 0;

/* ==================== 时间戳（可选）==================== */
static inline uint32_t GetTick(void) {
    return 0;  // 暂不实现，需要时可接入SysTick
}

/* ==================== 队列操作 ==================== */

/**
 * @brief  推送普通键事件（ISR调用）
 * @note   队列满时覆盖最旧事件
 */
static inline void PushKeyEvent(uint8_t key, uint8_t type, uint32_t tick) {
    uint8_t next = (s_key_wr + 1) & (KEY_QUEUE_SIZE - 1);
    
    if (next == s_key_rd) {
        // 队列满：移动读指针，覆盖最旧事件
        s_key_rd = (s_key_rd + 1) & (KEY_QUEUE_SIZE - 1);
    }
    
    s_key_queue[s_key_wr].key = key;
    s_key_queue[s_key_wr].type = type;
    s_key_queue[s_key_wr].tick_ms = tick;
    s_key_wr = next;
}

/**
 * @brief  推送功能键事件（ISR调用）
 */
static inline void PushFnKeyEvent(uint8_t id, uint32_t tick) {
    uint8_t next = (s_fnkey_wr + 1) & (FNKEY_QUEUE_SIZE - 1);
    
    if (next == s_fnkey_rd) {
        s_fnkey_rd = (s_fnkey_rd + 1) & (FNKEY_QUEUE_SIZE - 1);
    }
    
    s_fnkey_queue[s_fnkey_wr].id = id;
    s_fnkey_queue[s_fnkey_wr].tick_ms = tick;
    s_fnkey_wr = next;
}

/* ==================== GPIO操作封装 ==================== */

static inline void ConfigPinInput(const kbd_key_pin_t *pin) {
    if (pin->port == GPIO_PORT_A)
        GPIOA_ModeCfg(pin->pin, GPIO_ModeIN_PU);
    else
        GPIOB_ModeCfg(pin->pin, GPIO_ModeIN_PU);
}

static inline void ConfigPinFalling(const kbd_key_pin_t *pin) {
    if (pin->port == GPIO_PORT_A)
        GPIOA_ITModeCfg(pin->pin, GPIO_ITMode_FallEdge);
    else
        GPIOB_ITModeCfg(pin->pin, GPIO_ITMode_FallEdge);
}

static inline void ConfigPinRising(const kbd_key_pin_t *pin) {
    if (pin->port == GPIO_PORT_A)
        GPIOA_ITModeCfg(pin->pin, GPIO_ITMode_RiseEdge);
    else
        GPIOB_ITModeCfg(pin->pin, GPIO_ITMode_RiseEdge);
}

static inline uint32_t ReadPortFlags(gpio_port_t port) {
    return (port == GPIO_PORT_A) ? GPIOA_ReadITFlagPort() : GPIOB_ReadITFlagPort();
}

static inline void ClearPinFlag(gpio_port_t port, uint32_t pin) {
    if (port == GPIO_PORT_A)
        GPIOA_ClearITFlagBit(pin);
    else
        GPIOB_ClearITFlagBit(pin);
}

static inline void EnablePinIRQ(gpio_port_t port, uint32_t pin) {
    if (port == GPIO_PORT_A)
        R16_PA_INT_EN |= (uint16_t)pin;
    else
        R16_PB_INT_EN |= (uint16_t)pin;
}

static inline void DisablePinIRQ(gpio_port_t port, uint32_t pin) {
    if (port == GPIO_PORT_A)
        R16_PA_INT_EN &= (uint16_t)(~pin);
    else
        R16_PB_INT_EN &= (uint16_t)(~pin);
}

/* ==================== 定时器管理 ==================== */

static inline void StartTimer(void) {
    if (s_timer_active)
        return;
    
    TMR0_TimerInit(FREQ_SYS / 1000);  // 1ms周期
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    PFIC_SetPriority(TMR0_IRQn, TIMER_IRQ_PRIORITY);
    PFIC_EnableIRQ(TMR0_IRQn);
    s_timer_active = 1;
}

static inline void StopTimer(void) {
    TMR0_ITCfg(DISABLE, TMR0_3_IT_CYC_END);
    PFIC_DisableIRQ(TMR0_IRQn);
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    s_timer_active = 0;
}

/* ==================== 核心：边沿处理 ==================== */

/**
 * @brief  处理普通键的边沿中断
 * @note   立即上报事件 + 进入锁定窗口
 */
static inline void HandleKeyEdge(uint8_t idx) {
    const kbd_key_pin_t *pin = &g_key_pins[idx];
    
    // 锁定期间忽略
    if (s_key_ctx[idx].lock_ms > 0)
        return;
    
    uint32_t tick = GetTick();
    
    if (s_key_ctx[idx].expect == 0) {
        // 按下（下降沿）
        s_key_ctx[idx].is_down = 1;
        PushKeyEvent(idx, KEY_EVT_PRESS, tick);
        
#if KEY_ENABLE_RELEASE_EVENT
        s_key_ctx[idx].expect = 1;
        ConfigPinRising(pin);
#else
        ConfigPinFalling(pin);
#endif
    } else {
        // 释放（上升沿）
        s_key_ctx[idx].is_down = 0;
        PushKeyEvent(idx, KEY_EVT_RELEASE, tick);
        s_key_ctx[idx].expect = 0;
        ConfigPinFalling(pin);
    }
    
    // 进入锁定
    s_key_ctx[idx].lock_ms = KEY_LOCKOUT_MS;
    DisablePinIRQ(pin->port, pin->pin);
    StartTimer();
}

/**
 * @brief  处理功能键的下降沿（仅按下事件）
 */
static inline void HandleFnKeyEdge(uint8_t id) {
    const kbd_key_pin_t *pin = &g_fnkey_pins[id];
    
    if (s_fnkey_ctx[id].lock_ms > 0)
        return;
    
    PushFnKeyEvent(id, GetTick());
    
    s_fnkey_ctx[id].lock_ms = KEY_LOCKOUT_MS;
    DisablePinIRQ(pin->port, pin->pin);
    StartTimer();
}

/* ==================== GPIO中断处理 ==================== */

static inline void HandlePortIRQ(gpio_port_t port) {
    uint32_t flags = ReadPortFlags(port);
    uint16_t enabled = (port == GPIO_PORT_A) ? R16_PA_INT_EN : R16_PB_INT_EN;
    flags &= enabled;
    
    if (flags == 0)
        return;
    
    // 处理普通键
    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++) {
        if (g_key_pins[i].port != port)
            continue;
        
        uint32_t pin = g_key_pins[i].pin;
        if (flags & pin) {
            ClearPinFlag(port, pin);
            HandleKeyEdge(i);
        }
    }
    
    // 处理功能键
    for (uint8_t j = 0; j < KBD_FN_NUM_KEYS; j++) {
        if (g_fnkey_pins[j].port != port)
            continue;
        
        uint32_t pin = g_fnkey_pins[j].pin;
        if (flags & pin) {
            ClearPinFlag(port, pin);
            HandleFnKeyEdge(j);
        }
    }
}

__INTERRUPT __HIGH_CODE void GPIOA_IRQHandler(void) {
    HandlePortIRQ(GPIO_PORT_A);
}

__INTERRUPT __HIGH_CODE void GPIOB_IRQHandler(void) {
    HandlePortIRQ(GPIO_PORT_B);
}

/* ==================== 定时器中断：解锁倒计时 ==================== */

__INTERRUPT __HIGH_CODE void TMR0_IRQHandler(void) {
    if (!TMR0_GetITFlag(TMR0_3_IT_CYC_END))
        return;
    
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    
    uint8_t any_locked = 0;
    
    // 普通键倒计时
    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++) {
        uint16_t lm = s_key_ctx[i].lock_ms;
        if (lm == 0)
            continue;
        
        any_locked = 1;
        lm--;
        s_key_ctx[i].lock_ms = lm;
        
        if (lm == 0) {
            // 解锁
            gpio_port_t port = g_key_pins[i].port;
            uint32_t pin = g_key_pins[i].pin;
            ClearPinFlag(port, pin);
            EnablePinIRQ(port, pin);
        }
    }
    
    // 功能键倒计时
    for (uint8_t j = 0; j < KBD_FN_NUM_KEYS; j++) {
        uint16_t lm = s_fnkey_ctx[j].lock_ms;
        if (lm == 0)
            continue;
        
        any_locked = 1;
        lm--;
        s_fnkey_ctx[j].lock_ms = lm;
        
        if (lm == 0) {
            gpio_port_t port = g_fnkey_pins[j].port;
            uint32_t pin = g_fnkey_pins[j].pin;
            ClearPinFlag(port, pin);
            EnablePinIRQ(port, pin);
        }
    }
    
    // 所有键都解锁：关闭定时器
    if (!any_locked) {
        StopTimer();
    }
}

/* ==================== 对外API实现 ==================== */

uint8_t Key_GetEvent(key_event_t *evt) {
    if (!evt)
        return 0;
    
    if (s_key_rd == s_key_wr)
        return 0;
    
    *evt = s_key_queue[s_key_rd];
    s_key_rd = (s_key_rd + 1) & (KEY_QUEUE_SIZE - 1);
    return 1;
}

uint8_t FnKey_GetEvent(fnkey_event_t *evt) {
    if (!evt)
        return 0;
    
    if (s_fnkey_rd == s_fnkey_wr)
        return 0;
    
    *evt = s_fnkey_queue[s_fnkey_rd];
    s_fnkey_rd = (s_fnkey_rd + 1) & (FNKEY_QUEUE_SIZE - 1);
    return 1;
}

int8_t Key_IsDown(uint8_t key_index) {
    if (key_index >= KBD_NUM_KEYS)
        return -1;
    return s_key_ctx[key_index].is_down ? 1 : 0;
}

void Key_Init(void) {
    // 清空状态
    memset((void *)s_key_ctx, 0, sizeof(s_key_ctx));
    memset((void *)s_fnkey_ctx, 0, sizeof(s_fnkey_ctx));
    s_key_rd = s_key_wr = 0;
    s_fnkey_rd = s_fnkey_wr = 0;
    s_timer_active = 0;
    
    // 配置普通键
    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++) {
        const kbd_key_pin_t *pin = &g_key_pins[i];
        
        ConfigPinInput(pin);
        s_key_ctx[i].expect = 0;
        s_key_ctx[i].is_down = 0;
        ConfigPinFalling(pin);
        ClearPinFlag(pin->port, pin->pin);
        EnablePinIRQ(pin->port, pin->pin);
    }
    
    // 配置功能键
    for (uint8_t j = 0; j < KBD_FN_NUM_KEYS; j++) {
        const kbd_key_pin_t *pin = &g_fnkey_pins[j];
        
        ConfigPinInput(pin);
        ConfigPinFalling(pin);
        ClearPinFlag(pin->port, pin->pin);
        EnablePinIRQ(pin->port, pin->pin);
    }
    
    // 设置中断优先级并使能
    PFIC_SetPriority(GPIO_A_IRQn, KEY_IRQ_PRIORITY);
    PFIC_SetPriority(GPIO_B_IRQn, KEY_IRQ_PRIORITY);
    PFIC_EnableIRQ(GPIO_A_IRQn);
    PFIC_EnableIRQ(GPIO_B_IRQn);
}

void Key_EnterSleep(void) {
    // 清除锁定，恢复中断使能
    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++) {
        const kbd_key_pin_t *pin = &g_key_pins[i];
        s_key_ctx[i].expect = 0;
        s_key_ctx[i].lock_ms = 0;
        ConfigPinFalling(pin);
        ClearPinFlag(pin->port, pin->pin);
        EnablePinIRQ(pin->port, pin->pin);
    }
    
    for (uint8_t j = 0; j < KBD_FN_NUM_KEYS; j++) {
        const kbd_key_pin_t *pin = &g_fnkey_pins[j];
        s_fnkey_ctx[j].lock_ms = 0;
        ConfigPinFalling(pin);
        ClearPinFlag(pin->port, pin->pin);
        EnablePinIRQ(pin->port, pin->pin);
    }
    
    if (s_timer_active) {
        StopTimer();
    }
}

void Key_ExitSleep(void) {
    // 预留：恢复系统时钟等
}