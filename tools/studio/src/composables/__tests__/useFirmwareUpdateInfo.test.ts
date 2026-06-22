import { describe, expect, it, vi, beforeEach } from 'vitest';
import { useFirmwareUpdateInfo } from '@/composables/useFirmwareUpdateInfo';
import { DeviceProtocol } from '@/types/protocol';

const mocks = vi.hoisted(() => ({
  deviceStore: {
    deviceInfo: { protocol: 'ch592' },
    isDevFirmware: false,
    firmwareVersion: '1.0.0',
  },
  releaseStore: {
    latestVersions: {
      ch552: '2.0.0',
      ch592: '3.1.36',
    },
  },
}));

vi.mock('@/stores/deviceStore', () => ({
  useDeviceStore: () => mocks.deviceStore,
}));

vi.mock('@/stores/releaseStore', () => ({
  useReleaseStore: () => mocks.releaseStore,
}));

describe('useFirmwareUpdateInfo', () => {
  beforeEach(() => {
    mocks.deviceStore.deviceInfo = { protocol: DeviceProtocol.CH592 };
    mocks.deviceStore.isDevFirmware = false;
    mocks.deviceStore.firmwareVersion = '1.0.0';
    mocks.releaseStore.latestVersions.ch552 = '2.0.0';
    mocks.releaseStore.latestVersions.ch592 = '3.1.36';
  });

  it('offers normal firmware upgrades by version comparison', () => {
    const info = useFirmwareUpdateInfo();

    expect(info.latestFirmwareVersion.value).toBe('3.1.36');
    expect(info.firmwareUpdateKind.value).toBe('upgrade');
    expect(info.firmwareUpdateAvailable.value).toBe(true);
    expect(info.firmwareUpdateTooltip.value).toBe('可下载到 v3.1.36');
  });

  it('offers release flashing when the connected firmware is dev', () => {
    mocks.deviceStore.isDevFirmware = true;
    mocks.deviceStore.firmwareVersion = 'dev';

    const info = useFirmwareUpdateInfo();

    expect(info.firmwareUpdateKind.value).toBe('dev-to-release');
    expect(info.firmwareUpdateAvailable.value).toBe(true);
    expect(info.firmwareUpdateTooltip.value).toBe('可刷正式版 v3.1.36');
    expect(info.firmwareStateLabel.value).toBe('dev，可刷正式版 v3.1.36');
  });

  it('does not offer an update when the release version is not newer', () => {
    mocks.deviceStore.firmwareVersion = '3.1.36';

    const info = useFirmwareUpdateInfo();

    expect(info.firmwareUpdateKind.value).toBe('none');
    expect(info.firmwareUpdateAvailable.value).toBe(false);
    expect(info.firmwareStateLabel.value).toBe('固件已是最新');
  });
});
