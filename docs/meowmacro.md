# MeowMacro 宏语言

MeowMacro 是 BinaryKeyboard Studio 内置的宏编辑语言，用于快速编写键盘宏。你可以在 Studio 的宏编辑器中切换到 **MeowMacro** 模式直接编写。

::: tip 切换方式
在宏编辑器右上角点击 **MeowMacro** 按钮即可切换到代码模式，支持语法高亮、自动补全和实时错误提示。
:::

## 快速上手

```meow
# 复制粘贴
tap Ctrl+C
delay 50ms
tap Ctrl+V

# 连续输入 Hello
tap H e l l o
```

每一行是一条指令。`#` 后面的内容会被当作注释忽略。

## 指令参考

### tap — 按键点击

按下并释放按键。支持组合键、顺序按键和运算符。

```meow
tap A                          # 单键
tap Ctrl+C                     # 组合键
tap Ctrl+C Ctrl+V              # 顺序执行多个组合
tap H e l l o                  # 快速顺序按键
tap mouse:left                 # 鼠标左键点击
```

**运算符：**

| 运算符 | 格式 | 说明 |
|:-------|:-----|:-----|
| `*` | `* N` | 重复前面的按键组 N 次，可多次使用 |
| `hold` | `hold 50ms` | 按住指定时间后释放 |
| `wait` | `wait 100ms` | 每轮释放后等待 |

```meow
tap A * 3                      # 按 A 三次
tap A hold 50ms                # 按住 A 50ms
tap A * 2 wait 50ms            # 双击 A，间隔 50ms
tap Enter hold 50ms wait 100ms # 综合使用
tap A * 2 B * 3                # A A B B B（每组独立重复）
tap Ctrl+C * 2 Ctrl+V          # Ctrl+C 两次，再 Ctrl+V
```

### down / up — 按下 / 释放

手动控制按键的按下和释放，适用于拖拽等场景。

```meow
down Shift              # 按住 Shift
tap A B C               # 输入大写 A B C
up Shift                # 释放 Shift

down mouse:left         # 按住鼠标左键
delay 500ms
up mouse:left           # 释放鼠标左键
```

::: warning
`down` 按下的键必须手动 `up` 释放，否则编辑器会发出警告。
:::

### delay — 延时

在动作之间插入等待时间。

```meow
delay 50ms              # 等待 50 毫秒
delay 1s                # 等待 1 秒
delay 0.5s              # 等待 500 毫秒
delay 200               # 纯数字默认毫秒
```

### mouse — 鼠标按键

鼠标按键通过 `mouse:` 前缀统一到 `tap`/`down`/`up` 指令中：

```meow
tap mouse:left          # 点击左键（自动按下+释放）
tap mouse:right         # 右键点击
tap mouse:middle        # 中键点击
down mouse:left         # 按下左键（不释放）
up mouse:left           # 释放左键
```

**支持的按键：**`mouse:left` `mouse:right` `mouse:middle` `mouse:back` `mouse:forward`

::: tip 兼容语法
`mouse left` / `mouse down left` / `mouse up left` 仍然可用，但推荐使用统一的 `tap`/`down`/`up` + `mouse:` 语法。
:::

### wheel — 滚轮

```meow
wheel up                # 滚轮向上
wheel down              # 滚轮向下
```

### consumer — 媒体键

```meow
consumer play_pause     # 播放/暂停
consumer volume_up      # 音量增大
consumer next_track     # 下一首
consumer calculator     # 打开计算器
```

<details>
<summary>全部媒体键名称</summary>

| 名称 | 说明 |
|:-----|:-----|
| `play_pause` | 播放/暂停 |
| `next_track` | 下一曲 |
| `prev_track` | 上一曲 |
| `stop` | 停止 |
| `fast_forward` | 快进 |
| `rewind` | 快退 |
| `mute` | 静音 |
| `volume_up` | 音量增 |
| `volume_down` | 音量减 |
| `calculator` | 计算器 |
| `explorer` | 文件管理器 |
| `mail` | 邮件 |
| `browser_home` | 浏览器主页 |
| `browser_back` | 浏览器后退 |
| `browser_forward` | 浏览器前进 |
| `browser_refresh` | 浏览器刷新 |
| `browser_favorites` | 浏览器收藏 |
| `power` | 电源 |
| `sleep` | 睡眠 |
| `brightness_up` | 亮度增 |
| `brightness_down` | 亮度减 |

</details>

## 组合键语法

用 `+` 连接修饰键和普通键，表示同时按下：

```meow
tap Ctrl+C              # Ctrl + C
tap Ctrl+Shift+T        # Ctrl + Shift + T
tap Alt+F4              # Alt + F4
tap Win+D               # Win + D
```

**修饰键名称：**

| 修饰键 | 别名 |
|:-------|:-----|
| `Ctrl` | `control`, `lctrl` |
| `Shift` | `lshift` |
| `Alt` | `lalt` |
| `Win` | `gui`, `meta`, `cmd`, `super` |
| `RCtrl` | `rctrl`, `rcontrol` |
| `RShift` | `rshift` |
| `RAlt` | `ralt` |
| `RWin` | `rgui`, `rmeta` |

所有名称不区分大小写。

## 特殊键名

| 按键名 | 别名 |
|:-------|:-----|
| `Escape` | `esc` |
| `Enter` | `return` |
| `Space` | `spacebar` |
| `Backspace` | — |
| `Tab` | — |
| `Delete` | `del` |
| `Insert` | — |
| `Home` / `End` | — |
| `PageUp` / `PageDown` | `pageup`, `pagedown` |
| `Left` / `Right` / `Up` / `Down` | 方向键 |
| `PrintScreen` | `prtsc` |
| `Menu` | `apps` |

字母 `A`-`Z`、数字 `0`-`9`、`F1`-`F12` 等标准键名均可直接使用。

## 实用示例

### Ctrl+C → Ctrl+V 复制粘贴

```meow
tap Ctrl+C
delay 50ms
tap Ctrl+V
```

### 输入大写字母

```meow
down Shift
tap H e l l o
up Shift
```

### 游戏连点

```meow
tap Space * 10 wait 50ms     # 快速连按空格 10 次
```

### 截图并粘贴

```meow
tap Win+Shift+S             # 截图
delay 1s                    # 等待截图完成
tap Ctrl+V                  # 粘贴
```

### 鼠标拖拽

```meow
down mouse:left
delay 500ms
up mouse:left
```

### 浏览器刷新

```meow
consumer browser_refresh
```

## 硬件限制

| 参数 | 无线版 (CH592F) | 经典版 (CH552G) |
|:-----|:---------------|:---------------|
| 宏槽位数 | 8 | 2 |
| 最大动作数 | 1000 | 31 |
| 最大数据 | 2024 字节 | 62 字节 |
| 同时按键数 | 6 键 | 6 键 |

::: info 经典版提示
CH552G 每槽最多 31 个动作（约 15 次按键），适合编写简短的快捷宏。
复杂的宏序列建议使用无线版。
:::
