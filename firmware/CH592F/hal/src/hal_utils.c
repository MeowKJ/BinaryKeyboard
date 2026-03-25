#include "hal_utils.h"

#include "CH59x_common.h"
#include "iap_config.h"

__HIGH_CODE
void Hal_JumpToBootloader(void) {
    __attribute__((aligned(4))) uint8_t buf[4] = {0};

    EEPROM_READ(IAP_DATAFLASH_ADD, (uint32_t *)buf, 4);
    if (buf[0] != IMAGE_IAP_FLAG) {
        EEPROM_ERASE(IAP_DATAFLASH_ADD, EEPROM_BLOCK_SIZE);
        buf[0] = IMAGE_IAP_FLAG;
        EEPROM_WRITE(IAP_DATAFLASH_ADD, (uint32_t *)buf, 4);
    }

    SYS_ResetExecute();
    while (1);
}

__HIGH_CODE
void Hal_Reset(void) {
    SYS_ResetExecute();
    while (1);
}
