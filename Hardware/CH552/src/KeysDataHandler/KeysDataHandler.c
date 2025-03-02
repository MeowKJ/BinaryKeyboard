#include <Arduino.h>
#include "KeysDataHandler.h"

typedef struct {
    uint8_t type;
    uint16_t value;
} KeyConfig;

static KeyConfig keySettings[8];

void KeysDataInit(void) 
{
    if(eeprom_read_byte(EEPROM_VERSION_ADDR) == 0xFF || 
       !validateEepromHeader()) 
    {
        initDefaultConfig();
        saveKeysToEEPROM();
    }
    loadKeysFromEEPROM();
}

static uint8_t validateEepromHeader(void)
{
    return (eeprom_read_byte(EEPROM_VERSION_ADDR) == CURRENT_FW_VERSION) &&
           (eeprom_read_byte(EEPROM_DEVTYPE_ADDR) == EXPECT_DEVICE_TYPE);
}

static void initDefaultConfig(void)
{
    eeprom_write_byte(EEPROM_VERSION_ADDR, CURRENT_FW_VERSION);
    eeprom_write_byte(EEPROM_DEVTYPE_ADDR, EXPECT_DEVICE_TYPE);

    KeyConfig defaults[8] = {
        {KEY_TYPE_KB,    'A'},
        {KEY_TYPE_KB,    'B'},
        {KEY_TYPE_KB,    'C'},
        {KEY_TYPE_KB,    'D'},
        {KEY_TYPE_MOUSE, 0x01},     // 鼠标左键
        {KEY_TYPE_MEDIA, 0xEA},     // 音量-
        {KEY_TYPE_MEDIA, 0xE9},     // 音量+
        {KEY_TYPE_INVALID, 0}
    };
    
    for(uint8_t i=0; i<8; i++) {
        keySettings[i] = defaults[i];
    }
}

void setKey(uint8_t index, uint8_t type, uint16_t value)
{
    if(index >= 8) return;
    keySettings[index].type = type;
    keySettings[index].value = value;
}

uint8_t getKeyType(uint8_t index)
{
    return (index < 8) ? keySettings[index].type : KEY_TYPE_INVALID;
}

uint16_t getKeyValue(uint8_t index)
{
    return (index < 8) ? keySettings[index].value : 0;
}

void saveKeysToEEPROM(void)
{
    for(uint8_t i=0; i<8; i++) {
        const uint16_t addr = EEPROM_KEYDATA_START + i * KEY_CONFIG_SIZE;
        eeprom_write_byte(addr,   keySettings[i].type);
        eeprom_write_byte(addr+1, (uint8_t)(keySettings[i].value >> 8));
        eeprom_write_byte(addr+2, (uint8_t)(keySettings[i].value & 0xFF));
    }
}

void loadKeysFromEEPROM(void)
{
    for(uint8_t i=0; i<8; i++) {
        const uint16_t addr = EEPROM_KEYDATA_START + i * KEY_CONFIG_SIZE;
        keySettings[i].type = eeprom_read_byte(addr);
        const uint8_t high = eeprom_read_byte(addr+1);
        const uint8_t low  = eeprom_read_byte(addr+2);
        keySettings[i].value = ((uint16_t)high << 8) | low;
    }
}