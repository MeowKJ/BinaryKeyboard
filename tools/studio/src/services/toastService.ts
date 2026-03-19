/**
 * 全局 Toast 服务
 *
 * 在 App.vue onMounted 时通过 init() 注入 PrimeVue Toast 实例，
 * 之后任何非 Vue 上下文（Service、Store 等）均可直接调用 showToast()。
 */

import { ref } from 'vue';
import type { ToastServiceMethods } from 'primevue/toastservice';

let _toast: ToastServiceMethods | null = null;

export type ToastSeverity = 'success' | 'info' | 'warn' | 'error';

/** 最近一次 Toast 的 severity，供猫咪助手响应 */
export const lastToastSeverity = ref<ToastSeverity | null>(null);

let _moodTimer: ReturnType<typeof setTimeout> | null = null;

/** 由 App.vue 在 setup 阶段调用一次完成注入 */
export function initToastService(toast: ToastServiceMethods): void {
  _toast = toast;
}

/** 显示 Toast 通知 */
export function showToast(
  severity: ToastSeverity,
  summary: string,
  detail: string,
  life = 2500,
): void {
  if (!_toast) {
    console.warn('[toastService] 未初始化，降级为 console:', severity, summary, detail);
    return;
  }
  _toast.add({ severity, summary, detail, life });

  /* 更新猫咪助手表情 */
  lastToastSeverity.value = severity;
  if (_moodTimer) clearTimeout(_moodTimer);
  _moodTimer = setTimeout(() => {
    lastToastSeverity.value = null;
    _moodTimer = null;
  }, life);
}
