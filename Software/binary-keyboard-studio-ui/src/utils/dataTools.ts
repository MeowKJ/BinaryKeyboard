// 验证工具函数
export const validateKeyIndex = (index: number) => {
  if (index < 0 || index >= 8) {
    throw new Error(`Invalid key index: ${index}, must be 0-7`);
  }
};

export const validateKeyValueRange = (value: number) => {
  if (value < 0 || value > 0xffff) {
    throw new Error(`Value out of range: ${value}`);
  }
};

// 数据转换函数
export const convertMappingsToUint8Array = (
  mappings: KeyConfig[]
): Uint8Array => {
  if (mappings.length !== 8) {
    throw new Error("Must have exactly 8 key configurations");
  }

  const buffer = new Uint8Array(24);

  mappings.forEach((key, index) => {
    validateKeyIndex(index);
    validateKeyValueRange(key.value);

    const offset = index * 3;
    buffer[offset] = key.type & 0xff;
    buffer[offset + 1] = (key.value >> 8) & 0xff; // 高字节
    buffer[offset + 2] = key.value & 0xff; // 低字节
  });

  return buffer;
};

export const parseUint8ArrayToMappings = (data: Uint8Array): KeyConfig[] => {
  if (data.length !== 24) {
    throw new Error("Data length must be 24 bytes");
  }

  return Array.from({ length: 8 }, (_, index) => {
    const offset = index * 3;
    return {
      type: data[offset],
      value: (data[offset + 1] << 8) | data[offset + 2],
    };
  });
};

// 辅助函数
export const clamp = (value: number, min: number, max: number): number => {
  return Math.min(Math.max(value, min), max);
};
