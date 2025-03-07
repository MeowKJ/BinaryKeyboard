<template>
    <div class="keyboard-container">
        <div class="button-grid">
            <KnobButton :label="['5', '4', '6']" :index="[5, 4, 6]" :current-index="currentIndex"
                @open-dialog="currentIndex = $event" />
            <NormalButton v-for="(_, i) in 4" :key="i" :index="i" :current-index="currentIndex" :label="i.toString()"
                :type="i === 1 ? 'vertical-bar' : 'square'" @click="currentIndex = i" />
        </div>
        <KeySelector v-model:visible="seletorVisible" :current-index="currentIndex" />
    </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';

import NormalButton from '@/widgets/buttons/NormalButton.vue';
import KeySelector from '@/widgets/selector/Selector.vue';
import KnobButton from '@/widgets/buttons/KnobButton.vue';

const seletorVisible = ref(false); // 键位选择器是否显示

const currentIndex = computed({
    get: () => seletorVisible.value ? currentIndexValue.value : -1,
    set: (index) => {
        currentIndexValue.value = index;
        seletorVisible.value = index !== -1;
    }
});

const currentIndexValue = ref(-1); // 当前激活的按钮索引

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
