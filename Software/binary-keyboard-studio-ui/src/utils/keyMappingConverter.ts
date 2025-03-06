import type { KeyMapping } from "@/types/keyboard";
/**
 * 将包含 8 个键、每键 3 字节数据的 Uint8Array 解析为 KeyMapping 数组
 * @param data Uint8Array 数据，长度必须为 24 字节
 * @returns KeyMapping 数组，共 8 个元素
 */
export function parseKeyMappingsFromUint8Array(data: Uint8Array): KeyMapping[] {
  if (data.length !== 24) {
    throw new Error("数据长度错误，期望 24 个字节");
  }
  const mappings: KeyMapping[] = [];
  for (let i = 0; i < 8; i++) {
    const offset = i * 3;
    const type = data[offset];
    const byte2 = data[offset + 1];
    const byte3 = data[offset + 2];
    if (type === 0x01) {
      // 键盘：第二字节为修饰符，第三字节为按键
      mappings.push({ type: 0x01, modifier: byte2, key: byte3 });
    } else if (type === 0x02) {
      // 媒体：将第二、第三字节组合成一个 16 位数字（高位在前）
      const mediaData = (byte2 << 8) | byte3;
      mappings.push({ type: 0x02, mediaData });
    } else if (type === 0x03) {
      // 鼠标：第二字节忽略，第三字节为鼠标数据
      mappings.push({ type: 0x03, mouseData: byte3 });
    } else {
      //初始化为键盘类型

      mappings.push({ type: 0x01, modifier: 0, key: 0 });

      throw new Error(`未知的按键类型: 0x${type.toString(16)}`);
    }
  }
  return mappings;
}

/**
 * 将 KeyMapping 数组转换为 Uint8Array 数据
 * @param mappings 包含 8 个 KeyMapping 的数组
 * @returns Uint8Array 数据，共 24 字节
 */
export function convertKeyMappingsToUint8Array(
  mappings: KeyMapping[]
): Uint8Array {
  if (mappings.length !== 8) {
    throw new Error("按键映射数组必须包含 8 个键");
  }
  const data = new Uint8Array(24);
  for (let i = 0; i < mappings.length; i++) {
    const offset = i * 3;
    const mapping = mappings[i];
    data[offset] = mapping.type;
    if (mapping.type === 0x01) {
      data[offset + 1] = mapping.modifier;
      data[offset + 2] = mapping.key;
    } else if (mapping.type === 0x02) {
      data[offset + 1] = mapping.mediaData >> 8; // 高字节
      data[offset + 2] = mapping.mediaData & 0xff; // 低字节
    } else if (mapping.type === 0x03) {
      data[offset + 1] = 0; // 保留为空
      data[offset + 2] = mapping.mouseData;
    } else {
      console.warn(`未知的按键类型, 初始化为键盘类型`);
      data[offset] = 0x01; // 默认为键盘类型
      data[offset + 1] = 0;
      data[offset + 2] = 0;
    }
  }
  return data;
}
