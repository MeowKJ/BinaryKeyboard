import { type DeviceCapabilities, DeviceProtocol } from '@/types/protocol';
import { createDeviceUiDefinition, type DeviceUiDefinition, type DeviceUiSectionComponent } from '@/types/deviceUi';

export function getCh552UiDefinition(capabilities: DeviceCapabilities): DeviceUiDefinition {
  const sections: DeviceUiSectionComponent[] = [
    'device-info',
    'keyboard-status',
    ...(capabilities.multiLayer ? ['layer-panel'] as const : []),
    ...(capabilities.rgb ? ['rgb-panel'] as const : []),
    'actions-panel',
    'debug-terminal',
  ];

  return createDeviceUiDefinition(DeviceProtocol.CH552, sections);
}
