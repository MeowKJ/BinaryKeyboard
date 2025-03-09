<template>
    <div class="tab-panels">
        <p>
            <span>{{ isListening ? "正在监听键盘输入..." : "点击按钮从键盘录入" }}</span>
        </p>
        <Divider />
        <p>
            <Button @click="toggleListenKeyboard" :label="buttonLabel" variant="outlined" size="large" />
        </p>
        <p class="modifierRow">
            <span class="modifireKeysLabel">Left: </span>
            <SelectButton v-model="leftModifierKeysValue" :options="modifierKeysList" multiple />
        </p>
        <p class="modifierRow">
            <span class="modifireKeysLabel">Right: </span>
            <SelectButton v-model="rightModifierKeysValue" :options="modifierKeysList" multiple />
        </p>
        <p> {{ getKeyNameCombination(keyConfig) }}</p>
    </div>
</template>

<script setup lang="ts">
import type { KeyboardConfig } from '@/types';
import { computed, watch, onUnmounted, ref } from 'vue';
import { getKeyNameCombination } from '@/utils/hidConverters/keyboardHIDConverter';


const props = defineProps<{ keyConfig: KeyboardConfig; tapValue: string; }>();
const emit = defineEmits(["update:keyConfig", "onKeyboardListen"]);

const keyConfig = computed({
    get() {
        return props.keyConfig;
    },
    set(newValue) {
        emit('update:keyConfig', newValue);
    }
});

const buttonLabel = computed(() => {
    return keyConfig.value.code || keyConfig.value.key || "无输入";
});


const modifierKeysList = ["Ctrl", "Alt", "Shift", "Meta"];

const leftModifierKeysValue = computed({
    get: () => {
        const keys: string[] = [];
        if (keyConfig.value.leftMetaKey) keys.push("Meta");
        if (keyConfig.value.leftCtrlKey) keys.push("Ctrl");
        if (keyConfig.value.leftAltKey) keys.push("Alt");
        if (keyConfig.value.leftShiftKey) keys.push("Shift");
        return keys;
    },
    set: (value: string[]) => {
        keyConfig.value.leftMetaKey = value.includes("Meta");
        keyConfig.value.leftCtrlKey = value.includes("Ctrl");
        keyConfig.value.leftAltKey = value.includes("Alt");
        keyConfig.value.leftShiftKey = value.includes("Shift");
    },
});

const rightModifierKeysValue = computed({
    get: () => {
        const keys: string[] = [];
        if (keyConfig.value.rightMetaKey) keys.push("Meta");
        if (keyConfig.value.rightCtrlKey) keys.push("Ctrl");
        if (keyConfig.value.rightAltKey) keys.push("Alt");
        if (keyConfig.value.rightShiftKey) keys.push("Shift");
        return keys;
    },
    set: (value: string[]) => {
        keyConfig.value.rightMetaKey = value.includes("Meta");
        keyConfig.value.rightCtrlKey = value.includes("Ctrl");
        keyConfig.value.rightAltKey = value.includes("Alt");
        keyConfig.value.rightShiftKey = value.includes("Shift");
    },
});


let lastLocation = 0;
let lastKey = "";
const keydownHandler = (event: KeyboardEvent) => {
    // 如果不是修饰键，就更新键盘配置
    if (!['Control', 'Alt', "Shift", "Meta"].includes(event.key)) {
        keyConfig.value.key = event.key;
        keyConfig.value.code = event.code;
    }

    if (event.location === 2) {
        keyConfig.value.rightMetaKey = event.metaKey;
        keyConfig.value.rightCtrlKey = event.ctrlKey;
        keyConfig.value.rightAltKey = event.altKey;
        keyConfig.value.rightShiftKey = event.shiftKey;
    } else if (event.location === 1) {
        keyConfig.value.leftMetaKey = event.metaKey;
        keyConfig.value.leftCtrlKey = event.ctrlKey;
        keyConfig.value.leftAltKey = event.altKey;
        keyConfig.value.leftShiftKey = event.shiftKey;

    } else {
        // RShift 的 location 为 0
        if (lastLocation === 1) {
            clearKeyboardModifierKeys('right');
            keyConfig.value.leftMetaKey = event.metaKey;
            keyConfig.value.leftCtrlKey = event.ctrlKey;
            keyConfig.value.leftAltKey = event.altKey;
            keyConfig.value.leftShiftKey = event.shiftKey;

        } else if (lastLocation === 2) {
            clearKeyboardModifierKeys('left');
            keyConfig.value.rightMetaKey = event.metaKey;
            keyConfig.value.rightCtrlKey = event.ctrlKey;
            keyConfig.value.rightAltKey = event.altKey;
            //keyConfig.value.rightShiftKey = event.shiftKey;
        } else {
            if (lastKey === "Shift" || event.key === "Shift"
            ) {
                clearKeyboardModifierKeys('left');
                keyConfig.value.rightShiftKey = true;
            } else {
                clearKeyboardModifierKeys('all')
            }
        }
    }
    lastLocation = event.location;
    lastKey = event.key;

    console.table({
        Key: event.key,
        Code: event.code,
        Location: event.location,
        Meta: event.metaKey,
        Ctrl: event.ctrlKey,
        Alt: event.altKey,
        Shift: event.shiftKey,
    });
    event.preventDefault();
};

const clearKeyboardModifierKeys = (type: 'left' | 'right' | 'all') => {
    if (type === 'left') {
        keyConfig.value.leftMetaKey = false;
        keyConfig.value.leftCtrlKey = false;
        keyConfig.value.leftAltKey = false;
        keyConfig.value.leftShiftKey = false;
    } else if (type === 'right') {
        keyConfig.value.rightMetaKey = false;
        keyConfig.value.rightCtrlKey = false;
        keyConfig.value.rightAltKey = false;
        keyConfig.value.rightShiftKey = false;
    } else {
        keyConfig.value.leftMetaKey = false;
        keyConfig.value.leftCtrlKey = false;
        keyConfig.value.leftAltKey = false;
        keyConfig.value.leftShiftKey = false;
        keyConfig.value.rightMetaKey = false;
        keyConfig.value.rightCtrlKey = false;
        keyConfig.value.rightAltKey = false;
        keyConfig.value.rightShiftKey = false;
    }

};

const isListening = ref(false);

watch(props, (newVal) => {
    if (newVal.tapValue === '0') {
        return
    }
    stopListenKeyboard();

});

const startListenKeyboard = () => {
    console.log("监听到键盘配置");
    if (isListening.value) {
        return;
    }
    isListening.value = true;
    emit("onKeyboardListen", true);
    window.addEventListener("keydown", keydownHandler);
}

const toggleListenKeyboard = () => {
    if (isListening.value) {
        stopListenKeyboard();
    } else {
        startListenKeyboard();
    }
}

const stopListenKeyboard = () => {
    console.log("移除键盘配置监听");
    isListening.value = false;
    emit("onKeyboardListen", false);
    window.removeEventListener("keydown", keydownHandler);
}

onUnmounted(() => {
    stopListenKeyboard();
});

</script>