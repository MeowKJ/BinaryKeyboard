<template>
    <p>选择要分配的鼠标按键或滚轮滚动量</p>
    <Divider />
    <div class="card flex flex-wrap gap-4">
        <div class="flex items-center gap-2">
            <Checkbox v-model="selectedButtons" inputId="ingredient1" name="mouseButton"
                :value="MouseButtonHID.LeftButton" />
            <label for="ingredient1"> 左键 </label>
        </div>
        <div class="flex items-center gap-2">
            <Checkbox v-model="selectedButtons" inputId="ingredient2" name="mouseButton"
                :value="MouseButtonHID.MiddleButton" />
            <label for="ingredient2"> 中键 </label>
        </div>
        <div class="flex items-center gap-2">
            <Checkbox v-model="selectedButtons" inputId="ingredient3" name="mouseButton"
                :value="MouseButtonHID.RightButton" />
            <label for="ingredient3"> 右键 </label>
        </div>
        <div class="flex items-center gap-2">
            <Checkbox v-model="selectedButtons" inputId="ingredient4" name="mouseButton"
                :value="MouseButtonHID.BackButton" />
            <label for="ingredient4"> 侧键1 </label>
        </div>
        <div class="flex items-center gap-2">
            <Checkbox v-model="selectedButtons" inputId="ingredient5" name="mouseButton"
                :value="MouseButtonHID.ForwardButton" />
            <label for="ingredient5"> 侧键2 </label>
        </div>
    </div>
    <Divider />
    <div class="text-center">

        <Knob v-model="mouseConfig.wheel" :max="127" :min="-128" :size="120" />
        <div class="flex gap-2 justify-center">
            <Button icon="pi pi-plus" @click="mouseConfig.wheel++" :disabled="mouseConfig.wheel >= 127" />
            <Button icon="pi pi-times" @click="mouseConfig.wheel = 0" :disabled="mouseConfig.wheel == 0" />
            <Button icon="pi pi-minus" @click="mouseConfig.wheel--" :disabled="mouseConfig.wheel <= -128" />
        </div>
    </div>

</template>

<script lang="ts" setup>
import type { MouseConfig } from '@/types';
import { MouseButtonHID } from '@/types';
import { computed, ref, watch } from 'vue';

const props = defineProps<{ mouseConfig: MouseConfig }>();
const emit = defineEmits(['update:mouseConfig']);

// 用于存储选中的鼠标按钮
const selectedButtons = ref<number[]>([]);

// 计算当前所有选中的按钮的二进制 OR 结果
const mouseConfig = computed({
    get: () => props.mouseConfig,
    set: (val) => emit('update:mouseConfig', val),
});

watch(selectedButtons, (newSelectedButtons) => {
    const newButtonValue = newSelectedButtons.reduce(
        (acc, current) => acc | current,
        0
    );

    // 打印16进制和2进制表示
    console.log('Selected Buttons:', newSelectedButtons);
    console.log('Hexadecimal:', newButtonValue.toString(16).toUpperCase()); // 转换为16进制
    console.log('Binary (8-bit):', newButtonValue.toString(2).padStart(8, '0')); // 转换为二进制，确保8位
});

</script>

<style scoped></style>