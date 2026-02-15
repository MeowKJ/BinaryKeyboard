/**
 * @file    kbd_command.h
 * @brief   MeowKeyboard USB HID 命令处理接口
 * @author  MeowKJ
 * @version V1.0.0
 * @date    2024-11-07
 *
 * @details
 * 本模块处理来自上位机的 USB HID 配置命令：
 * - 系统信息查询
 * - 配置读写与重置
 * - 按键映射管理
 * - RGB 灯效控制
 * - 宏数据传输
 * - FN 键配置
 *
 * @note    与 WebHID 上位机配合使用
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#ifndef __KBD_COMMAND_H
#define __KBD_COMMAND_H

#include "kbd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/**
 * @defgroup KBD_CMD_API 命令处理接口
 * @brief 设备信息常量已在 kbd_types.h 中定义
 * @{
 */

/**
 * @brief 初始化命令处理器
 */
void KBD_Command_Init(void);

/**
 * @brief 处理 HID 配置命令
 *
 * @param[in] frame 命令帧指针
 * @return 0 成功
 * @return -1 未知命令
 */
int KBD_Command_Process(const kbd_cmd_frame_t *frame);

/**
 * @brief 发送命令响应
 *
 * @param[in] cmd  命令码 (回显)
 * @param[in] sub  子命令/序号
 * @param[in] data 响应数据指针 (可为 NULL)
 * @param[in] len  响应数据长度
 */
void KBD_Command_SendResponse(uint8_t cmd, uint8_t sub, const uint8_t *data, uint8_t len);

/** @} */ /* end of KBD_CMD_API */

#ifdef __cplusplus
}
#endif

#endif /* __KBD_COMMAND_H */
