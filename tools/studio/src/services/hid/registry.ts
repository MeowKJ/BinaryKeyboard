import type { HidAdapter, HidDevicePlugin } from './common/types';
import type { DeviceProtocol } from '@/types/protocol';
import { ch552Plugin } from './devices/ch552';
import { ch592Plugin } from './devices/ch592';

export const HID_DEVICE_PLUGINS = [
  ch592Plugin,
  ch552Plugin,
];

export function createHidAdapters(): HidAdapter[] {
  return HID_DEVICE_PLUGINS.map((plugin) => plugin.createAdapter());
}

export function getHidDevicePlugin(protocol: DeviceProtocol): HidDevicePlugin | undefined {
  return HID_DEVICE_PLUGINS.find((plugin) => plugin.createAdapter().protocol === protocol);
}
