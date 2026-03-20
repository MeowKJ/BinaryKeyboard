<script setup lang="ts">
import { useTheme, type ThemeId } from '@/composables/useTheme';

const {
  themeId,
  themeMode,
  accentHue,
  accentSaturation,
  syncVersionHue,
  configuratorOpen,
  setThemeId,
  toggleMode,
  setAccentHue,
  setAccentSaturation,
  setSyncVersionHue,
  closeConfigurator,
  resetToDefault,
} = useTheme();

const themes: { id: ThemeId; label: string; emoji: string }[] = [
  { id: 'default', label: '默认', emoji: '🖥️' },
  { id: 'liuli', label: '琉璃', emoji: '🔮' },
  { id: 'neko', label: '猫咪', emoji: '🐱' },
  { id: 'frog', label: '青蛙', emoji: '🐸' },
  { id: 'angora', label: '兔子', emoji: '🐰' },
];

const canCustomizeHue = () => (themeId.value === 'default' || themeId.value === 'neko');

function onHueInput(e: Event) {
  setAccentHue(Number((e.target as HTMLInputElement).value));
}

function onSatInput(e: Event) {
  setAccentSaturation(Number((e.target as HTMLInputElement).value));
}

function onSyncToggle() {
  setSyncVersionHue(!syncVersionHue.value);
}
</script>

<template>
  <transition name="configurator">
    <div v-if="configuratorOpen" class="theme-configurator" @click.stop>
      <div class="configurator-header">
        <span class="configurator-title">
          <i class="pi pi-palette"></i>
          主题设置
        </span>
        <button class="configurator-close" @click="closeConfigurator">
          <i class="pi pi-times"></i>
        </button>
      </div>

      <!-- 主题选择 -->
      <div class="theme-cards">
        <button
          v-for="t in themes"
          :key="t.id"
          class="theme-card"
          :class="{ active: themeId === t.id }"
          @click="setThemeId(t.id)"
        >
          <span class="theme-card-emoji">{{ t.emoji }}</span>
          <span class="theme-card-label">{{ t.label }}</span>
        </button>
      </div>

      <!-- 明暗切换 -->
      <div v-if="canCustomizeHue()" class="config-section">
        <div class="config-row">
          <span class="config-label">明暗模式</span>
          <button class="mode-toggle" @click="toggleMode">
            <i :class="themeMode === 'dark' ? 'pi pi-moon' : 'pi pi-sun'"></i>
            {{ themeMode === 'dark' ? '深色' : '浅色' }}
          </button>
        </div>
      </div>

      <!-- 版本配色同步 -->
      <div v-if="canCustomizeHue()" class="config-section">
        <div class="config-row">
          <span class="config-label">同步版本配色</span>
          <button class="mode-toggle" :class="{ active: syncVersionHue }" @click="onSyncToggle">
            <i :class="syncVersionHue ? 'pi pi-lock' : 'pi pi-lock-open'"></i>
            {{ syncVersionHue ? '自动' : '手动' }}
          </button>
        </div>
      </div>

      <!-- 色相调节（同步关闭时才可用） -->
      <div v-if="canCustomizeHue() && !syncVersionHue" class="config-section">
        <div class="config-row">
          <span class="config-label">主题色相</span>
          <span class="config-value" :style="{ color: `hsl(${accentHue}, ${accentSaturation}%, 68%)` }">●</span>
        </div>
        <input
          type="range"
          min="0"
          max="360"
          :value="accentHue"
          class="hue-slider"
          @input="onHueInput"
        />
      </div>

      <div v-if="canCustomizeHue() && !syncVersionHue" class="config-section">
        <div class="config-row">
          <span class="config-label">饱和度</span>
          <span class="config-value">{{ accentSaturation }}%</span>
        </div>
        <input
          type="range"
          min="20"
          max="100"
          :value="accentSaturation"
          class="sat-slider"
          @input="onSatInput"
        />
      </div>

      <!-- 回到默认 -->
      <div class="config-section reset-section">
        <button class="reset-btn" @click="resetToDefault">
          <i class="pi pi-refresh"></i>
          回到默认
        </button>
      </div>
    </div>
  </transition>
</template>

<style scoped>
.theme-configurator {
  position: fixed;
  bottom: 126px;
  right: 24px;
  width: 280px;
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  box-shadow: 0 12px 40px rgba(0, 0, 0, 0.3);
  z-index: 200;
  overflow: hidden;
}

.configurator-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.75rem 1rem;
  border-bottom: 1px solid var(--c-border);
}

.configurator-title {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  font-size: 0.85rem;
  font-weight: 700;
  color: var(--c-text-primary);
}

.configurator-title i {
  color: var(--c-accent);
}

.configurator-close {
  width: 28px;
  height: 28px;
  border: none;
  background: none;
  color: var(--c-text-muted);
  cursor: pointer;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all var(--transition-fast);
}

.configurator-close:hover {
  background: var(--c-bg-hover);
  color: var(--c-text-primary);
}

.theme-cards {
  display: flex;
  flex-wrap: wrap;
  gap: 0.35rem;
  padding: 0.5rem;
}

.theme-card {
  display: flex;
  align-items: center;
  gap: 0.4rem;
  padding: 0.4rem 0.6rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-secondary);
  cursor: pointer;
  transition: all var(--transition-fast);
  font-family: inherit;
  font-size: 0.75rem;
  font-weight: 600;
}

.theme-card:hover {
  border-color: var(--c-accent);
}

.theme-card.active {
  border-color: var(--c-accent);
  background: var(--c-accent-soft);
  color: var(--c-accent);
}

.theme-card-emoji {
  font-size: 1rem;
}

.theme-card-label {
  font-size: 0.78rem;
  font-weight: 700;
}

.config-section {
  padding: 0.5rem 0.75rem;
  border-top: 1px solid var(--c-border-light);
}

.config-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 0.35rem;
}

.config-label {
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--c-text-muted);
}

.config-value {
  font-size: 0.85rem;
  font-weight: 700;
}

.mode-toggle {
  display: flex;
  align-items: center;
  gap: 0.35rem;
  padding: 0.25rem 0.65rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-secondary);
  font-family: inherit;
  font-size: 0.75rem;
  font-weight: 600;
  cursor: pointer;
  transition: all var(--transition-fast);
}

.mode-toggle:hover {
  border-color: var(--c-accent);
}

.mode-toggle.active {
  border-color: var(--c-accent);
  color: var(--c-accent);
  background: var(--c-accent-soft);
}

/* 版本配色按钮 */
.version-hue-btn {
  display: flex;
  align-items: center;
  gap: 0.35rem;
  margin-top: 0.4rem;
  padding: 0.3rem 0.6rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-muted);
  font-family: inherit;
  font-size: 0.7rem;
  font-weight: 600;
  cursor: pointer;
  transition: all var(--transition-fast);
}

.version-hue-btn:hover {
  border-color: var(--c-accent);
  color: var(--c-accent);
}

.version-hue-btn i {
  font-size: 0.7rem;
}

/* 重置按钮 */
.reset-section {
  padding: 0.5rem;
}

.reset-btn {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.4rem;
  width: 100%;
  padding: 0.45rem 0.75rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-tertiary);
  color: var(--c-text-muted);
  font-family: inherit;
  font-size: 0.75rem;
  font-weight: 600;
  cursor: pointer;
  transition: all var(--transition-fast);
}

.reset-btn:hover {
  border-color: var(--c-danger, #fb7185);
  color: var(--c-danger, #fb7185);
  background: rgba(251, 113, 133, 0.08);
}

/* 色相滑块 - 彩虹渐变 */
.hue-slider {
  width: 100%;
  height: 8px;
  -webkit-appearance: none;
  appearance: none;
  border-radius: 4px;
  background: linear-gradient(to right,
    hsl(0, 80%, 60%),
    hsl(60, 80%, 60%),
    hsl(120, 80%, 60%),
    hsl(180, 80%, 60%),
    hsl(240, 80%, 60%),
    hsl(300, 80%, 60%),
    hsl(360, 80%, 60%)
  );
  outline: none;
  cursor: pointer;
}

.hue-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 16px;
  height: 16px;
  border-radius: 50%;
  background: #fff;
  border: 2px solid var(--c-accent);
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.3);
  cursor: pointer;
}

.sat-slider {
  width: 100%;
  height: 6px;
  -webkit-appearance: none;
  appearance: none;
  border-radius: 3px;
  background: var(--c-bg-tertiary);
  outline: none;
  cursor: pointer;
}

.sat-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 14px;
  height: 14px;
  border-radius: 50%;
  background: var(--c-accent);
  cursor: pointer;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
}

/* 动画 */
.configurator-enter-active {
  transition: all 0.25s cubic-bezier(0.22, 1, 0.36, 1);
}

.configurator-leave-active {
  transition: all 0.2s ease;
}

.configurator-enter-from,
.configurator-leave-to {
  opacity: 0;
  transform: translateY(12px) scale(0.95);
}
</style>
