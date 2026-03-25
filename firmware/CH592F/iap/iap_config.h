/**
 * @file    iap_config.h
 * @brief   IAP (In-Application Programming) 共享配置
 * @author  MeowKJ
 *
 * @details
 * JumpIAP / 高地址 IAP / 应用固件共用的 Flash 分区定义与 IAP 标志。
 *
 * Flash 分区布局 (448KB):
 *   0x00000 - 0x00FFF  JumpIAP 跳板 (4KB, 仅负责 j 0x6D000)
 *   0x01000 - 0x36FFF  Image A — 运行中的应用 (216KB)
 *   0x37000 - 0x6CFFF  Image B — 暂存新固件 (216KB)
 *   0x6D000 - 0x6FFFF  IAP App — 官方风格高地址 IAP 程序 (12KB)
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#ifndef __IAP_CONFIG_H
#define __IAP_CONFIG_H

#include <stdint.h>

/*============================================================================*/
/*                            Flash 分区常量                                   */
/*============================================================================*/

/** Image 大小: 216KB */
#define IMAGE_SIZE              (216u * 1024u)

/** Image A: 当前运行的应用固件 */
#define IMAGE_A_FLAG            0x01
#define IMAGE_A_START_ADD       0x01000u
#define IMAGE_A_SIZE            IMAGE_SIZE

/** Image B: 新固件暂存区 */
#define IMAGE_B_FLAG            0x02
#define IMAGE_B_START_ADD       (IMAGE_A_START_ADD + IMAGE_A_SIZE)  /* 0x37000 */
#define IMAGE_B_SIZE            IMAGE_SIZE

/** IAP 标志: 置此标志后重启，高地址 IAP 程序会执行 B→A 拷贝 */
#define IMAGE_IAP_FLAG          0x03

/** 高地址 IAP 程序区域 */
#define IMAGE_IAP_START_ADD     (IMAGE_B_START_ADD + IMAGE_B_SIZE)  /* 0x6D000 */
#define IMAGE_IAP_SIZE          (12u * 1024u)

/*============================================================================*/
/*                          DataFlash (EEPROM) 地址                            */
/*============================================================================*/

/**
 * IAP 标志存储地址 (DataFlash 空间)
 * 选用 0x6000，远离键盘配置区 (0x0000-0x2FFF)
 */
#define IAP_DATAFLASH_ADD       0x6000u

/*============================================================================*/
/*                              Bootloader 常量                                */
/*============================================================================*/

/** B→A 拷贝时的块大小 (需适配 RAM) */
#define IAP_COPY_CHUNK_SIZE     1024u

/*============================================================================*/
/*                              IAP HID 命令码                                 */
/*============================================================================*/

#define KBD_CMD_IAP_INFO        0x80  /**< 查询 IAP 信息 */
#define KBD_CMD_IAP_PREPARE     0x81  /**< 擦除 Image B 区域 */
#define KBD_CMD_IAP_WRITE       0x82  /**< 写入固件数据块 */
#define KBD_CMD_IAP_VERIFY      0x83  /**< CRC32 校验 Image B */
#define KBD_CMD_IAP_ACTIVATE    0x84  /**< 设置 IAP 标志并复位 */

/*============================================================================*/
/*                              DataFlash 结构体                               */
/*============================================================================*/

/** IAP 标志在 DataFlash 中的存储格式 (4 字节对齐) */
typedef struct {
    uint8_t  image_flag;    /**< 当前 Image 标志 */
    uint8_t  reserved[3];   /**< 保留 */
} iap_dataflash_info_t;

#endif /* __IAP_CONFIG_H */
