<template>
    <div class="keyboard-container">
        <div class="button-grid">
            <NormalButton :index="1" label="1" type="circle" @click="openDialog(1)" />
            <NormalButton :index="1" label="1" type="square" @click="openDialog(2)" />
            <NormalButton :index="1" label="1" type="vertical-bar" />
            <NormalButton :index="1" label="1" type="square" />
            <NormalButton :index="1" label="1" type="square" />
        </div>
        <KeySelector v-model:visible="seletorVisible" :current-index="currentIndex" />

    </div>
</template>

<script setup lang="ts">

import NormalButton from '@/widgets/buttons/NormalButton.vue';
import KeySelector from '@/widgets/KeySelector.vue';

import { ref } from 'vue';


const seletorVisible = ref(false);

const currentIndex = ref(0)  // 当前激活的按钮索引


const openDialog = (index: number) => {
    seletorVisible.value = true;
    currentIndex.value = index;

    console.log('openDialog:', index);
};

</script>

<style scoped>
.keyboard-container {
    display: flex;
    justify-content: center;
    align-items: center;
}

.button-grid {
    display: grid;
    grid-template-columns: repeat(3, 8em);
    grid-template-rows: repeat(2, 8em);
    gap: 10px;
}
</style>