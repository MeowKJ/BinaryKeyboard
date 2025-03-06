<template>

    <Dialog v-model:visible="dialogVisible" :style="{ width: '25rem' }">

        <template #header>
            <span class="font-bold" style="font-size: 1.5em;">设置键位</span>
        </template>


        <Tabs v-model:value="tapValue">
            <TabList>
                <Tab value="0">键盘</Tab>
                <Tab value="1">多媒体</Tab>
                <Tab value="2">鼠标</Tab>
            </TabList>
            <TabPanels>
                <TabPanel value="0">
                    <div class="tab-panels">
                        <p>正在读取键盘输入...</p>
                        <p><Button :label="keyConfig.key || keyConfig.code || '无输入'" variant="outlined" size="large" />
                        </p>
                        <p class="modifierRow">
                            <span class="modifireKeysLabel">Left: </span>
                            <SelectButton v-model="leftModifierKeysValue" :options="modifierKeysList" multiple />
                        </p>
                        <p class="modifierRow">
                            <span class="modifireKeysLabel">Right: </span>
                            <SelectButton v-model="rightModifierKeysValue" :options="modifierKeysList" multiple />
                        </p>
                        <p> {{ getKeyNameCombination() }}</p>
                    </div>
                </TabPanel>
                <TabPanel value="1">
                    <p class="m-0"></p>
                </TabPanel>
                <TabPanel value="2">
                    <p class="m-0"></p>
                </TabPanel>
            </TabPanels>
        </Tabs>
        <template #footer>
        </template>
    </Dialog>
</template>

<script setup lang="ts">

import type { KeyboardConfig, KeyMapping } from '@/types/keyboard';
import { KEY_TYPE_KETBOARD, KEY_TYPE_MEDIA } from '@/utils/deviceConstants';
import { keyboardConfigToModifier, keyboardConfigToRawHID, modifierToKeyboardConfig } from '@/utils/keyboardHIDConverter';
import { computed, ref, watch, onMounted, onUnmounted } from 'vue';


import { useDeviceStore } from '@/stores/deviceStore';

const deviceStore = useDeviceStore();

const keyMappingList = computed({
    get: () => deviceStore.keyboardKeyMappings,
    set: (newValue) => deviceStore.keyboardKeyMappings = newValue,
});

const props = defineProps<{ visible: boolean; currentIndex: number }>();
const emit = defineEmits(["update:visible", "update:keyMapping"]);

const dialogVisible = computed({
    get: () => props.visible,
    set: (value) => emit("update:visible", value),
});


const tapValue = ref("0");

// --------------处理键盘配置--------------''
const keyConfig = ref<KeyboardConfig>({
    leftMetaKey: false,
    leftCtrlKey: false,
    leftAltKey: false,
    leftShiftKey: false,
    rightMetaKey: false,
    rightCtrlKey: false,
    rightAltKey: false,
    rightShiftKey: false,
    key: "",
    code: "",
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
const getKeyNameCombination = () => {
    const keys = new Set<string>(); // 使用 Set 自动去重

    leftModifierKeysValue.value.forEach((key) => keys.add(`L${key}`));
    rightModifierKeysValue.value.forEach((key) => keys.add(`R${key}`));

    if (keyConfig.value.key) {
        keys.add(keyConfig.value.code); // 加入主按键
    }

    return Array.from(keys).join(" + ");
};


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


watch(props, (newVal) => {
    if (newVal.visible) {
        window.addEventListener("keydown", keydownHandler);
    } else {
        window.removeEventListener("keydown", keydownHandler);
    }
});

onUnmounted(() => {
    window.removeEventListener("keydown", keydownHandler);
});

</script>
<style scoped>
.tab-panels {
    text-align: center;
}

.modifierRow {
    display: flex;
    align-items: center;
    margin-bottom: 0.5rem;
}

.modifireKeysLabel {
    display: inline-block;
    width: 70px;
    /* 根据实际需要调整宽度 */
    font-weight: bold;
    margin-right: 8px;
}
</style>

<style>
.p-tabpanel:focus {
    outline: none !important;
    border: none !important;
}
</style>
