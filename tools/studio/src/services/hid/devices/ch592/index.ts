import type { HidDevicePlugin } from '../../common/types';
import { Ch592HidAdapter } from './adapter';
import { getCh592UiDefinition } from './ui';

export { Ch592HidAdapter } from './adapter';
export { Ch592Codec } from './codec';
export { getCh592UiDefinition } from './ui';

export const ch592Plugin: HidDevicePlugin = {
  id: 'ch592',
  displayName: 'CH592F',
  createAdapter: () => new Ch592HidAdapter(),
  getUiDefinition: getCh592UiDefinition,
};
