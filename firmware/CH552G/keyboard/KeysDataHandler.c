#include "ch552_defs.h"
#include "KeysDataHandler.h"
#include "MacroStorage.h"
#include "config.h"
#include "rgb.h"

typedef struct
{
  uint8_t type;
  uint16_t value;
} KeyConfig;

static uint8_t validateEepromHeader(void);
static void initDefaultConfig(void);
static void loadRgbFromEEPROM(void);

static __xdata KeyConfig keySettings[MAX_LAYERS][KEY_CONFIG_SLOTS];
static __xdata uint8_t currentLayer = 0;

void KeysDataInit(void)
{
  if (eeprom_read_byte(EEPROM_VERSION_ADDR) == 0xFF || !validateEepromHeader())
  {
    initDefaultConfig();
  }
  else
  {
    loadAllFromEEPROM();
  }
  // 加载 RGB 配置
  loadRgbFromEEPROM();
  // 首次升级到 MeowFS: 格式化 flash 宏区
  if (eeprom_read_byte(EEPROM_MEOWFS_FMT_ADDR) != MEOWFS_FMT_MAGIC)
  {
    meowfs_format();
    eeprom_write_byte(EEPROM_MEOWFS_FMT_ADDR, MEOWFS_FMT_MAGIC);
  }
}

static uint8_t validateEepromHeader(void)
{
  return (eeprom_read_byte(EEPROM_VERSION_ADDR) == EEPROM_SCHEMA_ID) &&
         (eeprom_read_byte(EEPROM_DEVTYPE_ADDR) == EXPECT_DEVICE_TYPE);
}

static void initDefaultConfig(void)
{
  memset(keySettings, 0, sizeof(keySettings));

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
  for (uint8_t layer = 0; layer < MAX_LAYERS; layer++)
  {
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
  eeprom_write_byte(EEPROM_RGB_PRESS_EFFECT_ADDR, PRESS_EFFECT_NONE);
  // USB 轮询率默认 100Hz (bInterval=10)
  eeprom_write_byte(EEPROM_POLL_RATE_ADDR, 10);
  // MeowFS
  meowfs_format();
  eeprom_write_byte(EEPROM_MEOWFS_FMT_ADDR, MEOWFS_FMT_MAGIC);
}

void KeysDataFactoryReset(void)
{
  initDefaultConfig();
  loadRgbFromEEPROM();
}

// ── 当前层的快捷读取 ──

uint8_t getKeyType(uint8_t index)
{
  return getKeyTypeAt(currentLayer, index);
}

uint16_t getKeyValue(uint8_t index)
{
  return getKeyValueAt(currentLayer, index);
}

// ── 指定层的读取 ──

uint8_t getKeyTypeAt(uint8_t layer, uint8_t index)
{
  if (layer >= MAX_LAYERS || index >= KEY_CONFIG_SLOTS)
    return KEY_TYPE_INVALID;
  return keySettings[layer][index].type;
}

uint16_t getKeyValueAt(uint8_t layer, uint8_t index)
{
  if (layer >= MAX_LAYERS || index >= KEY_CONFIG_SLOTS)
    return 0;
  return keySettings[layer][index].value;
}

void setKey(uint8_t layer, uint8_t index, uint8_t type, uint16_t value)
{
  if (layer >= MAX_LAYERS || index >= KEY_CONFIG_SLOTS)
    return;
  keySettings[layer][index].type = type;
  keySettings[layer][index].value = value;
}

// ── 层管理 ──

uint8_t getCurrentLayer(void)
{
  return currentLayer;
}

void setCurrentLayer(uint8_t layer)
{
  if (layer >= MAX_LAYERS)
    return;
  if (currentLayer == layer)
    return;
  currentLayer = layer;
  eeprom_write_byte(EEPROM_LAYER_ADDR, layer);
}

// ── EEPROM 读写 ──

void saveLayerToEEPROM(uint8_t layer)
{
  if (layer >= MAX_LAYERS)
    return;
  uint16_t base = EEPROM_KEYDATA_START + (uint16_t)layer * LAYER_DATA_SIZE;
  for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++)
  {
    uint16_t addr = base + i * KEY_CONFIG_SIZE;
    eeprom_write_byte(addr, keySettings[layer][i].type);
    eeprom_write_byte(addr + 1, (uint8_t)(keySettings[layer][i].value >> 8));
    eeprom_write_byte(addr + 2, (uint8_t)(keySettings[layer][i].value & 0xFF));
  }
}

void saveCurrentLayerToEEPROM(void)
{
  eeprom_write_byte(EEPROM_LAYER_ADDR, currentLayer);
}

void loadAllFromEEPROM(void)
{
  currentLayer = eeprom_read_byte(EEPROM_LAYER_ADDR);
  if (currentLayer >= MAX_LAYERS)
    currentLayer = 0;

  for (uint8_t layer = 0; layer < MAX_LAYERS; layer++)
  {
    uint16_t base = EEPROM_KEYDATA_START + (uint16_t)layer * LAYER_DATA_SIZE;
    for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++)
    {
      uint16_t addr = base + i * KEY_CONFIG_SIZE;
      keySettings[layer][i].type = eeprom_read_byte(addr);
      uint8_t high = eeprom_read_byte(addr + 1);
      uint8_t low = eeprom_read_byte(addr + 2);
      keySettings[layer][i].value = ((uint16_t)high << 8) | low;
    }
  }
}

// ── HID 协议响应 ──

void fillSysInfoResponse(uint8_t *__xdata buf)
{
  memset(buf, 0, 31);
  buf[0] = HOST_CMD_READ;
  buf[1] = CH552_FEATURE_LEVEL;
  buf[2] = EXPECT_DEVICE_TYPE;
  buf[3] = currentLayer;
  buf[4] = MAX_LAYERS;
  for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++)
  {
    buf[5 + i * KEY_CONFIG_SIZE] = keySettings[0][i].type;
    buf[5 + i * KEY_CONFIG_SIZE + 1] = (uint8_t)(keySettings[0][i].value >> 8);
    buf[5 + i * KEY_CONFIG_SIZE + 2] = (uint8_t)(keySettings[0][i].value & 0xFF);
  }
}

void fillLayerResponse(uint8_t *__xdata buf, uint8_t layer)
{
  if (layer >= MAX_LAYERS)
    layer = 0;
  memset(buf, 0, 31);
  buf[0] = HOST_CMD_READ_LAYER;
  buf[1] = layer;
  for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++)
  {
    buf[2 + i * KEY_CONFIG_SIZE] = keySettings[layer][i].type;
    buf[2 + i * KEY_CONFIG_SIZE + 1] = (uint8_t)(keySettings[layer][i].value >> 8);
    buf[2 + i * KEY_CONFIG_SIZE + 2] = (uint8_t)(keySettings[layer][i].value & 0xFF);
  }
}

void fillRgbResponse(uint8_t *__xdata buf)
{
  memset(buf, 0, 31);
  buf[0] = HOST_CMD_READ_RGB;
  buf[1] = rgbEnabled;
  buf[2] = effectMode;
  buf[3] = currentBrightness;
  buf[4] = currentSpeed;
  buf[5] = currentColorR;
  buf[6] = currentColorG;
  buf[7] = currentColorB;
  buf[8] = 0;
  buf[9] = 0;
  buf[10] = pressEffect;
  buf[11] = eeprom_read_byte(EEPROM_POLL_RATE_ADDR);
  if (buf[11] == 0xFF || buf[11] == 0)
    buf[11] = 10; // 默认 100Hz
}

void applyRgbConfig(void)
{
  // 直接从 Ep1Buffer 读取, 无参数传递
  extern __xdata uint8_t Ep1Buffer[];
  __xdata uint8_t mode = Ep1Buffer[3];
  __xdata uint8_t pev = Ep1Buffer[11];
  __xdata uint8_t pollRate = Ep1Buffer[12];

  if (mode >= EFFECT_COUNT || mode == EFFECT_INDICATOR)
    mode = EFFECT_OFF;
  if (pev >= PRESS_EFFECT_COUNT)
    pev = PRESS_EFFECT_NONE;

  rgbEnabled = Ep1Buffer[2] ? 1 : 0;
  effectMode = mode;
  currentBrightness = Ep1Buffer[4];
  currentSpeed = Ep1Buffer[5];
  currentColorR = Ep1Buffer[6];
  currentColorG = Ep1Buffer[7];
  currentColorB = Ep1Buffer[8];
  pressEffect = pev;

  eeprom_write_byte(EEPROM_RGB_ENABLED_ADDR, rgbEnabled);
  eeprom_write_byte(EEPROM_RGB_MODE_ADDR, mode);
  eeprom_write_byte(EEPROM_RGB_BRIGHTNESS_ADDR, currentBrightness);
  eeprom_write_byte(EEPROM_RGB_SPEED_ADDR, currentSpeed);
  eeprom_write_byte(EEPROM_RGB_COLOR_R_ADDR, currentColorR);
  eeprom_write_byte(EEPROM_RGB_COLOR_G_ADDR, currentColorG);
  eeprom_write_byte(EEPROM_RGB_COLOR_B_ADDR, currentColorB);
  eeprom_write_byte(EEPROM_RGB_PRESS_EFFECT_ADDR, pev);

  // 轮询率: 仅支持 1/2/5/10, 重启后生效
  if (pollRate == 1 || pollRate == 2 || pollRate == 5 || pollRate == 10)
    eeprom_write_byte(EEPROM_POLL_RATE_ADDR, pollRate);
}

static void loadRgbFromEEPROM(void)
{
  __xdata uint8_t v;

  v = eeprom_read_byte(EEPROM_RGB_ENABLED_ADDR);
  rgbEnabled = (v == 0xFF || v) ? 1 : 0;

  v = eeprom_read_byte(EEPROM_RGB_MODE_ADDR);
  effectMode = (v == 0xFF || v >= EFFECT_COUNT || v == EFFECT_INDICATOR)
                   ? EFFECT_RAINBOW
                   : v;

  v = eeprom_read_byte(EEPROM_RGB_BRIGHTNESS_ADDR);
  currentBrightness = (v == 0xFF) ? 128 : v;

  v = eeprom_read_byte(EEPROM_RGB_SPEED_ADDR);
  currentSpeed = (v == 0xFF) ? 128 : v;

  v = eeprom_read_byte(EEPROM_RGB_COLOR_R_ADDR);
  currentColorR = v; // 0xFF is valid (white)

  v = eeprom_read_byte(EEPROM_RGB_COLOR_G_ADDR);
  currentColorG = v;

  v = eeprom_read_byte(EEPROM_RGB_COLOR_B_ADDR);
  currentColorB = v;

  v = eeprom_read_byte(EEPROM_RGB_PRESS_EFFECT_ADDR);
  pressEffect = (v == 0xFF || v >= PRESS_EFFECT_COUNT) ? PRESS_EFFECT_NONE : v;
}
