<template>
    <div class="keyboard-container">
        <div class="button-grid">
            <NormalButton :index="4" :current-index="currentIndex" label="1" type="circle" @click="openDialog(4)" />
            <NormalButton :index="0" :current-index="currentIndex" label="1" type="square" @click="openDialog(0)" />
            <NormalButton :index="1" :current-index="currentIndex" label="1" type="vertical-bar"
                @click="openDialog(1)" />
            <NormalButton :index="2" :current-index="currentIndex" label="1" type="square" @click="openDialog(2)" />
            <NormalButton :index="3" :current-index="currentIndex" label="1" type="square" @click="openDialog(3)" />
        </div>
        <KeySelector v-model:visible="seletorVisible" :current-index="currentIndex" />

    </div>
</template>

<script setup lang="ts">
import { ref, watch } from 'vue';

import NormalButton from '@/widgets/buttons/NormalButton.vue';
import KeySelector from '@/widgets/selector/Selector.vue';

const seletorVisible = ref(false); // 键位选择器是否显示

const currentIndex = ref(0)  // 当前激活的按钮索引

// 打开键位选择器
const openDialog = (index: number) => {
    seletorVisible.value = true;
    currentIndex.value = index;
};

// 监听键位选择器是否关闭
watch(seletorVisible, (val) => {
    if (!val) {
        currentIndex.value = -1;
    }
});

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