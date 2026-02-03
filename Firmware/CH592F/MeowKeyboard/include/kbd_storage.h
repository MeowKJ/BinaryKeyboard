/**
 * @file    kbd_storage.h
 * @brief   MeowKeyboard 配置存储接口
 * @author  MeowKJ
 * @version V1.0.0
 * @date    2024-11-07
 *
 * @details
 * 本模块提供基于 CH592F DataFlash 的配置持久化功能：
 * - 配置数据的读取与保存
 * - 按键映射和 FN 键配置管理
 * - 宏数据的分块读写
 * - CRC32 校验确保数据完整性
 *
 * @note    DataFlash 总容量 32KB，擦除粒度为 4KB
 *
 * @copyright Copyright (c) 2024 MeowKJ. All rights reserved.
 */

#ifndef __KBD_STORAGE_H
#define __KBD_STORAGE_H

#include "kbd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/**
 * @defgroup KBD_Storage_Init 初始化函数
 * @{
 */

/**
 * @brief 初始化配置存储系统
 *
 * 从 DataFlash 加载配置，如果配置无效则使用默认值
 *
 * @return 0 成功
 * @return -1 Flash 读取失败 (使用默认配置)
 * @return -2 配置校验失败 (使用默认配置)
 */
int KBD_Storage_Init(void);

/** @} */ /* end of KBD_Storage_Init */

/*============================================================================*/
/**
 * @defgroup KBD_Storage_Config 配置管理函数
 * @{
 */

/**
 * @brief 从 DataFlash 加载配置
 *
 * @return 0 成功
 * @return -1 魔数不匹配
 * @return -2 版本不兼容
 */
int KBD_Config_Load(void);

/**
 * @brief 保存配置到 DataFlash
 *
 * @note 此操作会擦除并重写 Flash 块，耗时约 50ms
 *
 * @return 0 成功
 * @return -1 擦除失败
 * @return -2 写入失败
 */
int KBD_Config_Save(void);

/**
 * @brief 恢复出厂设置
 *
 * 重置所有配置为默认值并保存到 Flash
 *
 * @return 0 成功
 * @return 负数 保存失败
 */
int KBD_Config_Reset(void);

/** @} */ /* end of KBD_Storage_Config */

/*============================================================================*/
/**
 * @defgroup KBD_Storage_Access 配置访问函数
 * @{
 */

/**
 * @brief 获取系统配置指针
 * @return 系统配置结构指针 (RAM 缓存)
 */
kbd_system_config_t* KBD_GetSystemConfig(void);

/**
 * @brief 获取按键映射配置指针
 * @return 按键映射配置结构指针 (RAM 缓存)
 */
kbd_keymap_t* KBD_GetKeymap(void);

/**
 * @brief 获取 FN 键配置指针
 * @return FN 键配置结构指针 (RAM 缓存)
 */
kbd_fnkey_config_t* KBD_GetFnKeyConfig(void);

/**
 * @brief 获取 RGB 配置指针
 * @return RGB 配置结构指针 (RAM 缓存)
 */
kbd_rgb_config_t* KBD_GetRgbConfig(void);

/** @} */ /* end of KBD_Storage_Access */

/*============================================================================*/
/**
 * @defgroup KBD_Storage_Layer 层操作函数
 * @{
 */

/**
 * @brief 获取当前激活层号
 * @return 当前层号 (0 ~ num_layers-1)
 */
uint8_t KBD_GetCurrentLayer(void);

/**
 * @brief 设置当前激活层
 *
 * @param[in] layer 目标层号
 * @return 0 成功
 * @return -1 层号超出范围
 */
int KBD_SetCurrentLayer(uint8_t layer);

/**
 * @brief 切换到下一层
 *
 * 如果当前是最后一层，则回到第 0 层
 *
 * @return 切换后的层号
 */
uint8_t KBD_NextLayer(void);

/**
 * @brief 切换到上一层
 *
 * 如果当前是第 0 层，则跳到最后一层
 *
 * @return 切换后的层号
 */
uint8_t KBD_PrevLayer(void);

/**
 * @brief 获取指定按键在当前层的动作
 *
 * @param[in] key_index 按键索引 (0 ~ KBD_MAX_KEYS-1)
 * @return 按键动作指针
 * @return NULL 索引无效
 */
const kbd_action_t* KBD_GetKeyAction(uint8_t key_index);

/** @} */ /* end of KBD_Storage_Layer */

/*============================================================================*/
/**
 * @defgroup KBD_Storage_Macro 宏存储函数
 * @{
 */

/**
 * @brief 获取宏槽位信息
 *
 * @param[in]  slot   槽位号 (0 ~ KBD_MACRO_SLOTS-1)
 * @param[out] header 输出宏头部信息 (可为 NULL)
 * @return 0 成功
 * @return -1 槽位号无效
 * @return -2 槽位为空
 */
int Kbd_Macro_GetInfo(uint8_t slot, kbd_macro_header_t *header);

/**
 * @brief 读取宏数据
 *
 * @param[in]  slot   槽位号
 * @param[in]  offset 数据偏移量
 * @param[out] buf    输出缓冲区
 * @param[in]  len    请求读取长度
 * @return 实际读取的字节数
 * @return 负数 错误
 */
int Kbd_Macro_Read(uint8_t slot, uint16_t offset, uint8_t *buf, uint16_t len);

/**
 * @brief 开始写入宏
 *
 * @param[in] slot   槽位号
 * @param[in] header 宏头部信息
 * @return 0 成功
 * @return KBD_RESP_ERR_TOO_LARGE 宏数据过大
 * @return 负数 其他错误
 */
int Kbd_Macro_BeginWrite(uint8_t slot, const kbd_macro_header_t *header);

/**
 * @brief 写入宏数据块
 *
 * @param[in] slot   槽位号
 * @param[in] offset 数据偏移量
 * @param[in] buf    数据缓冲区
 * @param[in] len    数据长度
 * @return 0 成功
 * @return 负数 错误
 */
int Kbd_Macro_WriteChunk(uint8_t slot, uint16_t offset, const uint8_t *buf, uint16_t len);

/**
 * @brief 完成宏写入
 *
 * 标记宏为有效状态
 *
 * @param[in] slot 槽位号
 * @return 0 成功
 * @return 负数 错误
 */
int Kbd_Macro_EndWrite(uint8_t slot);

/**
 * @brief 删除宏
 *
 * @param[in] slot 槽位号
 * @return 0 成功
 * @return 负数 错误
 */
int Kbd_Macro_Delete(uint8_t slot);

/**
 * @brief 检查宏槽位是否有效
 *
 * @param[in] slot 槽位号
 * @return true 有效
 * @return false 无效或为空
 */
bool Kbd_Macro_IsValid(uint8_t slot);

/**
 * @brief 获取已使用的宏槽位数
 * @return 已使用槽位数量
 */
uint8_t Kbd_Macro_GetUsedCount(void);

/** @} */ /* end of KBD_Storage_Macro */

/*============================================================================*/
/**
 * @defgroup KBD_Storage_Utils 工具函数
 * @{
 */

/**
 * @brief 计算 CRC32 校验码
 *
 * @param[in] data 数据指针
 * @param[in] len  数据长度
 * @return CRC32 值
 */
uint32_t KBD_CalcCRC32(const uint8_t *data, uint32_t len);

/** @} */ /* end of KBD_Storage_Utils */

#ifdef __cplusplus
}
#endif

#endif /* __KBD_STORAGE_H */
