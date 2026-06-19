<script setup lang="ts">
import gsap from 'gsap';
import { hierarchy, treemap, treemapSquarify } from 'd3-hierarchy';
import type { HierarchyRectangularNode } from 'd3-hierarchy';
import { computed, nextTick, ref, watch } from 'vue';
import { hidService } from '@/services/HidService';
import { showToast } from '@/services/toastService';
import { useDeviceStore } from '@/stores/deviceStore';
import {
  Command,
  DeviceProtocol,
  FRAME_SIZE,
  ResponseCode,
} from '@/types/protocol';

type FocusLevel = 'die' | 'region' | 'page' | 'cluster' | 'byte';

interface DataFlashInfo {
  total: number;
  page: number;
  configEnd: number;
  runtimeBase: number;
  macroBase: number;
  macroSize: number;
  bleSnvBase: number;
  bleSnvSize: number;
  rawAvailable: boolean;
}

interface FlashRegion {
  id: string;
  name: string;
  start: number;
  size: number;
  kind: 'config' | 'runtime' | 'macro' | 'reserved' | 'protected';
}

interface LayoutDatum {
  size?: number;
  children?: FlashRegion[];
}

interface DieRegionNode {
  region: FlashRegion;
  x: number;
  y: number;
  width: number;
  height: number;
}

interface PageNode {
  index: number;
  offset: number;
  start: number;
  size: number;
  x: number;
  y: number;
  width: number;
  height: number;
}

interface ClusterNode {
  index: number;
  startOffset: number;
  endOffset: number;
  x: number;
  y: number;
  width: number;
  height: number;
}

interface ByteNode {
  offset: number;
  abs: number;
  value: number;
  x: number;
  y: number;
  width: number;
  height: number;
}

const deviceStore = useDeviceStore();
const props = defineProps<{ visible: boolean }>();
const emit = defineEmits<{ 'update:visible': [value: boolean] }>();

const dialogVisible = computed({
  get: () => props.visible,
  set: (value: boolean) => emit('update:visible', value),
});

const dfInfo = ref<DataFlashInfo>({
  total: 0x8000,
  page: 0x0100,
  configEnd: 0x0c00,
  runtimeBase: 0x0c00,
  macroBase: 0x1000,
  macroSize: 0x2000,
  bleSnvBase: 0x7e00,
  bleSnvSize: 0x0100,
  rawAvailable: false,
});

const level = ref<FocusLevel>('die');
const selectedRegionId = ref('config');
const selectedPageOffset = ref(0);
const selectedClusterIndex = ref(0);
const selectedByteOffset = ref(0);
const rangeAnchor = ref<number | null>(null);
const rangeFocus = ref<number | null>(null);
const isDraggingRange = ref(false);
const pageBytes = ref<Uint8Array>(new Uint8Array(0x100).fill(0xff));
const isBusy = ref(false);
const statusText = ref('offline');
const byteDraft = ref('FF');
const dangerConfirm = ref('');
const hudEl = ref<HTMLElement | null>(null);
const dieEl = ref<SVGSVGElement | null>(null);

const regions = computed<FlashRegion[]>(() => {
  const info = dfInfo.value;
  const reservedStart = info.macroBase + info.macroSize;
  const protectedStart = info.bleSnvBase + info.bleSnvSize;
  const list: FlashRegion[] = [
    { id: 'config', name: '配置槽', start: 0x0000, size: info.configEnd, kind: 'config' },
    { id: 'runtime', name: '热数据', start: info.runtimeBase, size: info.macroBase - info.runtimeBase, kind: 'runtime' },
    { id: 'macro', name: 'MeowFS', start: info.macroBase, size: info.macroSize, kind: 'macro' },
    { id: 'reserved', name: '保留区', start: reservedStart, size: Math.max(0, info.bleSnvBase - reservedStart), kind: 'reserved' },
    { id: 'ble', name: 'BLE SNV', start: info.bleSnvBase, size: info.bleSnvSize, kind: 'protected' },
    { id: 'tail', name: 'LOCK', start: protectedStart, size: Math.max(0, info.total - protectedStart), kind: 'protected' },
  ];
  return list.filter((region) => region.size > 0);
});

const selectedRegion = computed(
  () => regions.value.find((region) => region.id === selectedRegionId.value) ?? regions.value[0],
);

const pageSize = computed(() => dfInfo.value.page || 0x100);
const selectedPageStart = computed(() => selectedRegion.value.start + selectedPageOffset.value);
const selectedByteAbs = computed(() => selectedPageStart.value + selectedByteOffset.value);
const selectedByteValue = computed(() => pageBytes.value[selectedByteOffset.value] ?? 0xff);
const selectedByteMeaning = computed(() => describeByte(selectedByteAbs.value, selectedByteValue.value));
const rangeStart = computed(() => Math.min(rangeAnchor.value ?? selectedByteOffset.value, rangeFocus.value ?? selectedByteOffset.value));
const rangeEnd = computed(() => Math.max(rangeAnchor.value ?? selectedByteOffset.value, rangeFocus.value ?? selectedByteOffset.value));
const hasRange = computed(() => rangeStart.value !== rangeEnd.value);
const selectedRangeAbsStart = computed(() => selectedPageStart.value + rangeStart.value);
const selectedRangeAbsEnd = computed(() => selectedPageStart.value + rangeEnd.value);
const canWriteByte = computed(
  () =>
    !hasRange.value &&
    deviceStore.deviceInfo?.protocol === DeviceProtocol.CH592 &&
    dfInfo.value.rawAvailable &&
    dangerConfirm.value.trim().toUpperCase() === 'DANGER' &&
    /^[0-9a-fA-F]{2}$/.test(byteDraft.value.trim()),
);

const dieRegionNodes = computed<DieRegionNode[]>(() => {
  const root = hierarchy<LayoutDatum>({ children: regions.value })
    .sum((datum) => datum.size ?? 0)
    .sort((a, b) => (b.value ?? 0) - (a.value ?? 0));
  const layoutRoot = treemap<LayoutDatum>()
    .tile(treemapSquarify.ratio(1.48))
    .size([1000, 620])
    .paddingInner(9)
    .round(true)(root);

  return (layoutRoot as HierarchyRectangularNode<LayoutDatum>).leaves().map((node) => ({
    region: node.data as FlashRegion,
    x: node.x0,
    y: node.y0,
    width: Math.max(0, node.x1 - node.x0),
    height: Math.max(0, node.y1 - node.y0),
  }));
});

const pageNodes = computed<PageNode[]>(() => {
  const count = Math.max(1, Math.ceil(selectedRegion.value.size / pageSize.value));
  const root = hierarchy<{ size?: number; children?: Array<{ size: number; index: number }> }>({
    children: Array.from({ length: count }, (_, index) => ({
      index,
      size: Math.min(pageSize.value, selectedRegion.value.size - index * pageSize.value),
    })),
  }).sum((datum) => datum.size ?? 0);

  const layoutRoot = treemap<{ size?: number; children?: Array<{ size: number; index: number }> }>()
    .tile(treemapSquarify.ratio(1.2))
    .size([1000, 620])
    .paddingInner(7)
    .round(true)(root);

  return (layoutRoot as HierarchyRectangularNode<{ size?: number; index?: number }>).leaves().map((node) => {
    const index = node.data.index ?? 0;
    const offset = index * pageSize.value;
    return {
      index,
      offset,
      start: selectedRegion.value.start + offset,
      size: Math.min(pageSize.value, selectedRegion.value.size - offset),
      x: node.x0,
      y: node.y0,
      width: Math.max(0, node.x1 - node.x0),
      height: Math.max(0, node.y1 - node.y0),
    };
  });
});

const clusterNodes = computed<ClusterNode[]>(() => {
  const cols = 4;
  const rows = 4;
  const gap = 10;
  const width = (1000 - gap * (cols - 1)) / cols;
  const height = (620 - gap * (rows - 1)) / rows;
  return Array.from({ length: 16 }, (_, index) => ({
    index,
    startOffset: index * 16,
    endOffset: Math.min(index * 16 + 15, pageBytes.value.length - 1),
    x: (index % cols) * (width + gap),
    y: Math.floor(index / cols) * (height + gap),
    width,
    height,
  }));
});

const byteNodes = computed<ByteNode[]>(() => {
  const cluster = clusterNodes.value[selectedClusterIndex.value] ?? clusterNodes.value[0];
  const cols = 4;
  const gap = 12;
  const width = (1000 - gap * (cols - 1)) / cols;
  const height = (620 - gap * (cols - 1)) / cols;
  return Array.from({ length: 16 }, (_, index) => {
    const offset = Math.min(cluster.startOffset + index, pageBytes.value.length - 1);
    return {
      offset,
      abs: selectedPageStart.value + offset,
      value: pageBytes.value[offset] ?? 0xff,
      x: (index % cols) * (width + gap),
      y: Math.floor(index / cols) * (height + gap),
      width,
      height,
    };
  });
});

const hudText = computed(() => {
  if (level.value === 'die') return `${hex(0)} - ${hex(dfInfo.value.total - 1)} · DataFlash · ${dfInfo.value.total}B · ${statusText.value}`;
  if (level.value === 'region') return `${hex(selectedRegion.value.start)} - ${hex(selectedRegion.value.start + selectedRegion.value.size - 1)} · ${selectedRegion.value.name} · ${selectedRegion.value.size}B`;
  if (level.value === 'page') return `${hex(selectedPageStart.value)} - ${hex(selectedPageStart.value + pageSize.value - 1)} · ${selectedRegion.value.name} / page ${selectedPageOffset.value / pageSize.value}`;
  if (hasRange.value) {
    return `${hex(selectedRangeAbsStart.value)} - ${hex(selectedRangeAbsEnd.value)} · ${selectedRegion.value.name} / page ${selectedPageOffset.value / pageSize.value} · ${rangeEnd.value - rangeStart.value + 1}B`;
  }
  return `${hex(selectedByteAbs.value)} · ${selectedRegion.value.name} / page ${selectedPageOffset.value / pageSize.value} / byte ${hex(selectedByteOffset.value, 2)} · ${byteHex(selectedByteValue.value)} · ${selectedByteMeaning.value.field}`;
});

function hex(value: number, width = 4): string {
  return `0x${value.toString(16).toUpperCase().padStart(width, '0')}`;
}

function byteHex(value: number): string {
  return value.toString(16).toUpperCase().padStart(2, '0');
}

function byteBits(value: number): string {
  return value.toString(2).padStart(8, '0').replace(/(.{4})/, '$1 ');
}

function buildFrame(cmd: Command, sub = 0, data = new Uint8Array(0)): Uint8Array {
  const frame = new Uint8Array(FRAME_SIZE);
  frame[0] = cmd;
  frame[1] = sub;
  frame[2] = data.length;
  frame.set(data, 3);
  return frame;
}

async function sendCommand(cmd: Command, sub = 0, data = new Uint8Array(0), timeout = 3000): Promise<DataView> {
  const transport = hidService.getIapTransport();
  if (!transport) throw new Error('HID 传输未就绪');
  return transport.sendAndWait(buildFrame(cmd, sub, data), { timeout });
}

function expectOk(resp: DataView, name: string): number {
  const status = resp.getUint8(3);
  if (status !== ResponseCode.OK) throw new Error(`${name} 失败: 0x${status.toString(16)}`);
  return 3;
}

async function readDataFlashInfo(): Promise<DataFlashInfo> {
  const resp = await sendCommand(Command.DATAFLASH_INFO, 0, new Uint8Array(0), 1500);
  const d = expectOk(resp, 'DATAFLASH_INFO');
  return {
    total: resp.getUint16(d + 1, false),
    page: resp.getUint16(d + 3, false),
    configEnd: resp.getUint16(d + 5, false),
    runtimeBase: resp.getUint16(d + 7, false),
    macroBase: resp.getUint16(d + 9, false),
    macroSize: resp.getUint16(d + 11, false),
    bleSnvBase: resp.getUint16(d + 13, false),
    bleSnvSize: resp.getUint16(d + 15, false),
    rawAvailable: true,
  };
}

async function readDataFlash(offset: number, length: number): Promise<Uint8Array> {
  const result = new Uint8Array(length);
  let pos = 0;
  while (pos < length) {
    const chunkLen = Math.min(58, length - pos);
    const abs = offset + pos;
    const resp = await sendCommand(
      Command.DATAFLASH_READ,
      0,
      new Uint8Array([(abs >> 8) & 0xff, abs & 0xff, chunkLen]),
      2000,
    );
    const d = expectOk(resp, 'DATAFLASH_READ');
    const readLen = resp.getUint8(d + 1);
    for (let i = 0; i < readLen && pos + i < result.length; i++) {
      result[pos + i] = resp.getUint8(d + 2 + i);
    }
    pos += readLen || chunkLen;
  }
  return result;
}

async function writeDataFlashByte(offset: number, value: number): Promise<void> {
  const resp = await sendCommand(
    Command.DATAFLASH_WRITE,
    0,
    new Uint8Array([(offset >> 8) & 0xff, offset & 0xff, value & 0xff]),
    5000,
  );
  expectOk(resp, 'DATAFLASH_WRITE');
}

function animateFocus(): void {
  void nextTick(() => {
    if (dieEl.value) {
      gsap.fromTo(dieEl.value, { scale: 0.982, filter: 'brightness(1.25)' }, { scale: 1, filter: 'brightness(1)', duration: 0.42, ease: 'power3.out' });
    }
    if (hudEl.value) {
      gsap.fromTo(hudEl.value, { y: 8, opacity: 0.55 }, { y: 0, opacity: 1, duration: 0.28, ease: 'power2.out' });
    }
  });
}

async function refreshSnapshot(): Promise<void> {
  if (deviceStore.deviceInfo?.protocol !== DeviceProtocol.CH592) return;
  isBusy.value = true;
  statusText.value = 'sync';
  try {
    dfInfo.value = await readDataFlashInfo();
    await readCurrentPage();
    statusText.value = 'live';
  } catch (error) {
    dfInfo.value = { ...dfInfo.value, rawAvailable: false };
    statusText.value = 'firmware';
    showToast('error', 'DataFlash 不可用', error instanceof Error ? error.message : '当前固件不支持 DataFlash 调试命令');
  } finally {
    isBusy.value = false;
  }
}

async function readCurrentPage(): Promise<void> {
  const length = Math.min(pageSize.value, selectedRegion.value.size - selectedPageOffset.value);
  pageBytes.value = dfInfo.value.rawAvailable
    ? await readDataFlash(selectedPageStart.value, length)
    : new Uint8Array(length).fill(0xff);
  selectedByteOffset.value = Math.min(selectedByteOffset.value, Math.max(0, pageBytes.value.length - 1));
  byteDraft.value = byteHex(selectedByteValue.value);
}

async function enterRegion(region: FlashRegion): Promise<void> {
  selectedRegionId.value = region.id;
  selectedPageOffset.value = 0;
  selectedClusterIndex.value = 0;
  clearSelection();
  if (dfInfo.value.rawAvailable) await readCurrentPage().catch(() => {});
  level.value = 'region';
  animateFocus();
}

async function enterPage(page: PageNode): Promise<void> {
  selectedPageOffset.value = page.offset;
  selectedClusterIndex.value = 0;
  clearSelection();
  if (dfInfo.value.rawAvailable) await readCurrentPage().catch(() => {});
  level.value = 'page';
  animateFocus();
}

function enterCluster(cluster: ClusterNode): void {
  selectedClusterIndex.value = cluster.index;
  selectByte(cluster.startOffset);
  level.value = 'cluster';
  animateFocus();
}

function enterByte(node: ByteNode): void {
  selectByte(node.offset);
  level.value = 'byte';
  animateFocus();
}

function stepBack(): void {
  if (level.value === 'byte') level.value = 'cluster';
  else if (level.value === 'cluster') level.value = 'page';
  else if (level.value === 'page') level.value = 'region';
  else if (level.value === 'region') level.value = 'die';
  clearSelection(false);
  animateFocus();
}

function selectByte(offset: number): void {
  selectedByteOffset.value = Math.max(0, Math.min(offset, pageBytes.value.length - 1));
  rangeAnchor.value = selectedByteOffset.value;
  rangeFocus.value = selectedByteOffset.value;
  byteDraft.value = byteHex(selectedByteValue.value);
  dangerConfirm.value = '';
}

function beginRange(node: ByteNode, event: PointerEvent): void {
  selectedByteOffset.value = node.offset;
  rangeAnchor.value = event.shiftKey && rangeAnchor.value !== null ? rangeAnchor.value : node.offset;
  rangeFocus.value = node.offset;
  isDraggingRange.value = true;
  byteDraft.value = byteHex(node.value);
  dangerConfirm.value = '';
}

function updateRange(node: ByteNode): void {
  if (!isDraggingRange.value) return;
  rangeFocus.value = node.offset;
  selectedByteOffset.value = node.offset;
}

function endRange(): void {
  isDraggingRange.value = false;
}

function clearSelection(resetByte = true): void {
  if (resetByte) selectedByteOffset.value = 0;
  rangeAnchor.value = selectedByteOffset.value;
  rangeFocus.value = selectedByteOffset.value;
  dangerConfirm.value = '';
}

async function commitByteWrite(): Promise<void> {
  if (!canWriteByte.value) return;
  const next = Number.parseInt(byteDraft.value.trim(), 16);
  isBusy.value = true;
  try {
    await writeDataFlashByte(selectedByteAbs.value, next);
    await readCurrentPage();
    selectByte(selectedByteOffset.value);
    dangerConfirm.value = '';
    showToast('success', '字节已写入', `${hex(selectedByteAbs.value)} = ${byteHex(next)}`);
  } catch (error) {
    showToast('error', '写入失败', error instanceof Error ? error.message : '未知错误');
  } finally {
    isBusy.value = false;
  }
}

function describeByte(abs: number, value: number): { area: string; field: string } {
  const region = regions.value.find((item) => abs >= item.start && abs < item.start + item.size);
  if (!region) return { area: '未知', field: '越界' };

  if (region.id === 'config') {
    const slot = Math.floor((abs - region.start) / 0x400);
    const off = (abs - region.start) % 0x400;
    const prefix = `配置槽 ${slot}`;
    if (off < 0x20) return { area: prefix, field: configHeaderField(off) };
    if (off >= 0x100 && off < 0x140) return { area: prefix, field: systemField(off - 0x100) };
    if (off >= 0x200 && off < 0x2a4) return { area: prefix, field: keymapField(off - 0x200) };
    if (off >= 0x300 && off < 0x320) return { area: prefix, field: fnField(off - 0x300) };
    if (off >= 0x340 && off < 0x360) return { area: prefix, field: rgbField(off - 0x340) };
    return { area: prefix, field: `reserved ${hex(off, 3)}` };
  }

  if (region.id === 'runtime') return { area: `runtime 页 ${Math.floor((abs - region.start) / 0x100)}`, field: runtimeField((abs - region.start) % 0x100) };
  if (region.id === 'macro') return { area: 'MeowFS', field: value === 0xff ? `empty ${hex(abs - region.start)}` : `payload ${hex(abs - region.start)}` };
  if (region.id === 'ble') return { area: 'BLE SNV', field: `SNV ${hex(abs - region.start, 3)}` };
  return { area: region.name, field: `offset ${hex(abs - region.start)}` };
}

function configHeaderField(off: number): string {
  if (off <= 3) return `magic[${off}]`;
  if (off <= 5) return `version[${off - 4}]`;
  if (off <= 7) return `flags[${off - 6}]`;
  if (off <= 11) return `save_count[${off - 8}]`;
  if (off >= 0x1c && off <= 0x1f) return `crc32[${off - 0x1c}]`;
  return `header ${hex(off, 2)}`;
}

function systemField(off: number): string {
  const names = ['default_mode', 'auto_sleep_min', 'debounce_ms', 'log_enabled', 'deep_sleep_min', 'os_mode'];
  return names[off] ?? `system ${hex(off, 2)}`;
}

function keymapField(off: number): string {
  if (off === 0) return 'num_layers';
  if (off === 1) return 'current_layer';
  if (off === 2) return 'default_layer';
  const dataOff = off - 4;
  const layerIndex = Math.floor(dataOff / 32);
  const inLayer = dataOff % 32;
  const key = Math.floor(inLayer / 4);
  const part = ['type', 'modifier', 'param1', 'param2'][inLayer % 4];
  return `L${layerIndex + 1}.K${key + 1}.${part}`;
}

function fnField(off: number): string {
  const fn = Math.floor(off / 8);
  const part = ['click_action', 'click_param', 'long_action', 'long_param', 'long_ms_lo', 'long_ms_hi', 'reserved0', 'reserved1'][off % 8];
  return `FN${fn + 1}.${part}`;
}

function rgbField(off: number): string {
  const names = ['enabled', 'mode', 'brightness', 'speed', 'color_r', 'color_g', 'color_b', 'indicator_enabled', 'indicator_brightness', 'press_effect'];
  return names[off] ?? `rgb ${hex(off, 2)}`;
}

function runtimeField(off: number): string {
  if (off <= 3) return `magic[${off}]`;
  if (off <= 5) return `version[${off - 4}]`;
  if (off <= 7) return `flags[${off - 6}]`;
  if (off <= 11) return `seq[${off - 8}]`;
  if (off === 12) return 'current_layer';
  if (off === 13) return 'last_mode';
  if (off >= 252) return `crc32[${off - 252}]`;
  return `runtime ${hex(off, 2)}`;
}

let hasAutoLoaded = false;
watch(() => props.visible, (visible) => {
  if (visible) {
    level.value = 'die';
    animateFocus();
    if (!hasAutoLoaded) {
      hasAutoLoaded = true;
      void refreshSnapshot();
    }
  }
});

watch(hudText, animateFocus);
</script>

<template>
  <Dialog
    v-model:visible="dialogVisible"
    :style="{ width: 'min(1380px, calc(100vw - 18px))' }"
    modal
    class="storm-dataflash-dialog"
    :showHeader="false"
  >
    <div class="die-explorer" @pointerup="endRange" @pointerleave="endRange" @keydown.esc="stepBack" tabindex="0">
      <div class="die-stars"></div>
      <div class="die-scan"></div>

      <div ref="hudEl" class="die-hud" :class="{ danger: level === 'byte' || hasRange }">
        <span>{{ hudText }}</span>
        <code v-if="level === 'byte' && !hasRange">{{ byteBits(selectedByteValue) }}</code>
      </div>

      <div class="die-controls">
        <button type="button" class="die-icon" :disabled="level === 'die'" title="返回上一层" @click="stepBack">
          <i class="pi pi-arrow-left"></i>
        </button>
        <button type="button" class="die-icon" :disabled="isBusy" title="同步 DataFlash" @click="refreshSnapshot">
          <i class="pi pi-sync" :class="{ spinning: isBusy }"></i>
        </button>
        <button type="button" class="die-icon" title="关闭" @click="dialogVisible = false">
          <i class="pi pi-times"></i>
        </button>
      </div>

      <svg
        ref="dieEl"
        class="die-canvas"
        viewBox="0 0 1000 620"
        role="img"
        aria-label="DataFlash die explorer"
      >
        <defs>
          <pattern id="storm-grid" width="28" height="28" patternUnits="userSpaceOnUse">
            <path d="M 28 0 L 0 0 0 28" fill="none" stroke="rgba(125,211,252,0.13)" stroke-width="1" />
          </pattern>
          <filter id="storm-glow">
            <feGaussianBlur stdDeviation="7" result="blur" />
            <feMerge>
              <feMergeNode in="blur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
          <linearGradient id="storm-trace" x1="0" x2="1" y1="0" y2="1">
            <stop offset="0%" stop-color="#22d3ee" stop-opacity="0.9" />
            <stop offset="52%" stop-color="#7dd3fc" stop-opacity="0.3" />
            <stop offset="100%" stop-color="#fb7185" stop-opacity="0.7" />
          </linearGradient>
        </defs>

        <rect class="die-backplate" x="0" y="0" width="1000" height="620" rx="34" />
        <rect class="die-grid" x="18" y="18" width="964" height="584" rx="26" />
        <path class="die-trace trace-a" d="M80 88 H310 V170 H472 V96 H840" />
        <path class="die-trace trace-b" d="M116 512 H430 V448 H642 V532 H900" />
        <path class="die-trace trace-c" d="M65 292 H218 V344 H410 V278 H706 V334 H944" />

        <g v-if="level === 'die'" class="die-layer regions">
          <g
            v-for="node in dieRegionNodes"
            :key="node.region.id"
            class="die-cell region-cell"
            :class="node.region.kind"
            role="button"
            tabindex="0"
            @click="enterRegion(node.region)"
            @keydown.enter.prevent="enterRegion(node.region)"
          >
            <rect :x="node.x" :y="node.y" :width="node.width" :height="node.height" rx="18" />
            <text v-if="node.width > 92 && node.height > 50" :x="node.x + 22" :y="node.y + 38">{{ node.region.name }}</text>
            <text v-if="node.width > 150 && node.height > 86" class="micro" :x="node.x + 22" :y="node.y + 66">
              {{ hex(node.region.start) }} - {{ hex(node.region.start + node.region.size - 1) }}
            </text>
          </g>
        </g>

        <g v-else-if="level === 'region'" class="die-layer pages">
          <g
            v-for="page in pageNodes"
            :key="page.start"
            class="die-cell page-cell"
            :class="{ active: selectedPageOffset === page.offset }"
            @click="enterPage(page)"
          >
            <rect :x="page.x" :y="page.y" :width="page.width" :height="page.height" rx="14" />
            <text v-if="page.width > 72 && page.height > 42" :x="page.x + 16" :y="page.y + 30">{{ page.index }}</text>
          </g>
        </g>

        <g v-else-if="level === 'page'" class="die-layer clusters">
          <g
            v-for="cluster in clusterNodes"
            :key="cluster.index"
            class="die-cell cluster-cell"
            :class="{ active: selectedClusterIndex === cluster.index }"
            @click="enterCluster(cluster)"
          >
            <rect :x="cluster.x" :y="cluster.y" :width="cluster.width" :height="cluster.height" rx="16" />
            <text :x="cluster.x + 20" :y="cluster.y + 34">{{ hex(selectedPageStart + cluster.startOffset, 4) }}</text>
            <text class="micro" :x="cluster.x + 20" :y="cluster.y + cluster.height - 20">16B</text>
          </g>
        </g>

        <g v-else class="die-layer bytes">
          <g
            v-for="node in byteNodes"
            :key="node.offset"
            class="die-cell byte-cell"
            :class="{
              active: selectedByteOffset === node.offset,
              selected: node.offset >= rangeStart && node.offset <= rangeEnd,
              empty: node.value === 0xff,
              zero: node.value === 0x00
            }"
            @pointerdown.prevent="beginRange(node, $event)"
            @pointerenter="updateRange(node)"
            @dblclick.stop="enterByte(node)"
            @click.stop="level === 'cluster' ? undefined : enterByte(node)"
          >
            <rect :x="node.x" :y="node.y" :width="node.width" :height="node.height" rx="20" />
            <text :x="node.x + node.width / 2" :y="node.y + node.height / 2 - 8" text-anchor="middle">{{ byteHex(node.value) }}</text>
            <text class="micro" :x="node.x + node.width / 2" :y="node.y + node.height / 2 + 28" text-anchor="middle">{{ hex(node.abs, 4) }}</text>
          </g>
        </g>
      </svg>

      <div v-if="level === 'byte' || hasRange" class="die-arm" :class="{ armed: canWriteByte }">
        <span>{{ hasRange ? `${hex(selectedRangeAbsStart)} - ${hex(selectedRangeAbsEnd)}` : `${hex(selectedByteAbs)} = ${byteHex(selectedByteValue)}` }}</span>
        <input v-if="!hasRange" v-model="byteDraft" maxlength="2" spellcheck="false" aria-label="新值 HEX" />
        <input v-if="!hasRange" v-model="dangerConfirm" spellcheck="false" aria-label="输入 DANGER 解锁" placeholder="DANGER" />
        <button v-if="!hasRange" type="button" :disabled="!canWriteByte || isBusy" @click="commitByteWrite">WRITE</button>
        <code>{{ hex(selectedPageStart, 4) }} - {{ hex(selectedPageStart + pageSize - 1, 4) }}</code>
      </div>
    </div>
  </Dialog>
</template>

<style>
.storm-dataflash-dialog {
  border: 1px solid rgba(125, 211, 252, 0.2) !important;
  background: rgba(0, 6, 18, 0.66) !important;
  box-shadow: 0 28px 90px rgba(0, 0, 0, 0.62), 0 0 90px rgba(14, 165, 233, 0.18) !important;
  backdrop-filter: blur(20px) saturate(1.25);
  -webkit-backdrop-filter: blur(20px) saturate(1.25);
}

.storm-dataflash-dialog .p-dialog-content {
  padding: 0 !important;
  background: transparent !important;
  overflow: hidden !important;
}

.die-explorer,
.die-explorer * {
  box-sizing: border-box;
}

.die-explorer button,
.die-explorer input {
  appearance: none;
  -webkit-appearance: none;
  border: 0;
  font: inherit;
}

.die-explorer {
  position: relative;
  height: calc(100vh - 24px);
  min-height: 660px;
  color: #dbeafe;
  background:
    radial-gradient(circle at 50% 34%, rgba(14, 165, 233, 0.16), transparent 38%),
    radial-gradient(circle at 78% 78%, rgba(244, 63, 94, 0.12), transparent 24%),
    linear-gradient(135deg, #020617 0%, #050816 48%, #020617 100%);
  overflow: hidden;
  outline: none;
}

.die-stars,
.die-scan {
  position: absolute;
  inset: 0;
  pointer-events: none;
}

.die-stars {
  background:
    radial-gradient(circle at 18% 28%, rgba(125, 211, 252, 0.16) 0 1px, transparent 2px),
    radial-gradient(circle at 62% 14%, rgba(186, 230, 253, 0.12) 0 1px, transparent 2px),
    radial-gradient(circle at 78% 66%, rgba(251, 113, 133, 0.12) 0 1px, transparent 2px),
    repeating-linear-gradient(90deg, rgba(148, 163, 184, 0.025) 0 1px, transparent 1px 28px);
}

.die-scan {
  background: linear-gradient(112deg, transparent 0 42%, rgba(186, 230, 253, 0.2) 46%, transparent 50% 100%);
  opacity: 0.28;
  animation: dieSweep 5.8s ease-in-out infinite;
}

.die-canvas {
  position: absolute;
  inset: 4.4rem 1.35rem 4.6rem;
  width: calc(100% - 2.7rem);
  height: calc(100% - 9rem);
  filter: drop-shadow(0 28px 60px rgba(0, 0, 0, 0.38));
  transform-origin: 50% 50%;
}

.die-backplate {
  fill: rgba(2, 6, 23, 0.72);
  stroke: rgba(125, 211, 252, 0.2);
  stroke-width: 2;
}

.die-grid {
  fill: url(#storm-grid);
  stroke: rgba(125, 211, 252, 0.18);
  stroke-width: 1.4;
}

.die-trace {
  fill: none;
  stroke: url(#storm-trace);
  stroke-width: 2.2;
  stroke-linecap: round;
  stroke-dasharray: 12 22;
  opacity: 0.35;
  animation: traceFlow 3.4s linear infinite;
}

.trace-b { animation-duration: 4.2s; opacity: 0.26; }
.trace-c { animation-duration: 5.1s; opacity: 0.22; }

.die-layer {
  filter: url(#storm-glow);
}

.die-cell {
  cursor: pointer;
  outline: none;
}

.die-cell rect {
  fill: rgba(15, 23, 42, 0.76);
  stroke: rgba(148, 163, 184, 0.22);
  stroke-width: 1.8;
  transition: fill 0.24s ease, stroke 0.24s ease, opacity 0.24s ease, transform 0.24s ease;
  transform-box: fill-box;
  transform-origin: center;
}

.die-cell:hover rect,
.die-cell.active rect,
.die-cell.selected rect {
  stroke: rgba(186, 230, 253, 0.92);
  transform: scale(0.988);
}

.die-cell text {
  fill: #e0f2fe;
  pointer-events: none;
  font-size: 24px;
  font-weight: 900;
  letter-spacing: 0;
}

.die-cell .micro {
  fill: #7dd3fc;
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 16px;
  font-weight: 800;
}

.region-cell.config rect { fill: rgba(37, 99, 235, 0.28); }
.region-cell.runtime rect { fill: rgba(20, 184, 166, 0.25); }
.region-cell.macro rect { fill: rgba(126, 34, 206, 0.32); }
.region-cell.reserved rect { fill: rgba(51, 65, 85, 0.58); }
.region-cell.protected rect { fill: rgba(159, 18, 57, 0.32); }

.page-cell rect {
  fill: rgba(8, 47, 73, 0.46);
}

.page-cell:nth-child(3n) rect {
  fill: rgba(30, 41, 59, 0.56);
}

.cluster-cell rect {
  fill: rgba(14, 116, 144, 0.32);
}

.byte-cell rect {
  fill: rgba(15, 23, 42, 0.82);
}

.byte-cell.empty rect {
  fill: rgba(30, 41, 59, 0.64);
}

.byte-cell.zero rect {
  fill: rgba(8, 145, 178, 0.34);
}

.byte-cell.selected rect {
  fill: rgba(14, 165, 233, 0.34);
  stroke: rgba(103, 232, 249, 0.96);
}

.byte-cell.active rect {
  fill: rgba(244, 63, 94, 0.36);
  stroke: rgba(251, 113, 133, 0.98);
}

.byte-cell text {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 42px;
}

.byte-cell .micro {
  font-size: 18px;
}

.die-hud {
  position: absolute;
  left: 50%;
  bottom: 1.15rem;
  z-index: 3;
  display: flex;
  align-items: center;
  gap: 0.85rem;
  max-width: calc(100% - 3rem);
  min-height: 2.35rem;
  padding: 0 1rem;
  border: 1px solid rgba(125, 211, 252, 0.28);
  border-radius: 999px;
  background: rgba(2, 6, 23, 0.62);
  color: #dbeafe;
  box-shadow: 0 0 30px rgba(14, 165, 233, 0.12);
  transform: translateX(-50%);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.die-hud.danger {
  border-color: rgba(251, 113, 133, 0.4);
  box-shadow: 0 0 30px rgba(244, 63, 94, 0.16);
}

.die-hud span,
.die-hud code,
.die-arm,
.die-arm input {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
}

.die-hud span {
  overflow: hidden;
  text-overflow: ellipsis;
  font-size: 0.78rem;
  font-weight: 900;
}

.die-hud code {
  flex: 0 0 auto;
  color: #67e8f9;
  font-size: 0.72rem;
}

.die-controls {
  position: absolute;
  top: 1.1rem;
  right: 1.1rem;
  z-index: 4;
  display: flex;
  gap: 0.45rem;
}

.die-icon {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 2.35rem;
  height: 2.35rem;
  border: 1px solid rgba(125, 211, 252, 0.28) !important;
  border-radius: 10px;
  background: rgba(8, 47, 73, 0.42);
  color: #bae6fd;
  cursor: pointer;
}

.die-icon:disabled {
  opacity: 0.3;
  cursor: default;
}

.die-arm {
  position: absolute;
  right: 1.1rem;
  bottom: 1.05rem;
  z-index: 4;
  display: flex;
  align-items: center;
  gap: 0.45rem;
  max-width: min(760px, calc(100% - 2.2rem));
  min-height: 2.55rem;
  padding: 0.35rem 0.48rem 0.35rem 0.75rem;
  border: 1px solid rgba(251, 113, 133, 0.32);
  border-radius: 999px;
  background: rgba(69, 10, 10, 0.48);
  color: #fecdd3;
  box-shadow: 0 0 26px rgba(244, 63, 94, 0.12);
}

.die-arm.armed {
  border-color: rgba(251, 113, 133, 0.78);
  box-shadow: 0 0 34px rgba(244, 63, 94, 0.28);
}

.die-arm span,
.die-arm code {
  font-size: 0.68rem;
  font-weight: 900;
  white-space: nowrap;
}

.die-arm code {
  color: #fda4af;
  opacity: 0.86;
}

.die-arm input {
  width: 5.4rem;
  height: 1.9rem;
  border: 1px solid rgba(251, 113, 133, 0.25);
  border-radius: 999px;
  background: rgba(2, 6, 23, 0.54);
  color: #fecdd3;
  padding: 0 0.62rem;
  outline: none;
}

.die-arm button {
  height: 1.9rem;
  padding: 0 0.85rem;
  border-radius: 999px;
  background: rgba(185, 28, 28, 0.46);
  color: #fee2e2;
  font-weight: 900;
  cursor: pointer;
}

.die-arm button:disabled {
  opacity: 0.38;
  cursor: default;
}

.spinning {
  animation: spin 1s linear infinite;
}

@media (max-width: 900px) {
  .die-explorer {
    min-height: 620px;
  }

  .die-canvas {
    inset: 4rem 0.75rem 5.3rem;
    width: calc(100% - 1.5rem);
    height: calc(100% - 9.3rem);
  }

  .die-hud {
    left: 0.75rem;
    right: 0.75rem;
    transform: none;
    max-width: none;
  }

  .die-arm {
    left: 0.75rem;
    right: 0.75rem;
    overflow-x: auto;
  }
}

@keyframes dieSweep {
  0%, 100% { transform: translateX(-12%); opacity: 0.18; }
  50% { transform: translateX(12%); opacity: 0.36; }
}

@keyframes traceFlow {
  to { stroke-dashoffset: -68; }
}

@keyframes spin {
  to { transform: rotate(1turn); }
}
</style>
