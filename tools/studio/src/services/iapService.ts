/**
 * IAP (In-Application Programming) 固件更新服务
 *
 * 完整流程:
 *   1. 通过 GitHub Releases API 下载 .bin 固件
 *   2. 通过 HID 发送 IAP_PREPARE 擦除 Image B
 *   3. 分块发送固件数据 (IAP_WRITE)
 *   4. CRC32 校验 (IAP_VERIFY)
 *   5. 触发更新重启 (IAP_ACTIVATE)
 *   6. 等待设备重新连接
 */

import { Command, FRAME_SIZE, ResponseCode } from '@/types/protocol';
import { RELEASE_FEED } from '@/generated/versionConfig';

// ============================================================================
// 类型定义
// ============================================================================

export type IapStage =
  | 'idle'
  | 'downloading'
  | 'preparing'
  | 'writing'
  | 'verifying'
  | 'activating'
  | 'rebooting'
  | 'done'
  | 'error';

export interface IapProgress {
  stage: IapStage;
  /** 总进度 0-100 */
  percent: number;
  /** 当前阶段描述 */
  message: string;
  /** 错误信息 (stage === 'error') */
  error?: string;
}

export type IapProgressCallback = (progress: IapProgress) => void;

/** 底层 HID 传输接口 (与 codec 解耦) */
export interface IapTransport {
  sendAndWait(frame: Uint8Array, options?: { timeout?: number }): Promise<DataView>;
}

interface GitHubReleaseAsset {
  name: string;
  url: string;
  browser_download_url: string;
}

interface GitHubRelease {
  tag_name: string;
  assets: GitHubReleaseAsset[];
}

// ============================================================================
// 常量
// ============================================================================

/** IAP_WRITE 每帧有效载荷 (64 - 3 header - 2 offset = 59, 取 56 对齐) */
const WRITE_PAYLOAD_SIZE = 56;

/** Image B 分区大小 (与固件 iap_config.h 一致) */
const IMAGE_B_SIZE = 216 * 1024;

/** 页大小 (寻址单位) */
const PAGE_SIZE = 256;

// ============================================================================
// CRC32 (与固件端一致的 ISO 3309)
// ============================================================================

function crc32(data: Uint8Array): number {
  let crc = 0xFFFFFFFF;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i];
    for (let j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >>> 1) ^ 0xEDB88320;
      } else {
        crc >>>= 1;
      }
    }
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

// ============================================================================
// 帧构建辅助
// ============================================================================

function buildFrame(cmd: number, sub: number, data: Uint8Array = new Uint8Array(0)): Uint8Array {
  const frame = new Uint8Array(FRAME_SIZE);
  frame[0] = cmd;
  frame[1] = sub;
  frame[2] = data.length;
  frame.set(data, 3);
  return frame;
}

function checkResponse(resp: DataView, cmdName: string): void {
  const status = resp.getUint8(3); // [CMD][SUB][LEN][STATUS...]
  if (status !== ResponseCode.OK) {
    throw new Error(`${cmdName} 失败: status=0x${status.toString(16)}`);
  }
}

// ============================================================================
// 固件下载
// ============================================================================

const GITHUB_RELEASE_API_URL = `https://api.github.com/repos/${RELEASE_FEED.repository}/releases/latest`;

function buildFirmwareAssetName(version: string, model: string): string {
  return `CH592F-${model}-${version}.bin`;
}

async function resolveFirmwareAsset(
  version: string,
  model: string,
): Promise<{ asset: GitHubReleaseAsset; releaseTag: string }> {
  const response = await fetch(GITHUB_RELEASE_API_URL, {
    headers: {
      Accept: 'application/vnd.github+json',
    },
  });

  if (!response.ok) {
    throw new Error(`查询 GitHub Release 失败: HTTP ${response.status}`);
  }

  const release = (await response.json()) as GitHubRelease;
  const assetName = buildFirmwareAssetName(version, model);
  const asset = release.assets.find((item) => item.name === assetName);

  if (!asset) {
    throw new Error(`最新 Release (${release.tag_name}) 尚未包含 ${assetName}`);
  }

  return { asset, releaseTag: release.tag_name };
}

/**
 * 从 GitHub Release 下载固件 .bin
 */
async function downloadFirmware(
  version: string,
  model: string,
  onProgress: IapProgressCallback,
): Promise<Uint8Array> {
  const { asset, releaseTag } = await resolveFirmwareAsset(version, model);
  onProgress({ stage: 'downloading', percent: 0, message: `正在下载固件 v${version}...` });

  const response = await fetch(asset.url, {
    headers: {
      Accept: 'application/octet-stream',
    },
  });
  if (!response.ok) {
    throw new Error(`下载失败: HTTP ${response.status} — ${asset.name} @ ${releaseTag}`);
  }

  const contentLength = Number(response.headers.get('content-length') || 0);
  const reader = response.body?.getReader();
  if (!reader) {
    throw new Error('浏览器不支持 ReadableStream');
  }

  const chunks: Uint8Array[] = [];
  let received = 0;

  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    chunks.push(value);
    received += value.length;
    const pct = contentLength > 0 ? Math.floor((received / contentLength) * 100) : 0;
    onProgress({ stage: 'downloading', percent: pct, message: `正在下载固件... ${(received / 1024).toFixed(0)} KB` });
  }

  const firmware = new Uint8Array(received);
  let offset = 0;
  for (const chunk of chunks) {
    firmware.set(chunk, offset);
    offset += chunk.length;
  }

  if (firmware.length > IMAGE_B_SIZE) {
    throw new Error(`固件文件过大: ${firmware.length} > ${IMAGE_B_SIZE}`);
  }

  onProgress({ stage: 'downloading', percent: 100, message: `下载完成 (${(firmware.length / 1024).toFixed(1)} KB)` });
  return firmware;
}

// ============================================================================
// IAP 流程
// ============================================================================

/**
 * 执行完整的 IAP 固件更新流程
 */
export async function performIapUpdate(
  transport: IapTransport,
  version: string,
  model: string,
  onProgress: IapProgressCallback,
): Promise<void> {
  try {
    // 1. 下载固件
    const firmware = await downloadFirmware(version, model, onProgress);

    // 2. IAP_PREPARE — 擦除 Image B
    onProgress({ stage: 'preparing', percent: 0, message: '正在擦除暂存区...' });
    const prepResp = await transport.sendAndWait(
      buildFrame(Command.IAP_PREPARE, 0),
      { timeout: 30_000 },  // 擦除 216KB 可能需要较长时间
    );
    checkResponse(prepResp, 'IAP_PREPARE');
    onProgress({ stage: 'preparing', percent: 100, message: '暂存区已就绪' });

    // 3. IAP_WRITE — 分块写入固件
    const totalPages = Math.ceil(firmware.length / PAGE_SIZE);
    let pageIdx = 0;

    for (let offset = 0; offset < firmware.length; ) {
      const currentPage = Math.floor(offset / PAGE_SIZE);
      const pageOffset = currentPage; // 以 256B 页为单位的偏移

      // 一页内可能需要多帧
      const pageEnd = Math.min((currentPage + 1) * PAGE_SIZE, firmware.length);

      while (offset < pageEnd) {
        const chunkLen = Math.min(WRITE_PAYLOAD_SIZE, pageEnd - offset);
        const data = new Uint8Array(2 + chunkLen);
        data[0] = (pageOffset >> 8) & 0xFF;
        data[1] = pageOffset & 0xFF;
        data.set(firmware.subarray(offset, offset + chunkLen), 2);

        const seq = currentPage & 0xFF;
        const writeResp = await transport.sendAndWait(
          buildFrame(Command.IAP_WRITE, seq, data),
          { timeout: 5_000 },
        );
        checkResponse(writeResp, 'IAP_WRITE');
        offset += chunkLen;
      }

      pageIdx++;
      const pct = Math.floor((pageIdx / totalPages) * 100);
      onProgress({
        stage: 'writing',
        percent: pct,
        message: `正在写入固件... ${pct}% (${(offset / 1024).toFixed(0)}/${(firmware.length / 1024).toFixed(0)} KB)`,
      });
    }

    // 4. IAP_VERIFY — CRC32 校验
    onProgress({ stage: 'verifying', percent: 0, message: '正在校验固件...' });
    const fwCrc = crc32(firmware);
    const verifyData = new Uint8Array(8);
    const dv = new DataView(verifyData.buffer);
    dv.setUint32(0, firmware.length, true);  // size LE
    dv.setUint32(4, fwCrc, true);            // expected CRC LE

    const verifyResp = await transport.sendAndWait(
      buildFrame(Command.IAP_VERIFY, 0, verifyData),
      { timeout: 10_000 },
    );
    checkResponse(verifyResp, 'IAP_VERIFY');
    onProgress({ stage: 'verifying', percent: 100, message: '校验通过' });

    // 5. IAP_ACTIVATE — 触发更新
    onProgress({ stage: 'activating', percent: 0, message: '正在激活更新...' });
    const activateResp = await transport.sendAndWait(
      buildFrame(Command.IAP_ACTIVATE, 0),
      { timeout: 5_000 },
    );
    checkResponse(activateResp, 'IAP_ACTIVATE');

    // 6. 设备重启中
    onProgress({
      stage: 'rebooting',
      percent: 50,
      message: '设备正在重启，请等待自动重连...',
    });
  } catch (err) {
    const message = err instanceof Error ? err.message : '未知错误';
    onProgress({ stage: 'error', percent: 0, message, error: message });
    throw err;
  }
}
