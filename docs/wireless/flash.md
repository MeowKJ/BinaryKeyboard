# 无线版固件刷写

## 固件类型

- **首次烧录 / 救砖恢复**：刷 `CH592F-<MODEL>-<version>-full.hex`
- **Studio 在线更新**：用 `CH592F-<MODEL>-<version>.bin`
- OTA `bin` 不用于第一次 ISP 首刷

## 准备

- 安装 `Python 3`
- 打开这个仓库根目录
- 用 `PY` 终端控制台刷写：

```bash
python tools/scripts/console.py
```

## 进入 Bootloader 模式

1. 拔掉 USB-C
2. **长按 BOOT 按钮**
3. 按住不放的同时插入 USB-C
4. 松开按钮

::: tip
这里进入的是芯片 ROM 自带下载模式，不是应用层更新流程。
:::

## Windows

1. 在仓库根目录运行：

```bash
python tools/scripts/console.py
```

2. 进入 `Home`
3. 先点 `Install or update wchisp`
4. 再点 `Probe ISP devices`，确认已经识别到设备
5. 五键款保持当前 `Layout = 5KEY`
6. 旋钮款先点 `Toggle layout` 切到 `KNOB`
7. 点 `Flash selected preset`
8. 等待刷写完成
9. 拔插 USB，测试蓝牙配对

## macOS / Linux

1. 在仓库根目录运行：

```bash
python tools/scripts/console.py
```

2. 进入 `Home`
3. 先点 `Install or update wchisp`
4. 再点 `Probe ISP devices`
5. 五键款保持 `Layout = 5KEY`
6. 旋钮款先点 `Toggle layout` 切到 `KNOB`
7. 点 `Flash selected preset`
8. 等待刷写完成

## 说明

- `Flash selected preset` 当前刷的是 **`build-full` 产物**，也就是 `-full.hex`
- `-full.hex` 里面已经包含 `JumpIAP + app + 高地址 IAP`
- `普通 .bin` 是给 Studio 热更新走的，不是给 ISP 首刷的
- `Home -> Toggle layout` 用来切换 `5KEY / KNOB`
- `Home -> Toggle build type` 一般保持 `release`
- `Home -> Flash selected preset` 会先构建，再刷写
- 如果只是想检查设备在不在，可以用 `Home -> Probe ISP devices`

## Studio 热更新

设备进入系统后，后续更新通过 Studio 完成：

1. 用 USB 连接键盘
2. 打开 BinaryKeyboard Studio
3. 在设备信息卡片里点固件更新
4. Studio 会下载当前协议对应的最新 `bin`
5. 固件先写到 `Image B`，重启后由高地址 IAP 完成搬运

::: tip
如果 Release 已经更新、但 Studio 仍提示下载文件不存在，通常表示 GitHub Pages 上的固件镜像仍在同步。
:::

## 蓝牙配对测试

TODO
