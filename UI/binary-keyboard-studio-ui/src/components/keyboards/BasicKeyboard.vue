<template>
    <div class="keyboard-container">
        <div class="button-grid">

            <NormalButton :index="0" :current-index="currentIndex" label="0" type="square" @click="currentIndex = 0" />
            <NormalButton :index="1" :current-index="currentIndex" label="1" type="square" @click="currentIndex = 1" />
            <NormalButton :index="2" :current-index="currentIndex" label="2" type="vertical-bar"
                @click="currentIndex = 2" />
            <NormalButton :index="3" :current-index="currentIndex" label="3" type="horizontal-bar"
                @click="currentIndex = 3" />

        </div>
        <KeySelector v-model:visible="seletorVisible" :current-index="currentIndex" />
    </div>
</template>

<script setup lang="ts">
import { ref, computed, watch } from 'vue';

import NormalButton from '@/widgets/buttons/NormalButton.vue';
import KeySelector from '@/widgets/selector/Selector.vue';

const seletorVisible = ref(false); // 键位选择器是否显示

const currentIndex = computed({
    get: () => seletorVisible.value ? currentIndexValue.value : -1,
    set: (index) => {
        currentIndexValue.value = index;
        seletorVisible.value = index !== -1;
    }
});

const currentIndexValue = ref(-1); // 当前激活的按钮索引

const emit = defineEmits(['onSaveData']);

// 监听 `seletorVisible` 变化，关闭时触发 `saveData`
watch(seletorVisible, (newValue) => {
    if (!newValue) {
        emit('onSaveData');
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
