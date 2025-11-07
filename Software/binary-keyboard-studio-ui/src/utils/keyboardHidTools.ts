import { WebHidTools } from "./WebHidTools";

// 发送请求键盘数据（Report ID 5）
export const sendInitData = async (device: HIDDevice, reportId: number = 5) => {
  if (!device) throw new Error("设备未连接");
  // 构造数据包：31字节，第二位为0x01
  const data = new Uint8Array(31);
  data[0] = 0x01;
  const success = await WebHidTools.sendData(device, reportId, data);
  if (!success) {
    throw new Error("初始化数据发送失败");
  }
};

export const sendData = async (
  device: HIDDevice,
  reportId: number,
  data: Uint8Array
) => {
  if (!device) throw new Error("设备未连接");
  const success = await WebHidTools.sendData(device, reportId, data);
  if (!success) {
    throw new Error("数据发送失败");
  }
};
