<p align="center">
  <img src="https://github.com/user-attachments/assets/1fc84ca3-9192-4450-850b-dca9ef54fc58" width="300">
  
</p>

<p align="center">
  ✨ <b>BinaryKeyboard</b> ✨
</p>

<hr>

# 配置开发环境

## 1. 安装 Arduino IDE
本程序推荐使用 Arduino IDE 作为开发环境。

## 2. 使用 Arduino IDE 打开 `main.ino`
在 Arduino IDE 中打开你的项目文件 `main.ino`。

## 3. 添加 CH55x 的板卡管理器 URL

1. 点击菜单栏上的 **文件** -> **首选项** -> **其他开发板管理器地址**。
2. 在弹出的窗口中，添加以下链接：
https://cos.thinkcreate.us/package_ch55xduino_mcs51_newest_cloudflare_index.json

markdown
复制
编辑
3. 然后点击 **确定**。

## 4. 安装 CH55xDuino 包

1. 在菜单栏点击 **工具** -> **开发板** -> **开发板管理器**。
2. 在搜索框中输入 `ch`，查找 `Ch55xDuino` 包。
3. 找到 `Ch55xDuino` 后，点击 **安装** 按钮。

## 5. 选择开发板

1. 点击 **工具** -> **开发板**，选择 **CH552Board**，然后点击 **确定**。

## 6. 设置 USB 配置

1. 点击 **工具** -> **USB Setting**，选择 **USER CODE w/148B USB ram**。

## 7. 上传代码

初次使用时，可能需要断开板子与电脑的连接，然后按住 **BOOT** 按钮进入 Bootloader 模式。你可以编译并上传代码到开发板。
