#include <Arduino.h>
#include "KeysDataHandler.h"
#include "config.h"
#include "rgb.h"

typedef struct {
  uint8_t type;
  uint16_t value;
} KeyConfig;

static uint8_t validateEepromHeader(void);
static void initDefaultConfig(void);
static void clearLayer(uint8_t layer);
static void loadRgbFromEEPROM(void);

static __xdata KeyConfig keySettings[MAX_LAYERS][KEY_CONFIG_SLOTS];
static __xdata uint8_t currentLayer = 0;

void KeysDataInit(void) {
  if (eeprom_read_byte(EEPROM_VERSION_ADDR) == 0xFF || !validateEepromHeader()) {
    initDefaultConfig();
  } else {
    loadAllFromEEPROM();
  }
  // 加载 RGB 配置
  loadRgbFromEEPROM();
}

static uint8_t validateEepromHeader(void) {
  return (eeprom_read_byte(EEPROM_VERSION_ADDR) == EEPROM_SCHEMA_ID) &&
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
  eeprom_write_byte(EEPROM_VERSION_ADDR, EEPROM_SCHEMA_ID);
  eeprom_write_byte(EEPROM_DEVTYPE_ADDR, EXPECT_DEVICE_TYPE);
  eeprom_write_byte(EEPROM_LAYER_ADDR, 0);
  for (uint8_t layer = 0; layer < MAX_LAYERS; layer++) {
    saveLayerToEEPROM(layer);
  }
  // RGB 默认值
  eeprom_write_byte(EEPROM_RGB_ENABLED_ADDR, 1);
  eeprom_write_byte(EEPROM_RGB_MODE_ADDR, EFFECT_RAINBOW);
  eeprom_write_byte(EEPROM_RGB_BRIGHTNESS_ADDR, 128);
  eeprom_write_byte(EEPROM_RGB_SPEED_ADDR, 128);
  eeprom_write_byte(EEPROM_RGB_COLOR_R_ADDR, 255);
  eeprom_write_byte(EEPROM_RGB_COLOR_G_ADDR, 255);
  eeprom_write_byte(EEPROM_RGB_COLOR_B_ADDR, 255);
  eeprom_write_byte(EEPROM_RGB_INDICATOR_ENABLED_ADDR, 0);
  eeprom_write_byte(EEPROM_RGB_INDICATOR_BRIGHTNESS_ADDR, 13);
  eeprom_write_byte(EEPROM_RGB_PRESS_EFFECT_ADDR, PRESS_EFFECT_NONE);
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
  buf[1] = CH552_FEATURE_LEVEL;
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
  buf[4] = 0;
  buf[5] = 0;
  buf[6] = 0;
  buf[7] = 0;
}

// ── RGB 配置 ──

void fillRgbResponse(uint8_t *__xdata buf) {
  for (uint8_t i = 0; i < 31; i++) {
    buf[i] = 0;
  }
  buf[0] = HOST_CMD_READ_RGB;
  buf[1] = rgbEnabled;
  buf[2] = effectMode;
  buf[3] = currentBrightness;
  buf[4] = currentSpeed;
  buf[5] = currentColorR;
  buf[6] = currentColorG;
  buf[7] = currentColorB;
  buf[8] = indicatorEnabled;
  buf[9] = indicatorBrightness;
  buf[10] = pressEffect;
}

void applyRgbConfig(
  uint8_t enabled,
  uint8_t mode,
  uint8_t brightness,
  uint8_t speed,
  uint8_t colorR,
  uint8_t colorG,
  uint8_t colorB,
  uint8_t indicatorEnable,
  uint8_t indicatorBrightnessValue,
  uint8_t pressEffectValue) __reentrant {
  if (mode >= EFFECT_COUNT) mode = EFFECT_OFF;
  if (pressEffectValue >= PRESS_EFFECT_COUNT) pressEffectValue = PRESS_EFFECT_NONE;
  if (indicatorBrightnessValue < 13) indicatorBrightnessValue = 13;

  rgbEnabled = enabled ? 1 : 0;
  effectMode = mode;
  currentBrightness = brightness;
  currentSpeed = speed;
  currentColorR = colorR;
  currentColorG = colorG;
  currentColorB = colorB;
  indicatorEnabled = indicatorEnable ? 1 : 0;
  indicatorBrightness = indicatorBrightnessValue;
  pressEffect = pressEffectValue;

  // 保存到 EEPROM
  eeprom_write_byte(EEPROM_RGB_ENABLED_ADDR, rgbEnabled);
  eeprom_write_byte(EEPROM_RGB_MODE_ADDR, mode);
  eeprom_write_byte(EEPROM_RGB_BRIGHTNESS_ADDR, brightness);
  eeprom_write_byte(EEPROM_RGB_SPEED_ADDR, speed);
  eeprom_write_byte(EEPROM_RGB_COLOR_R_ADDR, colorR);
  eeprom_write_byte(EEPROM_RGB_COLOR_G_ADDR, colorG);
  eeprom_write_byte(EEPROM_RGB_COLOR_B_ADDR, colorB);
  eeprom_write_byte(EEPROM_RGB_INDICATOR_ENABLED_ADDR, indicatorEnabled);
  eeprom_write_byte(EEPROM_RGB_INDICATOR_BRIGHTNESS_ADDR, indicatorBrightness);
  eeprom_write_byte(EEPROM_RGB_PRESS_EFFECT_ADDR, pressEffect);
}

static void loadRgbFromEEPROM(void) {
  uint8_t enabled = eeprom_read_byte(EEPROM_RGB_ENABLED_ADDR);
  uint8_t mode = eeprom_read_byte(EEPROM_RGB_MODE_ADDR);
  uint8_t brightness = eeprom_read_byte(EEPROM_RGB_BRIGHTNESS_ADDR);
  uint8_t speed = eeprom_read_byte(EEPROM_RGB_SPEED_ADDR);
  uint8_t colorR = eeprom_read_byte(EEPROM_RGB_COLOR_R_ADDR);
  uint8_t colorG = eeprom_read_byte(EEPROM_RGB_COLOR_G_ADDR);
  uint8_t colorB = eeprom_read_byte(EEPROM_RGB_COLOR_B_ADDR);
  uint8_t indicatorEnable = eeprom_read_byte(EEPROM_RGB_INDICATOR_ENABLED_ADDR);
  uint8_t indicatorBrightnessValue = eeprom_read_byte(EEPROM_RGB_INDICATOR_BRIGHTNESS_ADDR);
  uint8_t pressEffectValue = eeprom_read_byte(EEPROM_RGB_PRESS_EFFECT_ADDR);

  if (enabled == 0xFF) enabled = 1;
  if (mode == 0xFF || mode >= EFFECT_COUNT) mode = EFFECT_RAINBOW;
  if (brightness == 0xFF) brightness = 128;
  if (speed == 0xFF) speed = 128;
  if (colorR == 0xFF) colorR = 255;
  if (colorG == 0xFF) colorG = 255;
  if (colorB == 0xFF) colorB = 255;
  if (indicatorEnable == 0xFF) indicatorEnable = 0;
  if (indicatorBrightnessValue == 0xFF || indicatorBrightnessValue < 13) indicatorBrightnessValue = 13;
  if (pressEffectValue == 0xFF || pressEffectValue >= PRESS_EFFECT_COUNT) {
    pressEffectValue = PRESS_EFFECT_NONE;
  }

  rgbEnabled = enabled ? 1 : 0;
  effectMode = mode;
  currentBrightness = brightness;
  currentSpeed = speed;
  currentColorR = colorR;
  currentColorG = colorG;
  currentColorB = colorB;
  indicatorEnabled = indicatorEnable ? 1 : 0;
  indicatorBrightness = indicatorBrightnessValue;
  pressEffect = pressEffectValue;
}
