/**
 * @file    kbd_iap.c
 * @brief   IAP (In-Application Programming) HID 命令处理实现
 * @author  MeowKJ
 *
 * @details
 * 实现通过 HID 通道接收固件，写入 Image B，校验后触发更新。
 *
 * 写入协议:
 *   帧格式: [0x82][seq_hi][len][offset_hi, offset_lo, data × 56]
 *   - seq_hi: 页序号 (0, 1, 2, ...)
 *   - offset: 写入偏移 (相对 Image B 起始)
 *   - data: 最多 56 字节有效载荷
 *   - 以 256 字节页为单位缓冲，满页或 ACTIVATE 前 flush
 *
 * CRC32 校验:
 *   使用标准 CRC-32 (ISO 3309) 对 Image B 的有效区域计算校验值
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#include "kbd_iap.h"
#include "kbd_command.h"
#include "iap_config.h"
#include "hal_utils.h"
#include "debug.h"
#include "CH59x_common.h"

#include <string.h>

#define TAG "IAP"

/*============================================================================*/
/*                              IAP 状态                                       */
/*============================================================================*/

/** IAP 会话状态 */
typedef enum
{
    IAP_STATE_IDLE = 0,  /**< 空闲 */
    IAP_STATE_PREPARING, /**< 正在擦除 Image B */
    IAP_STATE_READY,     /**< Image B 已擦除，等待写入 */
    IAP_STATE_WRITING,   /**< 正在写入 */
    IAP_STATE_WRITTEN,   /**< 写入完成，等待校验 */
} iap_state_t;

static iap_state_t s_state = IAP_STATE_IDLE;

/** 已写入的总字节数 */
static uint32_t s_written_bytes = 0;

/** 页缓冲 (256 字节，Flash 写入最优单位) */
__attribute__((aligned(8))) static uint8_t s_page_buf[EEPROM_PAGE_SIZE];

/** 页缓冲当前填充位置 */
static uint16_t s_page_offset = 0;

/** 当前页的 Flash 目标地址 */
static uint32_t s_page_flash_addr = IMAGE_B_START_ADD;

/*============================================================================*/
/*                              CRC32 实现                                     */
/*============================================================================*/

/**
 * @brief 计算 CRC32 (ISO 3309 / ITU-T)
 * @note  无查表，节省 Flash；速度对 IAP 场景足够
 */
static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    while (len--)
    {
        crc ^= *data++;
        for (int i = 0; i < 8; i++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/*============================================================================*/
/*                              内部函数                                       */
/*============================================================================*/

/**
 * @brief 将页缓冲写入 Flash 并重置
 */
static int FlushPageBuffer(void)
{
    if (s_page_offset == 0)
        return 0;

    /* 补齐到 4 字节对齐 (Flash 写入最小单位) */
    while (s_page_offset & 0x03)
        s_page_buf[s_page_offset++] = 0xFF;

    uint32_t ret = FLASH_ROM_WRITE(s_page_flash_addr, s_page_buf, s_page_offset);
    if (ret != 0)
    {
        LOG_E(TAG, "Flash write fail @0x%05X len=%d ret=%d",
              s_page_flash_addr, s_page_offset, ret);
        return -1;
    }

    s_page_flash_addr += s_page_offset;
    s_page_offset = 0;
    return 0;
}

/*============================================================================*/
/*                          命令处理: IAP_INFO (0x80)                          */
/*============================================================================*/

/**
 * @brief 查询 IAP 信息
 *
 * 响应 (12 字节):
 *   [0]  status
 *   [1]  iap_state
 *   [2..5]  Image B 起始地址 (LE)
 *   [6..9]  Image B 大小 (LE)
 *   [10..11] 已写入字节数高低 (用于断点续传预留)
 */
static void HandleIapInfo(const kbd_cmd_frame_t *frame)
{
    uint8_t resp[12];
    resp[0] = KBD_RESP_OK;
    resp[1] = (uint8_t)s_state;
    resp[2] = (IMAGE_B_START_ADD >> 0) & 0xFF;
    resp[3] = (IMAGE_B_START_ADD >> 8) & 0xFF;
    resp[4] = (IMAGE_B_START_ADD >> 16) & 0xFF;
    resp[5] = (IMAGE_B_START_ADD >> 24) & 0xFF;
    resp[6] = (IMAGE_B_SIZE >> 0) & 0xFF;
    resp[7] = (IMAGE_B_SIZE >> 8) & 0xFF;
    resp[8] = (IMAGE_B_SIZE >> 16) & 0xFF;
    resp[9] = (IMAGE_B_SIZE >> 24) & 0xFF;
    resp[10] = (s_written_bytes >> 8) & 0xFF;
    resp[11] = (s_written_bytes >> 0) & 0xFF;

    KBD_Command_SendResponse(KBD_CMD_IAP_INFO, 0, resp, 12);
}

/*============================================================================*/
/*                        命令处理: IAP_PREPARE (0x81)                         */
/*============================================================================*/

/**
 * @brief 擦除 Image B 全区
 *
 * 请求: [0x81][0][0]
 * 响应: [0x81][0][1][status]
 */
static void HandleIapPrepare(const kbd_cmd_frame_t *frame)
{
    uint8_t resp[1];

    LOG_I(TAG, "Preparing: erase Image B (0x%05X, %dKB)",
          IMAGE_B_START_ADD, IMAGE_B_SIZE / 1024);

    s_state = IAP_STATE_PREPARING;

    /* 擦除 Image B */
    uint32_t ret = FLASH_ROM_ERASE(IMAGE_B_START_ADD, IMAGE_B_SIZE);
    if (ret != 0)
    {
        LOG_E(TAG, "Erase failed: %d", ret);
        s_state = IAP_STATE_IDLE;
        resp[0] = KBD_RESP_ERR_FLASH;
        KBD_Command_SendResponse(KBD_CMD_IAP_PREPARE, 0, resp, 1);
        return;
    }

    /* 重置写入状态 */
    s_written_bytes = 0;
    s_page_offset = 0;
    s_page_flash_addr = IMAGE_B_START_ADD;

    s_state = IAP_STATE_READY;
    resp[0] = KBD_RESP_OK;
    KBD_Command_SendResponse(KBD_CMD_IAP_PREPARE, 0, resp, 1);

    LOG_I(TAG, "Image B erased, ready for data");
}

/*============================================================================*/
/*                         命令处理: IAP_WRITE (0x82)                          */
/*============================================================================*/

/**
 * @brief 写入固件数据块
 *
 * 请求: [0x82][seq][len][offset_hi, offset_lo, data...]
 *   seq: 页序号 (仅用于日志)
 *   data[0..1]: 写入偏移 (相对 Image B，big-endian，以 256B 为单位)
 *   data[2..]: 有效载荷 (最多 56 字节)
 *
 * 响应: [0x82][seq][4][status, written_hi, written_mid, written_lo]
 */
static void HandleIapWrite(const kbd_cmd_frame_t *frame)
{
    uint8_t resp[4];
    uint8_t seq = frame->sub;

    if (s_state != IAP_STATE_READY && s_state != IAP_STATE_WRITING)
    {
        resp[0] = KBD_RESP_ERR_BUSY;
        resp[1] = 0;
        resp[2] = 0;
        resp[3] = 0;
        KBD_Command_SendResponse(KBD_CMD_IAP_WRITE, seq, resp, 4);
        return;
    }

    if (frame->len < 3)
    {
        resp[0] = KBD_RESP_ERR_PARAM;
        resp[1] = 0;
        resp[2] = 0;
        resp[3] = 0;
        KBD_Command_SendResponse(KBD_CMD_IAP_WRITE, seq, resp, 4);
        return;
    }

    s_state = IAP_STATE_WRITING;

    /* 解析偏移和载荷 */
    uint32_t offset = ((uint32_t)frame->data[0] << 8) | frame->data[1];
    offset *= EEPROM_PAGE_SIZE; /* 以 256B 页为寻址单位 */
    uint8_t payload_len = frame->len - 2;
    const uint8_t *payload = &frame->data[2];

    /* 边界检查 */
    if (offset + payload_len > IMAGE_B_SIZE)
    {
        resp[0] = KBD_RESP_ERR_TOO_LARGE;
        resp[1] = 0;
        resp[2] = 0;
        resp[3] = 0;
        KBD_Command_SendResponse(KBD_CMD_IAP_WRITE, seq, resp, 4);
        return;
    }

    /* 如果偏移跳到新页，先 flush 当前缓冲 */
    uint32_t target_page_start = IMAGE_B_START_ADD + offset;
    if (target_page_start != s_page_flash_addr)
    {
        if (FlushPageBuffer() != 0)
        {
            resp[0] = KBD_RESP_ERR_FLASH;
            resp[1] = 0;
            resp[2] = 0;
            resp[3] = 0;
            KBD_Command_SendResponse(KBD_CMD_IAP_WRITE, seq, resp, 4);
            return;
        }
        s_page_flash_addr = target_page_start;
    }

    /* 写入页缓冲 */
    uint8_t remaining = payload_len;
    const uint8_t *src = payload;
    while (remaining > 0)
    {
        uint16_t space = EEPROM_PAGE_SIZE - s_page_offset;
        uint8_t chunk = (remaining < space) ? remaining : (uint8_t)space;

        memcpy(&s_page_buf[s_page_offset], src, chunk);
        s_page_offset += chunk;
        src += chunk;
        remaining -= chunk;

        /* 页满则 flush */
        if (s_page_offset >= EEPROM_PAGE_SIZE)
        {
            if (FlushPageBuffer() != 0)
            {
                resp[0] = KBD_RESP_ERR_FLASH;
                resp[1] = 0;
                resp[2] = 0;
                resp[3] = 0;
                KBD_Command_SendResponse(KBD_CMD_IAP_WRITE, seq, resp, 4);
                return;
            }
        }
    }

    s_written_bytes = offset + payload_len;

    resp[0] = KBD_RESP_OK;
    resp[1] = (s_written_bytes >> 16) & 0xFF;
    resp[2] = (s_written_bytes >> 8) & 0xFF;
    resp[3] = (s_written_bytes >> 0) & 0xFF;
    KBD_Command_SendResponse(KBD_CMD_IAP_WRITE, seq, resp, 4);
}

/*============================================================================*/
/*                        命令处理: IAP_VERIFY (0x83)                          */
/*============================================================================*/

/**
 * @brief CRC32 校验 Image B
 *
 * 请求: [0x83][0][8][size(4B LE), expected_crc(4B LE)]
 *   size: 固件实际大小 (字节)
 *   expected_crc: 上位机预计算的 CRC32
 *
 * 响应: [0x83][0][5][status, actual_crc(4B LE)]
 */
static void HandleIapVerify(const kbd_cmd_frame_t *frame)
{
    uint8_t resp[5];

    /* Flush 残余数据 */
    if (s_page_offset > 0)
    {
        if (FlushPageBuffer() != 0)
        {
            resp[0] = KBD_RESP_ERR_FLASH;
            memset(&resp[1], 0, 4);
            KBD_Command_SendResponse(KBD_CMD_IAP_VERIFY, 0, resp, 5);
            return;
        }
    }

    if (frame->len < 8)
    {
        resp[0] = KBD_RESP_ERR_PARAM;
        memset(&resp[1], 0, 4);
        KBD_Command_SendResponse(KBD_CMD_IAP_VERIFY, 0, resp, 5);
        return;
    }

    /* 解析参数 */
    uint32_t fw_size =
        ((uint32_t)frame->data[0]) |
        ((uint32_t)frame->data[1] << 8) |
        ((uint32_t)frame->data[2] << 16) |
        ((uint32_t)frame->data[3] << 24);

    uint32_t expected_crc =
        ((uint32_t)frame->data[4]) |
        ((uint32_t)frame->data[5] << 8) |
        ((uint32_t)frame->data[6] << 16) |
        ((uint32_t)frame->data[7] << 24);

    if (fw_size == 0 || fw_size > IMAGE_B_SIZE)
    {
        resp[0] = KBD_RESP_ERR_PARAM;
        memset(&resp[1], 0, 4);
        KBD_Command_SendResponse(KBD_CMD_IAP_VERIFY, 0, resp, 5);
        return;
    }

    LOG_I(TAG, "Verify: size=%d expected_crc=0x%08X", fw_size, expected_crc);

    /* 对 Image B 计算 CRC32 (Flash 是内存映射的，直接读) */
    uint32_t actual_crc = crc32_calc((const uint8_t *)IMAGE_B_START_ADD, fw_size);

    resp[0] = (actual_crc == expected_crc) ? KBD_RESP_OK : KBD_RESP_ERR_FLASH;
    resp[1] = (actual_crc >> 0) & 0xFF;
    resp[2] = (actual_crc >> 8) & 0xFF;
    resp[3] = (actual_crc >> 16) & 0xFF;
    resp[4] = (actual_crc >> 24) & 0xFF;

    if (actual_crc == expected_crc)
    {
        s_state = IAP_STATE_WRITTEN;
        LOG_I(TAG, "Verify OK: CRC32=0x%08X", actual_crc);
    }
    else
    {
        LOG_E(TAG, "Verify FAIL: expected=0x%08X actual=0x%08X",
              expected_crc, actual_crc);
    }

    KBD_Command_SendResponse(KBD_CMD_IAP_VERIFY, 0, resp, 5);
}

/*============================================================================*/
/*                       命令处理: IAP_ACTIVATE (0x84)                         */
/*============================================================================*/

/**
 * @brief 设置 IAP 标志并复位
 * 请求: [0x84][0][0]
 * 响应: [0x84][0][1][status]  (响应后立即复位，下一次从 JumpIAP 进入高地址 IAP)
 */
static void HandleIapActivate(const kbd_cmd_frame_t *frame)
{
    uint8_t resp[1];

    if (s_state != IAP_STATE_WRITTEN)
    {
        LOG_W(TAG, "Activate rejected: state=%d (need WRITTEN)", s_state);
        resp[0] = KBD_RESP_ERR_BUSY;
        KBD_Command_SendResponse(KBD_CMD_IAP_ACTIVATE, 0, resp, 1);
        return;
    }

    LOG_I(TAG, "Activating: set IAP flag and reset");

    /* 写入 IAP 标志到 DataFlash */
    __attribute__((aligned(4))) uint8_t buf[4] = {0};
    EEPROM_READ(IAP_DATAFLASH_ADD, (uint32_t *)buf, 4);
    EEPROM_ERASE(IAP_DATAFLASH_ADD, EEPROM_BLOCK_SIZE);
    buf[0] = IMAGE_IAP_FLAG;
    EEPROM_WRITE(IAP_DATAFLASH_ADD, (uint32_t *)buf, 4);

    /* 先发响应，让上位机知道操作成功 */
    resp[0] = KBD_RESP_OK;
    KBD_Command_SendResponse(KBD_CMD_IAP_ACTIVATE, 0, resp, 1);

    /* 延时让 USB 响应发出 */
    mDelaymS(200);

    /* 系统复位 → JumpIAP 先跳高地址 IAP，再由 IAP 执行 B→A 拷贝 */
    Hal_Reset();
}

/*============================================================================*/
/*                              公共接口                                       */
/*============================================================================*/

void KBD_IAP_Init(void)
{
    s_state = IAP_STATE_IDLE;
    s_written_bytes = 0;
    s_page_offset = 0;
    s_page_flash_addr = IMAGE_B_START_ADD;
    LOG_I(TAG, "IAP module init (B=0x%05X, size=%dKB)",
          IMAGE_B_START_ADD, IMAGE_B_SIZE / 1024);
}

int KBD_IAP_Process(const kbd_cmd_frame_t *frame)
{
    switch (frame->cmd)
    {
    case KBD_CMD_IAP_INFO:
        HandleIapInfo(frame);
        break;
    case KBD_CMD_IAP_PREPARE:
        HandleIapPrepare(frame);
        break;
    case KBD_CMD_IAP_WRITE:
        HandleIapWrite(frame);
        break;
    case KBD_CMD_IAP_VERIFY:
        HandleIapVerify(frame);
        break;
    case KBD_CMD_IAP_ACTIVATE:
        HandleIapActivate(frame);
        break;
    default:
        return -1;
    }
    return 0;
}
