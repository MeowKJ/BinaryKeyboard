#include "key.h"
#include "config.h"

// 按键状态定义
#define KEY_STATE_IDLE      0   // 空闲状态
#define KEY_STATE_DEBOUNCE  1   // 消抖状态
#define KEY_STATE_PRESSED   2   // 按下状态
#define KEY_STATE_HOLD      3   // 保持按下状态

// 消抖时间(ms)
#define KEY_DEBOUNCE_TIME   5

// 按键信息结构体
typedef struct {
    uint8_t state;          // 当前状态
    uint8_t debounce_cnt;   // 消抖计数器
    uint8_t last_level;     // 上次电平
    uint8_t pressed_flag;   // 按下标志
    uint8_t released_flag;  // 释放标志
} key_info_t;

// 按键信息数组
static key_info_t key_info[KBD_NUM_KEYS] = {0};

void Key_Init(void)
{
    // 按键初始化
    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++)
    {
        kbd_key_pin_t key_pin = g_kbd_key_pin_map[i];
        if (key_pin.port == GPIO_PORT_A)
        {
            GPIOA_ModeCfg((uint32_t)key_pin.pin, GPIO_ModeIN_PU);
        }
        else if (key_pin.port == GPIO_PORT_B)
        {
            GPIOB_ModeCfg((uint32_t)key_pin.pin, GPIO_ModeIN_PU);
        }
        
        // 初始化按键状态
        key_info[i].state = KEY_STATE_IDLE;
        key_info[i].debounce_cnt = 0;
        key_info[i].last_level = 1; // 上拉输入，默认高电平
        key_info[i].pressed_flag = 0;
        key_info[i].released_flag = 0;
    }

    // 定时器初始化 1ms扫描一次按键
    TMR0_TimerInit(FREQ_SYS / 1000); // 1ms
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    PFIC_EnableIRQ(TMR0_IRQn);
}

/*********************************************************************
 * @fn      Key_ReadPin
 *
 * @brief   读取按键引脚电平
 *
 * @param   key_index - 按键索引
 *
 * @return  引脚电平 (0-按下, 1-释放)
 */
static uint8_t Key_ReadPin(uint8_t key_index)
{
    kbd_key_pin_t key_pin = g_kbd_key_pin_map[key_index];
    
    if (key_pin.port == GPIO_PORT_A)
    {
        return GPIOA_ReadPortPin((uint32_t)key_pin.pin) ? 1 : 0;
    }
    else if (key_pin.port == GPIO_PORT_B)
    {
        return GPIOB_ReadPortPin((uint32_t)key_pin.pin) ? 1 : 0;
    }
    
    return 1; // 默认返回高电平
}

/*********************************************************************
 * @fn      Key_Scan
 *
 * @brief   按键扫描函数(在定时器中断中调用)
 *
 * @return  none
 */
void Key_Scan(void)
{
    for (uint8_t i = 0; i < KBD_NUM_KEYS; i++)
    {
        uint8_t current_level = Key_ReadPin(i);
        
        switch (key_info[i].state)
        {
            case KEY_STATE_IDLE:
                if (current_level == 0) // 检测到低电平(按键按下)
                {
                    key_info[i].state = KEY_STATE_DEBOUNCE;
                    key_info[i].debounce_cnt = 0;
                }
                break;
                
            case KEY_STATE_DEBOUNCE:
                if (current_level == 0)
                {
                    key_info[i].debounce_cnt++;
                    if (key_info[i].debounce_cnt >= KEY_DEBOUNCE_TIME)
                    {
                        key_info[i].state = KEY_STATE_PRESSED;
                        key_info[i].pressed_flag = 1; // 设置按下标志
                    }
                }
                else
                {
                    key_info[i].state = KEY_STATE_IDLE;
                    key_info[i].debounce_cnt = 0;
                }
                break;
                
            case KEY_STATE_PRESSED:
                key_info[i].state = KEY_STATE_HOLD;
                break;
                
            case KEY_STATE_HOLD:
                if (current_level == 1) // 检测到高电平(按键释放)
                {
                    key_info[i].state = KEY_STATE_IDLE;
                    key_info[i].released_flag = 1; // 设置释放标志
                }
                break;
                
            default:
                key_info[i].state = KEY_STATE_IDLE;
                break;
        }
        
        key_info[i].last_level = current_level;
    }
}

/*********************************************************************
 * @fn      Key_IsPressed
 *
 * @brief   检测按键是否被按下(单次触发)
 *
 * @param   key_index - 按键索引
 *
 * @return  1-按键被按下, 0-按键未按下
 */
uint8_t Key_IsPressed(uint8_t key_index)
{
    if (key_index >= KBD_NUM_KEYS)
        return 0;
    
    if (key_info[key_index].pressed_flag)
    {
        key_info[key_index].pressed_flag = 0; // 清除标志
        return 1;
    }
    
    return 0;
}

/*********************************************************************
 * @fn      Key_IsReleased
 *
 * @brief   检测按键是否被释放(单次触发)
 *
 * @param   key_index - 按键索引
 *
 * @return  1-按键被释放, 0-按键未释放
 */
uint8_t Key_IsReleased(uint8_t key_index)
{
    if (key_index >= KBD_NUM_KEYS)
        return 0;
    
    if (key_info[key_index].released_flag)
    {
        key_info[key_index].released_flag = 0; // 清除标志
        return 1;
    }
    
    return 0;
}

/*********************************************************************
 * @fn      Key_GetState
 *
 * @brief   获取按键当前状态(持续状态)
 *
 * @param   key_index - 按键索引
 *
 * @return  1-按键按下, 0-按键释放
 */
uint8_t Key_GetState(uint8_t key_index)
{
    if (key_index >= KBD_NUM_KEYS)
        return 0;
    
    return (key_info[key_index].state == KEY_STATE_HOLD) ? 1 : 0;
}

/*********************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   TMR0中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler(void) // TMR0 定时中断
{
    if(TMR0_GetITFlag(TMR0_3_IT_CYC_END))
    {
        TMR0_ClearITFlag(TMR0_3_IT_CYC_END); // 清除中断标志
        
        Key_Scan(); // 执行按键扫描
    }
}