<script setup lang="ts">
const emit = defineEmits<{
  (e: 'open'): void;
}>();
</script>

<template>
  <div
    class="panel storm-dataflash-entry"
    role="button"
    tabindex="0"
    aria-label="Open DataFlash die"
    @click="emit('open')"
    @keydown.enter.prevent="emit('open')"
    @keydown.space.prevent="emit('open')"
  >
    <div class="entry-die" aria-hidden="true">
      <span v-for="index in 16" :key="index" :class="{ hot: index === 1 || index === 12 }"></span>
    </div>

    <div class="entry-copy">
      <strong>DataFlash</strong>
      <code>0x0000-0x7FFF</code>
    </div>

    <span class="entry-chip">DIE</span>

    <button type="button" class="entry-open" title="Open DataFlash die" @click.stop="emit('open')">
      <i class="pi pi-bolt"></i>
    </button>
  </div>
</template>

<style scoped>
.storm-dataflash-entry {
  position: relative;
  display: grid;
  grid-template-columns: 3.4rem minmax(0, 1fr) auto 2.55rem;
  align-items: center;
  gap: 0.72rem;
  min-height: 76px;
  padding: 0.72rem 0.78rem !important;
  overflow: hidden;
  border-color: rgba(125, 211, 252, 0.3) !important;
  background:
    linear-gradient(135deg, rgba(2, 6, 23, 0.86), rgba(8, 47, 73, 0.24) 62%, rgba(127, 29, 29, 0.18)),
    repeating-linear-gradient(90deg, rgba(125, 211, 252, 0.045) 0 1px, transparent 1px 18px) !important;
  box-shadow:
    inset 0 0 0 1px rgba(186, 230, 253, 0.045),
    inset 0 0 28px rgba(14, 165, 233, 0.055),
    0 0 28px rgba(14, 165, 233, 0.08) !important;
  cursor: pointer;
  transition: border-color 0.16s ease, box-shadow 0.16s ease, transform 0.16s ease;
}

.storm-dataflash-entry:hover,
.storm-dataflash-entry:focus-visible {
  border-color: rgba(103, 232, 249, 0.68) !important;
  box-shadow:
    inset 0 0 0 1px rgba(186, 230, 253, 0.08),
    inset 0 0 30px rgba(14, 165, 233, 0.08),
    0 0 32px rgba(14, 165, 233, 0.18),
    0 0 26px rgba(244, 63, 94, 0.08) !important;
  outline: none;
  transform: translateY(-1px);
}

.storm-dataflash-entry::before,
.storm-dataflash-entry::after {
  content: '';
  position: absolute;
  pointer-events: none;
}

.storm-dataflash-entry::before {
  inset: 0;
  background:
    linear-gradient(112deg, transparent 0 44%, rgba(186, 230, 253, 0.2) 47%, transparent 50% 100%);
  opacity: 0.24;
  animation: entrySweep 4.6s ease-in-out infinite;
}

.storm-dataflash-entry::after {
  inset: 1px;
  border: 1px solid rgba(251, 113, 133, 0.1);
  clip-path: polygon(0 0, calc(100% - 10px) 0, 100% 10px, 100% 100%, 10px 100%, 0 calc(100% - 10px));
}

.entry-die,
.entry-copy,
.entry-chip,
.entry-open {
  position: relative;
  z-index: 1;
}

.entry-die {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 2px;
  width: 3.2rem;
  height: 3.2rem;
  padding: 0.32rem;
  border: 1px solid rgba(125, 211, 252, 0.28);
  border-radius: 3px;
  background:
    linear-gradient(135deg, rgba(8, 47, 73, 0.52), rgba(15, 23, 42, 0.76)),
    repeating-linear-gradient(0deg, rgba(125, 211, 252, 0.08) 0 1px, transparent 1px 8px);
  box-shadow: inset 0 0 18px rgba(125, 211, 252, 0.08), 0 0 18px rgba(14, 165, 233, 0.12);
}

.entry-die span {
  min-width: 0;
  border: 1px solid rgba(125, 211, 252, 0.18);
  background: rgba(15, 23, 42, 0.78);
  box-shadow: inset 0 0 7px rgba(125, 211, 252, 0.045);
}

.entry-die span.hot {
  border-color: rgba(251, 113, 133, 0.48);
  background: rgba(127, 29, 29, 0.64);
  box-shadow: 0 0 8px rgba(244, 63, 94, 0.32), inset 0 0 8px rgba(251, 113, 133, 0.16);
}

.entry-copy {
  display: flex;
  min-width: 0;
  flex-direction: column;
  gap: 0.12rem;
}

.entry-copy strong,
.entry-copy code,
.entry-chip,
.entry-open {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
}

.entry-copy strong {
  color: #e0f2fe;
  font-size: 0.92rem;
  font-weight: 950;
  line-height: 1.05;
  text-shadow: 0 0 14px rgba(125, 211, 252, 0.18);
}

.entry-copy code {
  color: rgba(125, 211, 252, 0.78);
  font-size: 0.62rem;
  font-weight: 900;
  line-height: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.entry-chip {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 2.45rem;
  height: 1.34rem;
  border: 1px solid rgba(125, 211, 252, 0.24);
  border-radius: 2px;
  background: rgba(14, 116, 144, 0.14);
  color: #67e8f9;
  font-size: 0.58rem;
  font-weight: 950;
  letter-spacing: 0.04em;
}

.entry-open {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 2.35rem;
  height: 2.35rem;
  border: 1px solid rgba(125, 211, 252, 0.42);
  border-radius: 3px;
  background:
    linear-gradient(135deg, rgba(14, 116, 144, 0.36), rgba(127, 29, 29, 0.28)),
    rgba(2, 6, 23, 0.64);
  color: #e0f2fe;
  cursor: pointer;
  box-shadow: inset 0 0 16px rgba(186, 230, 253, 0.07), 0 0 16px rgba(14, 165, 233, 0.14);
  transition: transform 0.16s ease, border-color 0.16s ease, box-shadow 0.16s ease;
}

.entry-open i {
  font-size: 1rem;
  filter: drop-shadow(0 0 7px rgba(186, 230, 253, 0.62));
}

.entry-open:hover,
.storm-dataflash-entry:hover .entry-open {
  border-color: rgba(186, 230, 253, 0.78);
  box-shadow: 0 0 18px rgba(14, 165, 233, 0.22), 0 0 12px rgba(251, 113, 133, 0.12);
}

.entry-open:hover {
  transform: translateY(-1px);
}

@media (max-width: 760px) {
  .storm-dataflash-entry {
    grid-template-columns: 3.15rem minmax(0, 1fr) 2.35rem;
    min-height: 70px;
  }

  .entry-chip {
    display: none;
  }

  .entry-die {
    width: 2.95rem;
    height: 2.95rem;
  }
}

@keyframes entrySweep {
  0%, 100% { opacity: 0.16; transform: translateX(-8%); }
  50% { opacity: 0.32; transform: translateX(8%); }
}
</style>
