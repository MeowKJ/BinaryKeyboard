<template>
    <Button :label="label" :class="getClassName()" severity="secondary" @click="openDialog" raised />

    <Dialog v-model:visible="helpDialogVisible" header="改键帮助" :style="{ width: '25rem' }">

        <Panel header="按键类型" class="mb-4" toggleable>
            <p class="m-0">
                目前支持的按键类型有键盘按键、多媒体按键、鼠标按键。
            </p>
        </Panel>
        <Panel header="键盘按键" class="mb-4" toggleable>
            <p class="m-0">
                从键盘录入一个按键。你可以点击下方的按钮为其添加一个或多个修饰键。
            </p>
        </Panel>

        <Panel header="多媒体按键" class="mb-4" toggleable>
            <p class="m-0">
                从列举出的多媒体按钮中挑选一个。
            </p>
        </Panel>

        <Panel header="鼠标按键" class="mb-4" toggleable>
            <p class="m-0">
                从列举出的鼠标按钮中挑选一个。
            </p>
        </Panel>
    </Dialog>

    <Dialog v-model:visible="visible" :style="{ width: '25rem' }" :close-on-escape=false>

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
                        <p><Button :label="keyboardConfig.key || keyboardConfig.code || '无输入'" variant="outlined"
                                size="large" /></p>
                        <p class="modifierRow">
                            <span class="modifireKeysLabel">Left: </span>
                            <SelectButton v-model="leftModifierKeysValue" :options="modifierKeysList" multiple />
                        </p>
                        <p class="modifierRow">
                            <span class="modifireKeysLabel">Right: </span>
                            <SelectButton v-model="rightModifierKeysValue" :options="modifierKeysList" multiple />
                        </p>
                        <p>数据: {{ getKeyNameCombination() }}</p>
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
            <Button label="帮助" text severity="info" @click="helpDialogVisible = true" autofocus />
        </template>
    </Dialog>
</template>

<script setup lang="ts">
import { eventConnector } from '@primeuix/themes/aura/timeline';
import { computed, ref, watch, onMounted, onUnmounted } from 'vue';

const tapValue = ref("0");
const visible = ref(false);
const helpDialogVisible = ref(false);

const modifierKeysList = ["Ctrl", "Alt", "Shift", "Meta"];

const keyboardConfig = ref({
    key: "",
    code: "",
    leftMetaKey: false,
    leftCtrlKey: false,
    leftAltKey: false,
    leftShiftKey: false,
    rightMetaKey: false,
    rightCtrlKey: false,
    rightAltKey: false,
    rightShiftKey: false,
});

const leftModifierKeysValue = computed({
    get: () => {
        const keys: string[] = [];
        if (keyboardConfig.value.leftMetaKey) keys.push("Meta");
        if (keyboardConfig.value.leftCtrlKey) keys.push("Ctrl");
        if (keyboardConfig.value.leftAltKey) keys.push("Alt");
        if (keyboardConfig.value.leftShiftKey) keys.push("Shift");
        return keys;
    },
    set: (value: string[]) => {
        keyboardConfig.value.leftMetaKey = value.includes("Meta");
        keyboardConfig.value.leftCtrlKey = value.includes("Ctrl");
        keyboardConfig.value.leftAltKey = value.includes("Alt");
        keyboardConfig.value.leftShiftKey = value.includes("Shift");
    },
});

const rightModifierKeysValue = computed({
    get: () => {
        const keys: string[] = [];
        if (keyboardConfig.value.rightMetaKey) keys.push("Meta");
        if (keyboardConfig.value.rightCtrlKey) keys.push("Ctrl");
        if (keyboardConfig.value.rightAltKey) keys.push("Alt");
        if (keyboardConfig.value.rightShiftKey) keys.push("Shift");
        return keys;
    },
    set: (value: string[]) => {
        keyboardConfig.value.rightMetaKey = value.includes("Meta");
        keyboardConfig.value.rightCtrlKey = value.includes("Ctrl");
        keyboardConfig.value.rightAltKey = value.includes("Alt");
        keyboardConfig.value.rightShiftKey = value.includes("Shift");
    },
});
const getKeyNameCombination = () => {
    const keys = new Set<string>(); // 使用 Set 自动去重

    leftModifierKeysValue.value.forEach((key) => keys.add(`L${key}`));
    rightModifierKeysValue.value.forEach((key) => keys.add(`R${key}`));

    if (keyboardConfig.value.key) {
        keys.add(keyboardConfig.value.code); // 加入主按键
    }

    return Array.from(keys).join(" + ");
};


let lastLocation = 0;
let lastKey = "";
const keydownHandler = (event: KeyboardEvent) => {
    // 如果不是修饰键，就更新键盘配置
    if (!['Control', 'Alt', "Shift", "Meta"].includes(event.key)) {
        keyboardConfig.value.key = event.key;
        keyboardConfig.value.code = event.code;
    }

    if (event.location === 2) {
        keyboardConfig.value.rightMetaKey = event.metaKey;
        keyboardConfig.value.rightCtrlKey = event.ctrlKey;
        keyboardConfig.value.rightAltKey = event.altKey;
        keyboardConfig.value.rightShiftKey = event.shiftKey;
    } else if (event.location === 1) {
        keyboardConfig.value.leftMetaKey = event.metaKey;
        keyboardConfig.value.leftCtrlKey = event.ctrlKey;
        keyboardConfig.value.leftAltKey = event.altKey;
        keyboardConfig.value.leftShiftKey = event.shiftKey;

    } else {
        // RShift 的 location 为 0
        if (lastLocation === 1) {
            clearKeyboardModifierKeys('right');
            keyboardConfig.value.leftMetaKey = event.metaKey;
            keyboardConfig.value.leftCtrlKey = event.ctrlKey;
            keyboardConfig.value.leftAltKey = event.altKey;
            keyboardConfig.value.leftShiftKey = event.shiftKey;

        } else if (lastLocation === 2) {
            clearKeyboardModifierKeys('left');
            keyboardConfig.value.rightMetaKey = event.metaKey;
            keyboardConfig.value.rightCtrlKey = event.ctrlKey;
            keyboardConfig.value.rightAltKey = event.altKey;
            //keyboardConfig.value.rightShiftKey = event.shiftKey;
        } else {
            if (lastKey === "Shift" || event.key === "Shift"
            ) {
                clearKeyboardModifierKeys('left');
                keyboardConfig.value.rightShiftKey = true;
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
        keyboardConfig.value.leftMetaKey = false;
        keyboardConfig.value.leftCtrlKey = false;
        keyboardConfig.value.leftAltKey = false;
        keyboardConfig.value.leftShiftKey = false;
    } else if (type === 'right') {
        keyboardConfig.value.rightMetaKey = false;
        keyboardConfig.value.rightCtrlKey = false;
        keyboardConfig.value.rightAltKey = false;
        keyboardConfig.value.rightShiftKey = false;
    } else {
        keyboardConfig.value.leftMetaKey = false;
        keyboardConfig.value.leftCtrlKey = false;
        keyboardConfig.value.leftAltKey = false;
        keyboardConfig.value.leftShiftKey = false;
        keyboardConfig.value.rightMetaKey = false;
        keyboardConfig.value.rightCtrlKey = false;
        keyboardConfig.value.rightAltKey = false;
        keyboardConfig.value.rightShiftKey = false;
    }

};

// 监听对话框状态
const openDialog = () => {
    visible.value = true;
};

watch(visible, (newVal) => {
    if (newVal) {
        window.addEventListener("keydown", keydownHandler);
    } else {
        window.removeEventListener("keydown", keydownHandler);
    }
});

onUnmounted(() => {
    window.removeEventListener("keydown", keydownHandler);
});

const props = defineProps<{ label: string; type: "circle" | "square" | "vertical-bar" | "horizontal-bar" }>();

const getClassName = () => {
    switch (props.type) {
        case "circle":
            return "round-button";
        case "square":
            return "square-button";
        case "vertical-bar":
            return "tall-button";
        case "horizontal-bar":
            return "wide-button";
    }
};
</script>

<style scoped>
.square-button,
.round-button,
.tall-button {
    font-size: 1.5em;
}

.round-button {
    border-radius: 50%;
}

.tall-button {
    grid-row: span 2;
}

.wide-button {
    grid-column: span 2;
}

.tab-panels {
    text-align: center;
}

.modifierRow {
    display: flex;
    align-items: center;
    margin-bottom: 0.5rem;
    /* 可选，增加行间距 */
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
