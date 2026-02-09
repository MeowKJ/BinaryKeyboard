/**
 * @file    kbd_command.c
 * @brief   MeowKeyboard USB HID 命令处理实现
 * @author  MeowKJ
 * @version V1.0.0
 * @date    2024-11-07
 *
 * @details
 * 实现 USB HID 配置协议，支持上位机对键盘进行配置：
 * - 命令帧格式：[CMD:1][SUB:1][LEN:1][DATA:61]
 * - 响应帧格式：相同
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#include "kbd_command.h"
#include "kbd_storage.h"
#include "kbd_rgb.h"
#include "kbd_mode.h"
#include "kbd_config.h"
#include "debug.h"
#include <string.h>

/** @brief 模块日志标签 */
#define TAG "CMD"

/*============================================================================*/
/*                              外部函数声明                                   */
/*============================================================================*/

/**
 * @brief USB HID 配置响应发送函数
 * @note 在 usb_hid.c 中实现
 */
extern void USB_Config_SendResponse(uint8_t cmd, uint8_t *data, uint8_t len);

/*============================================================================*/
/*                              私有函数声明                                   */
/*============================================================================*/

static void HandleSysInfo(const kbd_cmd_frame_t *frame);
static void HandleSysStatus(const kbd_cmd_frame_t *frame);
static void HandleCfgSave(const kbd_cmd_frame_t *frame);
static void HandleCfgLoad(const kbd_cmd_frame_t *frame);
static void HandleCfgReset(const kbd_cmd_frame_t *frame);
static void HandleKeymapGet(const kbd_cmd_frame_t *frame);
static void HandleKeymapSet(const kbd_cmd_frame_t *frame);
static void HandleLayerGet(const kbd_cmd_frame_t *frame);
static void HandleLayerSet(const kbd_cmd_frame_t *frame);
static void HandleRgbGet(const kbd_cmd_frame_t *frame);
static void HandleRgbSet(const kbd_cmd_frame_t *frame);
static void HandleMacroInfo(const kbd_cmd_frame_t *frame);
static void HandleMacroGet(const kbd_cmd_frame_t *frame);
static void HandleMacroSet(const kbd_cmd_frame_t *frame);
static void HandleMacroDel(const kbd_cmd_frame_t *frame);
static void HandleFnkeyGet(const kbd_cmd_frame_t *frame);
static void HandleFnkeySet(const kbd_cmd_frame_t *frame);
static void HandleBattery(const kbd_cmd_frame_t *frame);

/*============================================================================*/
/*                              公共函数实现                                   */
/*============================================================================*/

void KBD_Command_Init(void)
{
    LOG_I(TAG, "Command handler init");
}

int KBD_Command_Process(const kbd_cmd_frame_t *frame)
{
    LOG_D(TAG, "cmd=0x%02X sub=0x%02X len=%d", frame->cmd, frame->sub, frame->len);

    switch (frame->cmd) {
        /* 系统命令 */
        case KBD_CMD_SYS_INFO:
            HandleSysInfo(frame);
            break;
        case KBD_CMD_SYS_STATUS:
            HandleSysStatus(frame);
            break;

        /* 配置管理 */
        case KBD_CMD_CFG_SAVE:
            HandleCfgSave(frame);
            break;
        case KBD_CMD_CFG_LOAD:
            HandleCfgLoad(frame);
            break;
        case KBD_CMD_CFG_RESET:
            HandleCfgReset(frame);
            break;

        /* 按键映射 */
        case KBD_CMD_KEYMAP_GET:
            HandleKeymapGet(frame);
            break;
        case KBD_CMD_KEYMAP_SET:
            HandleKeymapSet(frame);
            break;
        case KBD_CMD_LAYER_GET:
            HandleLayerGet(frame);
            break;
        case KBD_CMD_LAYER_SET:
            HandleLayerSet(frame);
            break;

        /* RGB 控制 */
        case KBD_CMD_RGB_GET:
            HandleRgbGet(frame);
            break;
        case KBD_CMD_RGB_SET:
            HandleRgbSet(frame);
            break;

        /* 宏管理 */
        case KBD_CMD_MACRO_INFO:
            HandleMacroInfo(frame);
            break;
        case KBD_CMD_MACRO_GET:
            HandleMacroGet(frame);
            break;
        case KBD_CMD_MACRO_SET:
            HandleMacroSet(frame);
            break;
        case KBD_CMD_MACRO_DEL:
            HandleMacroDel(frame);
            break;

        /* FN 键配置 */
        case KBD_CMD_FNKEY_GET:
            HandleFnkeyGet(frame);
            break;
        case KBD_CMD_FNKEY_SET:
            HandleFnkeySet(frame);
            break;

        /* 电池信息 */
        case KBD_CMD_BATTERY:
            HandleBattery(frame);
            break;

        default:
            LOG_W(TAG, "Unknown cmd 0x%02X", frame->cmd);
            {
                uint8_t resp[1] = { KBD_RESP_ERR_INVALID };
                KBD_Command_SendResponse(frame->cmd, 0, resp, 1);
            }
            return -1;
    }

    return 0;
}

void KBD_Command_SendResponse(uint8_t cmd, uint8_t sub, const uint8_t *data, uint8_t len)
{
    /* USB_ConfigReport_t 格式: [CMD:1][DATA:63]
     * 我们把 [SUB][LEN][actual_data] 放入 DATA 部分
     * 最终格式: [CMD][SUB][LEN][actual_data...]
     */
    uint8_t buf[63];
    uint8_t copy_len = 0;
    buf[0] = sub;
    buf[1] = len;
    if (data && len > 0) {
        copy_len = (len > 61) ? 61 : len;
        memcpy(&buf[2], data, copy_len);
    }
    USB_Config_SendResponse(cmd, buf, copy_len + 2);
}

/*============================================================================*/
/*                              命令处理实现                                   */
/*============================================================================*/

/**
 * @brief 处理系统信息查询
 * 
 * 响应格式 (14 字节):
 * [0]  KBD_RESP_OK
 * [1]  VID 高字节
 * [2]  VID 低字节
 * [3]  PID 高字节
 * [4]  PID 低字节
 * [5]  固件主版本
 * [6]  固件次版本
 * [7]  固件补丁版本
 * [8]  最大层数
 * [9]  最大按键数 (单层)
 * [10] 宏槽位数
 * [11] 键盘类型 (kbd_type_t)
 * [12] 实际按键数 (当前类型)
 * [13] FN 键数量
 */
static void HandleSysInfo(const kbd_cmd_frame_t *frame)
{
    uint8_t resp[16];
    resp[0] = KBD_RESP_OK;
    resp[1] = (KBD_VENDOR_ID >> 8) & 0xFF;
    resp[2] = KBD_VENDOR_ID & 0xFF;
    resp[3] = (KBD_PRODUCT_ID >> 8) & 0xFF;
    resp[4] = KBD_PRODUCT_ID & 0xFF;
    resp[5] = KBD_VERSION_MAJOR;
    resp[6] = KBD_VERSION_MINOR;
    resp[7] = KBD_VERSION_PATCH;
    resp[8] = KBD_MAX_LAYERS;
    resp[9] = KBD_MAX_KEYS;
    resp[10] = KBD_MACRO_SLOTS;
    resp[11] = (uint8_t)KBD_GetType();          /* 键盘类型 */
    resp[12] = KBD_GetTotalKeyCount();          /* 实际键位数 */
    resp[13] = KBD_FN_NUM_KEYS;                 /* FN 键数量 */

    KBD_Command_SendResponse(KBD_CMD_SYS_INFO, 0, resp, 14);
    LOG_D(TAG, "SysInfo sent: type=%d keys=%d", resp[11], resp[12]);
}

/**
 * @brief 处理系统状态查询
 */
static void HandleSysStatus(const kbd_cmd_frame_t *frame)
{
    uint8_t resp[8];
    resp[0] = KBD_RESP_OK;
    resp[1] = (uint8_t)DualMode_GetMode();
    resp[2] = (uint8_t)DualMode_GetConnState();
    resp[3] = KBD_GetCurrentLayer();
    resp[4] = 100;  /* 电池电量 (占位) */
    resp[5] = 0;    /* 充电状态 (占位) */

    KBD_Command_SendResponse(KBD_CMD_SYS_STATUS, 0, resp, 6);
}

/**
 * @brief 处理配置保存
 */
static void HandleCfgSave(const kbd_cmd_frame_t *frame)
{
    int ret = KBD_Config_Save();
    uint8_t resp[1] = { (ret == 0) ? KBD_RESP_OK : KBD_RESP_ERR_FLASH };
    KBD_Command_SendResponse(KBD_CMD_CFG_SAVE, 0, resp, 1);
    LOG_I(TAG, "Config save: %d", ret);
}

/**
 * @brief 处理配置加载
 */
static void HandleCfgLoad(const kbd_cmd_frame_t *frame)
{
    int ret = KBD_Config_Load();
    uint8_t resp[1] = { (ret == 0) ? KBD_RESP_OK : KBD_RESP_ERR_FLASH };
    KBD_Command_SendResponse(KBD_CMD_CFG_LOAD, 0, resp, 1);
    LOG_I(TAG, "Config load: %d", ret);
}

/**
 * @brief 处理配置重置
 */
static void HandleCfgReset(const kbd_cmd_frame_t *frame)
{
    int ret = KBD_Config_Reset();
    uint8_t resp[1] = { (ret == 0) ? KBD_RESP_OK : KBD_RESP_ERR_FLASH };
    KBD_Command_SendResponse(KBD_CMD_CFG_RESET, 0, resp, 1);
    LOG_I(TAG, "Config reset: %d", ret);
}

/**
 * @brief 处理按键映射获取
 */
static void HandleKeymapGet(const kbd_cmd_frame_t *frame)
{
    uint8_t layer = frame->sub;
    kbd_keymap_t *keymap = KBD_GetKeymap();

    if (layer >= keymap->num_layers) {
        uint8_t resp[1] = { KBD_RESP_ERR_PARAM };
        KBD_Command_SendResponse(KBD_CMD_KEYMAP_GET, layer, resp, 1);
        return;
    }

    uint8_t resp[40];
    resp[0] = KBD_RESP_OK;
    resp[1] = keymap->num_layers;
    resp[2] = keymap->current_layer;
    resp[3] = keymap->default_layer;
    memcpy(&resp[4], &keymap->layers[layer], sizeof(kbd_layer_t));

    KBD_Command_SendResponse(KBD_CMD_KEYMAP_GET, layer, resp, 4 + sizeof(kbd_layer_t));
}

/**
 * @brief 处理按键映射设置
 */
static void HandleKeymapSet(const kbd_cmd_frame_t *frame)
{
    uint8_t layer = frame->sub;
    kbd_keymap_t *keymap = KBD_GetKeymap();

    if (layer >= KBD_MAX_LAYERS) {
        uint8_t resp[1] = { KBD_RESP_ERR_PARAM };
        KBD_Command_SendResponse(KBD_CMD_KEYMAP_SET, layer, resp, 1);
        return;
    }

    /* 数据格式: [numLayers:1][reserved:1][defaultLayer:1][layer_data:32] = 35 bytes */
    if (frame->len >= 3 + sizeof(kbd_layer_t)) {
        uint8_t num_layers = frame->data[0];
        uint8_t default_layer = frame->data[2];

        LOG_D(TAG, "Keymap set: layer=%d numLayers=%d defLayer=%d len=%d", 
              layer, num_layers, default_layer, frame->len);

        if (num_layers > 0 && num_layers <= KBD_MAX_LAYERS) {
            keymap->num_layers = num_layers;
        }
        if (default_layer < keymap->num_layers) {
            keymap->default_layer = default_layer;
        }

        memcpy(&keymap->layers[layer], &frame->data[3], sizeof(kbd_layer_t));
    } else {
        LOG_W(TAG, "Keymap data too short: len=%d need=%d", frame->len, 3 + sizeof(kbd_layer_t));
    }

    uint8_t resp[1] = { KBD_RESP_OK };
    KBD_Command_SendResponse(KBD_CMD_KEYMAP_SET, layer, resp, 1);
    LOG_D(TAG, "Keymap set: layer=%d", layer);
}

/**
 * @brief 处理当前层获取
 */
static void HandleLayerGet(const kbd_cmd_frame_t *frame)
{
    kbd_keymap_t *keymap = KBD_GetKeymap();

    uint8_t resp[4];
    resp[0] = KBD_RESP_OK;
    resp[1] = keymap->current_layer;
    resp[2] = keymap->num_layers;
    resp[3] = keymap->default_layer;

    KBD_Command_SendResponse(KBD_CMD_LAYER_GET, 0, resp, 4);
}

/**
 * @brief 处理当前层设置
 */
static void HandleLayerSet(const kbd_cmd_frame_t *frame)
{
    uint8_t layer = frame->data[0];
    int ret = KBD_SetCurrentLayer(layer);

    uint8_t resp[2];
    resp[0] = (ret == 0) ? KBD_RESP_OK : KBD_RESP_ERR_PARAM;
    resp[1] = KBD_GetCurrentLayer();

    KBD_Command_SendResponse(KBD_CMD_LAYER_SET, 0, resp, 2);
    LOG_D(TAG, "Layer switch: %d", layer);
}

/**
 * @brief 处理 RGB 配置获取
 */
static void HandleRgbGet(const kbd_cmd_frame_t *frame)
{
    kbd_rgb_config_t *rgb = KBD_GetRgbConfig();

    uint8_t resp[12];
    resp[0] = KBD_RESP_OK;
    resp[1] = rgb->enabled;
    resp[2] = rgb->mode;
    resp[3] = rgb->brightness;
    resp[4] = rgb->speed;
    resp[5] = rgb->color_r;
    resp[6] = rgb->color_g;
    resp[7] = rgb->color_b;
    resp[8] = rgb->indicator_enabled;
    resp[9] = rgb->indicator_brightness;

    KBD_Command_SendResponse(KBD_CMD_RGB_GET, 0, resp, 10);
}

/**
 * @brief 处理 RGB 配置设置
 */
static void HandleRgbSet(const kbd_cmd_frame_t *frame)
{
    kbd_rgb_config_t *rgb = KBD_GetRgbConfig();

    if (frame->len >= 8) {
        rgb->enabled = frame->data[0];
        rgb->mode = frame->data[1];
        rgb->brightness = frame->data[2];
        rgb->speed = frame->data[3];
        rgb->color_r = frame->data[4];
        rgb->color_g = frame->data[5];
        rgb->color_b = frame->data[6];
        rgb->indicator_enabled = frame->data[7];
        /* indicator_brightness: 兼容旧协议，若 data[8] 存在则使用 */
        if (frame->len >= 9) {
            rgb->indicator_brightness = frame->data[8];
        }

        KBD_RGB_SetMode((kbd_rgb_mode_t)rgb->mode);
        KBD_RGB_SetBrightness(rgb->brightness);
        KBD_RGB_SetIndicatorBrightness(rgb->indicator_brightness);
    }

    uint8_t resp[1] = { KBD_RESP_OK };
    KBD_Command_SendResponse(KBD_CMD_RGB_SET, 0, resp, 1);
    LOG_D(TAG, "RGB config set");
}

/**
 * @brief 处理宏信息查询
 */
static void HandleMacroInfo(const kbd_cmd_frame_t *frame)
{
    uint8_t slot = frame->sub;

    if (slot == 0xFF) {
        /* 获取所有槽位概览 */
        uint8_t resp[12];
        resp[0] = KBD_RESP_OK;
        resp[1] = KBD_MACRO_SLOTS;
        resp[2] = Kbd_Macro_GetUsedCount();

        for (uint8_t i = 0; i < KBD_MACRO_SLOTS && i < 8; i++) {
            resp[3 + i] = Kbd_Macro_IsValid(i) ? 1 : 0;
        }

        KBD_Command_SendResponse(KBD_CMD_MACRO_INFO, 0xFF, resp, 3 + KBD_MACRO_SLOTS);
    } else {
        /* 获取指定槽位详情 */
        kbd_macro_header_t header;
        int ret = Kbd_Macro_GetInfo(slot, &header);

        if (ret == 0) {
            uint8_t resp[28];
            resp[0] = KBD_RESP_OK;
            memcpy(&resp[1], &header, sizeof(kbd_macro_header_t));
            KBD_Command_SendResponse(KBD_CMD_MACRO_INFO, slot, resp, 1 + sizeof(kbd_macro_header_t));
        } else {
            uint8_t resp[1] = { KBD_RESP_ERR_NOT_FOUND };
            KBD_Command_SendResponse(KBD_CMD_MACRO_INFO, slot, resp, 1);
        }
    }
}

/**
 * @brief 处理宏数据读取
 */
static void HandleMacroGet(const kbd_cmd_frame_t *frame)
{
    uint8_t slot = frame->sub;
    uint16_t offset = (frame->data[0] << 8) | frame->data[1];
    uint8_t req_len = frame->data[2];

    if (req_len > 56) req_len = 56;

    uint8_t resp[64];
    int read_len = Kbd_Macro_Read(slot, offset, &resp[5], req_len);

    if (read_len >= 0) {
        resp[0] = KBD_RESP_OK;
        resp[1] = (offset >> 8) & 0xFF;
        resp[2] = offset & 0xFF;
        resp[3] = (uint8_t)read_len;
        resp[4] = (read_len < req_len) ? 1 : 0;
        KBD_Command_SendResponse(KBD_CMD_MACRO_GET, slot, resp, 5 + read_len);
    } else {
        resp[0] = KBD_RESP_ERR_NOT_FOUND;
        KBD_Command_SendResponse(KBD_CMD_MACRO_GET, slot, resp, 1);
    }
}

/**
 * @brief 处理宏数据写入
 */
static void HandleMacroSet(const kbd_cmd_frame_t *frame)
{
    uint8_t slot = frame->sub;
    uint8_t seq = frame->data[0];

    if (seq == 0) {
        /* 开始写入 */
        kbd_macro_header_t header;
        memcpy(&header, &frame->data[1], sizeof(kbd_macro_header_t));

        int ret = Kbd_Macro_BeginWrite(slot, &header);

        uint8_t resp[2];
        if (ret == 0) {
            resp[0] = KBD_RESP_OK;
        } else if (ret == KBD_RESP_ERR_TOO_LARGE) {
            resp[0] = KBD_RESP_ERR_TOO_LARGE;
        } else {
            resp[0] = KBD_RESP_ERR_FLASH;
        }
        resp[1] = seq;
        KBD_Command_SendResponse(KBD_CMD_MACRO_SET, slot, resp, 2);
        LOG_D(TAG, "Macro write begin: slot=%d ret=%d", slot, ret);

    } else if (seq == 0xFF) {
        /* 完成写入 */
        int ret = Kbd_Macro_EndWrite(slot);

        uint8_t resp[2];
        resp[0] = (ret == 0) ? KBD_RESP_OK : KBD_RESP_ERR_FLASH;
        resp[1] = seq;
        KBD_Command_SendResponse(KBD_CMD_MACRO_SET, slot, resp, 2);
        LOG_D(TAG, "Macro write end: slot=%d ret=%d", slot, ret);

    } else {
        /* 数据块 */
        uint16_t offset = (frame->data[1] << 8) | frame->data[2];
        uint8_t len = frame->data[3];

        int ret = Kbd_Macro_WriteChunk(slot, offset, &frame->data[4], len);

        uint8_t resp[2];
        resp[0] = (ret == 0) ? KBD_RESP_OK : KBD_RESP_ERR_FLASH;
        resp[1] = seq;
        KBD_Command_SendResponse(KBD_CMD_MACRO_SET, slot, resp, 2);
    }
}

/**
 * @brief 处理宏删除
 */
static void HandleMacroDel(const kbd_cmd_frame_t *frame)
{
    uint8_t slot = frame->sub;
    int ret = Kbd_Macro_Delete(slot);

    uint8_t resp[1] = { (ret == 0) ? KBD_RESP_OK : KBD_RESP_ERR_PARAM };
    KBD_Command_SendResponse(KBD_CMD_MACRO_DEL, slot, resp, 1);
    LOG_D(TAG, "Macro delete: slot=%d ret=%d", slot, ret);
}

/**
 * @brief 处理 FN 键配置获取
 */
static void HandleFnkeyGet(const kbd_cmd_frame_t *frame)
{
    kbd_fnkey_config_t *fnkey = KBD_GetFnKeyConfig();

    uint8_t resp[36];
    resp[0] = KBD_RESP_OK;
    memcpy(&resp[1], fnkey, sizeof(kbd_fnkey_config_t));

    KBD_Command_SendResponse(KBD_CMD_FNKEY_GET, 0, resp, 1 + sizeof(kbd_fnkey_config_t));
}

/**
 * @brief 处理 FN 键配置设置
 */
static void HandleFnkeySet(const kbd_cmd_frame_t *frame)
{
    kbd_fnkey_config_t *fnkey = KBD_GetFnKeyConfig();

    if (frame->len >= sizeof(kbd_fnkey_config_t)) {
        memcpy(fnkey, frame->data, sizeof(kbd_fnkey_config_t));
    }

    uint8_t resp[1] = { KBD_RESP_OK };
    KBD_Command_SendResponse(KBD_CMD_FNKEY_SET, 0, resp, 1);
    LOG_D(TAG, "FN key config set");
}

/**
 * @brief 处理电池信息查询
 */
static void HandleBattery(const kbd_cmd_frame_t *frame)
{
    /* 电池功能占位 */
    uint8_t resp[4];
    resp[0] = KBD_RESP_OK;
    resp[1] = 100;  /* 电量百分比 */
    resp[2] = 0;    /* 充电状态 */
    resp[3] = 42;   /* 电压 (0.1V 单位) */

    KBD_Command_SendResponse(KBD_CMD_BATTERY, 0, resp, 4);
}
