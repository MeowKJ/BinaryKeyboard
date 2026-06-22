import { computed, toValue, type MaybeRefOrGetter } from 'vue';
import { useDeviceStore } from '@/stores/deviceStore';
import { useReleaseStore } from '@/stores/releaseStore';
import { DeviceProtocol } from '@/types/protocol';
import { isNewerVersion } from '@/utils/version';

export type FirmwareUpdateKind = 'none' | 'upgrade' | 'dev-to-release';

type FirmwareUpdateInfoOptions = {
  disabled?: MaybeRefOrGetter<boolean>;
};

export function useFirmwareUpdateInfo(options: FirmwareUpdateInfoOptions = {}) {
  const deviceStore = useDeviceStore();
  const releaseStore = useReleaseStore();

  const latestFirmwareVersion = computed(() => {
    const protocol = deviceStore.deviceInfo?.protocol;
    if (protocol === DeviceProtocol.CH552) return releaseStore.latestVersions.ch552;
    if (protocol === DeviceProtocol.CH592) return releaseStore.latestVersions.ch592;
    return '';
  });

  const firmwareUpdateKind = computed<FirmwareUpdateKind>(() => {
    if (toValue(options.disabled ?? false)) return 'none';
    if (!deviceStore.deviceInfo || !latestFirmwareVersion.value) return 'none';
    if (deviceStore.isDevFirmware) return 'dev-to-release';
    return isNewerVersion(latestFirmwareVersion.value, deviceStore.firmwareVersion) ? 'upgrade' : 'none';
  });

  const firmwareUpdateAvailable = computed(() => firmwareUpdateKind.value !== 'none');

  const firmwareUpdateTooltip = computed(() => {
    if (!latestFirmwareVersion.value) return '暂无可下载版本';
    if (firmwareUpdateKind.value === 'dev-to-release') return `可刷正式版 v${latestFirmwareVersion.value}`;
    return `可下载到 v${latestFirmwareVersion.value}`;
  });

  const firmwareStateLabel = computed(() => {
    if (!deviceStore.deviceInfo) return '未连接';
    if (firmwareUpdateKind.value === 'dev-to-release') return `dev，可刷正式版 v${latestFirmwareVersion.value}`;
    if (firmwareUpdateAvailable.value) return `可下载 v${latestFirmwareVersion.value}`;
    return '固件已是最新';
  });

  return {
    latestFirmwareVersion,
    firmwareUpdateKind,
    firmwareUpdateAvailable,
    firmwareUpdateTooltip,
    firmwareStateLabel,
  };
}
