import type { HidDevicePlugin } from '../../common/types';
import { Ch552HidAdapter } from './adapter';
import { getCh552UiDefinition } from './ui';

export { Ch552HidAdapter } from './adapter';
export { Ch552Codec } from './codec';
export { getCh552UiDefinition } from './ui';

export const ch552Plugin: HidDevicePlugin = {
  id: 'ch552',
  displayName: 'CH552G',
  createAdapter: () => new Ch552HidAdapter(),
  getUiDefinition: getCh552UiDefinition,
};
