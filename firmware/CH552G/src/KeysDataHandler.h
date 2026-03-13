#ifndef __KEY_HANDLER_H__
#define __KEY_HANDLER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 定义按键类别
#define KEY_TYPE_INVALID  0x00
#define KEY_TYPE_KB    0x01  // 键盘（8位值）
#define KEY_TYPE_MEDIA 0x02  // 媒体（16位值）
#define KEY_TYPE_MOUSE 0x03  // 鼠标（8位值）

// EEPROM 布局 (多层)
// [0x00] FW_VERSION
// [0x01] DEVICE_TYPE
// [0x02] CURRENT_LAYER
// [0x03..0x1A] Layer 0: 8 slots × 3 bytes
// [0x1B..0x32] Layer 1: 8 slots × 3 bytes
// [0x33..0x4A] Layer 2: 8 slots × 3 bytes
// [0x4B..0x62] Layer 3: 8 slots × 3 bytes
// Total: 99 bytes (fits in 128-byte data flash)

#define EEPROM_VERSION_ADDR     0x00
#define EEPROM_DEVTYPE_ADDR     0x01
#define EEPROM_LAYER_ADDR       0x02
#define EEPROM_KEYDATA_START    0x03
#define KEY_CONFIG_SIZE         3
#define KEY_CONFIG_SLOTS        8
#define LAYER_DATA_SIZE         (KEY_CONFIG_SLOTS * KEY_CONFIG_SIZE)  // 24

#define HOST_CMD_READ       0x01
#define HOST_CMD_WRITE      0x02
#define HOST_CMD_READ_LAYER 0x03
#define HOST_CMD_SET_LAYER  0x04
#define HOST_CMD_READ_META  0x05

// 函数声明
void KeysDataInit(void);
void setKey(uint8_t layer, uint8_t index, uint8_t type, uint16_t value);
uint8_t getKeyType(uint8_t index);     // 从当前层读
uint16_t getKeyValue(uint8_t index);   // 从当前层读
uint8_t getKeyTypeAt(uint8_t layer, uint8_t index);
uint16_t getKeyValueAt(uint8_t layer, uint8_t index);

// 层管理
uint8_t getCurrentLayer(void);
void setCurrentLayer(uint8_t layer);

// EEPROM 接口
void saveLayerToEEPROM(uint8_t layer);
void saveCurrentLayerToEEPROM(void);
void loadAllFromEEPROM(void);

// HID 协议响应
void fillSysInfoResponse(uint8_t *__xdata buf);
void fillLayerResponse(uint8_t *__xdata buf, uint8_t layer);
void fillMetaResponse(uint8_t *__xdata buf);

#ifdef __cplusplus
}
#endif

#endif  // KEY_HANDLER_H
