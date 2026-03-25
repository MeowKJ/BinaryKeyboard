#ifndef HAL_UTILS_H_
#define HAL_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  请求下一次复位进入高地址 IAP 程序
 * @note   通过写入 IMAGE_IAP_FLAG 后软复位实现，不再破坏 0x0000 起始区
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
