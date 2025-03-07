<template>
    <Card class="device-info">
        <template #header>
            <h2>设备信息</h2>
        </template>
        <template #content>
            <div class="card">
                <DataTable :value="deviceDataList">
                    <Column field="key" header="按键序号"></Column>
                    <Column field="oldValue" header="读取的值"></Column>
                    <Column field="newValue" header="已设定的值"></Column>
                </DataTable>
            </div>
        </template>
        <template #footer>
            <Button label="更新数据" class="disconnect-button" @click="$emit('onSend')"></Button>
        </template>
    </Card>
</template>

<script lang="ts" setup>
import { computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';

const emit = defineEmits(["onSend"]); // 使用 on 开头的事件名

const deviceStore = useDeviceStore();

const deviceDataList = computed(() => {
    let deviceDataList = [];
    for (let i = 0; i < 8; i++) {
        deviceDataList.push({
            key: i,
            oldValue: deviceStore.keyMappingsListOriginal[i],
            newValue: deviceStore.keyMappingsList[i],
        });
    }
    return deviceDataList;
});

</script>

<style scoped>
.device-info {
    min-width: 300px;
    margin-top: 20px;
    margin: 0 auto;
}

@media (min-width: 768px) {
    .device-info {
        min-width: 450px;
    }
}
</style>