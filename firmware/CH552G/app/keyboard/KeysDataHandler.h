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
// [0x03..] Layer data: MAX_LAYERS × KEY_CONFIG_SLOTS × 3 bytes
// [RGB_BASE..] RGB config: 10 bytes
// KEY_CONFIG_SLOTS 由 config.h 按变体定义 (BASIC=4, KNOB=7, 5KEY=5)

#define EEPROM_VERSION_ADDR     0x00
#define EEPROM_DEVTYPE_ADDR     0x01
#define EEPROM_LAYER_ADDR       0x02
#define EEPROM_KEYDATA_START    0x03
#define KEY_CONFIG_SIZE         3
#define LAYER_DATA_SIZE         (KEY_CONFIG_SLOTS * KEY_CONFIG_SIZE)

#define HOST_CMD_READ       0x01
#define HOST_CMD_WRITE      0x02
#define HOST_CMD_READ_LAYER 0x03
#define HOST_CMD_SET_LAYER  0x04
#define HOST_CMD_READ_META  0x05
#define HOST_CMD_READ_RGB   0x06
#define HOST_CMD_WRITE_RGB  0x07

// RGB EEPROM 地址 (紧接层数据后面, 动态计算)
#define EEPROM_RGB_BASE_ADDR                 (EEPROM_KEYDATA_START + MAX_LAYERS * LAYER_DATA_SIZE)
#define EEPROM_RGB_ENABLED_ADDR              (EEPROM_RGB_BASE_ADDR + 0)
#define EEPROM_RGB_MODE_ADDR                 (EEPROM_RGB_BASE_ADDR + 1)
#define EEPROM_RGB_BRIGHTNESS_ADDR           (EEPROM_RGB_BASE_ADDR + 2)
#define EEPROM_RGB_SPEED_ADDR                (EEPROM_RGB_BASE_ADDR + 3)
#define EEPROM_RGB_COLOR_R_ADDR              (EEPROM_RGB_BASE_ADDR + 4)
#define EEPROM_RGB_COLOR_G_ADDR              (EEPROM_RGB_BASE_ADDR + 5)
#define EEPROM_RGB_COLOR_B_ADDR              (EEPROM_RGB_BASE_ADDR + 6)
#define EEPROM_RGB_PRESS_EFFECT_ADDR         (EEPROM_RGB_BASE_ADDR + 9)

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
void fillRgbResponse(uint8_t *__xdata buf);
void applyRgbConfig(
  uint8_t enabled,
  uint8_t mode,
  uint8_t brightness,
  uint8_t speed,
  uint8_t colorR,
  uint8_t colorG,
  uint8_t colorB,
  uint8_t pressEffectValue) __reentrant;

#ifdef __cplusplus
}
#endif

#endif  // KEY_HANDLER_H
