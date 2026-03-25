/**
 * IAP (In-Application Programming) 固件更新服务
 *
 * 完整流程:
 *   1. 从 GitHub Pages 发布的 manifest / 静态文件下载 .bin 固件
 *   2. 通过 HID 发送 IAP_PREPARE 擦除 Image B
 *   3. 分块发送固件数据 (IAP_WRITE)
 *   4. CRC32 校验 (IAP_VERIFY)
 *   5. 触发更新重启 (IAP_ACTIVATE)
 *   6. 等待设备重新连接
 */

import { Command, FRAME_SIZE, ResponseCode } from '@/types/protocol';
import { LOCAL_RELEASE_MANIFEST, RELEASE_FEED } from '@/generated/versionConfig';

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
  /** 额外提示 */
  hint?: string;
}

export type IapProgressCallback = (progress: IapProgress) => void;

/** 底层 HID 传输接口 (与 codec 解耦) */
export interface IapTransport {
  sendAndWait(frame: Uint8Array, options?: { timeout?: number }): Promise<DataView>;
}

interface ReleaseFirmwareAsset {
  version?: string;
  binUrl?: string;
  fullHexUrl?: string;
  hexUrl?: string;
}

interface ReleaseManifest {
  artifacts?: {
    ch592?: Record<string, ReleaseFirmwareAsset>;
  };
}

function getCh592Asset(manifest: ReleaseManifest, model: string): ReleaseFirmwareAsset | undefined {
  return manifest.artifacts?.ch592?.[model];
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

function formatIapError(err: unknown): { message: string; error?: string; hint?: string } {
  const raw = err instanceof Error ? err.message : '未知错误';

  if (raw.includes('发布清单里缺少')) {
    return {
      message: '下载清单还没准备好',
      error: '当前版本的固件地址还没有出现在发布清单里。',
      hint: '如果这是刚发布的新版本，通常等 Pages 部署完成后再试即可。',
    };
  }

  if (raw.includes('发布清单版本不匹配')) {
    return {
      message: '固件版本还在同步中',
      error: raw,
      hint: '页面内置版本和线上清单还没完全对齐，稍后刷新重试。',
    };
  }

  if (raw.startsWith('下载失败: HTTP 404')) {
    return {
      message: '固件文件暂时还不可下载',
      error: '下载站上还没有这个版本的固件文件。',
      hint: '一般是 Release 已更新，但 Pages 静态文件还没同步完成。',
    };
  }

  if (raw.startsWith('下载失败: HTTP 403')) {
    return {
      message: '固件下载被拒绝',
      error: '下载站暂时拒绝了这次请求。',
      hint: '稍后重试；如果持续出现，再检查发布资源权限或缓存状态。',
    };
  }

  if (raw.startsWith('下载失败: HTTP ')) {
    return {
      message: '固件下载失败',
      error: raw,
      hint: '请稍后重试；如果问题持续存在，通常是发布链路还没完成。',
    };
  }

  return {
    message: raw,
    error: raw,
  };
}

// ============================================================================
// 固件下载
// ============================================================================

async function loadReleaseManifest(): Promise<ReleaseManifest> {
  try {
    const response = await fetch(RELEASE_FEED.manifestUrl, {
      headers: {
        Accept: 'application/json',
      },
    });

    if (!response.ok) {
      throw new Error(`manifest request failed: ${response.status}`);
    }

    return (await response.json()) as ReleaseManifest;
  } catch {
    return LOCAL_RELEASE_MANIFEST as ReleaseManifest;
  }
}

async function resolveFirmwareUrl(
  version: string,
  model: string,
) {
  const remoteManifest = await loadReleaseManifest();
  const localManifest = LOCAL_RELEASE_MANIFEST as ReleaseManifest;

  let asset = getCh592Asset(remoteManifest, model);
  if (!asset?.binUrl) {
    asset = getCh592Asset(localManifest, model);
  }

  if (!asset?.binUrl) {
    throw new Error(`发布清单里缺少 CH592F-${model} 的 bin 下载地址`);
  }
  if (asset.version && asset.version !== version) {
    const localAsset = getCh592Asset(localManifest, model);
    if (localAsset?.binUrl && localAsset.version === version) {
      return localAsset.binUrl;
    }
    throw new Error(`发布清单版本不匹配: 需要 ${version}，当前为 ${asset.version}`);
  }
  return asset.binUrl;
}

/**
 * 从 GitHub Release 下载固件 .bin
 */
async function downloadFirmware(
  version: string,
  model: string,
  onProgress: IapProgressCallback,
): Promise<Uint8Array> {
  const url = await resolveFirmwareUrl(version, model);
  onProgress({ stage: 'downloading', percent: 0, message: `正在下载固件 v${version}...` });

  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`下载失败: HTTP ${response.status} — ${url}`);
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
    const formatted = formatIapError(err);
    onProgress({
      stage: 'error',
      percent: 0,
      message: formatted.message,
      error: formatted.error,
      hint: formatted.hint,
    });
    throw err;
  }
}
