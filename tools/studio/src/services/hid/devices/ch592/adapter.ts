import {
  CH592_PRODUCT_ID,
  CH592_VENDOR_ID,
  REPORT_ID_COMMAND,
  REPORT_ID_RESPONSE,
} from '@/types/protocol';
import { BaseHidAdapter } from '../../common/BaseHidAdapter';
import { Ch592Codec } from './codec';

export const CH592_FILTER: HIDDeviceFilter = {
  vendorId: CH592_VENDOR_ID,
  productId: CH592_PRODUCT_ID,
  usagePage: 0xFF00,
  usage: 0x01,
};

export class Ch592HidAdapter extends BaseHidAdapter<DataView> {
  constructor() {
    super(new Ch592Codec(), [CH592_FILTER]);
  }

  matches(device: HIDDevice): boolean {
    return device.vendorId === CH592_VENDOR_ID && device.productId === CH592_PRODUCT_ID;
  }

  protected get commandReportId(): number {
    return REPORT_ID_COMMAND;
  }

  protected get responseReportId(): number {
    return REPORT_ID_RESPONSE;
  }

  protected mapResponseData(event: HIDInputReportEvent): DataView {
    return new DataView(event.data.buffer, event.data.byteOffset, event.data.byteLength);
  }

  protected responseFrameBytes(event: HIDInputReportEvent): Uint8Array {
    return new Uint8Array(event.data.buffer, event.data.byteOffset, event.data.byteLength);
  }
}
