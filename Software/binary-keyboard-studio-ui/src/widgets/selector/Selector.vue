<template>

    <Dialog v-model:visible="dialogVisible" :style="{ width: '25rem' }" :close-on-escape="closeOnEscape">

        <template #header>
            <span class="font-bold" style="font-size: 1.5em;">设置键位</span>
        </template>


        <Tabs v-model:value="tapValue">
            <TabList>
                <Tab value="0">键盘</Tab>
                <Tab value="1">多媒体</Tab>
                <Tab value="2">鼠标</Tab>
                <Tab value="3">宏</Tab>
            </TabList>
            <TabPanels>
                <TabPanel value="0">
                    <KeyboardSelector v-model:keyConfig="keyConfig" :tapValue="tapValue"
                        @on-keyboard-listen="onKeyboardListen" />
                </TabPanel>
                <TabPanel value="1">
                    <MediaSelector v-model:mediaConfig="mediaConfig" />
                </TabPanel>
                <TabPanel value="2">
                    <MouseSelector v-model:mouseConfig="mouseConfig" />
                </TabPanel>
                <TabPanel value="3">
                    <p class="m-0 text-center">
                        <span>该设备或固件不支持宏功能。</span>
                        <Divider />
                        <Tag>
                            {{ getProductStr() }}
                        </Tag>
                    </p>
                </TabPanel>
            </TabPanels>
        </Tabs>
        <template #footer>
        </template>
    </Dialog>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';

import { type KeyboardConfig, type MouseConfig, type MediaConfig, MouseButtonHID, MediaHIDCode, KeyType } from '@/types';
import KeyboardSelector from '@/widgets/selector/KeyboardSelector.vue';


import { useDeviceStore } from '@/stores/deviceStore';
import MouseSelector from './MouseSelector.vue';
import MediaSelector from './MediaSelector.vue';

const props = defineProps<{ visible: boolean; currentIndex: number }>();
const emit = defineEmits(["update:visible"]);
const deviceStore = useDeviceStore();

const getProductStr = () => {
    let str = deviceStore.device?.productName + ' - ' + deviceStore.getDeviceModelName() + ' - ' + deviceStore.getDeviceInfoList()[2].value;
    return str
}

const closeOnEscape = ref(true);

const onKeyboardListen = (v: boolean) => {
    closeOnEscape.value = !v;
};

const dialogVisible = computed({
    get: () => {
        if (props.visible) onDialogOpen();
        return props.visible
    },
    set: (val) => {
        emit("update:visible", val);
        if (!val) onDialogClose();
    },
});


const onDialogOpen = () => {
    // 载入键位配置
    const currentKeyMapping = deviceStore.keyMappingsList[props.currentIndex];
    switch (currentKeyMapping.type) {
        case KeyType.KEBOARD:
            tapValue.value = "0";
            keyConfig.value = currentKeyMapping.value
            break;
        case KeyType.MEDIA:
            tapValue.value = "1";
            mediaConfig.value = currentKeyMapping.value
            break;
        case KeyType.MOUSE:
            tapValue.value = "2";
            mouseConfig.value = currentKeyMapping.value
            break;

    }

}
const onDialogClose = () => {
    // 保存键位配置
    switch (tapValue.value) {
        case "0":
            deviceStore.keyMappingsList[props.currentIndex] = {
                type: KeyType.KEBOARD,
                value: keyConfig.value
            };
            break;
        case "1":
            deviceStore.keyMappingsList[props.currentIndex] = {
                type: KeyType.MEDIA,
                value: mediaConfig.value
            };
            break;
        case "2":
            deviceStore.keyMappingsList[props.currentIndex] = {
                type: KeyType.MOUSE,
                value: mouseConfig.value
            };
            break;
    }
};


const tapValue = ref("0");
// --------------处理多媒体配置--------------
const mediaConfig = ref<MediaConfig>({
    key: MediaHIDCode.None,
});

// --------------处理鼠标配置--------------
const mouseConfig = ref<MouseConfig>({
    button: MouseButtonHID.None,
    wheel: 0,
})

// --------------处理键盘配置--------------
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



</script>
<style scoped>
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

.tab-panels {
    text-align: center;
}
</style>
