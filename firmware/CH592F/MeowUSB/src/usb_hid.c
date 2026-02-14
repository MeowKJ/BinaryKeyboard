/********************************** USB HID Functions Implementation ***********
 * File Name          : usb_hid.c
 * Author             : Custom USB Library
 * Version            : V2.0
 * Date               : 2024/11/07
 * Description        : USB HID 功能实现（基于 WCH 官方库）
 *******************************************************************************/

#include "usb_hid.h"
#include "CH59x_usbdev.h"
#include "kbd_command.h"
#include "kbd_types.h"
#include "debug.h"
#include <string.h>

#define TAG "USB"

/* ==================== Global Variables ==================== */
USB_KeyboardReport_t g_KeyboardReport = {0};
USB_MouseReport_t    g_MouseReport = {0};
USB_ConsumerReport_t g_ConsumerReport = {0};
USB_ConfigReport_t   g_ConfigReport = {0};
static USB_ConfigReport_t s_PendingConfigReport = {0};
static volatile uint8_t s_PendingConfigValid = 0;
static USB_ConfigReport_t s_PendingOutReport = {0};
static volatile uint8_t s_PendingOutValid = 0;

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
    if(!EP1_GetINSta()) {
        return;
    }
    
    memcpy(pEP1_IN_DataBuf, &g_KeyboardReport, sizeof(USB_KeyboardReport_t));
    DevEP1_IN_Deal(sizeof(USB_KeyboardReport_t));
}

/**
 * @brief 设置键盘 LED 状态
 */
void USB_Keyboard_SetLEDs(uint8_t leds)
{
    g_KeyboardLEDs = leds;
    
    LOG_D(TAG, "LEDs: %s%s%s",
          (leds & LED_NUM_LOCK) ? "Num " : "",
          (leds & LED_CAPS_LOCK) ? "Caps " : "",
          (leds & LED_SCROLL_LOCK) ? "Scroll" : "");
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
    if(!EP2_GetINSta()) {
        return;
    }
    
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
    if(!EP2_GetINSta()) {
        return;
    }
    
    g_MouseReport.buttons = buttons;
    USB_Mouse_SendReport();
}

/**
 * @brief 释放所有鼠标按键
 */
void USB_Mouse_Release(void)
{
    // 等待上一次传输完成
    if(!EP2_GetINSta()) {
        return;
    }
    
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
    if(!EP3_GetINSta()) {
        return;
    }
    
    g_ConsumerReport.key = key;
    USB_Consumer_SendReport();
}

/**
 * @brief 释放多媒体键
 */
void USB_Consumer_Release(void)
{
    // 等待上一次传输完成
    if(!EP3_GetINSta()) {
        return;
    }
    
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
    memset(&s_PendingConfigReport, 0, sizeof(USB_ConfigReport_t));
    s_PendingConfigValid = 0;
    memset(&s_PendingOutReport, 0, sizeof(USB_ConfigReport_t));
    s_PendingOutValid = 0;
}

/**
 * @brief 发送配置响应
 */
void USB_Config_SendResponse(uint8_t cmd, uint8_t *data, uint8_t len)
{
    USB_ConfigReport_t report;
    report.cmd = cmd;
    memset(report.data, 0, sizeof(report.data));

    if (data && len > 0) {
        uint8_t copy_len = (len > 63) ? 63 : len;
        memcpy(report.data, data, copy_len);
    }

    // Non-blocking path:
    // - if EP4 is ready and no pending packet, send immediately.
    // - otherwise queue one pending packet and return immediately.
    if (EP4_GetINSta() && !s_PendingConfigValid) {
        g_ConfigReport = report;
        memcpy(pEP4_IN_DataBuf, &g_ConfigReport, sizeof(USB_ConfigReport_t));
        DevEP4_IN_Deal(sizeof(USB_ConfigReport_t));
        return;
    }

    // If endpoint is busy, keep only the latest non-log response.
    if (cmd == KBD_CMD_LOG && s_PendingConfigValid) {
        return;
    }

    s_PendingConfigReport = report;
    s_PendingConfigValid = 1;
}

/**
 * @brief 处理配置命令
 */
void USB_Config_ProcessCommand(USB_ConfigReport_t *report)
{
    /* 
     * 帧格式: [CMD][SUB][LEN][DATA...]
     * report->cmd = CMD
     * report->data[0] = SUB
     * report->data[1] = LEN
     * report->data[2+] = DATA
     */
    kbd_cmd_frame_t frame;
    frame.cmd = report->cmd;
    frame.sub = report->data[0];
    frame.len = report->data[1];  /* 使用包中的实际长度 */
    memcpy(frame.data, &report->data[2], 59);  /* 跳过 SUB 和 LEN，从实际 DATA 开始 */

    KBD_Command_Process(&frame);
}

/**
 * @brief 在主循环中处理 EP4 OUT 命令（避免在 USB 中断中执行复杂逻辑）
 */
void USB_Config_PollProcess(void)
{
    USB_ConfigReport_t report;
    if (!s_PendingOutValid) {
        return;
    }

    report = s_PendingOutReport;
    s_PendingOutValid = 0;
    USB_Config_ProcessCommand(&report);
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
    // Send pending response without blocking.
    if (s_PendingConfigValid && EP4_GetINSta()) {
        g_ConfigReport = s_PendingConfigReport;
        s_PendingConfigValid = 0;
        memcpy(pEP4_IN_DataBuf, &g_ConfigReport, sizeof(USB_ConfigReport_t));
        DevEP4_IN_Deal(sizeof(USB_ConfigReport_t));
    }
}

/**
 * @brief EP4 OUT 回调 (接收到配置数据)
 */
void DevEP4_OUT_Deal(uint8_t len)
{
    if(len >= sizeof(USB_ConfigReport_t)) {
        memcpy(&s_PendingOutReport, pEP4_OUT_DataBuf, sizeof(USB_ConfigReport_t));
        s_PendingOutValid = 1;
    }
}

/* 其他端点 OUT 处理（空实现） */
void DevEP1_OUT_Deal(uint8_t len) { }
void DevEP2_OUT_Deal(uint8_t len) { }
void DevEP3_OUT_Deal(uint8_t len) { }
