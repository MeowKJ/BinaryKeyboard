<template>
    <div :class="['knob-button', { 'active-button': index.includes(currentIndex) }]">
        <Button v-for="(segment, i) in 3" :key="i" :label="label[i]"
            :class="['knob-segment', `segment-${i}`, { 'active-button-segment': index[i] === currentIndex }]"
            severity="secondary" @click="handleClick(index[i])" />
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
    emit("open-dialog", i); // 触发打开 dialog 事件
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
}

.knob-button:hover {
    border: 2px solid var(--p-button-outlined-primary-border-color);
}

.knob-segment:hover {
    border: none !important;
    outline: none !important;
    box-shadow: none !important;
}

.active-button {
    border: 2px solid var(--p-button-outlined-primary-border-color);
}

.active-button-segment {
    background-color: var(--p-button-outlined-primary-border-color);
    color: var(--p-button-outlined-primary-text-color);
}

.active-button-segment:hover {
    background-color: var(--p-button-outlined-primary-border-color) !important;
    color: var(--p-button-outlined-primary-text-color);
}
</style>
