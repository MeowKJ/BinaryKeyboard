<template>
    <Divider />

    <Card class="device-info">
        <template #header>
            <h2>设备信息</h2>
        </template>
        <template #content>
            <div class="card">
                <DataTable :value="deviceInfoList">
                    <Column field="key" header="属性"></Column>
                    <Column field="value" header="值"></Column>
                </DataTable>
            </div>
        </template>
        <template #footer>
            <Button label="断开设备" class="disconnect-button" @click="$emit('onDisconnect')"></Button>
        </template>
    </Card>
</template>

<script lang="ts" setup>
defineProps({
    deviceInfoList: Array, // 设备信息列表
});

defineEmits(["onDisconnect"]); // 使用 on 开头的事件名
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