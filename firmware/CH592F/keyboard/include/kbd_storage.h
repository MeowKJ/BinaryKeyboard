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
 * @note    DataFlash 总容量 32KB；CH592F 支持 256B/4KB 擦除（配置区使用 256B 页级更新）
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

/**
 * @brief 刷新 runtime 热数据（如当前层）到 DataFlash
 *
 * @note 用于睡眠前/模式切换前强制落盘。若无待保存变更则直接返回成功。
 *
 * @return 0 成功（含无变更）
 * @return 负数 擦写失败
 */
int KBD_Storage_FlushRuntime(void);

/**
 * @brief 推迟 runtime 热数据写入（避免在 BLE 连接/配对窗口写 Flash）
 *
 * 取消当前待执行的 runtime save 定时器，重新调度到指定延迟后执行。
 * 仅在有待保存数据时生效，否则直接返回。
 *
 * @param[in] delay_ms 推迟时间（毫秒）
 */
void KBD_Storage_DeferRuntimeSave(uint32_t delay_ms);

/**
 * @brief 存储系统状态快照（调试/状态展示）
 */
typedef struct {
    uint8_t config_active_slot;
    uint8_t runtime_active_page;
    uint8_t runtime_dirty;
    uint8_t reserved;
    uint32_t config_save_count;
    uint32_t runtime_seq;
} kbd_storage_status_t;

/**
 * @brief 获取存储系统当前状态（非阻塞）
 *
 * @param[out] status 输出状态结构（可为 NULL）
 */
void KBD_Storage_PollStatus(kbd_storage_status_t *status);

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
 * @note CH592F 下采用冷热分离：配置槽位轮转 + runtime 热数据页环（256B）
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
 * @brief 获取上次持久化的工作模式
 *
 * 返回 runtime 热数据中保存的模式值，用于开机恢复。
 *
 * @return 0 USB 模式
 * @return 1 BLE 模式
 * @return 0xFF 未知（首次启动或 Flash 无效）
 */
uint8_t KBD_GetLastMode(void);

/**
 * @brief 设置工作模式并异步持久化到 runtime 热数据
 *
 * 通过 TMOS 200ms 防抖延迟写入，与 current_layer 共享同一页写操作。
 *
 * @param[in] mode 0=USB, 1=BLE
 * @return 0 成功
 */
int KBD_SetLastMode(uint8_t mode);

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
 * @brief 获取宏信息
 *
 * @param[in]  slot   宏索引（按有效条目顺序）
 * @param[out] header 输出宏头部信息 (可为 NULL)
 * @return 0 成功
 * @return -1 索引无效
 * @return -2 宏不存在
 */
int Kbd_Macro_GetInfo(uint8_t slot, kbd_macro_header_t *header);

/**
 * @brief 读取宏数据
 *
 * @param[in]  slot   宏索引
 * @param[in]  offset 数据偏移量
 * @param[out] buf    输出缓冲区
 * @param[in]  len    请求读取长度
 * @return 实际读取的字节数
 * @return 负数 错误
 */
int Kbd_Macro_Read(uint8_t slot, uint16_t offset, uint8_t *buf, uint16_t len);

/**
 * @brief 读取 MeowFS 原始数据
 * @param[in]  offset 区域内偏移
 * @param[out] buf    输出缓冲区
 * @param[in]  len    请求长度
 * @return 实际读取长度
 * @return 负数 错误
 */
int Kbd_Macro_ReadRaw(uint16_t offset, uint8_t *buf, uint16_t len);

/**
 * @brief 写入 MeowFS 原始数据
 *
 * @param[in] offset 数据偏移量
 * @param[in] buf    数据缓冲区
 * @param[in] len    数据长度
 * @return 0 成功
 * @return 负数 错误
 */
int Kbd_Macro_WriteRaw(uint16_t offset, const uint8_t *buf, uint16_t len);

/**
 * @brief 擦除指定 MeowFS 页
 * @param[in] page_index 页索引（0 ~ page_count-1）
 * @return 0 成功
 * @return 负数 错误
 */
int Kbd_Macro_ErasePage(uint8_t page_index);

/**
 * @brief 擦除整个 MeowFS 区域
 * @return 0 成功
 * @return 负数 错误
 */
int Kbd_Macro_EraseAll(void);

/**
 * @brief 删除宏
 *
 * @param[in] slot 宏索引
 * @return 0 成功
 * @return 负数 错误
 */
int Kbd_Macro_Delete(uint8_t slot);

/**
 * @brief 检查宏索引是否有效
 *
 * @param[in] slot 宏索引
 * @return true 有效
 * @return false 无效或为空
 */
bool Kbd_Macro_IsValid(uint8_t slot);

/**
 * @brief 获取已使用的宏数量
 * @return 有效宏数量
 */
uint8_t Kbd_Macro_GetUsedCount(void);

/**
 * @brief 获取已占用字节数（含已删除条目）
 */
uint16_t Kbd_Macro_GetUsedBytes(void);

/**
 * @brief 获取剩余可用字节数
 */
uint16_t Kbd_Macro_GetFreeBytes(void);

/**
 * @brief 获取 MeowFS 总容量
 */
uint16_t Kbd_Macro_GetTotalSize(void);

/**
 * @brief 获取 MeowFS 页大小
 */
uint16_t Kbd_Macro_GetPageSize(void);

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
