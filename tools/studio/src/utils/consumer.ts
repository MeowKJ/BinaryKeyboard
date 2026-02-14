/**
 * Consumer Control (多媒体键) 定义
 */

/** 多媒体键码定义 */
export const CONSUMER_KEYS: { code: number; name: string; icon?: string }[] = [
  // 播放控制
  { code: 0xcd, name: '播放/暂停', icon: 'pi-play' },
  { code: 0xb5, name: '下一曲', icon: 'pi-step-forward' },
  { code: 0xb6, name: '上一曲', icon: 'pi-step-backward' },
  { code: 0xb7, name: '停止', icon: 'pi-stop' },
  { code: 0xb3, name: '快进', icon: 'pi-forward' },
  { code: 0xb4, name: '快退', icon: 'pi-backward' },

  // 音量控制
  { code: 0xe2, name: '静音', icon: 'pi-volume-off' },
  { code: 0xe9, name: '音量+', icon: 'pi-volume-up' },
  { code: 0xea, name: '音量-', icon: 'pi-volume-down' },

  // 应用程序
  { code: 0x192, name: '计算器', icon: 'pi-calculator' },
  { code: 0x194, name: '文件管理器', icon: 'pi-folder' },
  { code: 0x18a, name: '邮件', icon: 'pi-envelope' },

  // 浏览器
  { code: 0x223, name: '浏览器主页', icon: 'pi-home' },
  { code: 0x224, name: '浏览器后退', icon: 'pi-arrow-left' },
  { code: 0x225, name: '浏览器前进', icon: 'pi-arrow-right' },
  { code: 0x227, name: '浏览器刷新', icon: 'pi-refresh' },
  { code: 0x22a, name: '浏览器收藏', icon: 'pi-star' },

  // 系统控制
  { code: 0x30, name: '电源', icon: 'pi-power-off' },
  { code: 0x32, name: '休眠', icon: 'pi-moon' },

  // 屏幕亮度
  { code: 0x6f, name: '亮度+', icon: 'pi-sun' },
  { code: 0x70, name: '亮度-', icon: 'pi-sun' },
];

/** 获取多媒体键名称 */
export function getConsumerName(code: number): string {
  const key = CONSUMER_KEYS.find(k => k.code === code);
  return key?.name || '';
}

/** 获取多媒体键图标 */
export function getConsumerIcon(code: number): string {
  const key = CONSUMER_KEYS.find(k => k.code === code);
  return key?.icon || 'pi-circle';
}
