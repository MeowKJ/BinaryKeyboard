#include "hal_utils.h"

#include "CH59x_common.h"

__HIGH_CODE
void Hal_JumpToBootloader(void) {
    FLASH_ROM_ERASE(0, EEPROM_BLOCK_SIZE); /* 营造空片现象 */
    FLASH_ROM_SW_RESET();
    sys_safe_access_enable();
    R16_INT32K_TUNE = 0xFFFF;
    sys_safe_access_disable();
    SYS_ResetExecute();
    while (1);
}

__HIGH_CODE
void Hal_Reset(void) {
    SYS_ResetExecute();
    while (1);
}
