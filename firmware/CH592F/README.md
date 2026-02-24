# CH592F 固件

无线版（USB / 蓝牙双模）固件，基于 **CH592F** RISC-V 芯片。

## 文档

完整开发文档：[无线版开发指南](../../docs/wireless/dev.md)

## 快速开始

```bash
# 首次使用：下载 wchisp 烧录工具
python setup.py

# 构建并烧录（先将键盘切换至 Bootloader 模式）
python flash.py flash --preset release
```

> 工具链路径通过 `CMakeUserPresets.json` 指定，参见 `CMakeUserPresets.json.example`。
