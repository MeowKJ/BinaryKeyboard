import { type DeviceCapabilities, DeviceProtocol } from '@/types/protocol';
import { createDeviceUiDefinition, type DeviceUiDefinition, type DeviceUiSectionComponent } from '@/types/deviceUi';

export function getCh592UiDefinition(capabilities: DeviceCapabilities): DeviceUiDefinition {
  const sections: DeviceUiSectionComponent[] = [
    'device-info',
    'keyboard-status',
    ...(capabilities.multiLayer ? ['layer-panel'] as const : []),
    ...(capabilities.fnKeys ? ['fn-panel'] as const : []),
    ...(capabilities.rgb ? ['rgb-panel'] as const : []),
    'actions-panel',
    'debug-terminal',
  ];

  return createDeviceUiDefinition(DeviceProtocol.CH592, sections);
}
