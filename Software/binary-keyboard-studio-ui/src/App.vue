<template>
  <div class="app-container">
    <DeviceFinder :filters="[KEYBOARD_FILTER]" @on-device-connected="initDevice"
      v-show="!deviceStore.device || !deviceStore.device.opened" />

    <div class="device-info" v-if="deviceStore.device && deviceStore.device.opened">
      <h1>设备已连接: {{ deviceStore.device?.productName }} ( {{ deviceStore.getDeviceModelName() }} )</h1>
    </div>

    <KnobKeyboard v-if="deviceStore.device && (deviceStore.deviceModelNumber === KEYBOARD_MODEL.KNOB)"
      v-model:key-config-list="deviceStore.keyMappingsList" />

    <div class="send-button text-center" v-if="deviceStore.device">
      <Button style="width: 200px;" @click="sendDatatoDevice">发送数据</Button>
    </div>

    <DeviceInfoCard :deviceInfoList="deviceStore.getDeviceInfoList()" @onDisconnect="disconnectDevice"
      v-if="deviceStore.device" />
  </div>

</template>

<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { WebHidTools } from '@/utils/WebHidTools';
import { sendInitData, sendData } from '@/utils/keyboardHidTools';
import { CMD_KEYBOARD_SEND, KEYBOARD_FILTER, KEYBOARD_MODEL, REPORT_ID_RECEIVE, REPORT_ID_SEND } from '@/utils/deviceConstants';
import { useToast } from "primevue/usetoast";

import DeviceFinder from '@/components/DeviceFinder.vue';
import KnobKeyboard from '@/components/keyboards/KnobKeyboard.vue';
import DeviceInfoCard from '@/components/DeviceInfoCard.vue';
import { defineStore } from 'pinia';

const deviceStore = useDeviceStore();
const toast = useToast();

const testvalue = ref(0);

const initDevice = async () => {
  console.log('设备已连接:', deviceStore.device);
  if (deviceStore.device && deviceStore.device.opened) {
    deviceStore.device.addEventListener('inputreport', onGetReport);
    await sendInitData(deviceStore.device, REPORT_ID_SEND);
  }
};

const sendDatatoDevice = async () => {
  if (deviceStore.device && deviceStore.device.opened) {
    console.log('发送数据');
    const data = new Uint8Array(31);
    data[0] = CMD_KEYBOARD_SEND;
    data[1] = deviceStore.deviceFirmwareVersion;
    data[2] = deviceStore.deviceModelNumber;
    const keyMappingsData = deviceStore.getkeyMappingsListAsUint8Array();
    data.set(keyMappingsData, 3);
    try {
      await sendData(deviceStore.device, REPORT_ID_SEND, data);
    } catch (error) {
      console.error('发送数据失败:', error);
      toast.add({ severity: 'error', summary: '发送数据失败', detail: error });
    }
  }
};

const onGetReport = async (event: HIDInputReportEvent) => {
  if (event.reportId === REPORT_ID_RECEIVE) {
    const data = event.data;
    const dataView = new DataView(data.buffer);
    const command = dataView.getUint8(0);

    deviceStore.deviceFirmwareVersion = dataView.getUint8(1);
    deviceStore.deviceModelNumber = dataView.getUint8(2);

    const uint8ArrayData = new Uint8Array(data.buffer, 3, 24);
    deviceStore.setkeyMappingsListFromUint8Array(uint8ArrayData);

    const hexData = Array.from(uint8ArrayData).map(byte => byte.toString(16).padStart(2, '0')).join(' ');
    console.log('键盘数据 (16进制): ', hexData);
  }
};

// 监听设备断开事件
const onDeviceDisconnected = (event: HIDConnectionEvent) => {
  if (deviceStore.device && event.device === deviceStore.device) {
    console.log('设备已断开:', event.device.productName);
    toast.add({ severity: 'warn', summary: '设备断开', detail: '键盘连接已丢失' });
    deviceStore.device = null; // 清除设备信息
  }
};

// 断开设备连接
const disconnectDevice = async () => {
  if (deviceStore.device) {
    await deviceStore.device.close();
    deviceStore.device = null;
  }
};

// 组件挂载时监听 HID 设备连接状态
onMounted(async () => {
  navigator.hid.addEventListener("disconnect", onDeviceDisconnected);

  const devices = await navigator.hid.getDevices();
  const matchedDevice = devices.find(
    (device) =>
      device.vendorId === KEYBOARD_FILTER.vendorId &&
      device.productId === KEYBOARD_FILTER.productId
  );
  if (matchedDevice) {
    deviceStore.device = matchedDevice;
    if (!matchedDevice.opened) {
      await WebHidTools.openDevice(matchedDevice);
    }
    initDevice();
  }
});

// 组件卸载时移除监听
onUnmounted(() => {
  navigator.hid.removeEventListener("disconnect", onDeviceDisconnected);
});

</script>

<style scoped>
.app-container {
  text-align: center;
  display: flex;
  flex-direction: column;
  justify-content: center;
  gap: 50px;
}



.disconnect-button {
  margin-top: 10px;
  background-color: #e74c3c;
  color: white;
  border: none;
  padding: 10px 20px;
  cursor: pointer;
  border-radius: 5px;
}

.disconnect-button:hover {
  background-color: #c0392b;
}
</style>
