# 无线版固件刷写

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
CH592F 的 Bootloader 与 CH552G 类似，但芯片类型选择不同。
:::

## Windows

1. 在仓库根目录运行：

```bash
python tools/scripts/console.py
```

2. 进入 `Home`
3. 先点 `Install or update wchisp`
4. 再点 `Probe ISP devices`，确认已经识别到设备
5. 如果你刷五键款，保持当前 `Layout = 5KEY`
6. 如果你刷旋钮款，先点 `Toggle layout` 切到 `KNOB`
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

- `Home -> Toggle layout` 用来切换 `5KEY / KNOB`
- `Home -> Toggle build type` 一般保持 `release`
- `Home -> Flash selected preset` 会先构建，再刷写
- 如果只是想检查设备在不在，可以用 `Home -> Probe ISP devices`

## 蓝牙配对测试

TODO
