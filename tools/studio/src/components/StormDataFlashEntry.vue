<script setup lang="ts">
const emit = defineEmits<{
  (e: 'open'): void;
}>();
</script>

<template>
  <div class="panel storm-dataflash-entry">
    <div class="entry-copy">
      <span class="entry-kicker">STORM CORE</span>
      <strong>DataFlash</strong>
      <small>危险字节显微镜</small>
    </div>
    <button type="button" class="entry-open" title="打开 DataFlash 工作台" @click="emit('open')">
      <i class="pi pi-bolt"></i>
    </button>
  </div>
</template>

<style scoped>
.storm-dataflash-entry {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.75rem;
  overflow: hidden;
  background:
    linear-gradient(135deg, rgba(8, 12, 28, 0.64), rgba(14, 116, 144, 0.18)),
    radial-gradient(circle at 14% 20%, rgba(125, 211, 252, 0.18), transparent 42%);
}

.storm-dataflash-entry::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(115deg, transparent 0 42%, rgba(186, 230, 253, 0.25) 45%, transparent 48% 100%),
    repeating-linear-gradient(90deg, rgba(148, 163, 184, 0.05) 0 1px, transparent 1px 18px);
  opacity: 0.28;
  animation: entrySweep 4s ease-in-out infinite;
  pointer-events: none;
}

.entry-copy,
.entry-open {
  position: relative;
  z-index: 1;
}

.entry-copy {
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.entry-kicker {
  color: #67e8f9;
  font-size: 0.58rem;
  font-weight: 900;
  letter-spacing: 0.08em;
}

.entry-copy strong {
  color: #e0f2fe;
  font-size: 0.95rem;
  line-height: 1.2;
}

.entry-copy small {
  color: #94a3b8;
  font-size: 0.66rem;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: normal;
  line-height: 1.25;
}

.entry-open {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 2.25rem;
  height: 2.25rem;
  border-radius: var(--radius-sm);
  border: 1px solid rgba(125, 211, 252, 0.35);
  background: rgba(8, 47, 73, 0.42);
  color: #bae6fd;
  cursor: pointer;
  transition: transform 0.16s ease, border-color 0.16s ease, background 0.16s ease;
}

.entry-open:hover {
  transform: translateY(-1px);
  border-color: rgba(186, 230, 253, 0.8);
  background: rgba(14, 116, 144, 0.34);
}

@keyframes entrySweep {
  0%, 100% { opacity: 0.2; transform: translateX(-8%); }
  50% { opacity: 0.42; transform: translateX(8%); }
}
</style>
