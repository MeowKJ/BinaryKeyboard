import { defineStore } from "pinia";
import { ref } from "vue";
import {
  parseKeyMappingsFromUint8Array,
  convertKeyMappingsToUint8Array,
} from "@/utils/hidConverters/keyMappingConverter";

import { mediaDescriptions } from "@/utils/hidConverters/mediaHIDConverter";

import type { KeyMapping, ComparedKeyMappingString } from "@/types";
import { KeyType } from "@/types";
import { getKeyNameCombination } from "@/utils/hidConverters/keyboardHIDConverter";
import { getMouseConfigString } from "@/utils/hidConverters/mouseHIDConverter";

export const useDeviceStore = defineStore("device", () => {
  // 设备信息
  const device = ref<HIDDevice | null>(null);
  const deviceFirmwareVersion = ref<number>(0);
  const deviceModelNumber = ref<number>(0);

  // 按键映射配置（初始化 8 个空配置）
const createDefaultKeyMapping = (): KeyMapping => ({
    type: KeyType.KEBOARD,
    value: {
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
    }
});

// 3. 使用 Array.from 生成独立的副本
const keyMappingsList = ref<KeyMapping[]>(
    Array.from({ length: 8 }, () => createDefaultKeyMapping())
);

const keyMappingsListOriginal = ref<KeyMapping[]>(
    Array.from({ length: 8 }, () => createDefaultKeyMapping())
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
      case 4:
        return "无线-基础款";
      case 5:
        return "无线-旋钮款";
      case 6:
        return "无线-五键款";
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
        case KeyType.KEBOARD:
          oldMapping.oldTypeString = "键盘";
          oldMapping.oldValue = getKeyNameCombination(km.value);
          break;
        case KeyType.MEDIA:
          oldMapping.oldTypeString = "多媒体";
          oldMapping.oldValue = mediaDescriptions[km.value.key];
          break;
        case KeyType.MOUSE:
          oldMapping.oldTypeString = "鼠标";
          oldMapping.oldValue = getMouseConfigString(km.value);
          break;
        default:
          oldMapping.oldTypeString = "未知";
          oldMapping.oldValue = "";
          break;
      }

      switch (kmNew.type) {
        case KeyType.KEBOARD:
          oldMapping.newTypeString = "键盘";
          oldMapping.newValue = getKeyNameCombination(kmNew.value);
          break;
        case KeyType.MEDIA:
          oldMapping.newTypeString = "多媒体";
          oldMapping.newValue = mediaDescriptions[kmNew.value.key];
          break;
        case KeyType.MOUSE:
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
