<template>
    <div class="knob-button">
        <Button v-for="(segment, i) in 3" :key="i"
            :class="['knob-segment', `segment-${i}`, { 'active-button-segment': index[i] === currentIndex }]"
            severity="secondary" @click="handleClick(index[i])">
            <div class="button-content">

                <i v-if="i === 0" class="pi pi-replay icon"></i> <!-- 第一个按钮：pi-refresh -->

                <i v-if="i === 2" class="pi  pi-refresh icon"></i> <!-- 第三个按钮：pi-replay -->
                <span>{{ label[i] }}</span>
            </div>
        </Button>
    </div>
</template>

<script setup lang="ts">
import { defineEmits } from 'vue';

const props = defineProps<{
    label: string[];
    index: number[];
    currentIndex: number;
}>();

const emit = defineEmits(["update:index", "open-dialog"]);

const handleClick = (i: number) => {
    emit("update:index", i);
    emit("open-dialog", i);
};
</script>

<style scoped>
.knob-button {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    border-radius: 50%;
    overflow: hidden;
    position: relative;
    border: 2px solid transparent;
}

.knob-segment {
    width: 100%;
    height: 100%;
    text-align: center;
    font-size: 1.2em;
    border: none;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
}

/* 激活的按钮 */
.active-button-segment {
    background-color: var(--p-button-outlined-primary-border-color);
    color: var(--p-button-outlined-primary-text-color);
}

/* 图标和文字垂直排列 */
.button-content {
    display: flex;
    flex-direction: column;
    /* 垂直排列 */
    align-items: center;
    gap: 4px;
    /* 图标和文字之间的间距 */
}

/* 图标样式优化 */
.icon {
    font-size: 1.5em;
}

.segment-1 {
    border-radius: 0;
    border-left: 3px solid var(--p-content-background) !important;
    border-right: 3px solid var(--p-content-background) !important;
    border-top: none !important;
    border-bottom: none !important;
}

.segment-0,
.segment-2 {
    border: none !important;
}
</style>
