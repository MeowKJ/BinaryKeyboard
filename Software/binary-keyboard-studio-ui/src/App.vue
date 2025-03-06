<template>
  <div class="app-container">
    <DeviceFinder :filters="[KEYBOARD_FILTER]" @on-device-connected="initDevice" v-show="!deviceStore.device" />

    <div class="device-info" v-show="deviceStore.device">
      <h1>设备已连接: {{ deviceStore.device?.productName }} ( {{ deviceStore.getDeviceModelName() }} )</h1>
    </div>

    <KnobKeyboard v-if="deviceStore.device && (deviceStore.deviceModelNumber === KEYBOARD_MODEL.KNOB)"
      v-model:key-config-list="deviceStore.keyboardKeyMappings" />

    <DeviceInfoCard :deviceInfoList="deviceStore.getDeviceInfoList()" @onDisconnect="disconnectDevice"
      v-show="deviceStore.device" />

  </div>

  <p> {{ testvalue }}</p>


</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue';
import DeviceFinder from '@/components/DeviceFinder.vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { WebHidTools } from '@/utils/WebHidTools';
import { sendInitData } from '@/utils/keyboardHidTools';
import KnobKeyboard from '@/components/keyboards/KnobKeyboard.vue';
import { KEYBOARD_FILTER, KEYBOARD_MODEL, REPORT_ID_RECEIVE, REPORT_ID_SEND } from '@/utils/deviceConstants';

const deviceStore = useDeviceStore();

const testvalue = ref(0);

const initDevice = async () => {
  console.log('设备已连接:', deviceStore.device);
  if (deviceStore.device && deviceStore.device.opened) {
    //发送第一位为0x01的读取指令, 数据长度31位, 其余为0x00
    deviceStore.device.addEventListener('inputreport', onGetReport);
    await sendInitData(deviceStore.device, REPORT_ID_SEND);

  }
}


const onGetReport = async (event: HIDInputReportEvent) => {
  if (event.reportId === REPORT_ID_RECEIVE) {
    console.log('接收到键盘数据:', event);
    const data = event.data;
    const dataView = new DataView(data.buffer);
    //第一位 Command 指令

    const command = dataView.getUint8(0);

    deviceStore.deviceFirmwareVersion = dataView.getUint8(1);
    deviceStore.deviceModelNumber = dataView.getUint8(2);
    //从第三位开始为键盘映射数据，长度24位
    const uint8ArrayData = new Uint8Array(data.buffer, 3, 24);
    deviceStore.setKeyboardKeyMappingsFromUint8Array(uint8ArrayData);

    console.log('键盘数据: ', dataView);
  }
}

// 挂载时执行
onMounted(async () => {
  const devices = await navigator.hid.getDevices();
  // 查找匹配的设备
  const matchedDevice = devices.find(
    (device) =>
      device.vendorId === KEYBOARD_FILTER.vendorId &&
      device.productId === KEYBOARD_FILTER.productId
  );
  if (matchedDevice) {
    deviceStore.device = matchedDevice; // 存储设备对象
    if (!matchedDevice.opened) {
      await WebHidTools.openDevice(matchedDevice); // 打开设备
    }
    initDevice(); // 初始化设备
  }
});

// 断开设备连接
const disconnectDevice = async () => {
  if (deviceStore.device) {
    await deviceStore.device.close(); // 关闭设备
    deviceStore.device = null; // 清除设备信息
  }
};

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
