<template>
    <Card class="device-info">
        <template #header>
            <h2>映射信息</h2>
        </template>
        <template #content>
            <div class="card">
                <DataTable :value="deviceStore.getComparedKeyMappingsListAsString()" responsiveLayout="scroll">
                    <Column field="index" header="按键序号"></Column>
                    <Column field="oldTypeString" header="已写入类型"></Column>
                    <Column field="oldValue" header="已写入值"></Column>
                    <Column field="newTypeString" header="当前设定"></Column>
                    <Column field="newValue" header="当前设定值"></Column>
                </DataTable>
            </div>
        </template>
        <template #footer>
            <Button label="发送数据" class="send-button" @click="sendDatatoDevice"></Button>
        </template>
    </Card>
</template>

<script lang="ts" setup>
import { useDeviceStore } from '@/stores/deviceStore';

const emit = defineEmits(["onSend"]); // 使用 on 开头的事件名

const deviceStore = useDeviceStore();

const sendDatatoDevice = async () => {
    emit('onSend');
};

</script>

<style scoped>
.device-info {
    text-align: center;
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