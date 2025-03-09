import { defineStore } from "pinia";
import { ref } from "vue";
import {
  parseKeyMappingsFromUint8Array,
  convertKeyMappingsToUint8Array,
} from "@/utils/hidConverters/keyMappingConverter";

import { mediaDescriptions } from "@/utils/hidConverters/mediaHIDConverter";

import type { KeyMapping, ComparedKeyMappingString } from "@/types";
import { KEY_TYPE_KETBOARD } from "@/utils/deviceConstants";
import { getKeyNameCombination } from "@/utils/hidConverters/keyboardHIDConverter";
import { getMouseConfigString } from "@/utils/hidConverters/mouseHIDConverter";

export const useDeviceStore = defineStore("device", () => {
  // 设备信息
  const device = ref<HIDDevice | null>(null);
  const deviceFirmwareVersion = ref<number>(0);
  const deviceModelNumber = ref<number>(0);

  // 按键映射配置（初始化 8 个空配置）
  const keyMappingsList = ref<KeyMapping[]>(
    Array(8).fill({ type: 0, modifier: 0, key: 0 })
  );

  const keyMappingsListOriginal = ref<KeyMapping[]>(
    Array(8).fill({ type: 0, modifier: 0, key: 0 })
  );
  /**
   * 获取设备型号名称
   * @returns {string} 设备型号名称
   */
  const getDeviceModelName = () => {
    if (!deviceModelNumber.value) {
      return "未知设备";
    }
    switch (deviceModelNumber.value) {
      case 1:
        return "基础款";
      case 2:
        return "旋钮款";
      case 3:
        return "五键款";
      default:
        return "未知型号";
    }
  };

  /**
   * 获取设备信息列表
   * @returns {Array} 设备信息列表
   */
  const getDeviceInfoList = () => {
    return [
      { key: "型号名称", value: getDeviceModelName() },
      {
        key: "型号代码",
        value: `0x${deviceModelNumber.value
          .toString(16)
          .toUpperCase()
          .padStart(2, "0")}`,
      },
      {
        key: "固件版本",
        value: `0x${deviceFirmwareVersion.value
          .toString(16)
          .toUpperCase()
          .padStart(2, "0")}`,
      },
    ];
  };

  /**
   * 从 Uint8Array 数据设置按键映射
   * @param {Uint8Array} data 数据数组
   */
  const setkeyMappingsListFromUint8Array = (data: Uint8Array) => {
    keyMappingsList.value = parseKeyMappingsFromUint8Array(data);
    keyMappingsListOriginal.value = parseKeyMappingsFromUint8Array(data);
  };

  /**
   * 获取当前的按键映射数据
   * @returns {Uint8Array} 当前的按键映射数据
   */
  const getkeyMappingsListAsUint8Array = () => {
    return convertKeyMappingsToUint8Array(keyMappingsList.value);
  };

  /**
   * 获取当前的按键映射数据的字符串表示
   */
  const getComparedKeyMappingsListAsString = (): ComparedKeyMappingString[] => {
    let KeyMappingStringList: ComparedKeyMappingString[] = [];
    const length = keyMappingsListOriginal.value.length;

    for (let i = 0; i < length; i++) {
      const km = keyMappingsListOriginal.value[i];
      const kmNew = keyMappingsList.value[i];

      const oldMapping: ComparedKeyMappingString = {
        index: i,
        oldTypeString: "",
        oldValue: "",
        newTypeString: "",
        newValue: "",
      };

      switch (km.type) {
        case KEY_TYPE_KETBOARD:
          oldMapping.oldTypeString = "键盘";
          oldMapping.oldValue = getKeyNameCombination(km.value);
          break;
        case 2:
          oldMapping.oldTypeString = "多媒体";
          oldMapping.oldValue = mediaDescriptions[km.value.key];
          break;
        case 3:
          oldMapping.oldTypeString = "鼠标";
          oldMapping.oldValue = getMouseConfigString(km.value);
          break;
        default:
          oldMapping.oldTypeString = "未知";
          oldMapping.oldValue = "";
          break;
      }

      switch (kmNew.type) {
        case KEY_TYPE_KETBOARD:
          oldMapping.newTypeString = "键盘";
          oldMapping.newValue = getKeyNameCombination(kmNew.value);
          break;
        case 2:
          oldMapping.newTypeString = "多媒体";
          oldMapping.newValue = mediaDescriptions[kmNew.value.key];
          break;
        case 3:
          oldMapping.newTypeString = "鼠标";
          oldMapping.newValue = getMouseConfigString(kmNew.value);
          break;
        default:
          oldMapping.newTypeString = "未知";
          oldMapping.newValue = "";
          break;
      }

      KeyMappingStringList.push(oldMapping);
    }

    return KeyMappingStringList;
  };

  return {
    device,
    deviceFirmwareVersion,
    deviceModelNumber,
    keyMappingsList,
    keyMappingsListOriginal,
    getDeviceModelName,
    getDeviceInfoList,
    setkeyMappingsListFromUint8Array,
    getkeyMappingsListAsUint8Array,
    getComparedKeyMappingsListAsString,
  };
});
