#ifndef HAL_UTILS_H_
#define HAL_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  跳转到 Bootloader
 */
void Hal_JumpToBootloader(void) __attribute__((noreturn));

/**
 * @brief  系统软复位
 * @note   不返回
 */
void Hal_Reset(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* HAL_UTILS_H_ */
