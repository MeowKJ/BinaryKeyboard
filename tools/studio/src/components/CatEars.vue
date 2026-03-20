<script setup lang="ts">
</script>

<template>
  <div class="cat-ears">
    <div class="ear left-ear">
      <div class="ear-outer"></div>
      <div class="ear-inner"></div>
    </div>
    <div class="ear right-ear">
      <div class="ear-outer"></div>
      <div class="ear-inner"></div>
    </div>
  </div>
</template>

<style scoped>
.cat-ears {
  display: flex;
  justify-content: center;
  gap: 110px;
  pointer-events: none;
}

.ear {
  position: relative;
  width: 64px;
  height: 56px;
  transform-origin: bottom center;
}

.ear-outer {
  position: absolute;
  bottom: 0;
  left: 0;
  width: 0;
  height: 0;
  border-left: 32px solid transparent;
  border-right: 32px solid transparent;
  border-bottom: 56px solid var(--c-accent, #f472b6);
  border-radius: 6px 6px 0 0;
  filter: drop-shadow(0 -4px 12px rgba(0, 0, 0, 0.3));
}

.ear-inner {
  position: absolute;
  bottom: 2px;
  left: 11px;
  width: 0;
  height: 0;
  border-left: 21px solid transparent;
  border-right: 21px solid transparent;
  border-bottom: 38px solid #fbb6ce;
  border-radius: 4px 4px 0 0;
  z-index: 1;
}

.left-ear {
  animation: ear-left-twitch 4s ease-in-out infinite;
}

.right-ear {
  animation: ear-right-twitch 4s ease-in-out infinite 1s;
}

@keyframes ear-left-twitch {
  0%, 65%, 100% { transform: rotate(-6deg); }
  68% { transform: rotate(-30deg) scaleY(1.12); }
  74% { transform: rotate(8deg); }
  80% { transform: rotate(-18deg) scaleY(1.06); }
  88% { transform: rotate(-6deg); }
}

@keyframes ear-right-twitch {
  0%, 65%, 100% { transform: rotate(6deg); }
  68% { transform: rotate(30deg) scaleY(1.12); }
  74% { transform: rotate(-8deg); }
  80% { transform: rotate(18deg) scaleY(1.06); }
  88% { transform: rotate(6deg); }
}
</style>
