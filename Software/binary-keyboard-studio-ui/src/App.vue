<template>
  <div class="app-container">
    <Toast />
    <div class="flex flex-column justify-center gap-6" v-if="deviceStore.device">
      <div class="text-center">
        <h1>设备已连接: {{ deviceStore.device?.productName }} ({{ deviceStore.getDeviceModelName() }})</h1>
      </div>

      <KnobKeyboard v-if="deviceStore.deviceModelNumber === KEYBOARD_MODEL.KNOB"
        v-model:key-config-list="deviceStore.keyMappingsList" />
      <DataInfoCard @onSend="sendDatatoDevice" />
      <Divider />
      <DeviceInfoCard :deviceInfoList="deviceStore.getDeviceInfoList()" @onDisconnect="disconnectDevice" />
    </div>

    <DeviceFinder :filters="[KEYBOARD_FILTER]" @on-device-connected="initDevice" v-else />
    <AppFooter />
  </div>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue';
import { useToast } from "primevue/usetoast";
import { useDeviceStore } from '@/stores/deviceStore';
import { WebHidTools } from '@/utils/WebHidTools';
import { sendInitData, sendData } from '@/utils/keyboardHidTools';
import { CMD_KEYBOARD_SEND, KEYBOARD_FILTER, KEYBOARD_MODEL, REPORT_ID_RECEIVE, REPORT_ID_SEND } from '@/utils/deviceConstants';
import DeviceFinder from '@/components/DeviceFinder.vue';
import KnobKeyboard from '@/components/keyboards/KnobKeyboard.vue';
import DeviceInfoCard from '@/components/DeviceInfoCard.vue';
import DataInfoCard from './components/DataInfoCard.vue';

const toast = useToast();
const deviceStore = useDeviceStore();

const showToast = (severity: 'success' | 'info' | 'warn' | 'error' | 'secondary' | 'contrast', summary: string, detail: string) => {
  toast.add({ severity, summary, detail, life: 1500 });
};

const initDevice = async () => {
  if (!deviceStore.device) return;
  console.log('设备已连接:', deviceStore.device);
  if (deviceStore.device.opened) {
    deviceStore.device.addEventListener('inputreport', onGetReport);
    await sendInitData(deviceStore.device, REPORT_ID_SEND);
  }
};

const sendDatatoDevice = async () => {
  if (!deviceStore.device?.opened) return;
  console.log('发送数据');
  const data = new Uint8Array(31);
  data.set([CMD_KEYBOARD_SEND, deviceStore.deviceFirmwareVersion, deviceStore.deviceModelNumber]);
  data.set(deviceStore.getkeyMappingsListAsUint8Array(), 3);
  try {
    await sendData(deviceStore.device, REPORT_ID_SEND, data);
    showToast('success', '发送数据成功', '数据已发送到设备');
    initDevice();
  } catch (error: any) {
    console.error('发送数据失败:', error);
    showToast('error', '发送数据失败', error.message || '未知错误');
  }
};

const updateDeviceData = (dataView: DataView) => {
  deviceStore.deviceFirmwareVersion = dataView.getUint8(1);
  deviceStore.deviceModelNumber = dataView.getUint8(2);
  deviceStore.setkeyMappingsListFromUint8Array(new Uint8Array(dataView.buffer, 3, 24));
  console.log('键盘数据 (16进制):', [...new Uint8Array(dataView.buffer, 3, 24)].map(byte => byte.toString(16).padStart(2, '0')).join(' '));
};

const onGetReport = (event: HIDInputReportEvent) => {
  if (event.reportId === REPORT_ID_RECEIVE) {
    updateDeviceData(new DataView(event.data.buffer));
  }
};

const onDeviceDisconnected = (event: HIDConnectionEvent) => {
  if (event.device === deviceStore.device) {
    console.log('设备已断开:', event.device.productName);
    showToast('warn', '设备断开', '键盘连接已丢失');
    deviceStore.device = null;
  }
};

const disconnectDevice = async () => {
  if (deviceStore.device) {
    await deviceStore.device.close();
    deviceStore.device = null;
  }
};

const autoConnectDevice = async () => {
  showToast('info', '正在连接设备', '尝试连接键盘设备');
  const matchedDevice = (await navigator.hid.getDevices()).find(device =>
    device.vendorId === KEYBOARD_FILTER.vendorId && device.productId === KEYBOARD_FILTER.productId
  );
  if (!matchedDevice) return;
  deviceStore.device = matchedDevice;
  if (!matchedDevice.opened && !(await WebHidTools.openDevice(matchedDevice))) {
    return showToast('error', '连接设备失败', '无法打开设备');
  }
  showToast('success', '连接设备成功', '设备已连接');
  initDevice();
};

onMounted(async () => {
  navigator.hid.addEventListener("disconnect", onDeviceDisconnected);
  await autoConnectDevice();
});

onUnmounted(() => {
  navigator.hid.removeEventListener("disconnect", onDeviceDisconnected);
});
</script>

<style scoped>
.app-container {
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
}
</style>