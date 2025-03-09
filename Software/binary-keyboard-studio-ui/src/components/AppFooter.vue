<template>
    <footer class="app-footer">
        <!-- 垂直布局容器 -->
        <div class="flex flex-column align-items-center gap-4">
            <!-- 版权信息 -->
            <span class="text-color-secondary font-medium text-lg">
                © {{ copyrightYears }} 喵喵的帕斯. All rights reserved.
            </span>

            <!-- 交互功能区 -->
            <div class="flex align-items-center gap-5">
                <!-- 社交链接 -->
                <div class="flex gap-3">
                    <Button v-for="link in socialLinks" :key="link.name" :icon="link.icon"
                        class="p-button-text p-button-rounded social-button" @click="openLink(link.url)" />
                </div>

                <!-- 分隔线 -->
                <div class="h-2rem border-right-1 surface-border"></div>

                <!-- 帮助按钮 -->
                <Button label="帮助中心" icon="pi pi-question-circle" class="p-button-help help-button"
                    @click="$emit('help-click')" />
            </div>
        </div>
    </footer>
</template>

<script setup lang="ts">
import { ref } from 'vue';


const initialYear = 2025
const currentYear = ref(new Date().getFullYear())

const copyrightYears = ref(
    currentYear.value > initialYear
        ? `${initialYear}-${currentYear.value}`
        : initialYear
)

// 社交链接配置（保持原有逻辑）
const socialLinks = ref([
    {
        name: 'github',
        icon: 'pi pi-github',
        url: 'https://github.com/MeowKJ/BinaryKeyboard'
    },
    {
        name: 'oshwhub',
        icon: 'pi pi-microchip',
        url: 'https://twitter.com/yourprofile'
    },
    {
        name: 'email',
        icon: 'pi pi-envelope',
        url: 'mailto:ijink@icloud.com'
    }
]);

const openLink = (url: string) => {
    window.open(url, '_blank', 'noopener,noreferrer');
};
</script>

<style scoped>
.app-footer {
    padding: 2rem 1rem;
    background: var(--surface-ground);
    border-top: 1px solid var(--surface-border);
    box-shadow: 0 -4px 12px rgba(0, 0, 0, 0.05);
}

/* 社交按钮动画 */
.social-button {
    transition: all 0.3s ease;
    color: var(--text-color-secondary) !important;

    &:hover {
        transform: translateY(-2px);
        color: var(--primary-color) !important;
        background: var(--surface-card) !important;
        box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
    }
}

/* 帮助按钮样式 */
.help-button {
    transition: all 0.3s ease;
    border: 1px solid var(--primary-color) !important;

    &:hover {
        background: var(--primary-color) !important;
        color: white !important;
        box-shadow: 0 2px 6px rgba(var(--primary-rgb), 0.2);
    }
}

/* 移动端适配 */
@media screen and (max-width: 576px) {
    .app-footer {
        padding: 1.5rem 1rem;
    }

    .text-lg {
        font-size: 0.875rem !important;
    }

    .gap-4 {
        gap: 1rem !important;
    }

    .help-button {
        padding: 0.5rem 1rem !important;
    }
}
</style>