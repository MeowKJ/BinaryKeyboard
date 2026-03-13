#include <Arduino.h>
#include "KeysDataHandler.h"
#include "config.h"

typedef struct {
  uint8_t type;
  uint16_t value;
} KeyConfig;

static uint8_t validateEepromHeader(void);
static void initDefaultConfig(void);
static void clearLayer(uint8_t layer);

static __xdata KeyConfig keySettings[MAX_LAYERS][KEY_CONFIG_SLOTS];
static __xdata uint8_t currentLayer = 0;

void KeysDataInit(void) {
  if (eeprom_read_byte(EEPROM_VERSION_ADDR) == 0xFF || !validateEepromHeader()) {
    initDefaultConfig();
  } else {
    loadAllFromEEPROM();
  }
}

static uint8_t validateEepromHeader(void) {
  return (eeprom_read_byte(EEPROM_VERSION_ADDR) == CURRENT_FW_VERSION) &&
         (eeprom_read_byte(EEPROM_DEVTYPE_ADDR) == EXPECT_DEVICE_TYPE);
}

static void clearLayer(uint8_t layer) {
  for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++) {
    keySettings[layer][i].type = KEY_TYPE_INVALID;
    keySettings[layer][i].value = 0;
  }
}

static void initDefaultConfig(void) {
  clearLayer(0);
  for (uint8_t layer = 1; layer < MAX_LAYERS; layer++) {
    clearLayer(layer);
  }

#ifdef USE_KNOB
  keySettings[0][0].type = KEY_TYPE_KB;
  keySettings[0][0].value = 0x0004;
  keySettings[0][1].type = KEY_TYPE_KB;
  keySettings[0][1].value = 0x0005;
  keySettings[0][2].type = KEY_TYPE_KB;
  keySettings[0][2].value = 0x0006;
  keySettings[0][3].type = KEY_TYPE_KB;
  keySettings[0][3].value = 0x0007;
  keySettings[0][4].type = KEY_TYPE_MOUSE;
  keySettings[0][4].value = 0x0001;
  keySettings[0][5].type = KEY_TYPE_MEDIA;
  keySettings[0][5].value = 0x00EA;
  keySettings[0][6].type = KEY_TYPE_MEDIA;
  keySettings[0][6].value = 0x00E9;
#endif

#ifdef USE_BASIC
  keySettings[0][0].type = KEY_TYPE_KB;
  keySettings[0][0].value = 0x0027;
  keySettings[0][1].type = KEY_TYPE_KB;
  keySettings[0][1].value = 0x001E;
  keySettings[0][2].type = KEY_TYPE_KB;
  keySettings[0][2].value = 0x0028;
  keySettings[0][3].type = KEY_TYPE_KB;
  keySettings[0][3].value = 0x002C;
#endif

#ifdef USE_5KEYS
  keySettings[0][0].type = KEY_TYPE_KB;
  keySettings[0][0].value = 0x001E;
  keySettings[0][1].type = KEY_TYPE_KB;
  keySettings[0][1].value = 0x001F;
  keySettings[0][2].type = KEY_TYPE_KB;
  keySettings[0][2].value = 0x0020;
  keySettings[0][3].type = KEY_TYPE_KB;
  keySettings[0][3].value = 0x0021;
  keySettings[0][4].type = KEY_TYPE_KB;
  keySettings[0][4].value = 0x0022;
#endif

  currentLayer = 0;

  // 写入 EEPROM
  eeprom_write_byte(EEPROM_VERSION_ADDR, CURRENT_FW_VERSION);
  eeprom_write_byte(EEPROM_DEVTYPE_ADDR, EXPECT_DEVICE_TYPE);
  eeprom_write_byte(EEPROM_LAYER_ADDR, 0);
  for (uint8_t layer = 0; layer < MAX_LAYERS; layer++) {
    saveLayerToEEPROM(layer);
  }
}

// ── 当前层的快捷读取 ──

uint8_t getKeyType(uint8_t index) {
  return getKeyTypeAt(currentLayer, index);
}

uint16_t getKeyValue(uint8_t index) {
  return getKeyValueAt(currentLayer, index);
}

// ── 指定层的读取 ──

uint8_t getKeyTypeAt(uint8_t layer, uint8_t index) {
  if (layer >= MAX_LAYERS || index >= KEY_CONFIG_SLOTS) return KEY_TYPE_INVALID;
  return keySettings[layer][index].type;
}

uint16_t getKeyValueAt(uint8_t layer, uint8_t index) {
  if (layer >= MAX_LAYERS || index >= KEY_CONFIG_SLOTS) return 0;
  return keySettings[layer][index].value;
}

void setKey(uint8_t layer, uint8_t index, uint8_t type, uint16_t value) {
  if (layer >= MAX_LAYERS || index >= KEY_CONFIG_SLOTS) return;
  keySettings[layer][index].type = type;
  keySettings[layer][index].value = value;
}

// ── 层管理 ──

uint8_t getCurrentLayer(void) {
  return currentLayer;
}

void setCurrentLayer(uint8_t layer) {
  if (layer >= MAX_LAYERS) return;
  if (currentLayer == layer) return;
  currentLayer = layer;
  eeprom_write_byte(EEPROM_LAYER_ADDR, layer);
}

// ── EEPROM 读写 ──

void saveLayerToEEPROM(uint8_t layer) {
  if (layer >= MAX_LAYERS) return;
  uint16_t base = EEPROM_KEYDATA_START + (uint16_t)layer * LAYER_DATA_SIZE;
  for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++) {
    uint16_t addr = base + i * KEY_CONFIG_SIZE;
    eeprom_write_byte(addr, keySettings[layer][i].type);
    eeprom_write_byte(addr + 1, (uint8_t)(keySettings[layer][i].value >> 8));
    eeprom_write_byte(addr + 2, (uint8_t)(keySettings[layer][i].value & 0xFF));
  }
}

void saveCurrentLayerToEEPROM(void) {
  eeprom_write_byte(EEPROM_LAYER_ADDR, currentLayer);
}

void loadAllFromEEPROM(void) {
  currentLayer = eeprom_read_byte(EEPROM_LAYER_ADDR);
  if (currentLayer >= MAX_LAYERS) currentLayer = 0;

  for (uint8_t layer = 0; layer < MAX_LAYERS; layer++) {
    uint16_t base = EEPROM_KEYDATA_START + (uint16_t)layer * LAYER_DATA_SIZE;
    for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++) {
      uint16_t addr = base + i * KEY_CONFIG_SIZE;
      keySettings[layer][i].type = eeprom_read_byte(addr);
      uint8_t high = eeprom_read_byte(addr + 1);
      uint8_t low  = eeprom_read_byte(addr + 2);
      keySettings[layer][i].value = ((uint16_t)high << 8) | low;
    }
  }
}

// ── HID 协议响应 ──

void fillSysInfoResponse(uint8_t *__xdata buf) {
  // [0]=cmd_echo, [1]=version, [2]=devType, [3]=curLayer, [4]=maxLayers, [5..28]=layer0
  for (uint8_t i = 0; i < 31; i++) {
    buf[i] = 0;
  }
  buf[0] = HOST_CMD_READ;
  buf[1] = CURRENT_FW_VERSION;
  buf[2] = EXPECT_DEVICE_TYPE;
  buf[3] = currentLayer;
  buf[4] = MAX_LAYERS;
  for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++) {
    buf[5 + i * KEY_CONFIG_SIZE] = keySettings[0][i].type;
    buf[5 + i * KEY_CONFIG_SIZE + 1] = (uint8_t)(keySettings[0][i].value >> 8);
    buf[5 + i * KEY_CONFIG_SIZE + 2] = (uint8_t)(keySettings[0][i].value & 0xFF);
  }
}

void fillLayerResponse(uint8_t *__xdata buf, uint8_t layer) {
  if (layer >= MAX_LAYERS) layer = 0;
  // [0]=cmd_echo, [1]=layerIdx, [2..25]=layer data
  for (uint8_t i = 0; i < 31; i++) {
    buf[i] = 0;
  }
  buf[0] = HOST_CMD_READ_LAYER;
  buf[1] = layer;
  for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++) {
    buf[2 + i * KEY_CONFIG_SIZE] = keySettings[layer][i].type;
    buf[2 + i * KEY_CONFIG_SIZE + 1] = (uint8_t)(keySettings[layer][i].value >> 8);
    buf[2 + i * KEY_CONFIG_SIZE + 2] = (uint8_t)(keySettings[layer][i].value & 0xFF);
  }
}

void fillMetaResponse(uint8_t *__xdata buf) {
  for (uint8_t i = 0; i < 31; i++) {
    buf[i] = 0;
  }
  buf[0] = HOST_CMD_READ_META;
  buf[1] = FW_VERSION_MAJOR;
  buf[2] = FW_VERSION_MINOR;
  buf[3] = FW_VERSION_PATCH;
  buf[4] = PROTOCOL_VERSION_MAJOR;
  buf[5] = PROTOCOL_VERSION_MINOR;
  buf[6] = STORAGE_VERSION_MAJOR;
  buf[7] = STORAGE_VERSION_MINOR;
}
