<script setup lang="ts">
import { WebHidTools } from '@/utils/WebHidTools';
import { useDeviceStore } from '@/stores/deviceStore';
import { defineProps } from 'vue';

const props = defineProps<{
  filters: HIDDeviceFilter[];
}>();

const emit = defineEmits(['onDeviceConnected']);
const deviceStore = useDeviceStore();

const selectDevice = async () => {
  const selectedDevice = await WebHidTools.requestDevice(props.filters);
  if (selectedDevice) {
    deviceStore.device = selectedDevice;
    await WebHidTools.openDevice(selectedDevice);
    emit('onDeviceConnected'); // 触发事件
  }
};
</script>

<template>

  <div class="content">
    <h1 class="title"> 等待连接设备 </h1>

    <ProgressSpinner style="width: 36px; height: 36px;" strokeWidth="8" fill="transparent" animationDuration=".8s"
      aria-label="=ProgressSpinner" />

    <Button label="选择设备" severity="info" @click="selectDevice" class="device-button" />



    <Card class="custom-card">
      <template #content>
        <div class="skeleton-grid">
          <Skeleton size="4rem" borderRadius="50%" /> <!-- 圆形骨架 -->
          <Skeleton size="4rem" />
          <Skeleton class="tall-skeleton" /> <!-- 垂直合并的骨架 -->
          <Skeleton size="4rem" />
          <Skeleton size="4rem" />
        </div>
      </template>
    </Card>


  </div>
</template>

<style scoped>
.content {
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 2rem;
  align-items: center;
  width: 100%;
}


.title {
  font-size: 48px;
  font-weight: bold;
  margin-bottom: 10px;
}

.custom-card {
  border: 2.5px solid;
  border-radius: 15px;
  box-shadow: none;
  padding: 0;
  --p-card-body-padding: 0.5rem !important;
  border-color: var(--p-skeleton-background);
}


.skeleton-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  /* 3 列 */
  grid-template-rows: repeat(2, 1fr);
  /* 2 行 */
  gap: 10px;
  /* 间距 */
  width: 100%;
}

.tall-skeleton {
  grid-row: span 2;
  /* 垂直合并两行 */
  height: 100%;
  /* 让其高度自动填充 */
  min-height: 9rem;
  /* 设定最小高度，保证不太小 */
}


/* 动态边框颜色变化 */
@keyframes borderColorAnimation {
  0% {
    border-color: var(--p-border-color-one);
  }

  25% {
    border-color: var(--p-border-color-two);
  }

  50% {
    border-color: var(--p-border-color-three);
  }

  75% {
    border-color: var(--p-border-color-four);
  }

  100% {
    border-color: var(--p-border-color-one);
  }
}
</style>