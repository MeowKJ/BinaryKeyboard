export class WebHidTools {
  // 请求并连接 HID 设备
  static async requestDevice(
    filters: HIDDeviceFilter[] = []
  ): Promise<HIDDevice | null> {
    try {
      const devices = await navigator.hid.requestDevice({ filters });
      if (devices.length === 0) {
        console.error("No HID devices found.");
        return null;
      }
      const device = devices[0];
      console.log("HID device selected:", device);
      return device;
    } catch (error) {
      console.error("Error requesting HID device:", error);
      return null;
    }
  }

  // 打开 HID 设备
  static async openDevice(device: HIDDevice): Promise<boolean> {
    try {
      await device.open();
      console.log("HID device opened successfully:", device);
      return true;
    } catch (error) {
      console.error("Error opening HID device:", error);
      return false;
    }
  }

  // 发送 Output Report 到 HID 设备
  static async sendData(
    device: HIDDevice,
    reportId: number,
    data: Uint8Array
  ): Promise<boolean> {
    try {
      await device.sendReport(reportId, data);
      console.log("Data sent to HID device:", data);
      return true;
    } catch (error) {
      console.error("Error sending data to HID device:", error);
      return false;
    }
  }

  // 发送 Feature Report 到 HID 设备
  static async sendFeatureReport(
    device: HIDDevice,
    reportId: number,
    data: Uint8Array
  ): Promise<boolean> {
    try {
      await device.sendFeatureReport(reportId, data);
      console.log("Feature report sent to HID device:", data);
      return true;
    } catch (error) {
      console.error("Error sending feature report to HID device:", error);
      return false;
    }
  }

  // 监听 Input Report
  static listenInputReport(
    device: HIDDevice,
    callback: (event: HIDInputReportEvent) => void
  ): void {
    const listener = (event: HIDInputReportEvent) => {
      console.log("Received input report:", event);
      callback(event);
    };
    device.addEventListener("inputreport", listener);
    console.log("Input report listener added.");
  }

  // 关闭 HID 设备
  static async closeDevice(device: HIDDevice): Promise<boolean> {
    try {
      await device.close();
      console.log("HID device closed successfully.");
      return true;
    } catch (error) {
      console.error("Error closing HID device:", error);
      return false;
    }
  }
}
