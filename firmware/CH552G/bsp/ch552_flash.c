/*
 * ch552_flash.c — CH552G Code Flash 读 / 擦 / 写
 *
 * 从 MacroStorage.c 中提取的底层 Flash 操作，供宏存储和未来 MeowFS 共用。
 *
 * Flash 擦除粒度: 64 字节 (一页)
 * Flash 写入粒度: 2 字节 (一个 word, lo+hi)
 * Flash 读取: 直接 __code 指针解引用
 */

// clang-format off
#include <stdint.h>
#include "include/ch5xx.h"
// clang-format on

uint8_t flash_read_byte(uint16_t addr) {
    return *(__code uint8_t *)addr;
}

void flash_erase_page(uint16_t page_addr) {
    __bit saved_ea = EA;
    EA = 0;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bCODE_WE;
    ROM_ADDR_H = (uint8_t)(page_addr >> 8);
    ROM_ADDR_L = (uint8_t)(page_addr);
    ROM_CTRL = 0xA6;           /* erase — unconditional */
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bCODE_WE;
    SAFE_MOD = 0;
    EA = saved_ea;
}

void flash_write_word(uint16_t addr, uint8_t lo, uint8_t hi) {
    __bit saved_ea = EA;
    EA = 0;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bCODE_WE;
    ROM_ADDR_H = (uint8_t)(addr >> 8);
    ROM_ADDR_L = (uint8_t)(addr);
    ROM_DATA_L = lo;
    ROM_DATA_H = hi;
    if (ROM_STATUS & bROM_ADDR_OK) {
        ROM_CTRL = ROM_CMD_WRITE;
    }
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bCODE_WE;
    SAFE_MOD = 0;
    EA = saved_ea;
}
