/********************************** USB HID Functions Implementation ***********
 * File Name          : usb_hid.c
 * Author             : Custom USB Library
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : USB HID 功能实现（基于 WCH 官方库）
 *******************************************************************************/

#include "usb_hid.h"
#include "CH59x_usbdev.h"
#include <string.h>

/* ==================== Global Variables ==================== */
USB_KeyboardReport_t g_KeyboardReport = {0};
USB_MouseReport_t    g_MouseReport = {0};
USB_ConsumerReport_t g_ConsumerReport = {0};
USB_ConfigReport_t   g_ConfigReport = {0};

uint8_t g_KeyboardLEDs = 0;

/* ==================== Keyboard Functions ==================== */

/**
 * @brief 初始化键盘
 */
void USB_Keyboard_Init(void)
{
    memset(&g_KeyboardReport, 0, sizeof(USB_KeyboardReport_t));
    g_KeyboardLEDs = 0;
}

/**
 * @brief 按下按键
 */
void USB_Keyboard_Press(uint8_t modifier, uint8_t *keys, uint8_t num_keys)
{
    g_KeyboardReport.modifier = modifier;
    
    uint8_t count = (num_keys > 6) ? 6 : num_keys;
    for(uint8_t i = 0; i < count; i++) {
        g_KeyboardReport.keycode[i] = keys[i];
    }
    for(uint8_t i = count; i < 6; i++) {
        g_KeyboardReport.keycode[i] = 0;
    }
    
    USB_Keyboard_SendReport();
}

/**
 * @brief 释放所有按键
 */
void USB_Keyboard_Release(void)
{
    memset(&g_KeyboardReport, 0, sizeof(USB_KeyboardReport_t));
    USB_Keyboard_SendReport();
}

/**
 * @brief 按下并释放一个按键
 */
void USB_Keyboard_Type(uint8_t modifier, uint8_t key)
{
    uint8_t keys[1] = {key};
    USB_Keyboard_Press(modifier, keys, 1);
    mDelaymS(20);
    USB_Keyboard_Release();
    mDelaymS(20);
}

/**
 * @brief 发送键盘报告
 */
void USB_Keyboard_SendReport(void)
{
    // 等待上一次传输完成
    while(!EP1_GetINSta());
    
    memcpy(pEP1_IN_DataBuf, &g_KeyboardReport, sizeof(USB_KeyboardReport_t));
    DevEP1_IN_Deal(sizeof(USB_KeyboardReport_t));
}

/**
 * @brief 设置键盘 LED 状态
 */
void USB_Keyboard_SetLEDs(uint8_t leds)
{
    g_KeyboardLEDs = leds;
    
    PRINT("Keyboard LEDs: ");
    if(leds & LED_NUM_LOCK) PRINT("[NumLock] ");
    if(leds & LED_CAPS_LOCK) PRINT("[CapsLock] ");
    if(leds & LED_SCROLL_LOCK) PRINT("[ScrollLock] ");
    PRINT("\n");
}

/* ==================== Mouse Functions ==================== */

/**
 * @brief 初始化鼠标
 */
void USB_Mouse_Init(void)
{
    memset(&g_MouseReport, 0, sizeof(USB_MouseReport_t));
}

/**
 * @brief 移动鼠标
 */
void USB_Mouse_Move(int8_t x, int8_t y, int8_t wheel)
{
    // 等待上一次传输完成
    while(!EP2_GetINSta());
    
    g_MouseReport.x = x;
    g_MouseReport.y = y;
    g_MouseReport.wheel = wheel;
    USB_Mouse_SendReport();
    
    // 发送后清除移动量，保留按键状态
    g_MouseReport.x = 0;
    g_MouseReport.y = 0;
    g_MouseReport.wheel = 0;
}

/**
 * @brief 点击鼠标按键
 */
void USB_Mouse_Click(uint8_t buttons)
{
    USB_Mouse_Press(buttons);
    mDelaymS(50);
    USB_Mouse_Release();
}

/**
 * @brief 按下鼠标按键
 */
void USB_Mouse_Press(uint8_t buttons)
{
    // 等待上一次传输完成
    while(!EP2_GetINSta());
    
    g_MouseReport.buttons = buttons;
    USB_Mouse_SendReport();
}

/**
 * @brief 释放所有鼠标按键
 */
void USB_Mouse_Release(void)
{
    // 等待上一次传输完成
    while(!EP2_GetINSta());
    
    g_MouseReport.buttons = 0;
    USB_Mouse_SendReport();
}

/**
 * @brief 发送鼠标报告
 */
void USB_Mouse_SendReport(void)
{
    memcpy(pEP2_IN_DataBuf, &g_MouseReport, sizeof(USB_MouseReport_t));
    DevEP2_IN_Deal(sizeof(USB_MouseReport_t));
}

/* ==================== Consumer Control Functions ==================== */

/**
 * @brief 初始化多媒体控制
 */
void USB_Consumer_Init(void)
{
    memset(&g_ConsumerReport, 0, sizeof(USB_ConsumerReport_t));
}

/**
 * @brief 按下多媒体键
 */
void USB_Consumer_Press(uint16_t key)
{
    // 等待上一次传输完成
    while(!EP3_GetINSta());
    
    g_ConsumerReport.key = key;
    USB_Consumer_SendReport();
}

/**
 * @brief 释放多媒体键
 */
void USB_Consumer_Release(void)
{
    // 等待上一次传输完成
    while(!EP3_GetINSta());
    
    g_ConsumerReport.key = 0;
    USB_Consumer_SendReport();
}

/**
 * @brief 发送多媒体控制报告
 */
void USB_Consumer_SendReport(void)
{
    memcpy(pEP3_IN_DataBuf, &g_ConsumerReport, sizeof(USB_ConsumerReport_t));
    DevEP3_IN_Deal(sizeof(USB_ConsumerReport_t));
}

/* ==================== Config Functions ==================== */

/**
 * @brief 初始化配置接口
 */
void USB_Config_Init(void)
{
    memset(&g_ConfigReport, 0, sizeof(USB_ConfigReport_t));
}

/**
 * @brief 发送配置响应
 */
void USB_Config_SendResponse(uint8_t cmd, uint8_t *data, uint8_t len)
{
    // 等待上一次传输完成
    while(!EP4_GetINSta());
    
    g_ConfigReport.cmd = cmd;
    memset(g_ConfigReport.data, 0, sizeof(g_ConfigReport.data));
    
    if(data && len > 0) {
        uint8_t copy_len = (len > 63) ? 63 : len;
        memcpy(g_ConfigReport.data, data, copy_len);
    }
    
    memcpy(pEP4_IN_DataBuf, &g_ConfigReport, sizeof(USB_ConfigReport_t));
    DevEP4_IN_Deal(sizeof(USB_ConfigReport_t));
}

/**
 * @brief 处理配置命令
 */
void USB_Config_ProcessCommand(USB_ConfigReport_t *report)
{
    uint8_t response_data[63] = {0};
    
    switch(report->cmd) {
        case CONFIG_CMD_GET_VERSION:
            // 返回版本信息
            response_data[0] = CONFIG_RESP_OK;
            response_data[1] = 2;  // 主版本号
            response_data[2] = 0;  // 次版本号
            USB_Config_SendResponse(CONFIG_CMD_GET_VERSION, response_data, 3);
            PRINT("Config: Get Version -> v2.0\n");
            break;
            
        case CONFIG_CMD_GET_STATUS:
            // 返回设备状态
            response_data[0] = CONFIG_RESP_OK;
            response_data[1] = 0x01;  // 设备就绪
            USB_Config_SendResponse(CONFIG_CMD_GET_STATUS, response_data, 2);
            PRINT("Config: Get Status -> Ready\n");
            break;
            
        case CONFIG_CMD_SET_CONFIG:
            // 设置配置
            PRINT("Config: Set Config - ");
            for(uint8_t i = 0; i < 8 && i < 63; i++) {
                PRINT("%02X ", report->data[i]);
            }
            PRINT("\n");
            
            response_data[0] = CONFIG_RESP_OK;
            USB_Config_SendResponse(CONFIG_CMD_SET_CONFIG, response_data, 1);
            break;
            
        case CONFIG_CMD_GET_CONFIG:
            // 获取配置
            response_data[0] = CONFIG_RESP_OK;
            response_data[1] = 0xAA;  // 示例配置数据
            response_data[2] = 0x55;
            response_data[3] = 0x12;
            response_data[4] = 0x34;
            USB_Config_SendResponse(CONFIG_CMD_GET_CONFIG, response_data, 5);
            PRINT("Config: Get Config\n");
            break;
            
        case CONFIG_CMD_RESET:
            // 重置设备
            PRINT("Config: Reset Device (not implemented)\n");
            response_data[0] = CONFIG_RESP_OK;
            USB_Config_SendResponse(CONFIG_CMD_RESET, response_data, 1);
            // 这里可以添加实际的重置代码
            // NVIC_SystemReset();
            break;
            
        default:
            // 无效命令
            response_data[0] = CONFIG_RESP_INVALID_CMD;
            USB_Config_SendResponse(report->cmd, response_data, 1);
            PRINT("Config: Invalid Command 0x%02X\n", report->cmd);
            break;
    }
}

/* ==================== USB Device Callbacks ==================== */

/**
 * @brief EP1 IN 回调 (键盘发送完成)
 */
void USB_DevEP1_IN_Callback(void)
{
    // 键盘数据发送完成
}

/**
 * @brief EP2 IN 回调 (鼠标发送完成)
 */
void USB_DevEP2_IN_Callback(void)
{
    // 鼠标数据发送完成
}

/**
 * @brief EP3 IN 回调 (多媒体发送完成)
 */
void USB_DevEP3_IN_Callback(void)
{
    // 多媒体数据发送完成
}

/**
 * @brief EP4 IN 回调 (配置发送完成)
 */
void USB_DevEP4_IN_Callback(void)
{
    // 配置数据发送完成
}

/**
 * @brief EP4 OUT 回调 (接收到配置数据)
 */
void DevEP4_OUT_Deal(uint8_t len)
{
    if(len >= sizeof(USB_ConfigReport_t)) {
        USB_ConfigReport_t *report = (USB_ConfigReport_t *)pEP4_OUT_DataBuf;
        USB_Config_ProcessCommand(report);
    }
}

/* 其他端点 OUT 处理（空实现） */
void DevEP1_OUT_Deal(uint8_t len) { }
void DevEP2_OUT_Deal(uint8_t len) { }
void DevEP3_OUT_Deal(uint8_t len) { }