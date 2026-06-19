import {
  CH552_PRODUCT_ID,
  CH552_REPORT_ID_COMMAND,
  CH552_REPORT_ID_RESPONSE,
  CH552_VENDOR_ID,
} from '@/types/protocol';
import { BaseHidAdapter } from '../../common/BaseHidAdapter';
import { Ch552Codec } from './codec';

const CH552_FILTER: HIDDeviceFilter = {
  vendorId: CH552_VENDOR_ID,
  productId: CH552_PRODUCT_ID,
  usagePage: 0xFF00,
  usage: 0x01,
};

const CH552_READ_LAYER_COMMAND = 0x03;
const CH552_MACRO_COMMAND = 0x40;

export class Ch552HidAdapter extends BaseHidAdapter<Uint8Array> {
  constructor() {
    super(new Ch552Codec(), [CH552_FILTER]);
  }

  matches(device: HIDDevice): boolean {
    return device.vendorId === CH552_VENDOR_ID && device.productId === CH552_PRODUCT_ID;
  }

  protected get commandReportId(): number {
    return CH552_REPORT_ID_COMMAND;
  }

  protected get responseReportId(): number {
    return CH552_REPORT_ID_RESPONSE;
  }

  protected matchesResponseFrame(requestFrame: Uint8Array, responseFrame: Uint8Array): boolean {
    if ((responseFrame[0] ?? 0) !== (requestFrame[0] ?? 0)) {
      return false;
    }

    const command = requestFrame[0] ?? 0;
    if (command === CH552_READ_LAYER_COMMAND || command === CH552_MACRO_COMMAND) {
      return (responseFrame[1] ?? 0) === (requestFrame[1] ?? 0);
    }

    return true;
  }

  protected mapResponseData(event: HIDInputReportEvent): Uint8Array {
    return new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
  }

  protected responseFrameBytes(event: HIDInputReportEvent): Uint8Array {
    return this.mapResponseData(event);
  }
}
