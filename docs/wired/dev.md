# 经典版固件开发

基于 **CH552G** 芯片的固件开发指南。

## 开发环境

### 推荐工具链

| 工具        | 说明                 |
| :---------- | :------------------- |
| Arduino IDE | 借助 CH55xduino 编译 |

### 配置开发环境

1. 安装 Arduino IDE
2. 安装 CH55xduino 库

## 版本配置

在 `src/config.h` 中选择外形版本：

```c
// 只启用一个
#define USE_BASIC    // 基础款
// #define USE_5KEYS // 五键款
// #define USE_KNOB  // 旋钮款
```

## 编译与烧录

### 使用 Arduino IDE

1. 打开 `CH552G.ino` 文件
2. 选择板子：**CH552**
3. 在 USB Setting 中选择：**USER CODE w/148B USB ram**
4. 点击编译并上传

## 常见改动点

### 1. 默认键位

修改 `KeysDataHandler.c` 中的按键映射表。

### 2. RGB 灯效

修改 `rgb.c` 中的灯效逻辑，支持 WS2812。

### 3. USB 描述符

修改 `USBConstant.c` 中的设备描述符。

### 4. 音乐节奏游戏延迟问题

修改 `config.h` 中的 DEBOUNCE_THRESHOLD 阈值，默认值为 5，单位为毫秒。如果用于音乐节奏游戏，可将阈值减小或者设置为0。

## HID 协议

经典版使用 USB HID 协议通信：

| Report ID | 功能        | 数据长度 |
| :-------- | :---------- | :------- |
| 1         | 键盘输入    | 8 字节   |
| 2         | 控制器      | 8 字节   |
| 3         | 鼠标输入    | 5 字节   |
| 4         | 主机 → 键盘 | 31 字节  |
| 5         | 键盘 → 主机 | 31 字节  |

::: tip
Report ID 4/5 用于改键工具与键盘通信，修改键位映射。
:::

## 参考资料

- [CH552 数据手册](https://www.wch.cn/products/CH552.html)
- [SDCC 文档](http://sdcc.sourceforge.net/)
- [CH55xduino](https://github.com/DeqingSun/ch55xduino)
