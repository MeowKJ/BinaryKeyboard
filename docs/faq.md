# 常见问题（FAQ）

## 1) 插上 USB 没反应 / 电脑不识别

- 尝试进入 Bootloader：拔线 → 按住 BOOT → 再插线
- 检查 USB 焊点与供电是否正常

## 2) 刷写成功但按键不对

- 你可能刷错了版本固件（基础/五键/旋钮不通用）
- 重新下载对应版本 `.hex` 再烧录

## 3) 改键网页连不上设备

- 尽量用 Chrome / Edge
- 检查是否被浏览器权限拦截（HID/USB 权限）

## 4) 侧边栏不显示（Docsify 站点问题）

- GitHub Pages 默认 Jekyll 可能会忽略 `_sidebar.md` 这类以下划线开头的文件
- 确保在 `docs/` 里放置了 `.nojekyll`
