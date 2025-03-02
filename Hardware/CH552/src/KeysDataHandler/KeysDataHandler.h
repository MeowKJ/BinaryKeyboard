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


#define EEPROM_HEADER_SIZE      2
#define EEPROM_VERSION_ADDR     0x00
#define EEPROM_DEVTYPE_ADDR     0x01
#define EEPROM_KEYDATA_START    0x02
#define KEY_CONFIG_SIZE         3

#define CURRENT_FW_VERSION      0x02
#define EXPECT_DEVICE_TYPE      0x02

// 函数声明
void KeysDataInit(void);
static void initDefaultConfig(void);
void setKey(uint8_t index, uint8_t type, uint16_t value);  // 参数类型升级为 uint16_t
uint8_t getKeyType(uint8_t index);
uint16_t getKeyValue(uint8_t index);  // 返回值类型升级为 uint16_t

// EEPROM 接口
static uint8_t validateEepromHeader(void);
void saveKeysToEEPROM(void);
void loadKeysFromEEPROM(void);

#ifdef __cplusplus
}
#endif

#endif  // KEY_HANDLER_H