# 经典版固件开发

基于 **CH552G** 芯片的固件开发指南。

## 开发环境

### 推荐工具链

| 工具  | 说明                    |
| :---- | :---------------------- |
| CMake | 项目生成与构建入口      |
| SDCC  | CH552G 8051 交叉编译器  |

### 配置开发环境

1. 安装 CMake
2. 安装 SDCC
3. 可选：下载通用烧录工具 `python tools/scripts/setup.py`
4. 可选：启动 TUI 控制台 `python tools/scripts/console.py`

## 版本配置

通过 CMake 变量选择外形版本：

```powershell
python tools/scripts/ch552g.py configure --variant BASIC
```

## 编译与烧录

### 使用 CMake + SDCC

1. 配置工程：

```powershell
python tools/scripts/ch552g.py configure --variant BASIC
```

2. 编译固件：

```powershell
python tools/scripts/ch552g.py build --variant BASIC
```

3. 在 `firmware/CH552G/build/basic` 下取得 `CH552G.hex` 或 `CH552G.bin`
4. 按住 **BOOT** 进入 Bootloader，执行：

```powershell
python tools/scripts/flash.py flash --file firmware/CH552G/build/basic/CH552G.bin
```

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
