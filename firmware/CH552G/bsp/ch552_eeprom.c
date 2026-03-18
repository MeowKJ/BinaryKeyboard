/*
 * ch552_eeprom.c — CH552G DataFlash (EEPROM) 读写
 *
 * CH552 DataFlash: 128 bytes (地址 0-127), 每字节 10K 次擦写寿命。
 * 从 ch55xduino eeprom.c 搬移，移除其他芯片分支。
 */

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
#include "include/ch5xx_usb.h"
// clang-format on

void eeprom_write_byte(__data uint8_t addr, __xdata uint8_t val) {
    if (addr >= 128)
        return;

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bDATA_WE;    /* Enable DataFlash write */
    SAFE_MOD = 0;

    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    ROM_ADDR_L = addr << 1;
    ROM_DATA_L = val;
    if (ROM_STATUS & bROM_ADDR_OK) {
        ROM_CTRL = ROM_CMD_WRITE;
    }

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bDATA_WE;   /* Disable DataFlash write */
    SAFE_MOD = 0;
}

uint8_t eeprom_read_byte(__data uint8_t addr) {
    ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
    ROM_ADDR_L = addr << 1;
    ROM_CTRL = ROM_CMD_READ;
    return ROM_DATA_L;
}
