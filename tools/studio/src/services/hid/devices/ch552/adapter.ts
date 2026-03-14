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

  protected mapResponseData(event: HIDInputReportEvent): Uint8Array {
    return new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
  }

  protected responseFrameBytes(event: HIDInputReportEvent): Uint8Array {
    return this.mapResponseData(event);
  }
}
