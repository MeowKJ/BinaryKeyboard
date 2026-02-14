# 常见问题（FAQ）

## 1) 插上 USB 没反应 / 电脑不识别

- 尝试进入 Bootloader：拔线 → 按住 BOOT → 再插线
- 检查 USB 焊点与供电是否正常（容易出现的问题）

## 2) 刷写成功但按键不对

- 你可能刷错了版本固件（基础/五键/旋钮不通用）
- 重新下载对应版本 `.hex` 再烧录

## 3) 改键网页连不上设备

- 尽量用 Chrome / Edge（需支持 WebHID）
- 检查是否被浏览器权限拦截（HID/USB 权限）
- 无线版：确认已用 USB 连接且键盘处于 USB 模式

## 4) 无线版：Debug Terminal 没有数据

- 在改键工具中打开底部 Debug Terminal，进入设置，开启「HID 日志」并点击保存
- 保存后需在工具内执行一次「保存配置」到设备，日志开关才会持久化

## 5) 无线版：BLE 连接后过一会儿显示未连接

现象：USB 切到 BLE、电脑配对后显示「正在连接」，过几秒或十几秒又变成「未连接」。

**先看断开原因**：用 USB 连接键盘，打开改键工具和 Debug Terminal，开启 HID 日志。再次用蓝牙连接，断开后看终端里**紧接着出现的一条 BLE 事件**，`opcode` 应为 `GAP_LINK_TERMINATED`，`reason` 即断开原因。常见含义：

| reason (hex) | 含义 |
| :----------- | :--- |
| 0x08 | CONNECTION_TIMEOUT（连接/监督超时） |
| 0x16 | LOCAL_HOST_TERMINATED（本机主动断开） |
| 0x3B | UNACCEPTABLE_CONN_PARAMS（主机不接受连接参数更新） |
| 0x13 | REMOTE_USER_TERMINATED（对端用户/系统断开） |
| 0x22 | LMP_LL_RESPONSE_TIMEOUT（链路层无响应） |

**可尝试**：

- **0x08 / 超时**：在 `Firmware/CH592F/MeowBLE/hid/include/kbd_mode_config.h` 中把 `KBD_BLE_CONN_TIMEOUT` 从 `500`（5 秒）改为 `600` 或 `800`，重新编译烧录。
- **0x3B / 参数不被接受**：同一文件里可适当放大 `KBD_BLE_CONN_INT_MIN/MAX`（如改为 16～32，即 20ms～40ms），或联系维护者调整连接参数更新时机。
- **0x16 / 本机断开**：多为系统/驱动或省电策略断开，可尝试关闭该设备的「允许关闭此设备以节约电源」、更新蓝牙驱动或换一台设备对比。

## 6) 无线版：BLE 配对完成后键盘卡死（RGB 不更新、按键无反应）

现象：FN 切到 BLE、长按进入广播，电脑连接/配对过程中键盘完全无响应（RGB、普通键、FN 均无反应），需重新上电或拔 USB 才能恢复。最后一条日志可能是 **连接建立后 STATE_CB_EXIT**、**SECURITY_REQUEST (0xB7)** 成功、或 **PAIR_CB_EXIT**（配对完成/保存绑定之后）。

**可能原因**：在 BLE 栈上下文中执行应用层代码（状态回调、日志、RGB 等）可能引发卡死；或配对/SNV 写路径、WCH BLE 库内部流程存在问题。

**可尝试**：

1. **固件已做缓解**：① **状态回调延后**：BLE 状态变化不再在栈上下文中直接调用应用（`onStateChange`），而是写入待处理队列，由主循环 `KBD_Mode_Process` 中取出并执行，从而避免在 BLE 栈里执行 RGB/连接状态等逻辑导致卡死。② 以下路径内默认不再打 BLE 诊断日志：SNV 写、配对/密码回调、安全请求事件、状态回调内日志（宏 `BLE_SNV_LOG_IN_WRITE`、`BLE_PAIRING_DIAG_LOG`、`BLE_SECURITY_REQ_DIAG_LOG`、`BLE_STATE_CB_DIAG_LOG` 均为 0）。需要调试时可把对应宏设为 1 再编译。
2. **关键验证：关闭绑定保存**：在 `Firmware/CH592F/MeowBLE/core/include/ble_config.h` 中把 `BLE_SNV` 设为 `FALSE` 后重新编译烧录，再测试“连接但不保存绑定”（每次重连需重新配对）：**若此时不再卡死，则问题在 SNV 写入或栈在写完成后的流程**；若仍卡死，则更可能是栈在配对完成回调返回后的其它路径有问题。
3. **更新 BLE 库 / 芯片固件**：查看 WCH 官方是否有 CH592 BLE ROM/SDK 更新或勘误说明，针对配对后死机有无修复或建议。
4. **缩短配对期负载**：确保在配对阶段不要同时做大量配置保存或其它 Flash 写（例如改键工具里暂不点“保存配置”），减少与 SNV 写争用或阻塞主循环的可能。

若你已确认最后一条日志的 opcode（如 0xBA = PAIR_CB_EXIT）和复现步骤，欢迎提 issue 附上终端日志片段，便于进一步排查。
