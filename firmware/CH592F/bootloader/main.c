#include <stdint.h>
#include <string.h>

#include "CH59x_common.h"
#include "iap_config.h"

#define jumpApp ((void (*)(void))((uint32_t)IMAGE_A_START_ADD))

static uint8_t ReadImageFlag(void)
{
    __attribute__((aligned(4))) uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    EEPROM_READ(IAP_DATAFLASH_ADD, (uint32_t *)buf, 4);
    if (buf[0] == IMAGE_A_FLAG || buf[0] == IMAGE_B_FLAG || buf[0] == IMAGE_IAP_FLAG) {
        return buf[0];
    }
    return IMAGE_A_FLAG;
}

static void WriteImageFlag(uint8_t flag)
{
    __attribute__((aligned(4))) uint8_t buf[4] = {0};
    EEPROM_ERASE(IAP_DATAFLASH_ADD, EEPROM_PAGE_SIZE);
    buf[0] = flag;
    EEPROM_WRITE(IAP_DATAFLASH_ADD, (uint32_t *)buf, 4);
}

static int ValidateImage(uint32_t addr)
{
    volatile uint32_t first_word = *(volatile uint32_t *)addr;
    if (first_word == 0xFFFFFFFF || first_word == 0x00000000) {
        return 0;
    }
    return 1;
}

static int CopyImageBtoA(void)
{
    __attribute__((aligned(4))) uint8_t buf[IAP_COPY_CHUNK_SIZE];
    uint32_t offset = 0;

    if (FLASH_ROM_ERASE(IMAGE_A_START_ADD, IMAGE_A_SIZE) != 0) {
        return -1;
    }

    while (offset < IMAGE_SIZE) {
        uint32_t chunk = IAP_COPY_CHUNK_SIZE;
        if (offset + chunk > IMAGE_SIZE) {
            chunk = IMAGE_SIZE - offset;
        }

        memcpy(buf, (const void *)(IMAGE_B_START_ADD + offset), chunk);
        if (FLASH_ROM_WRITE(IMAGE_A_START_ADD + offset, buf, chunk) != 0) {
            return -1;
        }
        offset += chunk;
    }

    return (*(volatile uint32_t *)IMAGE_A_START_ADD == *(volatile uint32_t *)IMAGE_B_START_ADD) ? 0 : -1;
}

int main(void)
{
#if (defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif

    SetSysClock(CLK_SOURCE_PLL_60MHz);

#if (defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif

    if (ReadImageFlag() == IMAGE_IAP_FLAG) {
        if (ValidateImage(IMAGE_B_START_ADD) && CopyImageBtoA() == 0) {
            WriteImageFlag(IMAGE_A_FLAG);
            FLASH_ROM_ERASE(IMAGE_B_START_ADD, IMAGE_B_SIZE);
        } else {
            WriteImageFlag(IMAGE_A_FLAG);
        }
    }

    if (ValidateImage(IMAGE_A_START_ADD)) {
        jumpApp();
    }

    while (1) {
    }
}
