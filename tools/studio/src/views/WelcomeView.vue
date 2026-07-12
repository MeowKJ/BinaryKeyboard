<script setup lang="ts">
import { useReleaseStore } from '@/stores/releaseStore';
import { useTheme } from '@/composables/useTheme';
import CatEmoji from '@/components/CatEmoji.vue';
import Button from 'primevue/button';

defineProps<{
  connecting?: boolean;
  welcomeReturning?: boolean;
}>();

const emit = defineEmits<{
  (e: 'connect'): void;
}>();

const releaseStore = useReleaseStore();
const { toggleMode, themeMode, canToggleMode } = useTheme();

const linkButtons = [
  {
    label: 'Bilibili',
    iconSrc: 'https://www.bilibili.com/favicon.ico',
    href: 'https://space.bilibili.com/1676918589',
  },
  {
    label: 'GitHub',
    iconSrc: 'https://github.githubassets.com/favicons/favicon.svg',
    href: 'https://github.com/MeowKJ/BinaryKeyboard',
  },
  {
    label: 'OSHWHub',
    iconSrc: 'https://oshwhub.com/favicon.ico',
    href: 'https://oshwhub.com/kjpig/works',
  },
];
</script>

<template>
  <div class="welcome-screen" :class="{ returning: welcomeReturning }">
    <!-- 主题切换按钮 -->
    <button v-if="canToggleMode" class="theme-toggle" @click="toggleMode">
      <i :class="themeMode === 'dark' ? 'pi pi-sun' : 'pi pi-moon'"></i>
    </button>

    <div class="welcome-content">
      <div class="logo-section">
        <div class="logo-icon"><CatEmoji type="grinning-animated" /></div>
        <h1 class="app-title">BinaryKeyboard</h1>
        <p class="app-subtitle">喵喵的帕斯 - 可爱小键盘改键工具</p>
      </div>

      <div class="connect-section">
        <div class="connect-card">
          <div class="keyboard-preview">
            <div class="preview-keys">
              <div class="preview-key"></div>
              <div class="preview-key"></div>
              <div class="preview-key tall"></div>
              <div class="preview-key"></div>
              <div class="preview-key"></div>
            </div>
          </div>

          <!-- 连接中状态 -->
          <div v-if="connecting" class="connecting-overlay">
            <CatEmoji type="hourglass-not-done-animated" class="connecting-hourglass" />
            <span class="connecting-text">正在连接...</span>
          </div>
          <template v-else>
            <Button label="连接键盘" icon="pi pi-usb" size="large" @click="emit('connect')" class="connect-button" />
            <p class="connect-hint">
              <i class="pi pi-info-circle"></i>
              无线款可保持蓝牙模式，插入 USB 后直接配置
            </p>
          </template>
        </div>
      </div>

      <div class="features-section">
        <div class="feature-item">
          <i class="pi pi-th-large"></i>
          <span>多层映射</span>
        </div>
        <div class="feature-item">
          <i class="pi pi-palette"></i>
          <span>RGB 灯效</span>
        </div>
        <div class="feature-item">
          <i class="pi pi-code"></i>
          <span>宏录制</span>
        </div>
        <div class="feature-item">
          <i class="pi pi-bolt"></i>
          <span>USB / 蓝牙</span>
        </div>
      </div>

      <div class="welcome-version-card">
        <div class="version-card-header">
          <i class="pi pi-box"></i>
          <span>版本信息</span>
        </div>
        <div class="version-badges">
          <div class="version-badge studio-badge">
            <div class="badge-icon"><CatEmoji type="desktop-computer-3d" /></div>
            <div class="badge-info">
              <span class="badge-label">Studio 最新</span>
              <span class="badge-version">v{{ releaseStore.latestStudioVersion }}</span>
            </div>
          </div>
          <div class="version-badge chip-badge ch552-badge">
            <div class="badge-chip-tag">CH552</div>
            <div class="badge-info">
              <span class="badge-label">固件</span>
              <span class="badge-version">v{{ releaseStore.latestVersions.ch552 }}</span>
            </div>
          </div>
          <div class="version-badge chip-badge ch592-badge">
            <div class="badge-chip-tag">CH592</div>
            <div class="badge-info">
              <span class="badge-label">固件</span>
              <span class="badge-version">v{{ releaseStore.latestVersions.ch592 }}</span>
            </div>
          </div>
        </div>
        <div v-if="releaseStore.errorMessage" class="version-error-tip">
          <i class="pi pi-exclamation-triangle"></i>
          <span>版本信息获取失败，显示的是本地缓存版本</span>
        </div>
        <a class="version-release-link" :href="`https://github.com/${releaseStore.repository}/releases`"
          target="_blank" rel="noopener">
          <i class="pi pi-github"></i>
          查看发布记录
          <i class="pi pi-external-link"></i>
        </a>
      </div>

      <nav class="welcome-links" aria-label="项目链接">
        <a
          v-for="link in linkButtons"
          :key="link.href"
          class="welcome-link-button"
          :href="link.href"
          target="_blank"
          rel="noopener"
        >
          <img class="welcome-link-icon" :src="link.iconSrc" :alt="`${link.label} 图标`" loading="lazy" />
          <span>{{ link.label }}</span>
          <i class="pi pi-external-link link-external-icon"></i>
        </a>
      </nav>
    </div>
  </div>
</template>

<style scoped>
.welcome-screen .theme-toggle {
  position: absolute;
  top: 1.5rem;
  right: 1.5rem;
}

.welcome-screen {
  height: 100vh;
  height: 100dvh;
  display: flex;
  align-items: flex-start;
  justify-content: center;
  position: relative;
  overflow-y: auto;
  padding: 2rem 0 4.5rem;
  background:
    radial-gradient(ellipse at top, var(--c-accent-soft) 0%, transparent 50%),
    var(--c-bg-primary);
  overscroll-behavior: contain;
}

.welcome-content {
  --keyboard-preview-width: 166px;
  text-align: center;
  max-width: 600px;
  width: min(100%, 600px);
  padding: 2rem;
  margin-block: auto;
}

.logo-section {
  margin-bottom: 3rem;
}

.logo-icon {
  width: 4.25rem;
  height: 4.25rem;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 1rem;
  font-size: 3.75rem;
  line-height: 1;
}

.logo-icon :deep(.cat-emoji) {
  display: block;
  width: 1em;
  height: 1em;
  object-fit: contain;
  vertical-align: 0;
}

.app-title {
  font-size: 2.75rem;
  font-weight: 800;
  margin: 0 0 0.5rem;
  background: var(--c-accent-gradient);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.app-subtitle {
  font-size: 1.1rem;
  color: var(--c-text-muted);
  margin: 0;
  font-weight: 500;
}

.connect-section {
  margin-bottom: 3rem;
  display: flex;
  justify-content: center;
}

.connect-card {
  width: min(100%, 360px);
  background: var(--c-bg-secondary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-lg);
  padding: 1.5rem 1.625rem;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);
  position: relative;
}

.keyboard-preview {
  margin-bottom: 1.5rem;
}

.preview-keys {
  display: grid;
  grid-template-columns: repeat(3, 50px);
  grid-template-rows: repeat(2, 50px);
  gap: 8px;
  justify-content: center;
}

.preview-key {
  background: var(--c-bg-tertiary);
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  animation: pulse 2s ease-in-out infinite;
}

.preview-key.tall {
  grid-row: span 2;
}

.connect-button {
  width: 11rem;
  max-width: 100%;
  margin-inline: 0;
  font-size: 1.1rem !important;
  font-weight: 700 !important;
  padding: 0.875rem 1.5rem !important;
  justify-content: center;
}

.connect-hint {
  margin: 1rem 0 0;
  font-size: 0.85rem;
  color: var(--c-text-muted);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.5rem;
}

/* 连接中覆盖层 */
.connecting-overlay {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.75rem;
  padding: 1rem 0;
}

.connecting-hourglass {
  width: 48px;
  height: 48px;
  animation: hourglass-spin 2s linear infinite;
}

@keyframes hourglass-spin {
  0% { transform: rotate(0deg); }
  50% { transform: rotate(180deg); }
  50.01% { transform: rotate(180deg); }
  100% { transform: rotate(360deg); }
}

.connecting-text {
  font-size: 0.95rem;
  font-weight: 600;
  color: var(--c-text-muted);
  animation: loadingPulse 1.5s ease-in-out infinite;
}

@keyframes loadingPulse {
  0%, 100% { opacity: 0.4; }
  50% { opacity: 1; }
}

.features-section {
  display: flex;
  justify-content: center;
  gap: 2rem;
  flex-wrap: wrap;
}

.feature-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.5rem;
  color: var(--c-text-muted);
  font-size: 0.85rem;
  font-weight: 600;
}

.feature-item i {
  font-size: 1.5rem;
  color: var(--c-accent);
}

.welcome-version-card {
  margin-top: 1.25rem;
  padding: 0;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-md);
  background: var(--c-bg-secondary);
  overflow: hidden;
}

.version-card-header {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.65rem 1rem;
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--c-text-muted);
  border-bottom: 1px solid var(--c-border);
  letter-spacing: 0.03em;
}

.version-badges {
  display: flex;
  gap: 0.5rem;
  padding: 0.75rem 1rem;
}

.version-badge {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.55rem 0.65rem;
  border-radius: var(--radius-sm);
  border: 1px solid var(--c-border-light);
  background: var(--c-bg-primary);
  transition: border-color var(--transition-fast), box-shadow var(--transition-fast);
}

.version-badge:hover {
  border-color: var(--c-accent);
  box-shadow: 0 0 12px -4px var(--c-accent-soft);
}

.badge-icon {
  font-size: 1.3rem;
  line-height: 1;
}

.badge-chip-tag {
  font-size: 0.6rem;
  font-weight: 800;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  padding: 0.15rem 0.4rem;
  border-radius: 4px;
  letter-spacing: 0.04em;
  line-height: 1.2;
  white-space: nowrap;
}

.ch552-badge .badge-chip-tag {
  background: rgba(96, 165, 250, 0.15);
  color: #60a5fa;
  border: 1px solid rgba(96, 165, 250, 0.3);
}

.ch592-badge .badge-chip-tag {
  background: rgba(74, 222, 128, 0.15);
  color: #4ade80;
  border: 1px solid rgba(74, 222, 128, 0.3);
}

.badge-info {
  display: flex;
  flex-direction: column;
  gap: 1px;
  min-width: 0;
}

.badge-label {
  font-size: 0.7rem;
  color: var(--c-text-muted);
  line-height: 1.2;
}

.badge-version {
  font-size: 0.78rem;
  font-weight: 700;
  font-family: 'JetBrains Mono', 'Fira Code', monospace;
  color: var(--c-text-primary);
  line-height: 1.2;
}

.studio-badge .badge-version {
  color: var(--c-accent-light);
}

.version-error-tip {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.35rem;
  padding: 0.4rem 0.75rem;
  font-size: 0.7rem;
  color: var(--c-warning, #e2a308);
  background: rgba(226, 163, 8, 0.08);
  border-top: 1px solid var(--c-border);
}

.version-error-tip .pi {
  font-size: 0.75rem;
}

.version-release-link {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 0.4rem;
  padding: 0.55rem 1rem;
  font-size: 0.75rem;
  color: var(--c-text-muted);
  text-decoration: none;
  border-top: 1px solid var(--c-border);
  transition: color var(--transition-fast), background var(--transition-fast);
}

.version-release-link:hover {
  color: var(--c-accent);
  background: var(--c-accent-soft);
}

.version-release-link .pi-external-link {
  font-size: 0.65rem;
}

.welcome-links {
  display: flex;
  justify-content: center;
  gap: 0.6rem;
  flex-wrap: wrap;
  margin-top: 1rem;
}

.welcome-link-button {
  min-width: 128px;
  min-height: 40px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.45rem;
  padding: 0.6rem 0.9rem;
  border: 1px solid var(--c-border);
  border-radius: var(--radius-sm);
  background: var(--c-bg-secondary);
  color: var(--c-text-secondary);
  font-size: 0.82rem;
  font-weight: 700;
  text-decoration: none;
  transition:
    color var(--transition-fast),
    border-color var(--transition-fast),
    background var(--transition-fast),
    transform var(--transition-fast),
    box-shadow var(--transition-fast);
}

.welcome-link-button:hover {
  transform: translateY(-1px);
  color: var(--c-accent);
  border-color: var(--c-accent);
  background: var(--c-accent-soft);
  box-shadow: 0 8px 20px -16px var(--c-accent);
}

.welcome-link-icon {
  width: 18px;
  height: 18px;
  object-fit: contain;
  flex: 0 0 auto;
}

.link-external-icon {
  font-size: 0.65rem;
  color: var(--c-text-muted);
}

@media (max-width: 640px) {
  .welcome-screen {
    align-items: flex-start;
    padding: 1rem 0 4rem;
  }

  .welcome-content {
    padding: 4.5rem 1rem 1.5rem;
  }

  .app-title {
    font-size: 2.1rem;
  }

  .logo-section,
  .connect-section {
    margin-bottom: 2rem;
  }

  .version-badges {
    flex-direction: column;
  }

  .welcome-link-button {
    flex: 1 1 150px;
  }
}

/* 回退动画 */
.welcome-screen.returning {
  animation: welcomeReturn 0.5s ease both;
}

@keyframes welcomeReturn {
  from {
    opacity: 0;
    transform: scale(1.03);
  }
}

@keyframes pulse {
  0%, 100% { opacity: 0.5; }
  50% { opacity: 0.8; }
}

/* 主题切换按钮 */
.theme-toggle {
  width: 40px;
  height: 40px;
  border-radius: 50%;
  border: 1px solid var(--c-border);
  background: var(--c-bg-secondary);
  color: var(--c-text-secondary);
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all var(--transition-fast);
  z-index: 10;
}

.theme-toggle:hover {
  border-color: var(--c-accent);
  color: var(--c-accent);
}
</style>
