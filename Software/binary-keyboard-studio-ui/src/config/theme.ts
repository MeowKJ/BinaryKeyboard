/**
 * BinaryKeyboard Studio 主题配置
 * 开源键盘配色风格
 */

export type ThemeMode = 'light' | 'dark';

export interface ThemeColors {
  // 背景色
  bgPrimary: string;
  bgSecondary: string;
  bgTertiary: string;
  bgHover: string;
  
  // 边框色
  border: string;
  borderLight: string;
  
  // 文字色
  textPrimary: string;
  textSecondary: string;
  textMuted: string;
  
  // 主色调
  accent: string;
  accentLight: string;
  accentSoft: string;
  accentGradient: string;
  
  // 功能色
  success: string;
  warning: string;
  danger: string;
  info: string;
  
  // 按键色
  keyBg: string;
  keyBorder: string;
  keyActiveBg: string;
  keyActiveBorder: string;
  keyShadow: string;
}

/** 深色主题 - 可爱粉紫色 */
export const DARK_THEME: ThemeColors = {
  bgPrimary: '#0d0a12',
  bgSecondary: '#16121d',
  bgTertiary: '#1e1828',
  bgHover: '#251e32',
  
  border: '#3d2f50',
  borderLight: '#2d2340',
  
  textPrimary: '#f5f0fa',
  textSecondary: '#c9bfdb',
  textMuted: '#8a7da0',
  
  accent: '#e879f9',        // 粉紫色
  accentLight: '#f0abfc',
  accentSoft: 'rgba(232, 121, 249, 0.15)',
  accentGradient: 'linear-gradient(135deg, #e879f9 0%, #a855f7 100%)',
  
  success: '#4ade80',
  warning: '#fbbf24',
  danger: '#fb7185',
  info: '#60a5fa',
  
  keyBg: '#1e1828',
  keyBorder: '#3d2f50',
  keyActiveBg: 'rgba(232, 121, 249, 0.2)',
  keyActiveBorder: '#e879f9',
  keyShadow: 'rgba(232, 121, 249, 0.25)',
};

/** 浅色主题 - 可爱天蓝色 */
export const LIGHT_THEME: ThemeColors = {
  bgPrimary: '#f8faff',
  bgSecondary: '#ffffff',
  bgTertiary: '#f0f4fa',
  bgHover: '#e8f0ff',
  
  border: '#d1dff0',
  borderLight: '#e5edf8',
  
  textPrimary: '#1a2744',
  textSecondary: '#4a5b78',
  textMuted: '#8094b0',
  
  accent: '#3b82f6',        // 天蓝色
  accentLight: '#60a5fa',
  accentSoft: 'rgba(59, 130, 246, 0.12)',
  accentGradient: 'linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%)',
  
  success: '#22c55e',
  warning: '#f59e0b',
  danger: '#ef4444',
  info: '#0ea5e9',
  
  keyBg: '#ffffff',
  keyBorder: '#d1dff0',
  keyActiveBg: 'rgba(59, 130, 246, 0.12)',
  keyActiveBorder: '#3b82f6',
  keyShadow: 'rgba(59, 130, 246, 0.2)',
};

/** 应用主题到 CSS 变量 */
export function applyTheme(mode: ThemeMode): void {
  const theme = mode === 'dark' ? DARK_THEME : LIGHT_THEME;
  const root = document.documentElement;
  
  root.style.setProperty('--c-bg-primary', theme.bgPrimary);
  root.style.setProperty('--c-bg-secondary', theme.bgSecondary);
  root.style.setProperty('--c-bg-tertiary', theme.bgTertiary);
  root.style.setProperty('--c-bg-hover', theme.bgHover);
  
  root.style.setProperty('--c-border', theme.border);
  root.style.setProperty('--c-border-light', theme.borderLight);
  
  root.style.setProperty('--c-text-primary', theme.textPrimary);
  root.style.setProperty('--c-text-secondary', theme.textSecondary);
  root.style.setProperty('--c-text-muted', theme.textMuted);
  
  root.style.setProperty('--c-accent', theme.accent);
  root.style.setProperty('--c-accent-light', theme.accentLight);
  root.style.setProperty('--c-accent-soft', theme.accentSoft);
  root.style.setProperty('--c-accent-gradient', theme.accentGradient);
  
  root.style.setProperty('--c-success', theme.success);
  root.style.setProperty('--c-warning', theme.warning);
  root.style.setProperty('--c-danger', theme.danger);
  root.style.setProperty('--c-info', theme.info);
  
  root.style.setProperty('--c-key-bg', theme.keyBg);
  root.style.setProperty('--c-key-border', theme.keyBorder);
  root.style.setProperty('--c-key-active-bg', theme.keyActiveBg);
  root.style.setProperty('--c-key-active-border', theme.keyActiveBorder);
  root.style.setProperty('--c-key-shadow', theme.keyShadow);
  
  // 设置 data-theme 属性用于 CSS 选择器
  root.setAttribute('data-theme', mode);
}

/** 获取系统偏好主题 */
export function getSystemTheme(): ThemeMode {
  if (typeof window !== 'undefined' && window.matchMedia) {
    return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
  }
  return 'dark';
}

/** 从 localStorage 获取保存的主题 */
export function getSavedTheme(): ThemeMode | null {
  if (typeof localStorage !== 'undefined') {
    return localStorage.getItem('theme') as ThemeMode | null;
  }
  return null;
}

/** 保存主题到 localStorage */
export function saveTheme(mode: ThemeMode): void {
  if (typeof localStorage !== 'undefined') {
    localStorage.setItem('theme', mode);
  }
}
