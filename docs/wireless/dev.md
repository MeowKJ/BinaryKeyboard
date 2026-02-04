# 无线版固件开发

基于 **CH592F** 芯片的固件开发指南。

## 开发环境

### 推荐工具链

| 工具                | 说明         |
| :------------------ | :----------- |
| MounRiver Studio II | WCH 官方 IDE |


### 固件目录

```
Firmware/CH592F/
├── Board/              # 板级支持
│   ├── include/
│   └── src/
├── MeowBLE/            # 蓝牙协议栈
│   ├── hal/
│   ├── lib/
│   ├── meow/
│   └── profile/
├── MeowKeyboard/       # 键盘核心逻辑
│   ├── include/
│   └── src/
├── MeowUSB/            # USB 支持
│   ├── include/
│   └── src/
├── StdPeriphDriver/    # 外设驱动
├── User/
│   └── Main.c
└── CH592F.wvproj       # 工程文件
```

## 核心模块

### MeowBLE - 蓝牙模块

- `ble_hid.c/h` - 蓝牙 HID 实现
- `ble_hid_service.c/h` - HID 服务
- `kbd_mode.c/h` - 键盘模式管理

### MeowKeyboard - 键盘核心

- `kbd_core.c/h` - 按键扫描与处理
- `kbd_rgb.c/h` - RGB 灯效
- `kbd_storage.c/h` - 配置存储
- `kbd_command.c/h` - 命令处理

### MeowUSB - USB 模块

- `usb_hid.c/h` - USB HID 实现
- `usb_device.c/h` - USB 设备管理
- `usb_descriptors.c/h` - USB 描述符

## 版本配置

在 `Board/include/kbd_config.h` 中配置：

```c
// 外形版本
#define KBD_TYPE_BASIC    0
#define KBD_TYPE_5KEYS    1
#define KBD_TYPE_KNOB     2

#define KBD_TYPE KBD_TYPE_BASIC
```

## 编译与烧录

### 使用 MounRiver Studio

1. 打开 `CH592F.wvproj`
2. 选择 Build 配置
3. 点击编译
4. 使用 WCHISPStudio 烧录

## HID 协议

无线版同时支持蓝牙 HID 和 USB HID：

| Report ID | 功能        | 数据长度 |
| :-------- | :---------- | :------- |
| 1         | 键盘输入    | 8 字节   |
| 2         | 控制器      | 8 字节   |
| 3         | 鼠标输入    | 5 字节   |
| 4         | 主机 → 键盘 | 31 字节  |
| 5         | 键盘 → 主机 | 31 字节  |

::: warning 注意
蓝牙模式下无法使用 Report ID 4/5 进行改键，需切换到 USB 模式。
:::

## 低功耗设计

无线版需要考虑功耗：

TODO    

## 参考资料

- [CH592 数据手册](https://www.wch.cn/products/CH592.html)
- [MounRiver Studio](http://www.mounriver.com/)