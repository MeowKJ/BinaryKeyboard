import { defineConfig } from 'vitepress'

export default defineConfig({
  lang: 'zh-CN',
  title: 'BinaryKeyboard',
  description: '可爱二进制键盘 - 开源迷你宏键盘项目',
  base: '/BinaryKeyboard/',
  
  head: [
    ['link', { rel: 'preconnect', href: 'https://fonts.googleapis.com' }],
    ['link', { rel: 'preconnect', href: 'https://fonts.gstatic.com', crossorigin: '' }],
    ['link', { href: 'https://fonts.googleapis.com/css2?family=Quicksand:wght@400;500;600;700&display=swap', rel: 'stylesheet' }],
  ],

  themeConfig: {
    logo: '/logo.svg',
    siteTitle: 'BinaryKeyboard 🐱',
    
    nav: [
      { text: '🏠 首页', link: '/' },
      { 
        text: '⚡ 经典版',
        link: '/wired/',
        activeMatch: '/wired/'
      },
      { 
        text: '📡 无线版',
        link: '/wireless/',
        activeMatch: '/wireless/'
      },
      { text: '🔧 开发', items: [
        { text: '⚡ 经典版开发', link: '/wired/dev' },
        { text: '📡 无线版开发', link: '/wireless/dev' },
      ]},
      { text: '❓ FAQ', link: '/faq' },
      { text: '🐱 MeowMacro', link: '/meowmacro' },
    ],

    sidebar: [
      {
        text: '⚡ 经典版',
        collapsed: false,
        items: [
          { text: '快速开始', link: '/wired/' },
          { text: '硬件复刻', link: '/wired/make' },
          { text: '刷写固件', link: '/wired/flash' },
          { text: '改键软件', link: '/wired/remap' },
          { text: '固件开发', link: '/wired/dev' },
        ]
      },
      {
        text: '📡 无线版',
        collapsed: false,
        items: [
          { text: '快速开始', link: '/wireless/' },
          { text: '物料清单 (BOM)', link: '/wireless/bom' },
          { text: '硬件复刻', link: '/wireless/make' },
          { text: '刷写固件', link: '/wireless/flash' },
          { text: '改键软件', link: '/wireless/remap' },
          { text: '固件开发', link: '/wireless/dev' },
          { text: '便捷开发工具', link: '/wireless/dev-tools' },
          { text: 'HID 通讯协议', link: '/wireless/hid' },
          { text: '低功耗蓝牙', link: '/wireless/ble' },
          { text: 'TMOS 调度', link: '/wireless/tmos' },
          { text: 'DataFlash 布局', link: '/wireless/dataflash' },
          { text: 'RGB 灯效架构', link: '/wireless/rgb-architecture' },
        ]
      },
      {
        text: '📚 其他',
        collapsed: true,
        items: [
          { text: '💾 MeowFS 宏存储', link: '/meowfs' },
          { text: 'MeowMacro 宏语言', link: '/meowmacro' },
          { text: '常见问题', link: '/faq' },
          { text: '更新日志', link: '/changelog' },
          { text: '许可与致谢', link: '/license' },
        ]
      }
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/MeowKJ/BinaryKeyboard' }
    ],

    repo: 'https://github.com/MeowKJ/BinaryKeyboard',
    docsDir: 'docs',
    docsBranch: 'main',
    
    editLink: {
      pattern: 'https://github.com/MeowKJ/BinaryKeyboard/edit/main/docs/:path',
      text: '在 GitHub 上编辑此页'
    },

    footer: {
      message: '基于 GPL-3.0 许可发布',
      copyright: 'Copyright © 2025 <a href="https://oshwhub.com/kjpig/" target="_blank">喵喵的帕斯</a>'
    },

    search: {
      provider: 'local',
      options: {
        translations: {
          button: { buttonText: '搜索文档', buttonAriaLabel: '搜索' },
          modal: {
            noResultsText: '没有找到结果',
            resetButtonTitle: '清除查询',
            footer: { selectText: '选择', navigateText: '导航', closeText: '关闭' }
          }
        }
      }
    },

    outline: {
      label: '页面导航',
      level: [2, 3]
    },

    docFooter: {
      prev: '上一页',
      next: '下一页'
    },

    lastUpdated: {
      text: '最后更新于'
    },

    darkModeSwitchLabel: '主题',
    lightModeSwitchTitle: '切换到浅色模式',
    darkModeSwitchTitle: '切换到深色模式',
    sidebarMenuLabel: '菜单',
    returnToTopLabel: '回到顶部',
  }
})
