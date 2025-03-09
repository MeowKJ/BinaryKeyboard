<template>
    <Card class="device-info">
        <template #header>
            <h2>映射信息</h2>
        </template>
        <template #content>
            <div class="card">
                <DataTable :value="comparedKeyMappings" tableStyle="min-width: 50rem" responsiveLayout="scroll">
                    <Column field="index" header="按键序号"></Column>
                    <Column field="oldTypeString" header="已写入类型"></Column>
                    <Column field="oldValue" header="已写入值"></Column>
                    <Column field="newTypeString" header="当前设定类型">
                        <template #body="slotProps1">
                            <Badge :value="slotProps1.data.newTypeString"
                                :severity="stockSeverityType(slotProps1.data)" />
                        </template>
                    </Column>
                    <Column field="newValue" header="当前设定值">

                        <template #body="slotProps2">
                            <Badge :value="slotProps2.data.newValue" :severity="stockSeverityValue(slotProps2.data)" />
                        </template>
                    </Column>
                </DataTable>
            </div>
        </template>
        <template #footer>
            <Button label="发送数据" class="send-button" @click="sendDataToDevice"></Button>
        </template>
    </Card>
</template>

<script lang="ts" setup>
import { computed } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';

const emit = defineEmits(["onSend"]);
const deviceStore = useDeviceStore();

// 计算属性，避免每次渲染时都调用函数
const comparedKeyMappings = computed(() => deviceStore.getComparedKeyMappingsListAsString());

const stockSeverityValue = (data: any) => {
    if (data.newValue !== data.oldValue) return 'warn';
    else return 'success';
}
const stockSeverityType = (data: any) => {
    if (data.newTypeString !== data.oldTypeString) return 'warn';
    else return 'success';
}


const sendDataToDevice = () => {
    emit('onSend');
};
</script>

<style scoped>
.device-info {
    text-align: center;
    min-width: 300px;
    margin: 0 auto;
    margin-top: 20px;
}

@media (min-width: 768px) {
    .device-info {
        min-width: 450px;
    }
}

/* 样式可以根据具体需要进行调整 */
</style>
