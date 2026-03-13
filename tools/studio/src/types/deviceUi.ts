import type { DeviceCapabilities, DeviceProtocol } from './protocol';

export type DeviceUiSectionComponent =
  | 'device-info'
  | 'keyboard-status'
  | 'layer-panel'
  | 'fn-panel'
  | 'rgb-panel'
  | 'actions-panel'
  | 'debug-terminal';

export interface DeviceUiSectionDefinition {
  id: string;
  component: DeviceUiSectionComponent;
  order: number;
}

export interface DeviceUiDefinition {
  protocol: DeviceProtocol | 'preview';
  sections: DeviceUiSectionDefinition[];
}

export function createDeviceUiDefinition(
  protocol: DeviceProtocol | 'preview',
  components: DeviceUiSectionComponent[],
): DeviceUiDefinition {
  return {
    protocol,
    sections: components.map((component, index) => ({
      id: `${protocol}-${component}`,
      component,
      order: index,
    })),
  };
}

export function hasUiSection(definition: DeviceUiDefinition, component: DeviceUiSectionComponent): boolean {
  return definition.sections.some((section) => section.component === component);
}

export interface DeviceUiProvider {
  getUiDefinition(capabilities: DeviceCapabilities): DeviceUiDefinition;
}
