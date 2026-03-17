// clang-format off
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "CustomUSBHID.h"
#include "USBConstant.h"
#include "USBHandler.h"
#include "KeysDataHandler.h"
#include "MacroStorage.h"
#include "config.h"
#include "rgb.h"
// clang-format on

// clang-format off
extern __xdata __at (EP0_ADDR) uint8_t Ep0Buffer[];
extern __xdata __at (EP1_ADDR) uint8_t Ep1Buffer[];
// clang-format on

__xdata uint8_t keyboardLedStatus = 0;

volatile __xdata uint8_t UpPoint1_Busy =
    0; // Flag of whether upload pointer is busy

__xdata uint8_t HIDKey[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
__xdata uint16_t HIDConsumer[4] = {0x0, 0x0, 0x0, 0x0};
__xdata uint8_t HIDMouse[4] = {0x0, 0x0, 0x0, 0x0};
__xdata uint8_t CustomBuf[31] = {0x01};

typedef void (*pTaskFn)(void);

void delayMicroseconds(uint16_t us);

void USBInit()
{
  USBDeviceCfg();         // Device mode configuration
  USBDeviceEndPointCfg(); // Endpoint configuration
  USBDeviceIntCfg();      // Interrupt configuration
  UEP0_T_LEN = 0;
  UEP1_T_LEN = 0; // Pre-use send length must be cleared
  UEP2_T_LEN = 0;
}

void USB_EP1_IN()
{
  UEP1_T_LEN = 0;
  UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; // Default NAK
  UpPoint1_Busy = 0;                                       // Clear busy flag
}

// 宏 HID 命令处理
// 统一入口 0x40, Ep1Buffer 格式:
// [0]=4, [1]=0x40, [2]=sub, [3]=slot, [4..]=data
// sub: 0=INFO, 1=READ, 2=ERASE, 3=WRITE
static void handle_macro_cmd(void)
{
  __xdata uint8_t sub = Ep1Buffer[2];
  __xdata uint8_t slot = Ep1Buffer[3];

  CustomBuf[0] = HOST_CMD_MACRO_INFO;
  CustomBuf[1] = sub;
  CustomBuf[2] = 0;

  if (sub == 0)
  {
    CustomBuf[3] = MACRO_SLOT_COUNT;
    CustomBuf[4] = MACRO_SLOT_SIZE;
    CustomBuf[5] = macro_is_valid(0);
    CustomBuf[6] = macro_is_valid(1);
  }
  else if (slot < MACRO_SLOT_COUNT)
  {
    __xdata uint8_t off = Ep1Buffer[5];
    __xdata uint8_t len = Ep1Buffer[6];
    __xdata uint16_t base = macro_slot_addr(slot) + off;

    if (sub == 1)
    {
      if (len > 26)
        len = 26;
      if (off + len > MACRO_SLOT_SIZE)
        len = MACRO_SLOT_SIZE - off;
      CustomBuf[3] = len;
      for (__xdata uint8_t i = 0; i < len; i++)
        CustomBuf[4 + i] = macro_read_byte(base + i);
    }
    else if (sub == 2)
    {
      macro_erase_slot(slot);
    }
    else if (sub == 3)
    {
      if (len > 22)
        len = 22;
      for (__xdata uint8_t i = 0; i < len; i += 2)
      {
        __xdata uint8_t hi = (i + 1 < len) ? Ep1Buffer[8 + i] : 0xFF;
        macro_write_word(base + i, Ep1Buffer[7 + i], hi);
      }
    }
  }

  USB_EP1_send(5);
}

void USB_EP1_OUT()
{
  if (U_TOG_OK) // Discard unsynchronized packets
  {
    if (Ep1Buffer[0] == 1)
    {
      keyboardLedStatus = Ep1Buffer[1];
    }
    else if (Ep1Buffer[0] == 4)
    {
      uint8_t cmd = Ep1Buffer[1];
      if (cmd == HOST_CMD_READ)
      {
        // 0x01: 读取系统信息 + layer 0
        fillSysInfoResponse(CustomBuf);
        USB_EP1_send(5);
      }
      else if (cmd == HOST_CMD_WRITE)
      {
        // 0x02: 写入指定层 — [cmd, version, devType, layerIdx, 24 bytes]
        if (Ep1Buffer[2] != CH552_FEATURE_LEVEL || Ep1Buffer[3] != EXPECT_DEVICE_TYPE)
        {
          return;
        }
        uint8_t layer = Ep1Buffer[4];
        if (layer >= MAX_LAYERS)
          return;
        for (uint8_t i = 0; i < KEY_CONFIG_SLOTS; i++)
        {
          uint16_t value = ((uint16_t)Ep1Buffer[i * 3 + 6] << 8) | Ep1Buffer[i * 3 + 7];
          setKey(layer, i, Ep1Buffer[i * 3 + 5], value);
        }
        saveLayerToEEPROM(layer);
      }
      else if (cmd == HOST_CMD_READ_LAYER)
      {
        // 0x03: 读取指定层
        uint8_t layer = Ep1Buffer[2];
        if (layer >= MAX_LAYERS)
          layer = 0;
        fillLayerResponse(CustomBuf, layer);
        USB_EP1_send(5);
      }
      else if (cmd == HOST_CMD_SET_LAYER)
      {
        // 0x04: 切换当前层
        uint8_t layer = Ep1Buffer[2];
        if (layer < MAX_LAYERS)
        {
          setCurrentLayer(layer);
          flashLayerColor(layer, 0);
        }
      }
      else if (cmd == HOST_CMD_READ_META)
      {
        // 0x05: 读取固件/协议/存储元信息 (inline)
        memset(CustomBuf, 0, sizeof(CustomBuf));
        CustomBuf[0] = HOST_CMD_READ_META;
        CustomBuf[1] = FW_VERSION_MAJOR;
        CustomBuf[2] = FW_VERSION_MINOR;
        CustomBuf[3] = FW_VERSION_PATCH;
        USB_EP1_send(5);
      }
      else if (cmd == HOST_CMD_READ_RGB)
      {
        // 0x06: 读取 RGB 配置
        fillRgbResponse(CustomBuf);
        USB_EP1_send(5);
      }
      else if (cmd == HOST_CMD_WRITE_RGB)
      {
        applyRgbConfig();
      }
      else if (cmd >= HOST_CMD_MACRO_INFO)
      {
        handle_macro_cmd();
      }
    }
  }
}

uint8_t USB_EP1_send(__data uint8_t reportID)
{
  if (UsbConfig == 0)
  {
    return 0;
  }

  __data uint16_t waitWriteCount = 0;

  waitWriteCount = 0;
  while (UpPoint1_Busy)
  { // wait for 250ms or give up
    waitWriteCount++;
    delayMicroseconds(5);
    if (waitWriteCount >= 50000)
      return 0;
  }

  __xdata uint8_t *src;
  __data uint8_t len;
  switch (reportID)
  {
  case 1:
    src = HIDKey;
    len = sizeof(HIDKey);
    break;
  case 2:
    src = (uint8_t *)HIDConsumer;
    len = sizeof(HIDConsumer);
    break;
  case 3:
    src = (uint8_t *)HIDMouse;
    len = sizeof(HIDMouse);
    break;
  case 5:
    src = CustomBuf;
    len = sizeof(CustomBuf);
    break;
  default:
    UEP1_T_LEN = 0;
    return 0;
  }
  Ep1Buffer[64] = reportID;
  for (__data uint8_t i = 0; i < len; i++)
  {
    Ep1Buffer[64 + 1 + i] = src[i];
  }
  UEP1_T_LEN = 1 + len;

  UpPoint1_Busy = 1;
  UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // upload data and respond ACK

  return 1;
}

void Keyboard_releaseAll(void)
{
  for (__data uint8_t i = 0; i < sizeof(HIDKey); i++)
  { // load data for upload
    HIDKey[i] = 0;
  }
  USB_EP1_send(1);
}

uint8_t Keyboard_rawPress(__data uint8_t k, __data uint8_t mod)
{
  if (k == 0)
    return 1;
  HIDKey[0] |= mod;

  for (uint8_t i = 2; i < 8; i++)
  {
    if (HIDKey[i] == k)
    {
      return 1;
    }
  }
  for (uint8_t i = 2; i < 8; i++)
  {
    if (HIDKey[i] == 0)
    {
      HIDKey[i] = k;
      USB_EP1_send(1);
      return 1;
    }
  }

  return 0;
}

uint8_t Keyboard_rawRelease(__data uint8_t k, __data uint8_t mod)
{
  if (k == 0)
    return 1;
  HIDKey[0] &= ~mod;

  for (uint8_t i = 2; i < 8; i++)
  {
    if (HIDKey[i] == k)
    {
      HIDKey[i] = 0;
      USB_EP1_send(1);
      return 1;
    }
  }
  return 0;
}

void Keyboard_sendReport(__data uint8_t mod, __near uint8_t *keys)
{
  // 设置修饰键
  HIDKey[0] = mod;

  // 清空之前的按键
  memset(HIDKey + 2, 0, 6);

  // 填充新按键（最多6个）
  for (uint8_t i = 0; i < 6 && keys[i] != 0; i++)
  {
    HIDKey[i + 2] = keys[i];
  }

  // 发送报告
  USB_EP1_send(1);
}

uint8_t Consumer_press(__data uint16_t k)
{
  __data uint8_t i;

  // Add k to the consumer report only if it's not already present
  // and if there is an empty slot.
  if (HIDConsumer[0] != k && HIDConsumer[1] != k && HIDConsumer[2] != k && HIDConsumer[3] != k)
  {

    for (i = 0; i < 4; i++)
    {
      if (HIDConsumer[i] == 0x00)
      {
        HIDConsumer[i] = k;
        break;
      }
    }
    if (i == 4)
    {
      // setWriteError();
      return 0;
    }
  }
  USB_EP1_send(2);
  return 1;
}

uint8_t Consumer_release(__data uint16_t k)
{
  __data uint8_t i;

  // Test the consumer report to see if k is present.  Clear it if it exists.
  // Check all positions in case the key is present more than once (which it
  // shouldn't be)
  for (i = 0; i < 4; i++)
  {
    if (0 != k && HIDConsumer[i] == k)
    {
      HIDConsumer[i] = 0x00;
    }
  }

  USB_EP1_send(2);
  return 1;
}

void Consumer_releaseAll(void)
{
  for (__data uint8_t i = 0; i < 4; i++)
  { // load data for upload
    HIDConsumer[i] = 0;
  }
  USB_EP1_send(2);
}

uint8_t Consumer_write(__data uint16_t c)
{
  __data uint8_t p = Consumer_press(c); // Keydown
  Consumer_release(c);                  // Keyup
  return p;                             // just return the result of press() since release() almost always
                                        // returns 1
}

uint8_t Mouse_press(__data uint8_t k)
{
  HIDMouse[0] |= k;
  USB_EP1_send(3);
  return 1;
}

uint8_t Mouse_release(__data uint8_t k)
{
  HIDMouse[0] &= ~k;
  USB_EP1_send(3);
  return 1;
}

void Mouse_releaseAll(void)
{
  HIDMouse[0] = 0;
  USB_EP1_send(3);
}

uint8_t Mouse_scroll(__data int8_t tilt)
{
  if (tilt == 0)
    return 0;
  HIDMouse[3] = tilt;
  USB_EP1_send(3);
  HIDMouse[3] = 0;
  return 1;
}
