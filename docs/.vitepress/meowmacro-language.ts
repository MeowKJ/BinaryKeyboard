export const meowMacroLanguage = {
  name: 'meowmacro',
  displayName: 'MeowMacro',
  scopeName: 'source.meowmacro',
  aliases: ['meowmacro', 'meow'],
  patterns: [
    { include: '#comment' },
    { include: '#keyword' },
    { include: '#duration' },
    { include: '#operator' },
    { include: '#mouse' },
    { include: '#consumer' },
    { include: '#wheel' },
    { include: '#modifier' },
    { include: '#key' },
  ],
  repository: {
    comment: {
      patterns: [
        {
          name: 'comment.line.number-sign.meowmacro',
          match: '#.*$',
        },
      ],
    },
    keyword: {
      patterns: [
        {
          name: 'keyword.control.meowmacro',
          match: '\\b(?:tap|down|up|delay|wheel|consumer|hold|wait|mouse)\\b',
        },
      ],
    },
    duration: {
      patterns: [
        {
          name: 'constant.numeric.meowmacro',
          match: '\\b\\d+(?:\\.\\d+)?(?:ms|s)?\\b',
        },
      ],
    },
    operator: {
      patterns: [
        {
          name: 'keyword.operator.meowmacro',
          match: '\\+|\\*',
        },
      ],
    },
    mouse: {
      patterns: [
        {
          name: 'support.type.mouse.meowmacro',
          match: '\\bmouse:(?:left|right|middle|back|forward)\\b',
        },
      ],
    },
    consumer: {
      patterns: [
        {
          name: 'support.function.consumer.meowmacro',
          match:
            '\\b(?:play_pause|next_track|prev_track|previous_track|stop|fast_forward|rewind|mute|volume_up|volume_down|calculator|explorer|file_explorer|mail|browser_home|browser_back|browser_forward|browser_refresh|browser_favorites|power|sleep|brightness_up|brightness_down)\\b',
        },
      ],
    },
    wheel: {
      patterns: [
        {
          name: 'support.constant.wheel.meowmacro',
          match: '\\b(?:up|down)\\b',
        },
      ],
    },
    modifier: {
      patterns: [
        {
          name: 'storage.modifier.meowmacro',
          match:
            '\\b(?:Ctrl|Control|LCtrl|Shift|LShift|Alt|LAlt|Win|Gui|Meta|Cmd|Super|LWin|LGui|RCtrl|RControl|RShift|RAlt|RWin|RGui|RMeta)\\b',
        },
      ],
    },
    key: {
      patterns: [
        {
          name: 'constant.language.key.meowmacro',
          match:
            '\\b(?:Escape|Esc|Enter|Return|Space|Spacebar|Backspace|Tab|Delete|Del|Insert|Home|End|PageUp|Pagedown|PageDown|Left|Right|Up|Down|PrintScreen|PrtSc|Menu|Apps|[A-Z]|[0-9]|F(?:[1-9]|1[0-2]))\\b',
        },
      ],
    },
  },
} as const
