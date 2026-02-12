# 低功耗蓝牙 (BLE)

MeowKeyboard 无线版基于 CH592F 的 BLE5.4 协议栈，实现 HID over GATT，支持标准 HID 设备配对与使用。

## 概览

| 项目 | 说明 |
| :--- | :--- |
| 芯片 | CH592F (RISC-V, BLE5.4) |
| 角色 | 外设 (Peripheral) |
| 连接数 | 单连接 |
| 设备名称 | MeowKeyboard |

## GATT 服务列表

| 服务 UUID | 名称 | 说明 |
| :-------- | :--- | :--- |
| 0x1800 | GAP | 通用访问，设备名、外观等 |
| 0x1801 | GATT | 通用属性，服务变更等 |
| 0x180A | Device Information | 设备信息（型号、厂商、固件版本等） |
| 0x180F | Battery Service | 电池电量 |
| 0x1812 | HID | 键盘、鼠标、多媒体输入 |
| 0x1813 | Scan Parameters | 扫描参数（主机写、从机通知） |

---

## HID 服务 (0x1812)

### 特性列表

| 特性 | UUID | 方向 | 说明 |
| :--- | :--- | :--- | :--- |
| HID Information | 0x2A4A | 读 | bcdHID、Country、Flags |
| Report Map | 0x2A4B | 读 | HID 报告描述符 |
| HID Control Point | 0x2A4C | 写无响应 | Suspend/Exit Suspend |
| Report | 0x2A4D | 读/写/通知 | 复合报告（键盘/鼠标/多媒体等） |
| Protocol Mode | 0x2A4E | 读/写 | Boot/Report 模式 |
| Boot Keyboard Input | 0x2A22 | 通知 | Boot 键盘输入 |
| Boot Keyboard Output | 0x2A32 | 读/写 | Boot 键盘 LED |
| Boot Mouse Input | 0x2A33 | 通知 | Boot 鼠标输入 |

### HID 报告 ID

| Report ID | 用途 | 方向 | 大小 |
| :-------- | :--- | :--- | :--- |
| 0 | 键盘输入 | 设备→主机 | 8B |
| 0 | LED 输出 | 主机→设备 | 1B |
| 1 | 鼠标输入 | 设备→主机 | 4B |
| 2 | 多媒体输入 | 设备→主机 | 2B |
| 4 | 电池电量 (HID Report Ref) | 设备→主机 | 1B |

键盘、鼠标、多媒体报告与 USB HID 格式一致，详见 [HID 通讯协议](./hid.md)。

::: info Report 协议模式
固件使用 **Report Protocol Mode**，主机需通过 Protocol Mode 特性选择 Report 模式，并通过 Report 特性读写各 Report ID 对应数据。
:::

---

## Device Information (0x180A)

| 特性 | UUID | 说明 |
| :--- | :--- | :--- |
| System ID | 0x2A23 | 系统标识 |
| Model Number | 0x2A24 | 型号字符串 |
| Serial Number | 0x2A25 | 序列号 |
| Firmware Revision | 0x2A26 | 固件版本 |
| Hardware Revision | 0x2A27 | 硬件版本 |
| Software Revision | 0x2A28 | 软件版本 |
| Manufacturer Name | 0x2A29 | 厂商名称 |
| PnP ID | 0x2A50 | PnP 标识 |

---

## Battery Service (0x180F)

| 特性 | UUID | 方向 | 说明 |
| :--- | :--- | :--- | :--- |
| Battery Level | 0x2A19 | 读/通知 | 电量百分比 (0–100) |

主机可订阅 Battery Level 通知以获取电量更新。

---

## Scan Parameters (0x1813)

| 特性 | UUID | 方向 | 说明 |
| :--- | :--- | :--- | :--- |
| Scan Interval Window | 0x2A4F | 写 | 主机写入扫描间隔/窗口 |
| Scan Parameter Refresh | 0x2A31 | 通知 | 从机通知刷新请求 |

用于主机控制从机广播参数，符合 BLE 规范。

---

## 配对与绑定

| 配置项 | 值 | 说明 |
| :----- | :--- | :--- |
| Pairing Mode | Wait For Req | 等待主机发起配对 |
| Bonding | 启用 | 保存绑定信息 |
| MITM | 禁用 | 无中间人保护 |
| IO Capabilities | NoInputNoOutput | 无显示/输入能力（默认 Just Works） |

绑定信息存储在 DataFlash **SNV 区** (0x7E00~0x7EFF)，由 WCH BLE 库管理，详见 [DataFlash 布局](./dataflash.md#ble-snv-区-0x7e00--0x7eff)。

::: tip 多设备配对
支持与多台主机配对绑定，断开后可自动重连。通过 FN 键可触发「清除绑定」等操作。
:::

---

## 广播与连接参数

### 广播

| 参数 | 值 | 单位 |
| :--- | :--- | :--- |
| 广播间隔最小 | 48 | 0.625ms → 30ms |
| 广播间隔最大 | 80 | 0.625ms → 50ms |
| 广播超时 | 60 | 秒 |

### 连接

| 参数 | 值 | 单位 |
| :--- | :--- | :--- |
| 连接间隔 | 8 | 1.25ms → 10ms |
| 从机延迟 | 20 | 连接事件 |
| 监督超时 | 500 | 10ms → 5s |

---

## BLE 协议栈配置 (ble_config.h)

| 配置项 | 默认值 | 说明 |
| :----- | :----- | :--- |
| BLE_SNV | TRUE | 启用 SNV 存储绑定 |
| BLE_SNV_ADDR | 0x77000 - FLASH_ROM_MAX_SIZE | SNV 地址 (DataFlash 末段) |
| BLE_SNV_BLOCK | 256 | SNV 块大小 |
| BLE_SNV_NUM | 1 | SNV 块数量 |
| BLE_BUFF_MAX_LEN | 27 | 单包最大长度 (ATT_MTU=23) |
| BLE_TX_NUM_EVENT | 1 | 单连接事件最多发包数 |
| PERIPHERAL_MAX_CONNECTION | 1 | 从机最大连接数 |

---

## 低功耗与唤醒

| 配置项 | 值 | 说明 |
| :----- | :--- | :--- |
| HID 空闲超时 | 60s | 无 HID 活动后的处理 |
| 按键唤醒 | 启用 | 按键可唤醒休眠 |
| USB 唤醒 | 启用 | USB 插入可唤醒 |

空闲一段时间后可进入低功耗，具体策略见固件 `kbd_mode_config.h`。

---

## 相关文档

- [HID 通讯协议](./hid.md) - 报告格式与配置命令
- [DataFlash 布局](./dataflash.md) - SNV 区与 BLE 存储
- [固件开发](./dev.md) - 编译与调试
