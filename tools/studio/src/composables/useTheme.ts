/**
 * 主题系统 composable
 * 支持多套主题 + HSL 色相调节 + 版本色相自动同步
 */
import { ref, computed } from 'vue';
import { applyTheme, getSavedTheme, saveTheme, getSystemTheme, type ThemeMode } from '@/config/theme';

export type ThemeId = 'default' | 'liuli' | 'neko' | 'frog' | 'angora' | 'storm';

export interface ThemeConfig {
  id: ThemeId;
  mode: ThemeMode;
  accentHue: number;       // 0-360
  accentSaturation: number; // 0-100
  syncVersionHue: boolean;  // 自动同步版本配色
}

const STORAGE_KEY = 'bk-theme-config';

function loadConfig(): ThemeConfig {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) {
      const parsed = JSON.parse(raw);
      // 兼容旧配置（没有 syncVersionHue 字段）
      if (parsed.syncVersionHue === undefined) parsed.syncVersionHue = true;
      return parsed;
    }
  } catch { /* ignore */ }
  return {
    id: 'default',
    mode: getSavedTheme() || getSystemTheme(),
    accentHue: 220,
    accentSaturation: 80,
    syncVersionHue: true, // 默认开启
  };
}

function persistConfig(config: ThemeConfig) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(config));
  saveTheme(config.mode);
}

/** 从 HSL 生成主题色系 CSS 变量 — 影响全局颜色 */
function applyAccentHsl(hue: number, sat: number, mode: ThemeMode) {
  const root = document.documentElement;
  const h = hue;
  const s = sat;

  root.style.setProperty('--c-accent', `hsl(${h}, ${s}%, 68%)`);
  root.style.setProperty('--c-accent-light', `hsl(${h}, ${s}%, 78%)`);
  root.style.setProperty('--c-accent-soft', `hsla(${h}, ${s}%, 68%, 0.15)`);
  root.style.setProperty('--c-accent-gradient', `linear-gradient(135deg, hsl(${h}, ${s}%, 78%) 0%, hsl(${h}, ${s}%, 68%) 100%)`);

  root.style.setProperty('--c-key-active-bg', `hsla(${h}, ${s}%, 68%, 0.2)`);
  root.style.setProperty('--c-key-active-border', `hsl(${h}, ${s}%, 68%)`);
  root.style.setProperty('--c-key-shadow', `hsla(${h}, ${s}%, 68%, 0.25)`);

  if (mode === 'dark') {
    root.style.setProperty('--c-border', `hsl(${h}, ${Math.max(s - 40, 15)}%, 22%)`);
    root.style.setProperty('--c-border-light', `hsl(${h}, ${Math.max(s - 45, 10)}%, 18%)`);
    root.style.setProperty('--c-bg-primary', `hsl(${h}, ${Math.max(s - 55, 10)}%, 5%)`);
    root.style.setProperty('--c-bg-secondary', `hsl(${h}, ${Math.max(s - 50, 10)}%, 8%)`);
    root.style.setProperty('--c-bg-tertiary', `hsl(${h}, ${Math.max(s - 45, 10)}%, 12%)`);
    root.style.setProperty('--c-bg-hover', `hsl(${h}, ${Math.max(s - 40, 10)}%, 15%)`);
    root.style.setProperty('--c-key-bg', `hsl(${h}, ${Math.max(s - 45, 10)}%, 12%)`);
    root.style.setProperty('--c-key-border', `hsl(${h}, ${Math.max(s - 40, 15)}%, 22%)`);
  } else {
    root.style.setProperty('--c-border', `hsl(${h}, ${Math.max(s - 30, 15)}%, 82%)`);
    root.style.setProperty('--c-border-light', `hsl(${h}, ${Math.max(s - 35, 10)}%, 88%)`);
    root.style.setProperty('--c-bg-primary', `hsl(${h}, ${Math.max(s - 40, 10)}%, 97%)`);
    root.style.setProperty('--c-bg-secondary', `hsl(${h}, 0%, 100%)`);
    root.style.setProperty('--c-bg-tertiary', `hsl(${h}, ${Math.max(s - 35, 10)}%, 95%)`);
    root.style.setProperty('--c-bg-hover', `hsl(${h}, ${Math.max(s - 30, 10)}%, 93%)`);
    root.style.setProperty('--c-key-bg', `hsl(${h}, 0%, 100%)`);
    root.style.setProperty('--c-key-border', `hsl(${h}, ${Math.max(s - 30, 15)}%, 82%)`);
  }
}

/**
 * 版本号 → 色相值（确定性映射）
 * DJB2 哈希 → 0-360
 */
export function versionToHue(version: string): number {
  // DJB2 hash → golden-ratio scatter for maximal hue separation between close versions
  let hash = 5381;
  for (let i = 0; i < version.length; i++) {
    hash = ((hash << 5) + hash + version.charCodeAt(i)) & 0xFFFFFFFF;
  }
  // Golden ratio multiplication spreads adjacent hash values far apart in [0,360)
  const golden = (hash * 2654435761) >>> 0;
  return (golden % 360);
}

// 单例状态
const config = ref<ThemeConfig>(loadConfig());
const configuratorOpen = ref(false);

export function useTheme() {

  const themeId = computed(() => config.value.id);
  const themeMode = computed(() => config.value.mode);
  const accentHue = computed(() => config.value.accentHue);
  const accentSaturation = computed(() => config.value.accentSaturation);
  const syncVersionHue = computed(() => config.value.syncVersionHue);

  function applyFull() {
    const root = document.documentElement;
    root.setAttribute('data-theme-id', config.value.id);

    if (config.value.id === 'liuli' || config.value.id === 'frog' || config.value.id === 'angora' || config.value.id === 'storm') {
      root.setAttribute('data-theme', config.value.id === 'angora' ? 'light' : 'dark');
      const hslProps = ['--c-accent', '--c-accent-light', '--c-accent-soft', '--c-accent-gradient',
        '--c-key-active-bg', '--c-key-active-border', '--c-key-shadow',
        '--c-border', '--c-border-light', '--c-bg-primary', '--c-bg-secondary', '--c-bg-tertiary',
        '--c-bg-hover', '--c-key-bg', '--c-key-border'];
      for (const p of hslProps) root.style.removeProperty(p);
    } else {
      applyTheme(config.value.mode);
      applyAccentHsl(config.value.accentHue, config.value.accentSaturation, config.value.mode);
    }
  }

  function init() {
    config.value = loadConfig();
    applyFull();
  }

  function setThemeId(id: ThemeId) {
    config.value = { ...config.value, id };
    persistConfig(config.value);
    applyFull();
  }

  function toggleMode() {
    if (config.value.id === 'liuli' || config.value.id === 'angora') return;
    const newMode: ThemeMode = config.value.mode === 'dark' ? 'light' : 'dark';
    config.value = { ...config.value, mode: newMode };
    persistConfig(config.value);
    applyFull();
  }

  function setAccentHue(hue: number) {
    config.value = { ...config.value, accentHue: hue };
    persistConfig(config.value);
    applyAccentHsl(hue, config.value.accentSaturation, config.value.mode);
  }

  function setAccentSaturation(sat: number) {
    config.value = { ...config.value, accentSaturation: sat };
    persistConfig(config.value);
    applyAccentHsl(config.value.accentHue, sat, config.value.mode);
  }

  function setSyncVersionHue(sync: boolean) {
    config.value = { ...config.value, syncVersionHue: sync };
    persistConfig(config.value);
  }

  /** 当设备连接时调用，如果开启同步则自动应用版本色相 */
  function syncFromVersion(fwVersion: string) {
    if (!config.value.syncVersionHue) return;
    if (config.value.id !== 'default' && config.value.id !== 'neko') return;
    const hue = versionToHue(fwVersion);
    config.value = { ...config.value, accentHue: hue };
    persistConfig(config.value);
    applyAccentHsl(hue, config.value.accentSaturation, config.value.mode);
  }

  function openConfigurator() { configuratorOpen.value = true; }
  function closeConfigurator() { configuratorOpen.value = false; }

  function resetToDefault() {
    localStorage.removeItem(STORAGE_KEY);
    config.value = {
      id: 'default',
      mode: getSystemTheme(),
      accentHue: 220,
      accentSaturation: 80,
      syncVersionHue: true,
    };
    const root = document.documentElement;
    root.style.cssText = '';
    persistConfig(config.value);
    applyFull();
    closeConfigurator();
  }

  return {
    config,
    themeId,
    themeMode,
    accentHue,
    accentSaturation,
    syncVersionHue,
    configuratorOpen,
    init,
    setThemeId,
    toggleMode,
    setAccentHue,
    setAccentSaturation,
    setSyncVersionHue,
    syncFromVersion,
    openConfigurator,
    closeConfigurator,
    resetToDefault,
  };
}
