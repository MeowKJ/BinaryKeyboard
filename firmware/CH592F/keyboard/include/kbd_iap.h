/**
 * @file    kbd_iap.h
 * @brief   IAP (In-Application Programming) HID 命令处理接口
 * @author  MeowKJ
 *
 * @details
 * 通过 HID 通道接收新固件，写入 Image B，校验后触发 Bootloader 更新。
 *
 * HID 命令:
 *   0x80 IAP_INFO      查询 IAP 信息 (分区大小、当前标志)
 *   0x81 IAP_PREPARE   擦除 Image B 区域
 *   0x82 IAP_WRITE     写入固件数据块
 *   0x83 IAP_VERIFY    CRC32 校验 Image B
 *   0x84 IAP_ACTIVATE  设置 IAP 标志并复位
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#ifndef __KBD_IAP_H
#define __KBD_IAP_H

#include "kbd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 IAP 模块
 */
void KBD_IAP_Init(void);

/**
 * @brief 处理 IAP HID 命令
 *
 * @param[in] frame 命令帧指针
 * @return 0 成功处理, -1 未知子命令
 */
int KBD_IAP_Process(const kbd_cmd_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* __KBD_IAP_H */
