const encoder = new TextEncoder();
const decoder = new TextDecoder();

export function utf8ByteLength(value: string): number {
  return encoder.encode(value).length;
}

export function truncateUtf8ByBytes(value: string, maxBytes: number): string {
  if (maxBytes <= 0 || value.length === 0) {
    return "";
  }

  let result = "";
  let usedBytes = 0;

  for (const char of value) {
    const charBytes = encoder.encode(char).length;
    if (usedBytes + charBytes > maxBytes) {
      break;
    }
    result += char;
    usedBytes += charBytes;
  }

  return result;
}

export function decodeUtf8CString(bytes: Uint8Array): string {
  const zeroIndex = bytes.indexOf(0);
  const effective = zeroIndex >= 0 ? bytes.slice(0, zeroIndex) : bytes;
  return decoder.decode(effective);
}
