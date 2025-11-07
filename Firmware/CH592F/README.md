<p align="center">
  <img src="https://github.com/user-attachments/assets/1fc84ca3-9192-4450-850b-dca9ef54fc58" width="300">
</p>

<p align="center">
  ✨ <b>BinaryKeyboard Wireless</b> ✨
</p>

<hr>

# 配置开发环境

## 1. 安装 MounRiver IDE II
本程序推荐使用 MounRiver IDE II 作为开发环境。

## 2. 使用 MounRiver IDE II 打开 `CH592F.wvproj`
在 MounRiver IDE II 中打开你的项目文件 `CH592F.wvproj`。

## 3. 配置按键版本 | Key Version Configuration

根据你使用的不同按键版本，修改 `config.h` 文件中的注释来启用对应的版本：

## 4. 编译代码
使用`F7`或菜单栏的**编译**选项来编译代码。

## 8. 上传代码

在`obj`文件夹中找到编译好的`CH592F.bin`文件。
首先断开板子与电脑的连接，然后按住 **BOOT** 按钮 ( 距离边缘更近的那个 ) 的同时连接到电脑从而进入 Bootloader 模式。
你可以使用`WCHISP`（ 工具- WCH编程工具）编译并上传代码到开发板。

