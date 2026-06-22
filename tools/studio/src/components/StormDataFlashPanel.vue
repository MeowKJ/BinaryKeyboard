<script setup lang="ts">
import gsap from 'gsap';
import { Application, Container, Graphics } from 'pixi.js';
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import { hidService } from '@/services/HidService';
import { showToast } from '@/services/toastService';
import StudioDialog from '@/components/StudioDialog.vue';
import { useDeviceStore } from '@/stores/deviceStore';
import {
  Command,
  DeviceProtocol,
  FRAME_SIZE,
  ResponseCode,
} from '@/types/protocol';

type FocusLevel = 'die' | 'region' | 'page' | 'cluster' | 'byte';
type RiskLevel = 'low' | 'medium' | 'high' | 'critical';

const WORLD_SIZE = 1000;
const DIE_SIZE = 960;
const DIE_MARGIN = 20;
const ADDRESS_COLUMNS = 256;
const ATLAS_PAGE_TILE = 54;
const ATLAS_PAGE_GAP = 0;
const CFG_SLOT_SIZE = 0x400;
const RUNTIME_PAGE_SIZE = 0x100;
const MEOWFS_PAGE_SIZE = 0x100;
const MEOWFS_ENTRY_HEADER_SIZE = 0x02;
const MEOWFS_ACTION_SIZE = 0x02;

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

interface RegionMeta {
  signal: string;
  detail: string;
  risk: RiskLevel;
}

interface DieRegionNode {
  region: FlashRegion;
  segmentStart?: number;
  segmentSize?: number;
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
  slotId?: number;
  label: string;
  detail: string;
  risk: RiskLevel;
  x: number;
  y: number;
  width: number;
  height: number;
}

interface SlotFrame {
  id: number;
  label: string;
  detail: string;
  start?: number;
  size?: number;
  risk?: RiskLevel;
  regionId?: string;
  x: number;
  y: number;
  width: number;
  height: number;
}

interface ClusterNode {
  index: number;
  startOffset: number;
  endOffset: number;
  label: string;
  detail: string;
  risk: RiskLevel;
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

interface HoverInfo {
  title: string;
  detail: string;
  range: string;
  risk: RiskLevel;
  x: number;
  y: number;
}

interface CameraCommand {
  x?: number;
  y?: number;
  clientX?: number;
  clientY?: number;
  scale?: number;
  tx?: number;
  ty?: number;
  raw?: boolean;
  duration?: number;
  address?: number;
}

interface CameraState {
  x: number;
  y: number;
  tx: number;
  ty: number;
  scale: number;
  lod: string;
  probe: ReturnType<typeof probeAddress>;
}

interface AreaAuditRow {
  id: string;
  bytes: number;
  byteRatio: number;
  areaRatio: number;
  error: number;
}

interface StormDataFlashCameraApi {
  goto: (x: number, y: number, scale?: number, duration?: number) => CameraState;
  raw: (tx: number, ty: number, scale?: number, duration?: number) => CameraState;
  zoom: (scale: number, x?: number, y?: number, duration?: number) => CameraState;
  zoomAtClient: (clientX: number, clientY: number, scale: number, duration?: number) => CameraState;
  screenToWorld: (clientX: number, clientY: number) => { x: number; y: number };
  focusAddress: (address: number, scale?: number, duration?: number) => Promise<CameraState>;
  probe: (x?: number, y?: number) => CameraState['probe'];
  areaAudit: () => AreaAuditRow[];
  pixiStats: () => { byteRenders: number; weatherRenders: number; pixiPresent: boolean; activeByte: ByteNode | null };
  reset: () => CameraState;
  get: () => CameraState;
}

declare global {
  interface Window {
    stormDataFlashCamera?: StormDataFlashCameraApi;
  }
}

const deviceStore = useDeviceStore();
const props = defineProps<{ visible: boolean }>();
const emit = defineEmits<{ 'update:visible': [value: boolean] }>();
const testParams = new URLSearchParams(window.location.search);
const isPerfTestMode = testParams.has('stormDataFlashPerf');
const isVisualTestMode = testParams.has('stormDataFlashTest') && !isPerfTestMode;

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
const explorerEl = ref<HTMLElement | null>(null);
const dieEl = ref<SVGSVGElement | null>(null);
const pixiHostEl = ref<HTMLElement | null>(null);
const hoverInfo = ref<HoverInfo | null>(null);
const meaningEl = ref<HTMLElement | null>(null);
const writePanelOpen = ref(false);
const cameraX = ref(0);
const cameraY = ref(0);
const cameraScale = ref(1);
const pointerWorldX = ref(500);
const pointerWorldY = ref(500);
const isCameraDragging = ref(false);
const cameraDragStart = ref({ x: 0, y: 0, cameraX: 0, cameraY: 0 });
const cameraMoved = ref(false);
const cameraVelocity = ref({ x: 0, y: 0 });
const lastDragViewport = ref({ x: 0, y: 0, t: 0 });
const hoveredRegionId = ref<string | null>(null);
const hoveredPageStart = ref<number | null>(null);

let pixiApp: Application | null = null;
let pixiWorld: Container | null = null;
let pixiByteLayer: Graphics | null = null;
let pixiWeatherLayer: Graphics | null = null;
let pixiResizeObserver: ResizeObserver | null = null;
let inertiaFrame = 0;
let weatherPhase = 0;
let cameraTween: gsap.core.Tween | null = null;
let wheelTarget: SVGSVGElement | null = null;
let cameraApiInstalled = false;
let weatherRenderMs = 0;
let pixiByteRenderCount = 0;
let pixiWeatherRenderCount = 0;

const regionMetaMap: Record<string, RegionMeta> = {
  config: {
    signal: 'CONFIG CORE',
    detail: '3 x 1KB rotating config slots: header, system, keymap, FN and RGB.',
    risk: 'high',
  },
  runtime: {
    signal: 'RUNTIME TRACE',
    detail: '4 x 256B hot pages for current layer and last mode, selected by seq + CRC.',
    risk: 'medium',
  },
  macro: {
    signal: 'MEOWFS VAULT',
    detail: '8KB append-style MeowFS stream: marker, action count, 2-byte actions.',
    risk: 'high',
  },
  reserved: {
    signal: 'QUIET ZONE',
    detail: 'Unassigned DataFlash between MeowFS and BLE SNV. Area is real reserved bytes.',
    risk: 'medium',
  },
  ble: {
    signal: 'BLE SNV',
    detail: '256B WCH BLE stack SNV block for pairing and bonding state.',
    risk: 'critical',
  },
  tail: {
    signal: 'UNASSIGNED TAIL',
    detail: 'Final 256B unassigned DataFlash page after BLE SNV; no firmware lock is inferred.',
    risk: 'medium',
  },
};

const regions = computed<FlashRegion[]>(() => {
  const info = dfInfo.value;
  const reservedStart = info.macroBase + info.macroSize;
  const tailStart = info.bleSnvBase + info.bleSnvSize;
  const list: FlashRegion[] = [
    { id: 'config', name: 'CONFIG', start: 0x0000, size: info.configEnd, kind: 'config' },
    { id: 'runtime', name: 'RUNTIME', start: info.runtimeBase, size: info.macroBase - info.runtimeBase, kind: 'runtime' },
    { id: 'macro', name: 'MeowFS', start: info.macroBase, size: info.macroSize, kind: 'macro' },
    { id: 'reserved', name: 'RESERVED', start: reservedStart, size: Math.max(0, info.bleSnvBase - reservedStart), kind: 'reserved' },
    { id: 'ble', name: 'BLE SNV', start: info.bleSnvBase, size: info.bleSnvSize, kind: 'protected' },
    { id: 'tail', name: 'TAIL RSV', start: tailStart, size: Math.max(0, info.total - tailStart), kind: 'reserved' },
  ];
  return list.filter((region) => region.size > 0);
});

const selectedRegion = computed(
  () => regions.value.find((region) => region.id === selectedRegionId.value) ?? regions.value[0],
);
const selectedRegionMeta = computed(() => regionMeta(selectedRegion.value?.id));

const pageSize = computed(() => dfInfo.value.page || 0x100);
const addressRows = computed(() => Math.max(1, Math.ceil(dfInfo.value.total / ADDRESS_COLUMNS)));
const byteWorldWidth = computed(() => DIE_SIZE / ADDRESS_COLUMNS);
const byteWorldHeight = computed(() => DIE_SIZE / addressRows.value);
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
const levelLabel = computed(() => level.value.toUpperCase());
const accessState = computed(() => {
  if (isBusy.value) return 'SYNCING';
  if (dfInfo.value.rawAvailable) return canWriteByte.value ? 'ARMED' : 'READ LIVE';
  if (deviceStore.deviceInfo?.protocol !== DeviceProtocol.CH592) return 'CH592 ONLY';
  return 'FIRMWARE LOCKED';
});
const dangerTone = computed(() => {
  if (canWriteByte.value) return 'critical';
  if (level.value === 'byte' || hasRange.value) return 'high';
  return selectedRegionMeta.value.risk;
});
const writePreview = computed(() => `${byteHex(selectedByteValue.value)} -> ${byteDraft.value.trim().toUpperCase().padStart(2, '0').slice(0, 2)}`);
const cameraTransform = computed(() => `matrix(${cameraScale.value.toFixed(4)} 0 0 ${cameraScale.value.toFixed(4)} ${cameraX.value.toFixed(2)} ${cameraY.value.toFixed(2)})`);
const cameraLod = computed(() => {
  if (cameraScale.value >= 120) return 'LOD-7 SUB-BYTE SCOPE';
  if (cameraScale.value >= 28) return 'LOD-6 BYTE MICROSCOPE';
  if (cameraScale.value >= 8) return 'LOD-5 BYTE SCOPE';
  if (cameraScale.value >= 3.4) return 'LOD-4 FIELD MATRIX';
  if (cameraScale.value >= 2.2) return 'LOD-3 FIELD LABELS';
  if (cameraScale.value >= 1.25) return 'LOD-2 PAGE GRID';
  return 'LOD-1 DIE';
});
const showPageLod = computed(() => cameraScale.value >= 0.7 || hoveredRegionId.value !== null);
const showFieldLod = computed(() => cameraScale.value >= 1.65);
const showPhysicalLod = computed(() => cameraScale.value >= 8);
const showByteLod = computed(() => showPhysicalLod.value && activeMapPage.value !== null);
const showBitScope = computed(() => cameraScale.value >= 120);
const showHoverTooltip = computed(() => cameraScale.value >= 0.95);
const regionLayerStyle = computed(() => ({
  opacity: String(cameraScale.value >= 80 ? 0.045 : cameraScale.value >= 8 ? 0.12 : cameraScale.value >= 3.1 ? 0.24 : cameraScale.value >= 1.65 ? 0.46 : 0.92),
}));
const pageLayerStyle = computed(() => ({
  opacity: String(cameraScale.value >= 80 ? 0.075 : cameraScale.value >= 8 ? 0.18 : clamp((cameraScale.value - 0.45) / 1.25, 0.38, cameraScale.value >= 3.1 ? 0.5 : 0.9)),
}));
const coordinateHudText = computed(() => {
  const probe = probeAddress(pointerWorldX.value, pointerWorldY.value);
  const page = Math.floor(probe.address / pageSize.value);
  const byte = probe.address % pageSize.value;
  const addrLabel = probe.exact ? 'ADDR' : 'ADDR~';
  return [
    `X ${Math.round(pointerWorldX.value).toString().padStart(4, '0')}`,
    `Y ${Math.round(pointerWorldY.value).toString().padStart(4, '0')}`,
    `${addrLabel} ${hex(probe.address, 4)}`,
    `PAGE ${page.toString().padStart(2, '0')}`,
    `BYTE ${byte.toString().padStart(3, '0')}`,
    `ZOOM ${Math.round(cameraScale.value * 100)}%`,
    cameraLod.value,
  ].join(' | ');
});
const areaAuditJson = computed(() => JSON.stringify(auditDieArea()));

const addressTicks = computed(() =>
  regions.value
    .flatMap((region) => [
      { address: region.start, label: hex(region.start, 4), kind: region.id },
      region.id === 'tail' ? { address: region.start + region.size, label: hex(region.start + region.size, 4), kind: 'end' } : null,
    ])
    .filter((tick): tick is { address: number; label: string; kind: string } => tick !== null)
    .map((tick) => ({
      ...tick,
      y: DIE_MARGIN + (tick.address / Math.max(1, dfInfo.value.total)) * DIE_SIZE,
    })),
);

const padRingNodes = computed(() => {
  const pads: Array<{ id: string; x: number; y: number; width: number; height: number }> = [];
  const size = 14;
  const pitch = 36;
  for (let i = 0; i < 24; i++) {
    pads.push({ id: `top-${i}`, x: 70 + i * pitch, y: 24, width: 20, height: size });
    pads.push({ id: `bottom-${i}`, x: 70 + i * pitch, y: 962, width: 20, height: size });
  }
  for (let i = 0; i < 24; i++) {
    pads.push({ id: `left-${i}`, x: 24, y: 70 + i * pitch, width: size, height: 20 });
    pads.push({ id: `right-${i}`, x: 962, y: 70 + i * pitch, width: size, height: 20 });
  }
  return pads;
});

const decoderRibs = computed(() =>
  Array.from({ length: 11 }, (_, index) => ({
    id: index,
    y: 104 + index * ATLAS_PAGE_TILE,
    hot: index === 1 || index === 4 || index >= 10,
  })),
);

const senseAmpNodes = computed(() =>
  Array.from({ length: 13 }, (_, index) => ({
    id: index,
    x: 68 + index * ATLAS_PAGE_TILE + ATLAS_PAGE_TILE * 0.22,
    y: 104 + 10 * ATLAS_PAGE_TILE + 78,
  })),
);

const hardwareBankNodes = computed(() =>
  Array.from({ length: 72 }, (_, index) => {
    const col = index % 6;
    const row = Math.floor(index / 6);
    return {
      id: index,
      x: 790 + col * 25,
      y: 132 + row * 42,
      width: 15,
      height: row % 3 === 1 ? 31 : 25,
      hot: (index + row) % 11 === 0,
    };
  }),
);

const capacityRulerTicks = computed(() => {
  const nodes = [...dieRegionNodes.value].sort((a, b) => a.region.start - b.region.start);
  const ticks = nodes.map((node) => ({
    key: `${node.region.id}-start`,
    label: hex(node.region.start, 4),
    name: node.region.name,
    x: node.x,
    y: node.y,
    width: node.width,
  }));
  const last = nodes[nodes.length - 1];
  if (last) {
    ticks.push({
      key: 'flash-end',
      label: hex(last.region.start + last.region.size, 4),
      name: 'END',
      x: last.x + last.width,
      y: last.y + last.height,
      width: 0,
    });
  }
  return ticks;
});

const chargePackets = [
  { id: 'cfg-runtime', x: 238, y: 158, r: 7, tone: 'cyan' },
  { id: 'macro-a', x: 522, y: 266, r: 10, tone: 'cyan' },
  { id: 'macro-b', x: 846, y: 266, r: 8, tone: 'red' },
  { id: 'reserved-hot-a', x: 230, y: 482, r: 12, tone: 'red' },
  { id: 'reserved-hot-b', x: 716, y: 536, r: 10, tone: 'red' },
  { id: 'ble-fuse', x: 96, y: 672, r: 14, tone: 'red' },
] as const;

const chargeTrails = [
  { id: 'trail-cfg', d: 'M118 158 H238 L286 212', tone: 'cyan' },
  { id: 'trail-macro', d: 'M314 266 H522 H846', tone: 'red' },
  { id: 'trail-reserved-a', d: 'M176 374 C230 420 230 452 230 482', tone: 'red' },
  { id: 'trail-reserved-b', d: 'M662 320 C716 386 716 472 716 536', tone: 'red' },
  { id: 'trail-ble', d: 'M96 644 V672 H176', tone: 'red' },
] as const;

interface LayoutRect {
  x: number;
  y: number;
  width: number;
  height: number;
}

function orderedAreaLayout<T extends { size: number }>(
  items: T[],
  bounds: LayoutRect,
  axis: 'vertical' | 'horizontal' = 'vertical',
): Array<T & LayoutRect> {
  const visibleItems = items.filter((item) => item.size > 0);
  const total = visibleItems.reduce((sum, item) => sum + item.size, 0);
  if (total <= 0) return [];

  let cursor = axis === 'vertical' ? bounds.y : bounds.x;
  return visibleItems.map((item, index) => {
    const isLast = index === visibleItems.length - 1;
    if (axis === 'vertical') {
      const height = isLast ? bounds.y + bounds.height - cursor : bounds.height * (item.size / total);
      const rect = { ...item, x: bounds.x, y: cursor, width: bounds.width, height };
      cursor += height;
      return rect;
    }
    const width = isLast ? bounds.x + bounds.width - cursor : bounds.width * (item.size / total);
    const rect = { ...item, x: cursor, y: bounds.y, width, height: bounds.height };
    cursor += width;
    return rect;
  });
}

function gridAreaLayout<T extends { size: number }>(
  items: T[],
  bounds: LayoutRect,
  columns: number,
): Array<T & LayoutRect> {
  const visibleItems = items.filter((item) => item.size > 0);
  if (!visibleItems.length) return [];
  const rows = Math.max(1, Math.ceil(visibleItems.length / columns));
  const cellWidth = bounds.width / columns;
  const cellHeight = bounds.height / rows;
  return visibleItems.map((item, index) => ({
    ...item,
    x: bounds.x + (index % columns) * cellWidth,
    y: bounds.y + Math.floor(index / columns) * cellHeight,
    width: cellWidth,
    height: cellHeight,
  }));
}

function sliceAxis(bounds: LayoutRect, depth = 0): 'vertical' | 'horizontal' {
  if (depth % 2 === 1) return bounds.width >= bounds.height ? 'horizontal' : 'vertical';
  return bounds.height >= bounds.width ? 'vertical' : 'horizontal';
}

function areaBox(area: number, ratio: number): { width: number; height: number } {
  const width = Math.sqrt(Math.max(1, area) * ratio);
  return { width, height: area / width };
}

function atlasRect(x: number, y: number, columns: number, rows: number): LayoutRect {
  return {
    x,
    y,
    width: columns * ATLAS_PAGE_TILE + Math.max(0, columns - 1) * ATLAS_PAGE_GAP,
    height: rows * ATLAS_PAGE_TILE + Math.max(0, rows - 1) * ATLAS_PAGE_GAP,
  };
}

function atlasSpecForRegion(region: FlashRegion): LayoutRect & { columns: number } {
  const x = 68;
  const y = 104;
  const specs: Record<string, LayoutRect & { columns: number }> = {
    config: { ...atlasRect(x, y, 12, 1), columns: 12 },
    runtime: { ...atlasRect(x, y + ATLAS_PAGE_TILE, 4, 1), columns: 4 },
    macro: { ...atlasRect(x, y + 2 * ATLAS_PAGE_TILE, 16, 2), columns: 16 },
    reserved: { ...atlasRect(x, y + 4 * ATLAS_PAGE_TILE, 13, 6), columns: 13 },
    ble: { ...atlasRect(x, y + 10 * ATLAS_PAGE_TILE, 1, 1), columns: 1 },
    tail: { ...atlasRect(x + ATLAS_PAGE_TILE, y + 10 * ATLAS_PAGE_TILE, 1, 1), columns: 1 },
  };
  return specs[region.id] ?? { ...atlasRect(DIE_MARGIN, DIE_MARGIN, Math.max(1, Math.ceil(region.size / pageSize.value)), 1), columns: 1 };
}

function atlasTileRect(bounds: LayoutRect, index: number, columns: number): LayoutRect {
  const col = index % columns;
  const row = Math.floor(index / columns);
  return {
    x: bounds.x + col * (ATLAS_PAGE_TILE + ATLAS_PAGE_GAP),
    y: bounds.y + row * (ATLAS_PAGE_TILE + ATLAS_PAGE_GAP),
    width: ATLAS_PAGE_TILE,
    height: ATLAS_PAGE_TILE,
  };
}

function atlasPageLayout<T extends { size: number }>(
  items: T[],
  bounds: LayoutRect,
  columns: number,
): Array<T & LayoutRect> {
  return items
    .filter((item) => item.size > 0)
    .map((item, index) => ({
      ...item,
      ...atlasTileRect(bounds, index, columns),
    }));
}

function atlasConfigSlotLayout<T extends { size: number }>(items: T[], bounds: LayoutRect): Array<T & LayoutRect> {
  if (bounds.width >= bounds.height * 2) {
    const width = bounds.width / Math.max(1, items.length);
    return items
      .filter((item) => item.size > 0)
      .map((item, index) => ({
        ...item,
        x: bounds.x + index * width,
        y: bounds.y,
        width,
        height: bounds.height,
      }));
  }
  return items
    .filter((item) => item.size > 0)
    .map((item, index) => ({
      ...item,
      x: bounds.x,
      y: bounds.y + index * (ATLAS_PAGE_TILE + ATLAS_PAGE_GAP),
      width: bounds.width,
      height: ATLAS_PAGE_TILE,
    }));
}

const dieRegionNodes = computed<DieRegionNode[]>(() => {
  const ordered = [...regions.value].sort((a, b) => a.start - b.start);
  return ordered.map((region) => ({
    region,
    segmentStart: region.start,
    segmentSize: region.size,
    ...atlasSpecForRegion(region),
  }));
});

type PageDatum = Omit<PageNode, 'x' | 'y' | 'width' | 'height'> & { children?: PageDatum[] };

const pageLayout = computed(() => {
  const slices = buildFunctionalSlices(selectedRegion.value);
  const children: PageDatum[] = selectedRegion.value.id === 'config'
    ? groupConfigSlotSlices(slices)
    : slices;
  const frameRects = orderedAreaLayout(children, { x: 0, y: 0, width: 1000, height: 620 }, 'vertical');
  const leaves = frameRects.flatMap((frame) => {
    if (!frame.children?.length) return [{ ...frame, children: undefined }];
    return orderedAreaLayout(frame.children, frame, 'vertical').map((leaf) => ({ ...leaf, children: undefined }));
  });

  const frames: SlotFrame[] = frameRects.map((node) => ({
    id: node.slotId ?? node.index,
    label: node.label,
    detail: node.detail,
    start: node.start,
    size: node.size,
    risk: node.risk,
    x: node.x,
    y: node.y,
    width: node.width,
    height: node.height,
  }));

  return { leaves, frames };
});

const pageNodes = computed<PageNode[]>(() => pageLayout.value.leaves);
const slotFrames = computed<SlotFrame[]>(() => pageLayout.value.frames);
const mapPageNodes = computed<MapPageNode[]>(() =>
  dieRegionNodes.value.flatMap((regionNode) => {
    const region = regionNode.region;
    const frames = layoutSemanticFrames(
      region,
      { x: regionNode.x, y: regionNode.y, width: regionNode.width, height: regionNode.height },
    );
    return frames.flatMap((frame) => {
      const leaves = frame.children?.length
        ? orderedAreaLayout(frame.children, frame, region.id === 'config' ? 'horizontal' : sliceAxis(frame, 1))
        : [frame];
      return leaves.map((data, index) => {
      return {
        ...data,
        index,
        regionId: region.id,
        regionName: region.name,
        pageNumber: Math.floor(data.start / pageSize.value),
        label: compactSlotLabel(data.label, data.slotId),
        detail: `${data.detail} / ${hex(data.start, 4)} - ${hex(data.start + data.size - 1, 4)} / ${data.size}B`,
        x: data.x,
        y: data.y,
        width: data.width,
        height: data.height,
      };
      });
    });
  }),
);
const mapFrameNodes = computed<SlotFrame[]>(() =>
  dieRegionNodes.value.flatMap((regionNode) => {
    const frames = layoutSemanticFrames(
      regionNode.region,
      { x: regionNode.x, y: regionNode.y, width: regionNode.width, height: regionNode.height },
    );
    return frames.map((frame) => ({
      id: frame.slotId ?? frame.index,
      label: frame.label,
      detail: frame.detail,
      start: frame.start,
      size: frame.size,
      risk: frame.risk,
      regionId: regionNode.region.id,
      x: frame.x,
      y: frame.y,
      width: frame.width,
      height: frame.height,
    }));
  }),
);
const physicalPageNodes = computed(() =>
  dieRegionNodes.value.flatMap((regionNode) => {
    const region = regionNode.region;
    const pageCount = Math.max(1, Math.ceil(region.size / pageSize.value));
    const spec = atlasSpecForRegion(region);
    return Array.from({ length: pageCount }, (_, index) => {
      const offset = index * pageSize.value;
      const rect = atlasTileRect(
        { x: regionNode.x, y: regionNode.y, width: regionNode.width, height: regionNode.height },
        index,
        spec.columns,
      );
      return {
        id: `${region.id}-${index}`,
        regionId: region.id,
        start: region.start + offset,
        pageNumber: Math.floor((region.start + offset) / pageSize.value),
        major: index === 0 || index === pageCount - 1 || index % 16 === 0,
        ...rect,
      };
    });
  }),
);
const activeMapPage = computed(() => {
  const hovered = hoveredPageStart.value;
  return mapPageNodes.value.find((page) => page.start === hovered)
    ?? mapPageNodes.value.find((page) => selectedByteAbs.value >= page.start && selectedByteAbs.value < page.start + page.size)
    ?? mapPageNodes.value.find((page) => selectedPageStart.value >= page.start && selectedPageStart.value < page.start + page.size)
    ?? null;
});
const mapByteNodes = computed<ByteNode[]>(() => {
  const page = activeMapPage.value;
  if (!page) return [];
  const count = Math.min(256, page.size);
  const columns = byteGridColumns(page, count);
  const rows = Math.max(1, Math.ceil(count / columns));
  const cellWidth = page.width / columns;
  const cellHeight = page.height / rows;
  return Array.from({ length: count }, (_, index) => {
    const abs = page.start + index;
    const pageOffset = abs - selectedPageStart.value;
    const isLoadedPage = pageOffset >= 0 && pageOffset < pageBytes.value.length;
    const value = isLoadedPage ? (pageBytes.value[pageOffset] ?? 0xff) : 0xff;
    return {
      offset: pageOffset,
      abs,
      value,
      x: page.x + (index % columns) * cellWidth,
      y: page.y + Math.floor(index / columns) * cellHeight,
      width: cellWidth,
      height: cellHeight,
    };
  });
});

const byteGridLines = computed(() => {
  const page = activeMapPage.value;
  if (!page) return [];
  const count = Math.min(256, page.size);
  const columns = byteGridColumns(page, count);
  const rows = Math.max(1, Math.ceil(count / columns));
  const cellWidth = page.width / columns;
  const cellHeight = page.height / rows;
  const vertical = Array.from({ length: columns + 1 }, (_, index) => ({
    d: `M${(page.x + index * cellWidth).toFixed(3)} ${page.y.toFixed(3)} V${(page.y + page.height).toFixed(3)}`,
  }));
  const horizontal = Array.from({ length: rows + 1 }, (_, index) => ({
    d: `M${page.x.toFixed(3)} ${(page.y + index * cellHeight).toFixed(3)} H${(page.x + page.width).toFixed(3)}`,
  }));
  return [...vertical, ...horizontal];
});

const selectedBitNodes = computed(() => {
  const byte = mapByteNodes.value.find((node) => node.offset === selectedByteOffset.value);
  if (!byte) return [];
  const gap = Math.min(byte.width, byte.height) * 0.08;
  const bitWidth = (byte.width - gap * 9) / 8;
  return Array.from({ length: 8 }, (_, bit) => {
    const on = ((byte.value >> (7 - bit)) & 1) === 1;
    return {
      bit,
      on,
      x: byte.x + gap + bit * (bitWidth + gap),
      y: byte.y + byte.height * 0.18,
      width: bitWidth,
      height: byte.height * 0.64,
    };
  });
});

const selectedContactNodes = computed(() => {
  const byte = mapByteNodes.value.find((node) => node.offset === selectedByteOffset.value);
  if (!byte) return [];
  const cols = 8;
  const rows = 3;
  const r = Math.min(byte.width, byte.height) * 0.025;
  return Array.from({ length: cols * rows }, (_, index) => ({
    id: index,
    x: byte.x + byte.width * (0.12 + (index % cols) * 0.105),
    y: byte.y + byte.height * (0.14 + Math.floor(index / cols) * 0.36),
    r,
  }));
});

const selectedByteGuides = computed(() => {
  const page = activeMapPage.value;
  const byte = mapByteNodes.value.find((node) => node.offset === selectedByteOffset.value);
  if (!page || !byte) return null;
  const cx = byte.x + byte.width / 2;
  const cy = byte.y + byte.height / 2;
  return {
    row: `M${page.x} ${cy} H${page.x + page.width}`,
    column: `M${cx} ${page.y} V${page.y + page.height}`,
    pageFrame: {
      x: page.x,
      y: page.y,
      width: page.width,
      height: page.height,
    },
    byteFrame: {
      x: byte.x,
      y: byte.y,
      width: byte.width,
      height: byte.height,
    },
  };
});

const selectedPageContextCells = computed(() =>
  Array.from({ length: 256 }, (_, index) => ({
    index,
    active: index === selectedByteOffset.value,
    row: Math.floor(index / 16),
    col: index % 16,
  })),
);

const selectedByteCircuit = computed(() => {
  const byte = mapByteNodes.value.find((node) => node.offset === selectedByteOffset.value);
  if (!byte) return null;
  const left = byte.x;
  const right = byte.x + byte.width;
  const top = byte.y;
  const bottom = byte.y + byte.height;
  const lanes = Array.from({ length: 8 }, (_, index) => {
    const y = top + byte.height * (0.16 + index * 0.096);
    return {
      id: index,
      y,
      on: ((byte.value >> (7 - index)) & 1) === 1,
      d: `M${left + byte.width * 0.08} ${y} H${right - byte.width * 0.08}`,
    };
  });
  const vias = Array.from({ length: 16 }, (_, index) => ({
    id: index,
    x: left + byte.width * (0.11 + (index % 8) * 0.11),
    y: index < 8 ? top + byte.height * 0.12 : bottom - byte.height * 0.12,
    on: ((byte.value >> (7 - (index % 8))) & 1) === 1,
    r: Math.min(byte.width, byte.height) * 0.018,
  }));
  return {
    byte,
    frame: {
      x: left + byte.width * 0.045,
      y: top + byte.height * 0.055,
      width: byte.width * 0.91,
      height: byte.height * 0.89,
    },
    lanes,
    vias,
  };
});

function byteGridColumns(bounds: LayoutRect, count: number): number {
  if (count >= 128) return 16;
  const ratio = Math.max(0.2, bounds.width / Math.max(1, bounds.height));
  return clamp(Math.ceil(Math.sqrt(Math.max(1, count) * ratio)), 1, Math.max(1, count));
}

function addressPointInMapNode(page: MapPageNode, address: number): { x: number; y: number } {
  const count = Math.min(256, page.size);
  const index = clamp(address - page.start, 0, Math.max(0, count - 1));
  const columns = byteGridColumns(page, count);
  const rows = Math.max(1, Math.ceil(count / columns));
  const cellWidth = page.width / columns;
  const cellHeight = page.height / rows;
  return {
    x: page.x + (index % columns + 0.5) * cellWidth,
    y: page.y + (Math.floor(index / columns) + 0.5) * cellHeight,
  };
}

function addressToCell(abs: number): { x: number; y: number; col: number; row: number } {
  const address = clamp(Math.floor(abs), 0, Math.max(0, dfInfo.value.total - 1));
  const col = address % ADDRESS_COLUMNS;
  const row = Math.floor(address / ADDRESS_COLUMNS);
  return {
    col,
    row,
    x: DIE_MARGIN + col * byteWorldWidth.value,
    y: DIE_MARGIN + row * byteWorldHeight.value,
  };
}

function addressRangeRects(start: number, size: number): Array<{ start: number; size: number; x: number; y: number; width: number; height: number }> {
  const total = dfInfo.value.total;
  const safeStart = clamp(Math.floor(start), 0, Math.max(0, total - 1));
  const end = clamp(Math.floor(start + Math.max(1, size) - 1), safeStart, Math.max(0, total - 1));
  const rects: Array<{ start: number; size: number; x: number; y: number; width: number; height: number }> = [];
  let cursor = safeStart;
  while (cursor <= end) {
    const col = cursor % ADDRESS_COLUMNS;
    const rowEnd = Math.min(end, cursor + (ADDRESS_COLUMNS - col) - 1);
    const count = rowEnd - cursor + 1;
    const cell = addressToCell(cursor);
    rects.push({
      start: cursor,
      size: count,
      x: cell.x,
      y: cell.y,
      width: count * byteWorldWidth.value,
      height: byteWorldHeight.value,
    });
    cursor = rowEnd + 1;
  }
  return rects;
}

function groupConfigSlotSlices(slices: PageDatum[]): PageDatum[] {
  return groupConfigSlicesForRegion(selectedRegion.value, slices);
}

function groupConfigSlicesForRegion(region: FlashRegion, slices: PageDatum[]): PageDatum[] {
  const slotSize = CFG_SLOT_SIZE;
  const slots = Math.max(1, Math.ceil(region.size / slotSize));
  return Array.from({ length: slots }, (_, slotId) => {
    const offset = slotId * slotSize;
    const slotSlices = slices.filter((slice) => slice.slotId === slotId);
    const risk: RiskLevel = slotId === 0 ? 'high' : 'medium';
    return {
      index: Math.floor(offset / pageSize.value),
      offset,
      start: region.start + offset,
      size: Math.min(slotSize, Math.max(1, region.size - offset)),
      slotId,
      label: `SLOT ${slotId}`,
      detail: `${slotSlices.length} ordered fields / ${Math.min(slotSize, Math.max(1, region.size - offset))}B`,
      risk,
      children: slotSlices,
    };
  });
}

function groupPageSlicesForRegion(region: FlashRegion, slices: PageDatum[], frameSize: number, labelPrefix: string): PageDatum[] {
  const count = Math.max(1, Math.ceil(region.size / frameSize));
  return Array.from({ length: count }, (_, pageId) => {
    const offset = pageId * frameSize;
    const pageSizeBytes = Math.min(frameSize, Math.max(1, region.size - offset));
    const children = slices.filter((slice) => slice.offset >= offset && slice.offset < offset + frameSize);
    const risk: RiskLevel = children.some((child) => child.risk === 'critical')
      ? 'critical'
      : children.some((child) => child.risk === 'high')
        ? 'high'
        : regionMeta(region.id).risk;
    return {
      index: Math.floor(offset / pageSize.value),
      offset,
      start: region.start + offset,
      size: pageSizeBytes,
      slotId: pageId,
      label: `${labelPrefix} ${String(pageId).padStart(2, '0')}`,
      detail: `${children.length ? `${children.length} ordered fields` : 'physical page'} / ${pageSizeBytes}B`,
      risk,
      children: children.length ? children : undefined,
    };
  });
}

function buildSemanticFrames(region: FlashRegion): PageDatum[] {
  const slices = buildFunctionalSlices(region);
  if (region.id === 'config') return groupConfigSlicesForRegion(region, slices);
  if (region.id === 'runtime') return groupPageSlicesForRegion(region, slices, RUNTIME_PAGE_SIZE, 'RT PAGE');
  if (region.id === 'macro') return groupPageSlicesForRegion(region, slices, MEOWFS_PAGE_SIZE, 'MEOWFS PAGE');
  if (region.id === 'reserved') return buildReservedBanks(region);
  return slices;
}

function buildReservedBanks(region: FlashRegion): PageDatum[] {
  const block = pageSize.value || 0x100;
  const count = Math.max(1, Math.ceil(region.size / block));
  return Array.from({ length: count }, (_, index): PageDatum => {
    const offset = index * block;
    const size = Math.min(block, Math.max(1, region.size - offset));
    const risk: RiskLevel = 'medium';
    return {
      index,
      offset,
      start: region.start + offset,
      size,
      slotId: index,
      label: `RSV PAGE ${String(index).padStart(2, '0')}`,
      detail: `unmapped 256B reserve page / ${size}B`,
      risk,
    };
  }).filter((page) => page.size > 0);
}

function layoutSemanticFrames(region: FlashRegion, bounds: LayoutRect): Array<PageDatum & LayoutRect> {
  const frames = buildSemanticFrames(region);
  const spec = atlasSpecForRegion(region);
  if (region.id === 'runtime') return atlasPageLayout(frames, bounds, spec.columns);
  if (region.id === 'macro') return atlasPageLayout(frames, bounds, spec.columns);
  if (region.id === 'reserved') return atlasPageLayout(frames, bounds, spec.columns);
  if (region.id === 'config') return atlasConfigSlotLayout(frames, bounds);
  if (region.id === 'ble' || region.id === 'tail') return atlasPageLayout(frames, bounds, spec.columns);
  return orderedAreaLayout(frames, bounds, sliceAxis(bounds));
}

function compactSlotLabel(label: string, slotId?: number): string {
  return slotId === undefined ? label : label.replace(`SLOT ${slotId} `, '');
}

function buildFunctionalSlices(region: FlashRegion): Array<Omit<PageNode, 'x' | 'y' | 'width' | 'height'>> {
  const make = (offset: number, size: number, label: string, detail: string, risk: RiskLevel, slotId?: number): Omit<PageNode, 'x' | 'y' | 'width' | 'height'> => ({
    index: Math.floor(offset / pageSize.value),
    offset,
    start: region.start + offset,
    size: Math.max(1, Math.min(Math.max(0, Math.floor(size)), Math.max(0, region.size - offset))),
    slotId,
    label,
    detail,
    risk,
  });

  if (region.id === 'config') {
    const slots = Math.max(1, Math.ceil(region.size / CFG_SLOT_SIZE));
    return Array.from({ length: slots }, (_, slot) => slot * CFG_SLOT_SIZE).flatMap((base) => {
      const slotId = base / CFG_SLOT_SIZE;
      const title = `SLOT ${slotId}`;
      return [
        make(base + 0x000, 0x020, `${title} HEADER`, 'kbd_config_header_t: magic/version/flags/crc32/save_count/reserved', 'critical', slotId),
        make(base + 0x020, 0x0e0, `${title} HEADER PAD`, 'reserved bytes before system config', 'medium', slotId),
        make(base + 0x100, 0x040, `${title} SYSTEM`, 'kbd_system_config_t: mode, sleep, debounce, log, deep sleep, OS', 'high', slotId),
        make(base + 0x140, 0x0c0, `${title} SYSTEM PAD`, 'reserved bytes before keymap', 'medium', slotId),
        make(base + 0x200, 0x004, `${title} KEYMAP META`, 'num_layers/current_layer/default_layer/reserved', 'high', slotId),
        make(base + 0x204, 0x020, `${title} LAYER 0`, '8 keys x kbd_action_t(4B)', 'high', slotId),
        make(base + 0x224, 0x020, `${title} LAYER 1`, '8 keys x kbd_action_t(4B)', 'high', slotId),
        make(base + 0x244, 0x020, `${title} LAYER 2`, '8 keys x kbd_action_t(4B)', 'high', slotId),
        make(base + 0x264, 0x020, `${title} LAYER 3`, '8 keys x kbd_action_t(4B)', 'high', slotId),
        make(base + 0x284, 0x020, `${title} LAYER 4`, '8 keys x kbd_action_t(4B)', 'high', slotId),
        make(base + 0x2a4, 0x05c, `${title} KEYMAP PAD`, 'reserved bytes after kbd_keymap_t', 'medium', slotId),
        make(base + 0x300, 0x008, `${title} FN1/fn[0]`, 'kbd_fnkey_entry_t: click, long press and threshold', 'high', slotId),
        make(base + 0x308, 0x008, `${title} FN2/fn[1]`, 'kbd_fnkey_entry_t: click, long press and threshold', 'high', slotId),
        make(base + 0x310, 0x008, `${title} FN3/fn[2]`, 'kbd_fnkey_entry_t: click, long press and threshold', 'high', slotId),
        make(base + 0x318, 0x008, `${title} FN4/fn[3]`, 'kbd_fnkey_entry_t: click, long press and threshold', 'high', slotId),
        make(base + 0x320, 0x020, `${title} FN PAD`, 'reserved bytes before RGB config', 'medium', slotId),
        make(base + 0x340, 0x00a, `${title} RGB CORE`, 'enabled/mode/brightness/speed/color/indicator/press_effect', 'medium', slotId),
        make(base + 0x34a, 0x016, `${title} RGB PAD`, 'kbd_rgb_config_t reserved[22]', 'medium', slotId),
        make(base + 0x360, 0x0a0, `${title} SLOT PAD`, 'remaining reserved bytes in the 1KB slot', 'medium', slotId),
      ];
    }).filter((slice) => slice.offset < region.size);
  }

  if (region.id === 'runtime') {
    const pages = Math.max(1, Math.ceil(region.size / RUNTIME_PAGE_SIZE));
    return Array.from({ length: pages }, (_, page) => page * RUNTIME_PAGE_SIZE).flatMap((base) => {
      const title = `RT PAGE ${String(base / RUNTIME_PAGE_SIZE).padStart(2, '0')}`;
      return [
        make(base + 0x00, 0x04, `${title} MAGIC`, 'KBD_RUNTIME_MAGIC "RUNT"', 'high'),
        make(base + 0x04, 0x02, `${title} VERSION`, 'KBD_RUNTIME_VERSION', 'high'),
        make(base + 0x06, 0x02, `${title} FLAGS`, 'runtime flags', 'medium'),
        make(base + 0x08, 0x04, `${title} SEQ`, 'monotonic runtime page sequence', 'medium'),
        make(base + 0x0c, 0x01, `${title} LAYER`, 'current_layer', 'medium'),
        make(base + 0x0d, 0x01, `${title} MODE`, 'last_mode: USB/BLE/unknown', 'medium'),
        make(base + 0x0e, 0x0ee, `${title} RESERVED`, 'reserved[238]', 'medium'),
        make(base + 0xfc, 0x04, `${title} CRC32`, 'runtime page CRC32', 'high'),
      ];
    }).filter((slice) => slice.offset < region.size);
  }

  if (region.id === 'macro') {
    const pages = Math.max(1, Math.ceil(region.size / MEOWFS_PAGE_SIZE));
    return Array.from({ length: pages }, (_, page) => {
      const offset = page * MEOWFS_PAGE_SIZE;
      const label = `MEOWFS PAGE ${String(page).padStart(2, '0')}`;
      return make(
        offset,
        MEOWFS_PAGE_SIZE,
        label,
        `256B physical stream page; records are [marker,count,actions...] and may begin or span here`,
        page === 0 ? 'high' : 'medium',
      );
    }).filter((slice) => slice.offset < region.size);
  }

  if (region.id === 'ble') {
    return [
      make(0x000, region.size, 'BLE SNV BLOCK', 'WCH BLE stack-owned non-volatile block; do not infer inner layout', 'critical'),
    ];
  }
  if (region.id === 'tail') {
    return [
      make(0x000, region.size, 'UNASSIGNED TAIL', 'final unassigned bytes after BLE SNV; not firmware-locked', 'medium'),
    ];
  }
  if (region.id === 'reserved') {
    const block = pageSize.value || 0x100;
    return Array.from({ length: Math.ceil(region.size / block) }, (_, index) =>
      make(index * block, block, `RESERVED PAGE ${String(index).padStart(2, '0')}`, 'unassigned flash page; keep read-only', 'medium'),
    );
  }

  return [make(0, region.size, region.name.toUpperCase(), regionMeta(region.id).detail, regionMeta(region.id).risk)];
}

const clusterNodes = computed<ClusterNode[]>(() => {
  const cols = 4;
  const rows = 4;
  const gap = 10;
  const width = (1000 - gap * (cols - 1)) / cols;
  const height = (620 - gap * (rows - 1)) / rows;
  return Array.from({ length: 16 }, (_, index) => {
    const startOffset = index * 16;
    const endOffset = Math.min(startOffset + 15, pageBytes.value.length - 1);
    const summary = describeByteGroup(selectedPageStart.value + startOffset, startOffset, endOffset);
    return {
      index,
      startOffset,
      endOffset,
      label: summary.label,
      detail: summary.detail,
      risk: summary.risk,
      x: (index % cols) * (width + gap),
      y: Math.floor(index / cols) * (height + gap),
      width,
      height,
    };
  });
});

const byteNodes = computed<ByteNode[]>(() => {
  const dense = level.value === 'byte';
  const cluster = clusterNodes.value[selectedClusterIndex.value] ?? clusterNodes.value[0];
  const cols = dense ? 16 : 4;
  const count = dense ? pageBytes.value.length : 16;
  const gap = dense ? 4 : 12;
  const width = (1000 - gap * (cols - 1)) / cols;
  const rows = Math.max(1, Math.ceil(count / cols));
  const height = (620 - gap * (rows - 1)) / rows;
  return Array.from({ length: count }, (_, index) => {
    const offset = dense
      ? index
      : Math.min(cluster.startOffset + index, pageBytes.value.length - 1);
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

function describeByteGroup(absStart: number, startOffset: number, endOffset: number): { label: string; detail: string; risk: RiskLevel } {
  const first = describeByte(absStart, pageBytes.value[startOffset] ?? 0xff);
  const last = describeByte(selectedPageStart.value + endOffset, pageBytes.value[endOffset] ?? 0xff);
  const bytes = pageBytes.value.slice(startOffset, endOffset + 1);
  const emptyCount = bytes.filter((value) => value === 0xff).length;
  const zeroCount = bytes.filter((value) => value === 0x00).length;
  const density = `${bytes.length - emptyCount}/${bytes.length} live`;
  const label = first.field === last.field ? first.field : `${first.field} -> ${last.field}`;
  const risk: RiskLevel =
    /magic|crc|keymap|FN|rgb|system|mode|flags|save/i.test(label) ? 'high' :
    emptyCount === bytes.length || zeroCount === bytes.length ? 'medium' :
    selectedRegionMeta.value.risk;

  return {
    label,
    detail: `${first.area} · ${density}`,
    risk,
  };
}

const hudText = computed(() => {
  if (level.value === 'die') return `${hex(0)} - ${hex(dfInfo.value.total - 1)} · DataFlash · ${dfInfo.value.total}B · ${statusText.value}`;
  if (level.value === 'region') return `${hex(selectedRegion.value.start)} - ${hex(selectedRegion.value.start + selectedRegion.value.size - 1)} · ${selectedRegion.value.name} · ${selectedRegion.value.size}B`;
  if (level.value === 'page') return `${hex(selectedPageStart.value)} - ${hex(selectedPageStart.value + pageSize.value - 1)} · ${selectedRegion.value.name} / page ${selectedPageOffset.value / pageSize.value}`;
  if (hasRange.value) {
    return `${hex(selectedRangeAbsStart.value)} - ${hex(selectedRangeAbsEnd.value)} · ${selectedRegion.value.name} / page ${selectedPageOffset.value / pageSize.value} · ${rangeEnd.value - rangeStart.value + 1}B`;
  }
  return `${hex(selectedByteAbs.value)} · ${selectedRegion.value.name} / page ${selectedPageOffset.value / pageSize.value} / byte ${hex(selectedByteOffset.value, 2)} · ${byteHex(selectedByteValue.value)} · ${selectedByteMeaning.value.field}`;
});

function regionMeta(id?: string): RegionMeta {
  return regionMetaMap[id ?? ''] ?? {
    signal: 'UNKNOWN SECTOR',
    detail: 'Unmapped sector. Observe in read-only mode.',
    risk: 'critical',
  };
}

interface MapPageNode extends PageNode {
  regionId: string;
  regionName: string;
  pageNumber: number;
}

function placeHover(event: PointerEvent): Pick<HoverInfo, 'x' | 'y'> {
  const width = 320;
  const margin = 18;
  return {
    x: Math.min(event.clientX + margin, window.innerWidth - width - margin),
    y: Math.min(event.clientY + margin, window.innerHeight - 156),
  };
}

function showRegionHover(node: DieRegionNode, event: PointerEvent): void {
  const meta = regionMeta(node.region.id);
  hoveredRegionId.value = node.region.id;
  hoveredPageStart.value = null;
  hoverInfo.value = {
    title: `${node.region.name} / ${meta.signal}`,
    detail: meta.detail,
    range: `${hex(node.region.start)} - ${hex(node.region.start + node.region.size - 1)} · ${node.region.size}B`,
    risk: meta.risk,
    ...placeHover(event),
  };
}

function showPageHover(page: PageNode, event: PointerEvent): void {
  hoverInfo.value = {
    title: page.label,
    detail: page.detail,
    range: `${hex(page.start, 4)} - ${hex(page.start + page.size - 1, 4)} · ${page.size}B`,
    risk: page.risk,
    ...placeHover(event),
  };
}

function showMapPageHover(page: MapPageNode, event: PointerEvent): void {
  hoveredRegionId.value = page.regionId;
  hoveredPageStart.value = page.start;
  hoverInfo.value = {
    title: page.label,
    detail: page.detail,
    range: `${hex(page.start, 4)} - ${hex(page.start + page.size - 1, 4)} · page ${page.pageNumber}`,
    risk: page.risk,
    ...placeHover(event),
  };
}

function moveHover(event: PointerEvent): void {
  if (!hoverInfo.value) return;
  hoverInfo.value = { ...hoverInfo.value, ...placeHover(event) };
}

function hideHover(): void {
  hoverInfo.value = null;
  hoveredRegionId.value = null;
  hoveredPageStart.value = null;
}

function clamp(value: number, min: number, max: number): number {
  return Math.min(max, Math.max(min, value));
}

function pointerToViewport(event: PointerEvent | WheelEvent | MouseEvent): { x: number; y: number } {
  return clientToViewport(event.clientX, event.clientY);
}

function clientToViewport(clientX: number, clientY: number): { x: number; y: number } {
  const svg = dieEl.value;
  const matrix = svg?.getScreenCTM();
  if (!svg || !matrix) {
    return { x: WORLD_SIZE / 2, y: WORLD_SIZE / 2 };
  }
  const point = new DOMPoint(clientX, clientY).matrixTransform(matrix.inverse());
  return {
    x: point.x,
    y: point.y,
  };
}

function pointerToWorld(event: PointerEvent | WheelEvent | MouseEvent): { x: number; y: number } {
  const point = pointerToViewport(event);
  return {
    x: (point.x - cameraX.value) / cameraScale.value,
    y: (point.y - cameraY.value) / cameraScale.value,
  };
}

function clientToWorld(clientX: number, clientY: number): { x: number; y: number } {
  const point = clientToViewport(clientX, clientY);
  return {
    x: (point.x - cameraX.value) / cameraScale.value,
    y: (point.y - cameraY.value) / cameraScale.value,
  };
}

function updatePointerProbe(event: PointerEvent | WheelEvent | MouseEvent): void {
  const point = pointerToWorld(event);
  pointerWorldX.value = clamp(point.x, 0, WORLD_SIZE);
  pointerWorldY.value = clamp(point.y, 0, WORLD_SIZE);
}

function probeAddress(x: number, y: number): { address: number; label: string; exact: boolean } {
  const gridAddress = showPhysicalLod.value ? worldToAddress(x, y) : null;
  if (gridAddress !== null) {
    const value = gridAddress >= selectedPageStart.value && gridAddress < selectedPageStart.value + pageBytes.value.length
      ? pageBytes.value[gridAddress - selectedPageStart.value] ?? 0xff
      : 0xff;
    const meaning = describeByte(gridAddress, value);
    return { address: gridAddress, label: meaning.field, exact: true };
  }

  const page = mapPageNodes.value.find((item) => x >= item.x && x <= item.x + item.width && y >= item.y && y <= item.y + item.height);
  if (page) {
    const nx = clamp((x - page.x) / Math.max(1, page.width), 0, 1);
    const ny = clamp((y - page.y) / Math.max(1, page.height), 0, 1);
    return {
      address: clamp(Math.floor(page.start + (ny * 0.8 + nx * 0.2) * page.size), page.start, page.start + page.size - 1),
      label: page.label,
      exact: false,
    };
  }

  const node = dieRegionNodes.value.find((item) => x >= item.x && x <= item.x + item.width && y >= item.y && y <= item.y + item.height);
  if (node) {
    const nx = clamp((x - node.x) / Math.max(1, node.width), 0, 1);
    const ny = clamp((y - node.y) / Math.max(1, node.height), 0, 1);
    return {
      address: clamp(Math.floor(node.region.start + (ny * 0.7 + nx * 0.3) * node.region.size), node.region.start, node.region.start + node.region.size - 1),
      label: node.region.name,
      exact: false,
    };
  }
  return { address: clamp(Math.floor((x / WORLD_SIZE) * dfInfo.value.total), 0, dfInfo.value.total - 1), label: 'DIE', exact: false };
}

function worldToAddress(x: number, y: number): number | null {
  const page = mapPageNodes.value.find((item) => x >= item.x && x <= item.x + item.width && y >= item.y && y <= item.y + item.height);
  if (!page) return null;
  const count = Math.min(256, page.size);
  const columns = byteGridColumns(page, count);
  const rows = Math.max(1, Math.ceil(count / columns));
  const col = clamp(Math.floor(((x - page.x) / Math.max(1, page.width)) * columns), 0, columns - 1);
  const row = clamp(Math.floor(((y - page.y) / Math.max(1, page.height)) * rows), 0, rows - 1);
  const index = clamp(row * columns + col, 0, Math.max(0, count - 1));
  return clamp(page.start + index, 0, dfInfo.value.total - 1);
}

function flyCameraToRect(rect: { x: number; y: number; width: number; height: number }, targetScale?: number): void {
  cancelInertia();
  const fitScale = Math.min(8, Math.max(0.72, Math.min(900 / Math.max(1, rect.width), 540 / Math.max(1, rect.height)) * 0.78));
  const scale = targetScale ?? fitScale;
  const centerX = rect.x + rect.width / 2;
  const centerY = rect.y + rect.height / 2;
  applyCenteredCamera(centerX, centerY, scale, 0.52);
}

function flyCameraToByteScope(node: ByteNode, scale: number, duration = 0.52): void {
  cancelInertia();
  applyCenteredCamera(node.x + node.width / 2, node.y + node.height / 2, scale, duration);
}

function atlasWorldBounds(): LayoutRect {
  const nodes = dieRegionNodes.value;
  if (!nodes.length) return { x: 0, y: 0, width: WORLD_SIZE, height: WORLD_SIZE };
  const minX = Math.min(...nodes.map((node) => node.x));
  const minY = Math.min(...nodes.map((node) => node.y));
  const maxX = Math.max(...nodes.map((node) => node.x + node.width));
  const maxY = Math.max(...nodes.map((node) => node.y + node.height));
  const margin = 34;
  return {
    x: Math.max(0, minX - margin),
    y: Math.max(0, minY - margin),
    width: Math.min(WORLD_SIZE, maxX + margin) - Math.max(0, minX - margin),
    height: Math.min(WORLD_SIZE, maxY + margin) - Math.max(0, minY - margin),
  };
}

function sanitizeScale(scale: number | undefined, fallback = cameraScale.value): number {
  return Number.isFinite(scale) && (scale ?? 0) > 0 ? Math.max(0.001, scale as number) : fallback;
}

function applyCenteredCamera(x: number, y: number, scale = cameraScale.value, duration = 0): CameraState {
  return applyRawCamera(WORLD_SIZE / 2 - x * sanitizeScale(scale), WORLD_SIZE / 2 - y * sanitizeScale(scale), scale, duration);
}

function applyRawCamera(tx: number, ty: number, scale = cameraScale.value, duration = 0): CameraState {
  cancelInertia();
  cameraTween?.kill();
  const next = {
    x: Number.isFinite(tx) ? tx : cameraX.value,
    y: Number.isFinite(ty) ? ty : cameraY.value,
    scale: sanitizeScale(scale),
  };
  if (duration <= 0) {
    cameraX.value = next.x;
    cameraY.value = next.y;
    cameraScale.value = next.scale;
    return getCameraState();
  }
  const current = { x: cameraX.value, y: cameraY.value, scale: cameraScale.value };
  cameraTween = gsap.to(current, {
    ...next,
    duration,
    ease: 'power3.out',
    onUpdate: () => {
      cameraX.value = current.x;
      cameraY.value = current.y;
      cameraScale.value = current.scale;
    },
    onComplete: () => {
      cameraTween = null;
    },
  });
  return getCameraState();
}

function applyZoomAtViewport(viewportX: number, viewportY: number, scale: number, duration = 0): CameraState {
  const nextScale = sanitizeScale(scale);
  const world = {
    x: (viewportX - cameraX.value) / cameraScale.value,
    y: (viewportY - cameraY.value) / cameraScale.value,
  };
  pointerWorldX.value = clamp(world.x, 0, WORLD_SIZE);
  pointerWorldY.value = clamp(world.y, 0, WORLD_SIZE);
  return applyRawCamera(viewportX - world.x * nextScale, viewportY - world.y * nextScale, nextScale, duration);
}

function applyZoomAtClient(clientX: number, clientY: number, scale: number, duration = 0): CameraState {
  const viewport = clientToViewport(clientX, clientY);
  return applyZoomAtViewport(viewport.x, viewport.y, scale, duration);
}

function resetCamera(): void {
  flyCameraToRect(atlasWorldBounds(), 1.34);
}

function getCameraState(): CameraState {
  const scale = sanitizeScale(cameraScale.value, 1);
  const centerX = (WORLD_SIZE / 2 - cameraX.value) / scale;
  const centerY = (WORLD_SIZE / 2 - cameraY.value) / scale;
  return {
    x: centerX,
    y: centerY,
    tx: cameraX.value,
    ty: cameraY.value,
    scale,
    lod: cameraLod.value,
    probe: probeAddress(centerX, centerY),
  };
}

async function focusAddress(address: number, scale = 96, duration = 0.28): Promise<CameraState> {
  const target = clamp(Math.floor(address), 0, dfInfo.value.total - 1);
  const page = mapPageNodes.value.find((item) => target >= item.start && target < item.start + item.size);
  if (page) {
    selectedRegionId.value = page.regionId;
    selectedPageOffset.value = Math.floor(page.offset / pageSize.value) * pageSize.value;
    selectedByteOffset.value = clamp(target - selectedPageStart.value, 0, Math.max(0, pageSize.value - 1));
    level.value = 'byte';
    if (dfInfo.value.rawAvailable) await readCurrentPage().catch(() => {});
    await nextTick();
    const point = addressPointInMapNode(page, target);
    pointerWorldX.value = clamp(point.x, 0, WORLD_SIZE);
    pointerWorldY.value = clamp(point.y, 0, WORLD_SIZE);
    return applyCenteredCamera(point.x, point.y, scale, duration);
  }

  const region = dieRegionNodes.value.find((item) => target >= item.region.start && target < item.region.start + item.region.size);
  if (region) {
    const pct = (target - region.region.start) / Math.max(1, region.region.size);
    const x = region.x + region.width * 0.5;
    const y = region.y + region.height * pct;
    pointerWorldX.value = clamp(x, 0, WORLD_SIZE);
    pointerWorldY.value = clamp(y, 0, WORLD_SIZE);
    return applyCenteredCamera(x, y, scale, duration);
  }
  return applyCenteredCamera(WORLD_SIZE / 2, WORLD_SIZE / 2, scale, duration);
}

function getDebugCameraApi(): StormDataFlashCameraApi {
  return {
    goto: (x, y, scale = cameraScale.value, duration = 0) => applyCenteredCamera(x, y, scale, duration),
    raw: (tx, ty, scale = cameraScale.value, duration = 0) => applyRawCamera(tx, ty, scale, duration),
    zoom: (scale, x = getCameraState().x, y = getCameraState().y, duration = 0) => applyCenteredCamera(x, y, scale, duration),
    zoomAtClient: applyZoomAtClient,
    screenToWorld: clientToWorld,
    focusAddress,
    probe: (x = getCameraState().x, y = getCameraState().y) => probeAddress(x, y),
    areaAudit: auditDieArea,
  pixiStats: () => ({
    byteRenders: pixiByteRenderCount,
    weatherRenders: pixiWeatherRenderCount,
    pixiPresent: !!pixiApp && !!pixiByteLayer,
    activeByte: mapByteNodes.value.find((node) => node.offset === selectedByteOffset.value) ?? null,
  }),
    reset: () => {
      resetCamera();
      return getCameraState();
    },
    get: getCameraState,
  };
}

function auditDieArea(): AreaAuditRow[] {
  const totalArea = dieRegionNodes.value.reduce((sum, node) => sum + Math.ceil(node.region.size / pageSize.value) * ATLAS_PAGE_TILE * ATLAS_PAGE_TILE, 0);
  const totalBytes = Math.max(1, dfInfo.value.total);
  return dieRegionNodes.value.map((node) => {
    const byteRatio = node.region.size / totalBytes;
    const areaRatio = (Math.ceil(node.region.size / pageSize.value) * ATLAS_PAGE_TILE * ATLAS_PAGE_TILE) / Math.max(1, totalArea);
    return {
      id: node.region.id,
      bytes: node.region.size,
      byteRatio,
      areaRatio,
      error: areaRatio - byteRatio,
    };
  });
}

function handleCameraCommand(event: Event): void {
  const detail = (event as CustomEvent<CameraCommand>).detail ?? {};
  if (typeof detail.address === 'number') {
    void focusAddress(detail.address, detail.scale, detail.duration);
    return;
  }
  if (detail.raw) {
    applyRawCamera(detail.tx ?? cameraX.value, detail.ty ?? cameraY.value, detail.scale, detail.duration);
    return;
  }
  if (typeof detail.clientX === 'number' && typeof detail.clientY === 'number') {
    applyZoomAtClient(detail.clientX, detail.clientY, detail.scale ?? cameraScale.value, detail.duration);
    return;
  }
  applyCenteredCamera(detail.x ?? getCameraState().x, detail.y ?? getCameraState().y, detail.scale, detail.duration);
}

function installCameraDebugApi(): void {
  window.stormDataFlashCamera = getDebugCameraApi();
  if (cameraApiInstalled) return;
  window.addEventListener('storm-dataflash-camera', handleCameraCommand);
  cameraApiInstalled = true;
}

function uninstallCameraDebugApi(): void {
  if (cameraApiInstalled) window.removeEventListener('storm-dataflash-camera', handleCameraCommand);
  cameraApiInstalled = false;
  if (window.stormDataFlashCamera?.get === getCameraState) delete window.stormDataFlashCamera;
}

function installWheelCapture(): void {
  uninstallWheelCapture();
  wheelTarget = dieEl.value;
  wheelTarget?.addEventListener('wheel', handleWheel, { passive: false, capture: true });
}

function uninstallWheelCapture(): void {
  wheelTarget?.removeEventListener('wheel', handleWheel, { capture: true } as AddEventListenerOptions);
  wheelTarget = null;
}

function beginCameraDrag(event: PointerEvent): void {
  if (event.button !== 2) return;
  event.preventDefault();
  cancelInertia();
  const target = event.target as Element | null;
  if (target?.closest?.('.die-meaning, .die-tooltip')) return;
  updatePointerProbe(event);
  hideHover();
  isCameraDragging.value = true;
  cameraMoved.value = false;
  cameraDragStart.value = {
    x: event.clientX,
    y: event.clientY,
    cameraX: cameraX.value,
    cameraY: cameraY.value,
  };
  lastDragViewport.value = { ...clientToViewport(event.clientX, event.clientY), t: performance.now() };
  cameraVelocity.value = { x: 0, y: 0 };
  (event.currentTarget as HTMLElement).setPointerCapture?.(event.pointerId);
}

function handleCanvasPointerMove(event: PointerEvent): void {
  updatePointerProbe(event);
  if (!isCameraDragging.value) return;
  event.preventDefault();
  if ((event.buttons & 2) === 0) {
    endCameraDrag(event);
    return;
  }
  const start = clientToViewport(cameraDragStart.value.x, cameraDragStart.value.y);
  const current = pointerToViewport(event);
  const dx = current.x - start.x;
  const dy = current.y - start.y;
  if (Math.abs(dx) + Math.abs(dy) > 3) cameraMoved.value = true;
  cameraX.value = cameraDragStart.value.cameraX + dx;
  cameraY.value = cameraDragStart.value.cameraY + dy;
  const now = performance.now();
  const dt = Math.max(12, now - lastDragViewport.value.t);
  const gain = 0.95 + Math.min(3.4, cameraScale.value / 12);
  cameraVelocity.value = {
    x: ((current.x - lastDragViewport.value.x) / dt) * 16.67 * gain,
    y: ((current.y - lastDragViewport.value.y) / dt) * 16.67 * gain,
  };
  lastDragViewport.value = { ...current, t: now };
}

function endCameraDrag(event?: PointerEvent): void {
  if (event && isCameraDragging.value) (event.currentTarget as HTMLElement).releasePointerCapture?.(event.pointerId);
  if (isCameraDragging.value) startInertia();
  isCameraDragging.value = false;
}

function handleWheel(event: WheelEvent): void {
  event.preventDefault();
  event.stopPropagation();
  cancelInertia();
  const viewport = pointerToViewport(event);
  const delta = event.deltaMode === WheelEvent.DOM_DELTA_LINE
    ? event.deltaY * 16
    : event.deltaMode === WheelEvent.DOM_DELTA_PAGE
      ? event.deltaY * 320
      : event.deltaY;
  const intensity = Math.exp(-delta * 0.0019);
  const nextScale = clamp(cameraScale.value * intensity, 0.015, 640);
  applyZoomAtViewport(viewport.x, viewport.y, nextScale, 0);
}

function cancelInertia(): void {
  if (!inertiaFrame) return;
  cancelAnimationFrame(inertiaFrame);
  inertiaFrame = 0;
}

function startInertia(): void {
  cancelInertia();
  let vx = cameraVelocity.value.x;
  let vy = cameraVelocity.value.y;
  if (Math.hypot(vx, vy) < 0.18) return;
  const friction = 0.91 + Math.min(0.055, cameraScale.value / 2200);
  const step = () => {
    cameraX.value += vx;
    cameraY.value += vy;
    vx *= friction;
    vy *= friction;
    if (Math.hypot(vx, vy) < 0.08) {
      inertiaFrame = 0;
      return;
    }
    inertiaFrame = requestAnimationFrame(step);
  };
  inertiaFrame = requestAnimationFrame(step);
}

async function initPixiLayer(): Promise<void> {
  if (isVisualTestMode) return;
  if (pixiApp || !pixiHostEl.value) return;
  const app = new Application();
  await app.init({
    backgroundAlpha: 0,
    antialias: false,
    autoDensity: true,
    powerPreference: 'high-performance',
    preserveDrawingBuffer: isPerfTestMode,
    resizeTo: pixiHostEl.value,
  });
  pixiApp = app;
  pixiWorld = new Container();
  pixiByteLayer = new Graphics();
  pixiWeatherLayer = new Graphics();
  pixiWorld.addChild(pixiByteLayer);
  pixiWorld.addChild(pixiWeatherLayer);
  app.stage.addChild(pixiWorld);
  app.canvas.className = 'pixi-byte-canvas';
  pixiHostEl.value.appendChild(app.canvas);
  pixiResizeObserver = new ResizeObserver(() => {
    updatePixiTransform();
    renderPixiBytes();
  });
  pixiResizeObserver.observe(pixiHostEl.value);
  if (!isPerfTestMode) {
    app.ticker.add((ticker) => {
      weatherPhase = (weatherPhase + ticker.deltaMS / 2400) % 1;
      weatherRenderMs += ticker.deltaMS;
      if (weatherRenderMs < 72) return;
      weatherRenderMs = 0;
      renderPixiWeather();
    });
  }
  updatePixiTransform();
  renderPixiBytes();
  renderPixiWeather();
}

function destroyPixiLayer(): void {
  pixiResizeObserver?.disconnect();
  pixiResizeObserver = null;
  pixiApp?.destroy(true);
  pixiApp = null;
  pixiWorld = null;
  pixiByteLayer = null;
  pixiWeatherLayer = null;
}

function updatePixiTransform(): void {
  const host = pixiHostEl.value;
  if (!host || !pixiWorld) return;
  const rect = host.getBoundingClientRect();
  const fit = Math.min(rect.width / WORLD_SIZE, rect.height / WORLD_SIZE);
  const x = (rect.width - WORLD_SIZE * fit) / 2 + cameraX.value * fit;
  const y = (rect.height - WORLD_SIZE * fit) / 2 + cameraY.value * fit;
  pixiWorld.position.set(x, y);
  pixiWorld.scale.set(cameraScale.value * fit);
}

function renderPixiBytes(): void {
  if (!pixiByteLayer) return;
  pixiByteRenderCount += 1;
  pixiByteLayer.clear();
  if (!showByteLod.value) return;
  const zoom = cameraScale.value;
  for (const node of mapByteNodes.value) {
    const active = selectedByteOffset.value === node.offset;
    const empty = node.value === 0xff;
    const zero = node.value === 0x00;
    const subByte = zoom >= 80;
    const fill = active ? 0xfb7185 : zero ? 0x0891b2 : empty ? 0x1e293b : 0x0f766e;
    const alpha = active ? (subByte ? 0.58 : 0.86) : subByte ? 0.16 : empty ? 0.42 : 0.62;
    pixiByteLayer
      .rect(node.x, node.y, Math.max(0.35, node.width), Math.max(0.35, node.height))
      .fill({ color: fill, alpha })
      .stroke({
        color: active ? 0xfda4af : 0x67e8f9,
        alpha: active ? 0.95 : subByte ? 0.12 : zoom >= 12 ? 0.46 : 0.22,
        width: active ? Math.max(0.08, 0.9 / zoom) : subByte ? Math.max(0.035, 0.28 / zoom) : Math.max(0.12, 0.42 / zoom),
      });
  }
  if (zoom >= 80) {
    const active = mapByteNodes.value.find((node) => node.offset === selectedByteOffset.value);
    const page = activeMapPage.value;
    if (active && page) {
      const cx = active.x + active.width / 2;
      const cy = active.y + active.height / 2;
      pixiByteLayer
        .rect(page.x, page.y, page.width, page.height)
        .stroke({ color: 0x67e8f9, alpha: 0.12, width: Math.max(0.02, 0.28 / zoom) });
      pixiByteLayer
        .moveTo(page.x, cy)
        .lineTo(page.x + page.width, cy)
        .stroke({ color: 0xfb7185, alpha: 0.3, width: Math.max(0.03, 0.42 / zoom) });
      pixiByteLayer
        .moveTo(cx, page.y)
        .lineTo(cx, page.y + page.height)
        .stroke({ color: 0x67e8f9, alpha: 0.24, width: Math.max(0.03, 0.42 / zoom) });
      pixiByteLayer
        .rect(active.x + active.width * 0.045, active.y + active.height * 0.055, active.width * 0.91, active.height * 0.89)
        .stroke({ color: 0xfecdd3, alpha: 0.74, width: Math.max(0.035, 0.5 / zoom) });
      for (let bit = 0; bit < 8; bit++) {
        const on = ((active.value >> (7 - bit)) & 1) === 1;
        const y = active.y + active.height * (0.16 + bit * 0.096);
        pixiByteLayer
          .moveTo(active.x + active.width * 0.08, y)
          .lineTo(active.x + active.width * 0.92, y)
          .stroke({ color: on ? 0xfecdd3 : 0x67e8f9, alpha: on ? 0.86 : 0.42, width: Math.max(on ? 0.04 : 0.028, (on ? 0.55 : 0.34) / zoom) });
      }
      for (let index = 0; index < 16; index++) {
        const bit = index % 8;
        const on = ((active.value >> (7 - bit)) & 1) === 1;
        pixiByteLayer
          .circle(
            active.x + active.width * (0.11 + bit * 0.11),
            index < 8 ? active.y + active.height * 0.12 : active.y + active.height * 0.88,
            Math.min(active.width, active.height) * 0.018,
          )
          .fill({ color: on ? 0xfb7185 : 0x7dd3fc, alpha: on ? 0.86 : 0.52 })
          .stroke({ color: on ? 0xfff1f2 : 0xe0f2fe, alpha: 0.45, width: Math.max(0.01, 0.16 / zoom) });
      }
    }
  }
  if (zoom >= 120) {
    const active = mapByteNodes.value.find((node) => node.offset === selectedByteOffset.value);
    if (active) {
      const bitCols = 4;
      const bitRows = 2;
      const bitW = active.width / bitCols;
      const bitH = active.height / bitRows;
      for (let bit = 0; bit < 8; bit++) {
        const on = ((active.value >> (7 - bit)) & 1) === 1;
        pixiByteLayer
          .rect(active.x + (bit % bitCols) * bitW, active.y + Math.floor(bit / bitCols) * bitH, bitW, bitH)
          .fill({ color: on ? 0xfda4af : 0x0f172a, alpha: on ? 0.86 : 0.72 })
          .stroke({ color: on ? 0xfb7185 : 0x67e8f9, alpha: 0.9, width: Math.max(0.05, 0.5 / zoom) });
      }
    }
  }
}

function renderPixiWeather(): void {
  if (!pixiWeatherLayer) return;
  pixiWeatherRenderCount += 1;
  pixiWeatherLayer.clear();
  if (isVisualTestMode) return;
  const page = activeMapPage.value;
  if (!page || cameraScale.value < 2.1) return;
  const t = weatherPhase % 1;
  const scanX = page.x + page.width * t;
  const pulse = 0.18 + Math.sin(weatherPhase * Math.PI * 2) * 0.08;
  pixiWeatherLayer
    .rect(scanX, page.y, Math.max(0.35, 2.4 / cameraScale.value), page.height)
    .fill({ color: dangerTone.value === 'critical' || dangerTone.value === 'high' ? 0xfb7185 : 0x67e8f9, alpha: pulse });
  pixiWeatherLayer
    .rect(page.x, page.y + page.height * ((t * 1.7) % 1), page.width, Math.max(0.3, 1.4 / cameraScale.value))
    .fill({ color: 0xbae6fd, alpha: 0.08 + pulse * 0.32 });
}

function handleCanvasBlankClick(event: MouseEvent): void {
  const target = event.target as Element | null;
  if (consumeClickAfterDrag()) return;
  if (target?.closest?.('.die-cell, .slot-frame')) return;
  hideHover();
  if (level.value !== 'die') stepBack();
  else resetCamera();
}

function consumeClickAfterDrag(): boolean {
  if (!cameraMoved.value) return false;
  cameraMoved.value = false;
  return true;
}

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
  if (!transport) throw new Error('HID transport is not ready');
  return transport.sendAndWait(buildFrame(cmd, sub, data), { timeout });
}

function expectOk(resp: DataView, name: string): number {
  const status = resp.getUint8(3);
  if (status !== ResponseCode.OK) throw new Error(`${name} failed: 0x${status.toString(16)}`);
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
  if (isVisualTestMode) return;
  void nextTick(() => {
    if (dieEl.value) {
      gsap.fromTo(dieEl.value, { filter: 'brightness(1.25)' }, { filter: 'brightness(1)', duration: 0.32, ease: 'power3.out' });
    }
    if (hudEl.value) {
      gsap.fromTo(hudEl.value, { y: 8, opacity: 0.55 }, { y: 0, opacity: 1, duration: 0.28, ease: 'power2.out' });
    }
  });
}

function animateMeaning(): void {
  if (isVisualTestMode) return;
  void nextTick(() => {
    if (!meaningEl.value) return;
    gsap.fromTo(
      meaningEl.value,
      { y: 18, opacity: 0.2, scale: 0.96, filter: 'brightness(1.55)' },
      { y: 0, opacity: 1, scale: 1, filter: 'brightness(1)', duration: 0.34, ease: 'power3.out' },
    );
  });
}

async function refreshSnapshot(): Promise<void> {
  if (deviceStore.deviceInfo?.protocol !== DeviceProtocol.CH592) return;
  hideHover();
  isBusy.value = true;
  statusText.value = 'sync';
  try {
    dfInfo.value = await readDataFlashInfo();
    await readCurrentPage();
    statusText.value = 'live';
  } catch (error) {
    dfInfo.value = { ...dfInfo.value, rawAvailable: false };
    statusText.value = 'firmware';
    showToast('error', 'DataFlash unavailable', error instanceof Error ? error.message : 'Current firmware does not expose DataFlash debug commands');
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

async function enterRegion(region: FlashRegion, rect?: { x: number; y: number; width: number; height: number }): Promise<void> {
  if (consumeClickAfterDrag()) return;
  hideHover();
  selectedRegionId.value = region.id;
  selectedPageOffset.value = 0;
  selectedClusterIndex.value = 0;
  clearSelection();
  if (dfInfo.value.rawAvailable) await readCurrentPage().catch(() => {});
  level.value = 'region';
  flyCameraToRect(rect ?? { x: 0, y: 0, width: 1000, height: 620 }, rect ? 1.55 : 1.12);
  animateFocus();
}

async function enterPage(page: PageNode): Promise<void> {
  hideHover();
  selectedPageOffset.value = page.offset;
  selectedClusterIndex.value = 0;
  clearSelection();
  if (dfInfo.value.rawAvailable) await readCurrentPage().catch(() => {});
  level.value = 'page';
  flyCameraToRect(page, 2.25);
  animateFocus();
}

async function focusMapPage(page: MapPageNode, event?: MouseEvent): Promise<void> {
  if (consumeClickAfterDrag()) return;
  if (event && showPhysicalLod.value) {
    const world = pointerToWorld(event);
    const byte = mapByteNodes.value.find((node) => world.x >= node.x && world.x <= node.x + node.width && world.y >= node.y && world.y <= node.y + node.height);
    if (byte) {
      focusMapByte(byte);
      return;
    }
  }
  hideHover();
  selectedRegionId.value = page.regionId;
  selectedPageOffset.value = Math.floor(page.offset / pageSize.value) * pageSize.value;
  selectedClusterIndex.value = 0;
  clearSelection();
  if (dfInfo.value.rawAvailable) await readCurrentPage().catch(() => {});
  level.value = 'page';
  flyCameraToRect(page, Math.max(3.2, cameraScale.value));
  animateFocus();
}

function focusMapByte(node: ByteNode): void {
  if (consumeClickAfterDrag()) return;
  hideHover();
  const page = activeMapPage.value;
  if (page) {
    selectedRegionId.value = page.regionId;
    const region = regions.value.find((item) => item.id === page.regionId);
    if (region) selectedPageOffset.value = Math.floor((node.abs - region.start) / pageSize.value) * pageSize.value;
  }
  selectByte(Math.max(0, node.abs - selectedPageStart.value));
  level.value = 'byte';
  flyCameraToByteScope(node, Math.max(128, cameraScale.value));
  animateFocus();
  animateMeaning();
}

function enterCluster(cluster: ClusterNode): void {
  hideHover();
  selectedClusterIndex.value = cluster.index;
  selectByte(cluster.startOffset);
  level.value = 'cluster';
  flyCameraToRect(cluster, 3.9);
  animateFocus();
}

function enterByte(node: ByteNode): void {
  hideHover();
  selectByte(node.offset);
  level.value = 'byte';
  flyCameraToByteScope(node, 128);
  animateFocus();
  animateMeaning();
}

function stepBack(): void {
  hideHover();
  if (level.value === 'byte') level.value = 'cluster';
  else if (level.value === 'cluster') level.value = 'page';
  else if (level.value === 'page') level.value = 'region';
  else if (level.value === 'region') level.value = 'die';
  clearSelection(false);
  if (level.value === 'die') resetCamera();
  else if (level.value === 'region') flyCameraToRect({ x: 0, y: 0, width: WORLD_SIZE, height: WORLD_SIZE }, 1.15);
  else if (level.value === 'page') flyCameraToRect({ x: 0, y: 0, width: WORLD_SIZE, height: WORLD_SIZE }, 1.75);
  else flyCameraToRect({ x: 0, y: 0, width: WORLD_SIZE, height: WORLD_SIZE }, 2.8);
  animateFocus();
}

function handleEscape(): void {
  if (level.value === 'die') {
    dialogVisible.value = false;
    return;
  }
  stepBack();
}

function selectByte(offset: number): void {
  selectedByteOffset.value = Math.max(0, Math.min(offset, pageBytes.value.length - 1));
  rangeAnchor.value = selectedByteOffset.value;
  rangeFocus.value = selectedByteOffset.value;
  byteDraft.value = byteHex(selectedByteValue.value);
  dangerConfirm.value = '';
  writePanelOpen.value = false;
}

function beginRange(node: ByteNode, event: PointerEvent): void {
  selectedByteOffset.value = node.offset;
  rangeAnchor.value = event.shiftKey && rangeAnchor.value !== null ? rangeAnchor.value : node.offset;
  rangeFocus.value = node.offset;
  isDraggingRange.value = true;
  byteDraft.value = byteHex(node.value);
  dangerConfirm.value = '';
  writePanelOpen.value = false;
  animateMeaning();
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
  writePanelOpen.value = false;
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
    showToast('success', 'Byte written', `${hex(selectedByteAbs.value)} = ${byteHex(next)}`);
  } catch (error) {
    showToast('error', 'Write failed', error instanceof Error ? error.message : 'Unknown error');
  } finally {
    isBusy.value = false;
  }
}

function describeByte(abs: number, value: number): { area: string; field: string } {
  const region = regions.value.find((item) => abs >= item.start && abs < item.start + item.size);
  if (!region) return { area: 'UNKNOWN', field: 'out of range' };

  if (region.id === 'config') {
    const slot = Math.floor((abs - region.start) / CFG_SLOT_SIZE);
    const off = (abs - region.start) % CFG_SLOT_SIZE;
    const prefix = `CONFIG SLOT ${slot}`;
    if (off < 0x20) return { area: prefix, field: configHeaderField(off) };
    if (off >= 0x100 && off < 0x140) return { area: prefix, field: systemField(off - 0x100) };
    if (off >= 0x200 && off < 0x2a4) return { area: prefix, field: keymapField(off - 0x200) };
    if (off >= 0x300 && off < 0x320) return { area: prefix, field: fnField(off - 0x300) };
    if (off >= 0x340 && off < 0x360) return { area: prefix, field: rgbField(off - 0x340) };
    return { area: prefix, field: `reserved ${hex(off, 3)}` };
  }

  if (region.id === 'runtime') return { area: `RUNTIME PAGE ${Math.floor((abs - region.start) / RUNTIME_PAGE_SIZE)}`, field: runtimeField((abs - region.start) % RUNTIME_PAGE_SIZE) };
  if (region.id === 'macro') return { area: `MEOWFS PAGE ${Math.floor((abs - region.start) / MEOWFS_PAGE_SIZE)}`, field: macroField(abs - region.start, value) };
  if (region.id === 'ble') return { area: 'BLE SNV', field: `SNV ${hex(abs - region.start, 3)}` };
  return { area: region.name, field: `offset ${hex(abs - region.start)}` };
}

function configHeaderField(off: number): string {
  if (off <= 3) return `magic[${off}]`;
  if (off <= 5) return `version[${off - 4}]`;
  if (off <= 7) return `flags[${off - 6}]`;
  if (off <= 11) return `crc32[${off - 8}]`;
  if (off <= 15) return `save_count[${off - 12}]`;
  return `header_reserved[${off - 16}]`;
}

function systemField(off: number): string {
  const names = ['default_mode', 'auto_sleep_min', 'debounce_ms', 'log_enabled', 'deep_sleep_min', 'os_mode'];
  return names[off] ?? `system_reserved[${off - names.length}]`;
}

function keymapField(off: number): string {
  if (off === 0) return 'num_layers';
  if (off === 1) return 'current_layer';
  if (off === 2) return 'default_layer';
  if (off === 3) return 'reserved';
  const dataOff = off - 4;
  if (dataOff < 0) return `keymap ${hex(off, 2)}`;
  const layerIndex = Math.floor(dataOff / 32);
  const inLayer = dataOff % 32;
  const key = Math.floor(inLayer / 4);
  const part = ['type', 'modifier', 'param1', 'param2'][inLayer % 4];
  return `layer${layerIndex}.key${key}.${part}`;
}

function fnField(off: number): string {
  const fn = Math.floor(off / 8);
  const part = ['click_action', 'click_param', 'long_action', 'long_param', 'long_ms_lo', 'long_ms_hi', 'reserved0', 'reserved1'][off % 8];
  return `FN${fn + 1}/fn[${fn}].${part}`;
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
  return `runtime_reserved[${off - 14}]`;
}

function macroField(offset: number, value: number): string {
  const pageOff = offset % MEOWFS_PAGE_SIZE;
  if (value === 0xff) return `erased / free byte ${hex(offset, 4)}`;
  if (value === 0xaa) return `possible record marker byte at ${hex(offset, 4)}`;
  if (value === 0x00) return `zero/deleted-like stream byte at ${hex(offset, 4)}`;
  if (offset < MEOWFS_ENTRY_HEADER_SIZE) {
    return offset === 0 ? 'possible stream record marker' : 'possible stream action_count';
  }
  if (pageOff === 0) return `page boundary stream byte ${hex(offset, 4)}`;
  if (pageOff === 1) return `possible count byte / stream byte ${hex(offset, 4)}`;
  const pairPhase = (offset - MEOWFS_ENTRY_HEADER_SIZE) % MEOWFS_ACTION_SIZE;
  return pairPhase === 0
    ? `possible action.type byte / unparsed MeowFS stream`
    : `possible action.param byte / unparsed MeowFS stream`;
}

let hasAutoLoaded = false;
onMounted(() => {
  installCameraDebugApi();
  void nextTick(() => {
    if (!isVisualTestMode) void initPixiLayer();
    installWheelCapture();
    resetCamera();
  });
});

onBeforeUnmount(() => {
  uninstallCameraDebugApi();
  uninstallWheelCapture();
  cancelInertia();
  destroyPixiLayer();
});

watch(() => props.visible, (visible) => {
  hideHover();
  if (visible) {
    level.value = 'die';
    installCameraDebugApi();
    void nextTick(() => {
      if (!isVisualTestMode) void initPixiLayer();
      installWheelCapture();
    });
    resetCamera();
    animateFocus();
    if (!hasAutoLoaded) {
      hasAutoLoaded = true;
      void refreshSnapshot();
    }
  }
});

watch(hudText, animateFocus);
watch([cameraX, cameraY, cameraScale], () => {
  updatePixiTransform();
});
watch([mapByteNodes, showByteLod, activeMapPage, selectedByteOffset], () => {
  renderPixiBytes();
  renderPixiWeather();
});
</script>

<template>
  <StudioDialog
    v-model:visible="dialogVisible"
    size="immersive"
    dismissable-mask
    class="storm-dataflash-dialog"
    :showHeader="false"
  >
    <div
      ref="explorerEl"
      class="die-explorer"
      :class="{ 'visual-test': isVisualTestMode }"
      :data-area-audit="areaAuditJson"
      :data-camera-state="JSON.stringify(getCameraState())"
      @pointerdown.capture="beginCameraDrag"
      @pointermove.capture="handleCanvasPointerMove"
      @pointerup.capture="endCameraDrag"
      @pointercancel.capture="endCameraDrag"
      @pointerup="endRange"
      @pointerleave="endRange"
      @keydown.esc="handleEscape"
      @contextmenu.prevent
      tabindex="0"
    >
      <div class="die-stars"></div>
      <div class="die-scan"></div>

      <div ref="hudEl" class="die-hud" :class="{ danger: level === 'byte' || hasRange, armed: canWriteByte }">
        <span class="hud-chip level">{{ levelLabel }}</span>
        <span>{{ hudText }}</span>
        <span class="hud-chip" :class="dangerTone">{{ accessState }}</span>
        <code v-if="level === 'byte' && !hasRange">{{ byteBits(selectedByteValue) }}</code>
      </div>

      <div class="die-coordinate-hud" :class="{ dragging: isCameraDragging }">
        {{ coordinateHudText }}
      </div>

      <div v-if="showBitScope" class="die-page-context" aria-hidden="true">
        <div class="context-head">
          <span>PAGE {{ Math.floor(selectedByteAbs / pageSize).toString().padStart(2, '0') }}</span>
          <code>16x16 · 256B</code>
        </div>
        <div class="context-grid">
          <span
            v-for="cell in selectedPageContextCells"
            :key="cell.index"
            :class="{ active: cell.active, axis: cell.row === Math.floor(selectedByteOffset / 16) || cell.col === selectedByteOffset % 16 }"
          ></span>
        </div>
      </div>

      <svg
        ref="dieEl"
        class="die-canvas"
        :class="{ grabbing: isCameraDragging }"
        viewBox="0 0 1000 1000"
        role="img"
        aria-label="DataFlash die explorer"
        @pointerleave="hideHover"
        @contextmenu.prevent
        @click="handleCanvasBlankClick"
      >
        <defs>
          <pattern id="storm-grid" width="28" height="28" patternUnits="userSpaceOnUse">
            <path d="M 28 0 L 0 0 0 28" fill="none" stroke="rgba(125,211,252,0.13)" stroke-width="1" />
          </pattern>
          <pattern id="config-core" width="24" height="24" patternUnits="userSpaceOnUse">
            <rect width="24" height="24" fill="rgba(30,64,175,0.28)" />
            <path d="M0 8 H24 M8 0 V24 M0 20 H24 M20 0 V24" stroke="rgba(125,211,252,0.22)" stroke-width="0.7" />
            <circle cx="8" cy="8" r="1.2" fill="rgba(186,230,253,0.38)" />
          </pattern>
          <pattern id="runtime-cache" width="18" height="18" patternUnits="userSpaceOnUse">
            <rect width="18" height="18" fill="rgba(13,148,136,0.24)" />
            <path d="M0 9 H18 M9 0 V18" stroke="rgba(94,234,212,0.24)" stroke-width="0.8" />
          </pattern>
          <pattern id="macro-stream" width="36" height="18" patternUnits="userSpaceOnUse">
            <rect width="36" height="18" fill="rgba(88,28,135,0.34)" />
            <path d="M0 9 C8 2 16 16 24 9 S32 2 36 9" fill="none" stroke="rgba(196,181,253,0.26)" stroke-width="1.1" />
            <path d="M0 17 H36" stroke="rgba(251,113,133,0.13)" stroke-width="0.8" />
          </pattern>
          <pattern id="reserved-seal" width="30" height="30" patternUnits="userSpaceOnUse">
            <rect width="30" height="30" fill="rgba(15,23,42,0.74)" />
            <path d="M0 0 L30 30 M30 0 L0 30" stroke="rgba(148,163,184,0.11)" stroke-width="0.8" />
            <path d="M15 0 V30 M0 15 H30" stroke="rgba(125,211,252,0.08)" stroke-width="0.7" stroke-dasharray="2 5" />
          </pattern>
          <pattern id="protected-fuse" width="20" height="20" patternUnits="userSpaceOnUse">
            <rect width="20" height="20" fill="rgba(159,18,57,0.32)" />
            <path d="M0 10 H7 L10 3 L13 17 L16 10 H20" fill="none" stroke="rgba(251,113,133,0.34)" stroke-width="1" />
          </pattern>
          <filter id="storm-glow">
            <feGaussianBlur stdDeviation="3" result="blur" />
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
          <linearGradient id="metal-route" x1="0" x2="1" y1="0" y2="0">
            <stop offset="0%" stop-color="#38bdf8" stop-opacity="0.08" />
            <stop offset="45%" stop-color="#bae6fd" stop-opacity="0.38" />
            <stop offset="100%" stop-color="#fb7185" stop-opacity="0.18" />
          </linearGradient>
          <radialGradient id="charge-cyan">
            <stop offset="0%" stop-color="#e0f2fe" stop-opacity="0.96" />
            <stop offset="34%" stop-color="#22d3ee" stop-opacity="0.56" />
            <stop offset="100%" stop-color="#22d3ee" stop-opacity="0" />
          </radialGradient>
          <radialGradient id="charge-red">
            <stop offset="0%" stop-color="#fff1f2" stop-opacity="0.92" />
            <stop offset="36%" stop-color="#fb7185" stop-opacity="0.68" />
            <stop offset="100%" stop-color="#fb7185" stop-opacity="0" />
          </radialGradient>
        </defs>

        <g class="die-world" :class="{ 'deep-zoom': cameraScale >= 4.8, 'sub-byte-zoom': cameraScale >= 80 }" :transform="cameraTransform">
        <rect class="die-backplate" x="0" y="0" width="1000" height="1000" rx="0.75" />
        <rect class="die-grid" x="18" y="18" width="964" height="964" rx="0.5" />
        <rect class="die-seal outer" x="46" y="46" width="908" height="908" rx="1.2" />
        <rect class="die-seal inner" x="62" y="62" width="876" height="876" rx="0.8" />
        <g class="pad-ring">
          <rect
            v-for="pad in padRingNodes"
            :key="pad.id"
            :x="pad.x"
            :y="pad.y"
            :width="pad.width"
            :height="pad.height"
            rx="0.35"
          />
        </g>
        <g class="decoder-ribs">
          <path
            v-for="rib in decoderRibs"
            :key="rib.id"
            :class="{ hot: rib.hot }"
            :d="`M96 ${rib.y + ATLAS_PAGE_TILE * 0.5} H140`"
          />
        </g>
        <g class="sense-amps">
          <rect
            v-for="amp in senseAmpNodes"
            :key="amp.id"
            :x="amp.x"
            :y="amp.y"
            width="18"
            height="38"
            rx="0.35"
          />
        </g>
        <g class="die-hardware-banks" aria-hidden="true">
          <rect
            v-for="bank in hardwareBankNodes"
            :key="bank.id"
            :class="{ hot: bank.hot }"
            :x="bank.x"
            :y="bank.y"
            :width="bank.width"
            :height="bank.height"
            rx="0.25"
          />
        </g>
        <path class="address-spine" d="M44 58 V934" />
        <g class="address-ticks">
          <g v-for="tick in addressTicks" :key="`${tick.kind}-${tick.address}`">
            <path :d="`M38 ${tick.y.toFixed(2)} H58`" />
            <text x="62" :y="tick.y + 4">{{ tick.label }}</text>
          </g>
        </g>
        <g class="capacity-ruler">
          <g v-for="tick in capacityRulerTicks" :key="tick.key">
            <path :d="`M${tick.x.toFixed(2)} ${tick.y.toFixed(2)} H${(tick.x + Math.max(18, tick.width)).toFixed(2)}`" />
            <text :x="tick.x + 2" :y="tick.y - 5">{{ tick.label }} · {{ tick.name }}</text>
          </g>
        </g>
        <path class="die-trace trace-a" d="M44 158 H68" />
        <path class="die-trace trace-b" d="M44 320 H68" />
        <path class="die-trace trace-c" d="M44 644 H68" />
        <path class="metal-bus bus-a" d="M392 92 H946 V760 H176" />
        <path class="metal-bus bus-b danger-flow" d="M68 644 H176 V724 H122" />
        <path class="metal-bus bus-c" d="M580 212 H946 V590 H770" />
        <g class="static-flow-lanes">
          <path d="M68 158 H284 V212 H608" />
          <path d="M68 320 H608 V482 H770" />
          <path d="M68 644 H176 V724 H122" />
        </g>
        <g class="charge-trails">
          <path
            v-for="trail in chargeTrails"
            :key="trail.id"
            :class="trail.tone"
            :d="trail.d"
          />
        </g>
        <g class="charge-packets">
          <circle
            v-for="packet in chargePackets"
            :key="packet.id"
            :class="packet.tone"
            :cx="packet.x"
            :cy="packet.y"
            :r="packet.r"
          />
        </g>
        <g class="danger-thermal-field">
          <rect x="68" y="614" width="162" height="112" rx="0.35" />
          <path d="M68 644 H176 V698 H122 V724 H68 Z" />
          <path d="M122 642 C196 616 260 638 320 604" />
          <path d="M176 698 C252 718 328 690 392 724" />
        </g>

        <g class="die-layer regions" :style="regionLayerStyle">
          <g
            v-for="node in dieRegionNodes"
            :key="node.region.id"
            class="die-cell region-cell"
            :class="[node.region.kind, { active: hoveredRegionId === node.region.id || selectedRegionId === node.region.id }]"
            :data-region-id="node.region.id"
            @click.stop="enterRegion(node.region, node)"
            @pointerenter="showRegionHover(node, $event)"
            @pointermove="moveHover"
            @pointerleave="hideHover"
          >
            <rect :x="node.x" :y="node.y" :width="node.width" :height="node.height" rx="0.6" />
            <clipPath :id="`region-clip-${node.region.id}`">
              <rect :x="node.x + 8" :y="node.y + 8" :width="Math.max(0, node.width - 16)" :height="Math.max(0, node.height - 16)" rx="0.6" />
            </clipPath>
            <g :clip-path="`url(#region-clip-${node.region.id})`">
              <rect v-if="node.width * cameraScale > 90 && node.height * cameraScale > 44" class="region-title-rail" :x="node.x + 8" :y="node.y + 8" :width="Math.max(0, node.width - 16)" :height="28" rx="0.4" />
              <text v-if="node.width * cameraScale > 95 && node.height * cameraScale > 42" class="region-title" :x="node.x + 14" :y="node.y + 28">{{ node.region.name }}</text>
              <text v-if="cameraScale >= 1.7 && node.width * cameraScale > 210 && node.height * cameraScale > 70" class="tag" :x="node.x + 14" :y="node.y + 49">
                {{ regionMeta(node.region.id).signal }}
              </text>
              <text v-if="cameraScale >= 1.85 && node.width * cameraScale > 240 && node.height * cameraScale > 94" class="micro" :x="node.x + 14" :y="node.y + 67">
                {{ hex(node.region.start) }} - {{ hex(node.region.start + node.region.size - 1) }}
              </text>
              <text v-if="cameraScale >= 1.55 && cameraScale < 2.8 && node.width * cameraScale > 420 && node.height * cameraScale > 122" class="detail" :x="node.x + 14" :y="node.y + 88">
                {{ regionMeta(node.region.id).detail }}
              </text>
              <text v-if="cameraScale < 2.8 && node.width * cameraScale > 260 && node.height * cameraScale > 150" class="risk" :class="regionMeta(node.region.id).risk" :x="node.x + 14" :y="node.y + node.height - 12">
                RISK {{ regionMeta(node.region.id).risk.toUpperCase() }}
              </text>
            </g>
          </g>
        </g>

        <g v-if="showPageLod" class="die-layer map-frames" :style="pageLayerStyle">
          <g
            v-for="frame in mapFrameNodes"
            :key="`${frame.regionId}-${frame.start}`"
            class="slot-frame map-frame"
            :class="[frame.risk, frame.regionId]"
            :data-region-id="frame.regionId"
          >
            <rect :x="frame.x" :y="frame.y" :width="frame.width" :height="frame.height" rx="0.35" />
            <text v-if="frame.width * cameraScale > 120 && frame.height * cameraScale > 28" :x="frame.x + 4" :y="frame.y + 15">
              {{ frame.label }}
            </text>
            <text v-if="frame.width * cameraScale > 260 && frame.height * cameraScale > 54 && cameraScale < 3.8" class="micro" :x="frame.x + 4" :y="frame.y + 30">
              {{ hex(frame.start ?? 0, 4) }} - {{ hex((frame.start ?? 0) + (frame.size ?? 1) - 1, 4) }}
            </text>
          </g>
        </g>

        <g v-if="showPageLod" class="die-layer pages map-pages" :class="{ fields: showFieldLod }" :style="pageLayerStyle">
          <g
            v-for="page in mapPageNodes"
            :key="page.start"
            class="die-cell page-cell"
            :class="[{ active: hoveredPageStart === page.start || selectedPageStart === page.start }, page.risk, page.regionId]"
            :data-region-id="page.regionId"
            @click.stop="focusMapPage(page, $event)"
            @pointerenter="showMapPageHover(page, $event)"
            @pointermove="moveHover"
            @pointerleave="hideHover"
          >
            <rect :x="page.x" :y="page.y" :width="page.width" :height="page.height" rx="0.45" />
            <clipPath :id="`page-clip-${page.start}`">
              <rect :x="page.x + 2" :y="page.y + 2" :width="Math.max(0, page.width - 4)" :height="Math.max(0, page.height - 4)" rx="0.45" />
            </clipPath>
            <g :clip-path="`url(#page-clip-${page.start})`">
              <text v-if="showFieldLod && page.width * cameraScale > 78 && page.height * cameraScale > 34" class="func-title" :x="page.x + 3" :y="page.y + 14">{{ page.label }}</text>
              <text v-if="showFieldLod && page.width * cameraScale > 180 && page.height * cameraScale > 62 && cameraScale < 5" class="detail" :x="page.x + 3" :y="page.y + 29">{{ page.regionName }} / {{ page.size }}B</text>
              <text v-if="cameraScale >= 2.8 && cameraScale < 5 && page.width * cameraScale > 180 && page.height * cameraScale > 88" class="micro" :x="page.x + 3" :y="page.y + 43">
                {{ hex(page.start, 4) }} - {{ hex(page.start + page.size - 1, 4) }}
              </text>
              <text v-if="cameraScale >= 3.1 && cameraScale < 4.2 && page.width * cameraScale > 200 && page.height * cameraScale > 110" class="risk" :class="page.risk" :x="page.x + 2" :y="page.y + page.height - 1">
                RISK {{ page.risk.toUpperCase() }}
              </text>
            </g>
          </g>
        </g>

        <g class="physical-page-audit" aria-hidden="true">
          <rect
            v-for="page in physicalPageNodes"
            :key="page.id"
            :class="[page.regionId, { major: page.major }]"
            :x="page.x"
            :y="page.y"
            :width="page.width"
            :height="page.height"
            rx="0.08"
          />
        </g>

        <g v-if="showByteLod && isVisualTestMode" class="die-layer map-bytes svg-byte-fallback">
          <g
            v-for="byte in mapByteNodes"
            :key="byte.abs"
            class="byte-cell dense"
            :class="{ active: selectedByteOffset === byte.offset, empty: byte.value === 0xff, zero: byte.value === 0x00 }"
          >
            <rect :x="byte.x" :y="byte.y" :width="byte.width" :height="byte.height" rx="0.08" />
          </g>
          <path v-for="(line, index) in byteGridLines" :key="`byte-grid-${index}`" class="byte-grid-line" :d="line.d" />
          <g v-if="showBitScope" class="selected-bit-scope">
            <g v-if="selectedByteGuides" class="selected-byte-guides">
              <rect
                :x="selectedByteGuides.pageFrame.x"
                :y="selectedByteGuides.pageFrame.y"
                :width="selectedByteGuides.pageFrame.width"
                :height="selectedByteGuides.pageFrame.height"
                rx="0.04"
              />
              <path class="row" :d="selectedByteGuides.row" />
              <path class="column" :d="selectedByteGuides.column" />
              <rect
                class="byte-frame"
                :x="selectedByteGuides.byteFrame.x"
                :y="selectedByteGuides.byteFrame.y"
                :width="selectedByteGuides.byteFrame.width"
                :height="selectedByteGuides.byteFrame.height"
                rx="0.02"
              />
            </g>
            <g v-if="selectedByteCircuit" class="selected-byte-circuit">
              <rect
                :x="selectedByteCircuit.frame.x"
                :y="selectedByteCircuit.frame.y"
                :width="selectedByteCircuit.frame.width"
                :height="selectedByteCircuit.frame.height"
                rx="0.03"
              />
              <path
                v-for="lane in selectedByteCircuit.lanes"
                :key="`lane-${lane.id}`"
                :class="{ on: lane.on }"
                :d="lane.d"
              />
              <circle
                v-for="via in selectedByteCircuit.vias"
                :key="`via-${via.id}`"
                :class="{ on: via.on }"
                :cx="via.x"
                :cy="via.y"
                :r="via.r"
              />
            </g>
            <rect
              v-for="bit in selectedBitNodes"
              :key="`bit-${bit.bit}`"
              :class="{ on: bit.on }"
              :x="bit.x"
              :y="bit.y"
              :width="bit.width"
              :height="bit.height"
              rx="0.04"
            />
            <circle
              v-for="contact in selectedContactNodes"
              :key="`contact-${contact.id}`"
              :cx="contact.x"
              :cy="contact.y"
              :r="contact.r"
            />
          </g>
        </g>

        </g>
      </svg>

      <div v-if="!isVisualTestMode" ref="pixiHostEl" class="pixi-byte-layer"></div>

      <div v-if="hoverInfo && showHoverTooltip" class="die-tooltip" :class="hoverInfo.risk" :style="{ left: `${hoverInfo.x}px`, top: `${hoverInfo.y}px` }">
        <span class="tooltip-kicker">RISK {{ hoverInfo.risk.toUpperCase() }}</span>
        <strong>{{ hoverInfo.title }}</strong>
        <p>{{ hoverInfo.detail }}</p>
        <code>{{ hoverInfo.range }}</code>
      </div>

      <div v-if="level === 'byte' || hasRange" ref="meaningEl" class="die-meaning" :class="{ armed: writePanelOpen }">
        <div class="meaning-main">
          <span class="meaning-kicker">{{ hasRange ? 'BYTE GROUP MEANING' : 'BYTE MEANING' }}</span>
          <strong>{{ hasRange ? `${hex(selectedRangeAbsStart)} - ${hex(selectedRangeAbsEnd)}` : selectedByteMeaning.field }}</strong>
          <p>{{ hasRange ? `${rangeEnd - rangeStart + 1} bytes selected in ${selectedRegion.name}` : selectedByteMeaning.area }}</p>
        </div>
        <div class="meaning-codes">
          <code>{{ hasRange ? `${hex(selectedRangeAbsStart)}..${hex(selectedRangeAbsEnd)}` : hex(selectedByteAbs) }}</code>
          <code v-if="!hasRange">{{ byteHex(selectedByteValue) }} / {{ byteBits(selectedByteValue) }}</code>
        </div>
      </div>
    </div>
  </StudioDialog>
</template>

<style>
.storm-dataflash-dialog {
  border: 1px solid rgba(34, 211, 238, 0.34) !important;
  border-radius: 10px !important;
  background: rgba(0, 6, 18, 0.34) !important;
  box-shadow:
    0 26px 90px rgba(0, 0, 0, 0.56),
    0 0 120px rgba(14, 165, 233, 0.2),
    0 0 46px rgba(244, 63, 94, 0.12) !important;
  backdrop-filter: blur(10px) saturate(1.08);
  -webkit-backdrop-filter: blur(10px) saturate(1.08);
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
    linear-gradient(90deg, rgba(125, 211, 252, 0.035) 0 1px, transparent 1px 80px),
    linear-gradient(0deg, rgba(125, 211, 252, 0.025) 0 1px, transparent 1px 80px),
    radial-gradient(circle at 50% 34%, rgba(14, 165, 233, 0.13), transparent 38%),
    radial-gradient(circle at 78% 78%, rgba(244, 63, 94, 0.13), transparent 24%),
    linear-gradient(135deg, #020617 0%, #050816 48%, #020617 100%);
  overflow: hidden;
  outline: none;
}

.die-explorer.visual-test *,
.die-explorer.visual-test *::before,
.die-explorer.visual-test *::after {
  animation: none !important;
  transition: none !important;
}

body:has(.die-explorer.visual-test) .p-toast {
  display: none !important;
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
  inset: 3.8rem 1rem 3.2rem;
  width: calc(100% - 2rem);
  height: calc(100% - 7rem);
  cursor: grab;
  touch-action: none;
  filter: drop-shadow(0 28px 60px rgba(0, 0, 0, 0.38));
  transform-origin: 50% 50%;
}

.pixi-byte-layer {
  position: absolute;
  inset: 3.8rem 1rem 3.2rem;
  z-index: 2;
  width: calc(100% - 2rem);
  height: calc(100% - 7rem);
  pointer-events: none;
  mix-blend-mode: screen;
}

.pixi-byte-canvas {
  display: block;
  width: 100%;
  height: 100%;
}

.die-canvas.grabbing {
  cursor: grabbing;
}

.die-world {
  will-change: transform;
}

.die-world.deep-zoom .address-ticks text,
.die-world.deep-zoom .region-cell text,
.die-world.deep-zoom .map-frame text,
.die-world.deep-zoom .page-cell text {
  display: none;
}

.die-world.deep-zoom .regions,
.die-world.deep-zoom .map-frames,
.die-world.deep-zoom .map-pages,
.die-world.deep-zoom .capacity-ruler text,
.die-world.deep-zoom .physical-page-audit,
.die-world.deep-zoom .die-hardware-banks,
.die-world.deep-zoom .charge-trails,
.die-world.deep-zoom .charge-packets,
.die-world.deep-zoom .static-flow-lanes,
.die-world.deep-zoom .metal-bus,
.die-world.deep-zoom .danger-thermal-field {
  display: none;
}

.die-backplate {
  fill: rgba(2, 6, 23, 0.5);
  stroke: rgba(125, 211, 252, 0.18);
  stroke-width: 0.75;
}

.die-grid {
  fill: url(#storm-grid);
  stroke: rgba(125, 211, 252, 0.13);
  stroke-width: 0.9;
}

.die-seal {
  fill: none;
  vector-effect: non-scaling-stroke;
  shape-rendering: crispEdges;
}

.die-seal.outer {
  stroke: rgba(186, 230, 253, 0.28);
  stroke-width: 1.25;
  filter: drop-shadow(0 0 10px rgba(14, 165, 233, 0.14));
}

.die-seal.inner {
  stroke: rgba(251, 113, 133, 0.16);
  stroke-width: 0.7;
  stroke-dasharray: 10 14;
}

.pad-ring rect {
  fill: rgba(125, 211, 252, 0.12);
  stroke: rgba(186, 230, 253, 0.36);
  stroke-width: 0.46;
  vector-effect: non-scaling-stroke;
  filter: drop-shadow(0 0 5px rgba(14, 165, 233, 0.12));
}

.decoder-ribs path {
  fill: none;
  stroke: rgba(125, 211, 252, 0.3);
  stroke-width: 1.1;
  stroke-dasharray: 9 8;
  vector-effect: non-scaling-stroke;
}

.decoder-ribs path.hot {
  stroke: rgba(251, 113, 133, 0.5);
  filter: drop-shadow(0 0 7px rgba(244, 63, 94, 0.2));
}

.sense-amps rect {
  fill: rgba(8, 47, 73, 0.34);
  stroke: rgba(94, 234, 212, 0.26);
  stroke-width: 0.45;
  vector-effect: non-scaling-stroke;
  shape-rendering: crispEdges;
}

.die-hardware-banks rect {
  fill: rgba(8, 47, 73, 0.15);
  stroke: rgba(125, 211, 252, 0.13);
  stroke-width: 0.32;
  vector-effect: non-scaling-stroke;
  shape-rendering: crispEdges;
}

.die-hardware-banks rect.hot {
  fill: rgba(127, 29, 29, 0.12);
  stroke: rgba(251, 113, 133, 0.22);
  filter: drop-shadow(0 0 5px rgba(244, 63, 94, 0.1));
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

.metal-bus,
.static-flow-lanes path {
  fill: none;
  vector-effect: non-scaling-stroke;
}

.metal-bus {
  stroke: url(#metal-route);
  stroke-width: 2.1;
  stroke-linecap: square;
  opacity: 0.62;
}

.metal-bus.danger-flow {
  stroke: rgba(251, 113, 133, 0.48);
  filter: drop-shadow(0 0 10px rgba(244, 63, 94, 0.18));
}

.static-flow-lanes path {
  stroke: rgba(103, 232, 249, 0.46);
  stroke-width: 1.4;
  stroke-dasharray: 16 14;
  filter: drop-shadow(0 0 8px rgba(14, 165, 233, 0.14));
}

.static-flow-lanes path:nth-child(3) {
  stroke: rgba(251, 113, 133, 0.54);
  filter: drop-shadow(0 0 10px rgba(244, 63, 94, 0.22));
}

.charge-trails path,
.charge-packets circle {
  mix-blend-mode: screen;
}

.charge-trails path {
  fill: none;
  stroke-width: 3.2;
  stroke-linecap: round;
  stroke-dasharray: 22 18 5 18;
  opacity: 0.64;
  vector-effect: non-scaling-stroke;
}

.charge-trails .cyan {
  stroke: rgba(103, 232, 249, 0.7);
  filter: drop-shadow(0 0 14px rgba(34, 211, 238, 0.36));
}

.charge-trails .red {
  stroke: rgba(251, 113, 133, 0.72);
  filter: drop-shadow(0 0 16px rgba(244, 63, 94, 0.4));
}

.charge-packets circle {
  opacity: 0.92;
}

.charge-packets .cyan {
  fill: url(#charge-cyan);
  filter: drop-shadow(0 0 12px rgba(34, 211, 238, 0.55));
}

.charge-packets .red {
  fill: url(#charge-red);
  filter: drop-shadow(0 0 16px rgba(244, 63, 94, 0.58));
}

.danger-thermal-field {
  pointer-events: none;
  mix-blend-mode: screen;
}

.danger-thermal-field rect {
  fill: rgba(244, 63, 94, 0.16);
  stroke: rgba(251, 113, 133, 0.55);
  stroke-width: 0.75;
  filter: drop-shadow(0 0 24px rgba(244, 63, 94, 0.36));
  vector-effect: non-scaling-stroke;
}

.danger-thermal-field path {
  fill: rgba(251, 113, 133, 0.055);
  stroke: rgba(251, 113, 133, 0.36);
  stroke-width: 1.4;
  stroke-dasharray: 10 7;
  filter: drop-shadow(0 0 12px rgba(244, 63, 94, 0.22));
  vector-effect: non-scaling-stroke;
}

.address-spine {
  fill: none;
  stroke: rgba(125, 211, 252, 0.28);
  stroke-width: 1.2;
  stroke-dasharray: 4 10;
  filter: drop-shadow(0 0 10px rgba(14, 165, 233, 0.2));
}

.address-ticks path {
  stroke: rgba(125, 211, 252, 0.42);
  stroke-width: 1;
  vector-effect: non-scaling-stroke;
}

.address-ticks text {
  fill: rgba(186, 230, 253, 0.68);
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 10px;
  font-weight: 900;
  paint-order: stroke;
  stroke: rgba(2, 6, 23, 0.88);
  stroke-width: 2.4px;
  stroke-linejoin: round;
}

.capacity-ruler {
  pointer-events: none;
}

.capacity-ruler path {
  stroke: rgba(186, 230, 253, 0.36);
  stroke-width: 0.58;
  stroke-dasharray: 2 5;
  vector-effect: non-scaling-stroke;
  shape-rendering: crispEdges;
}

.capacity-ruler text {
  fill: rgba(224, 242, 254, 0.72);
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 8px;
  font-weight: 900;
  paint-order: stroke;
  stroke: rgba(2, 6, 23, 0.92);
  stroke-width: 2px;
  stroke-linejoin: round;
}

.physical-page-audit {
  pointer-events: none;
  mix-blend-mode: screen;
}

.physical-page-audit rect {
  fill: none;
  stroke: rgba(224, 242, 254, 0.16);
  stroke-width: 0.22;
  vector-effect: non-scaling-stroke;
  shape-rendering: crispEdges;
}

.physical-page-audit rect.major {
  stroke: rgba(224, 242, 254, 0.42);
  stroke-width: 0.42;
}

.physical-page-audit rect.config {
  stroke: rgba(125, 211, 252, 0.34);
}

.physical-page-audit rect.runtime {
  stroke: rgba(94, 234, 212, 0.3);
}

.physical-page-audit rect.macro {
  stroke: rgba(196, 181, 253, 0.28);
}

.physical-page-audit rect.reserved {
  stroke: rgba(148, 163, 184, 0.18);
}

.physical-page-audit rect.ble,
.physical-page-audit rect.tail {
  stroke: rgba(251, 113, 133, 0.42);
}

.die-layer {
  filter: none;
}

.die-cell {
  cursor: pointer;
  outline: none;
}

.die-cell rect {
  fill: rgba(15, 23, 42, 0.76);
  stroke: rgba(148, 163, 184, 0.2);
  stroke-width: 0.42;
  transition: fill 0.24s ease, stroke 0.24s ease, opacity 0.24s ease, filter 0.24s ease;
  transform-box: fill-box;
  transform-origin: center;
  vector-effect: non-scaling-stroke;
  shape-rendering: geometricPrecision;
}

.die-cell:hover rect,
.die-cell.active rect,
.die-cell.selected rect {
  stroke: rgba(186, 230, 253, 0.92);
  filter: brightness(1.16) saturate(1.14);
}

.region-cell.active rect {
  stroke: rgba(165, 243, 252, 1);
  stroke-width: 1.8;
  filter: drop-shadow(0 0 18px rgba(34, 211, 238, 0.5));
}

.region-cell rect {
  filter: drop-shadow(0 16px 26px rgba(0, 0, 0, 0.24));
}

.map-pages {
  opacity: 0.68;
}

.map-frames {
  opacity: 0.94;
  pointer-events: none;
}

.map-frame rect {
  fill: transparent;
  stroke: rgba(186, 230, 253, 0.3);
  stroke-width: 0.72;
  stroke-dasharray: none;
  filter: drop-shadow(0 0 8px rgba(14, 165, 233, 0.12));
}

.map-frame.config rect {
  stroke: rgba(125, 211, 252, 0.46);
}

.map-frame.runtime rect {
  stroke: rgba(94, 234, 212, 0.42);
}

.map-frame.macro rect {
  stroke: rgba(196, 181, 253, 0.42);
}

.map-frame.high rect,
.map-frame.critical rect,
.map-frame.protected rect {
  stroke: rgba(251, 113, 133, 0.56);
  filter: drop-shadow(0 0 10px rgba(244, 63, 94, 0.16));
}

.map-frame text {
  fill: rgba(224, 242, 254, 0.86);
  paint-order: stroke;
  stroke: rgba(2, 6, 23, 0.88);
  stroke-width: 2.5px;
  stroke-linejoin: round;
}

.map-frame .micro {
  fill: rgba(125, 211, 252, 0.72);
}

.map-pages .page-cell rect {
  fill: rgba(8, 47, 73, 0.28);
  stroke: rgba(125, 211, 252, 0.27);
  stroke-width: 0.36;
  shape-rendering: crispEdges;
}

.map-pages .page-cell.config rect {
  fill: rgba(30, 64, 175, 0.24);
  stroke: rgba(125, 211, 252, 0.42);
}

.map-pages .page-cell.runtime rect {
  fill: rgba(13, 148, 136, 0.24);
  stroke: rgba(94, 234, 212, 0.36);
}

.map-pages .page-cell.macro rect {
  fill: rgba(88, 28, 135, 0.28);
  stroke: rgba(196, 181, 253, 0.36);
}

.map-pages .page-cell.reserved rect {
  fill: rgba(15, 23, 42, 0.44);
  stroke: rgba(148, 163, 184, 0.22);
}

.map-pages .page-cell.ble rect,
.map-pages .page-cell.protected rect {
  fill: rgba(159, 18, 57, 0.36);
  stroke: rgba(251, 113, 133, 0.5);
}

.map-pages .page-cell.tail rect {
  fill: rgba(15, 23, 42, 0.38);
  stroke: rgba(148, 163, 184, 0.28);
}

.map-pages.fields .page-cell rect {
  fill: rgba(8, 47, 73, 0.25);
}

.map-pages .page-cell.active rect {
  fill: rgba(14, 165, 233, 0.34);
  stroke: rgba(165, 243, 252, 1);
  stroke-width: 1.55;
  filter: drop-shadow(0 0 14px rgba(34, 211, 238, 0.42));
}

.map-pages .page-cell.high.active rect,
.map-pages .page-cell.critical.active rect {
  fill: rgba(127, 29, 29, 0.42);
  stroke: rgba(251, 113, 133, 0.98);
  filter: drop-shadow(0 0 12px rgba(244, 63, 94, 0.38));
}

.map-bytes {
  filter: drop-shadow(0 0 8px rgba(14, 165, 233, 0.18));
}

.map-bytes .byte-cell rect {
  stroke-width: 0.45;
  stroke: rgba(125, 211, 252, 0.32);
}

.map-bytes .byte-cell.active rect {
  stroke-width: 1.25;
}

.svg-byte-fallback .byte-cell rect {
  fill: rgba(8, 47, 73, 0.58);
  stroke: rgba(125, 211, 252, 0.62);
  stroke-width: 0.22;
  shape-rendering: crispEdges;
}

.die-world.sub-byte-zoom .svg-byte-fallback .byte-cell rect {
  fill: rgba(8, 47, 73, 0.16);
  stroke: rgba(125, 211, 252, 0.12);
  stroke-width: 0.035;
  opacity: 0.48;
}

.svg-byte-fallback .byte-cell.empty rect {
  fill: rgba(30, 41, 59, 0.76);
  stroke: rgba(125, 211, 252, 0.48);
}

.die-world.sub-byte-zoom .svg-byte-fallback .byte-cell.empty rect {
  fill: rgba(15, 23, 42, 0.2);
  stroke: rgba(125, 211, 252, 0.08);
}

.svg-byte-fallback .byte-cell.zero rect {
  fill: rgba(8, 145, 178, 0.46);
}

.die-world.sub-byte-zoom .svg-byte-fallback .byte-cell.zero rect {
  fill: rgba(8, 145, 178, 0.2);
}

.svg-byte-fallback .byte-cell.active rect {
  fill: rgba(244, 63, 94, 0.72);
  stroke: rgba(251, 113, 133, 0.98);
  stroke-width: 0.42;
  opacity: 1;
  filter: drop-shadow(0 0 0.75px rgba(251, 113, 133, 0.55));
}

.die-world.sub-byte-zoom .svg-byte-fallback .byte-cell.active rect {
  fill: rgba(244, 63, 94, 0.58);
  stroke: rgba(254, 205, 211, 0.98);
  stroke-width: 0.08;
}

.svg-byte-fallback .byte-grid-line {
  fill: none;
  stroke: rgba(125, 211, 252, 0.62);
  stroke-width: 0.18;
  shape-rendering: crispEdges;
}

.die-world.sub-byte-zoom .svg-byte-fallback .byte-grid-line {
  stroke: rgba(125, 211, 252, 0.08);
  stroke-width: 0.035;
}

.selected-bit-scope rect {
  fill: rgba(15, 23, 42, 0.88);
  stroke: rgba(125, 211, 252, 0.28);
  stroke-width: 0.035;
  shape-rendering: crispEdges;
}

.selected-bit-scope rect.on {
  fill: rgba(251, 113, 133, 0.82);
  stroke: rgba(254, 205, 211, 0.92);
  filter: drop-shadow(0 0 0.42px rgba(251, 113, 133, 0.52));
}

.selected-bit-scope circle {
  fill: rgba(224, 242, 254, 0.58);
  stroke: rgba(103, 232, 249, 0.44);
  stroke-width: 0.024;
  filter: drop-shadow(0 0 0.3px rgba(125, 211, 252, 0.42));
}

.selected-byte-circuit {
  pointer-events: none;
}

.selected-byte-circuit rect {
  fill: rgba(2, 6, 23, 0.18);
  stroke: rgba(254, 205, 211, 0.42);
  stroke-width: 0.035;
  filter: drop-shadow(0 0 0.55px rgba(251, 113, 133, 0.28));
}

.selected-byte-circuit path {
  fill: none;
  stroke: rgba(125, 211, 252, 0.44);
  stroke-width: 0.03;
  stroke-linecap: square;
  shape-rendering: crispEdges;
}

.selected-byte-circuit path.on {
  stroke: rgba(254, 205, 211, 0.86);
  stroke-width: 0.045;
}

.selected-byte-circuit circle {
  fill: rgba(125, 211, 252, 0.55);
  stroke: rgba(224, 242, 254, 0.48);
  stroke-width: 0.015;
}

.selected-byte-circuit circle.on {
  fill: rgba(251, 113, 133, 0.88);
  stroke: rgba(255, 241, 242, 0.82);
}

.selected-byte-guides {
  pointer-events: none;
}

.selected-byte-guides rect {
  fill: none;
  stroke: rgba(125, 211, 252, 0.13);
  stroke-width: 0.025;
  shape-rendering: crispEdges;
}

.selected-byte-guides path {
  fill: none;
  stroke: rgba(125, 211, 252, 0.16);
  stroke-width: 0.04;
  stroke-dasharray: 0.22 0.2;
  shape-rendering: crispEdges;
}

.selected-byte-guides .row {
  stroke: rgba(251, 113, 133, 0.34);
}

.selected-byte-guides .column {
  stroke: rgba(103, 232, 249, 0.26);
}

.selected-byte-guides .byte-frame {
  stroke: rgba(254, 205, 211, 0.98);
  stroke-width: 0.075;
  filter: drop-shadow(0 0 1px rgba(251, 113, 133, 0.58));
}

.slot-frame rect {
  fill: rgba(2, 6, 23, 0.08);
  stroke: rgba(125, 211, 252, 0.24);
  stroke-width: 0.82;
  stroke-dasharray: 8 7;
  pointer-events: none;
}

.slot-frame text {
  fill: rgba(186, 230, 253, 0.72);
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 13px;
  font-weight: 950;
  pointer-events: none;
}

.slot-frame .micro {
  fill: rgba(125, 211, 252, 0.56);
  font-size: 10px;
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

.die-cell .tag {
  fill: #a7f3d0;
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 11px;
  font-weight: 900;
  letter-spacing: 0.04em;
}

.region-title-rail {
  fill: rgba(2, 6, 23, 0.34);
  stroke: rgba(186, 230, 253, 0.12);
  stroke-width: 0.5;
  vector-effect: non-scaling-stroke;
}

.die-cell .region-title {
  fill: #e0f2fe;
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 18px;
  font-weight: 950;
  letter-spacing: 0.02em;
  paint-order: stroke;
  stroke: rgba(2, 6, 23, 0.9);
  stroke-width: 3px;
  stroke-linejoin: round;
}

.die-cell .func-title {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 13px;
  font-weight: 900;
  letter-spacing: 0.02em;
  paint-order: stroke;
  stroke: rgba(2, 6, 23, 0.9);
  stroke-width: 2.2px;
  stroke-linejoin: round;
}

.die-cell .detail {
  fill: #bfdbfe;
  font-size: 10px;
  font-weight: 800;
}

.die-cell .risk {
  fill: #93c5fd;
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 12px;
  font-weight: 900;
  letter-spacing: 0.05em;
}

.die-cell .risk.high,
.die-cell .risk.critical {
  fill: #fecdd3;
}

.die-cell .risk.medium {
  fill: #fde68a;
}

.region-cell.config rect {
  fill: url(#config-core);
  stroke: rgba(125, 211, 252, 0.7);
}
.region-cell.runtime rect {
  fill: url(#runtime-cache);
  stroke: rgba(94, 234, 212, 0.54);
}
.region-cell.macro rect {
  fill: url(#macro-stream);
  stroke: rgba(196, 181, 253, 0.55);
}
.region-cell.reserved rect {
  fill: url(#reserved-seal);
  stroke: rgba(148, 163, 184, 0.34);
}
.region-cell.protected rect {
  fill: url(#protected-fuse);
  stroke: rgba(251, 113, 133, 0.62);
}

.map-frame.reserved rect,
.page-cell.reserved rect {
  stroke-dasharray: 6 7;
}

.page-cell.reserved rect {
  fill: rgba(15, 23, 42, 0.32);
}

.page-cell rect {
  fill: rgba(8, 47, 73, 0.28);
}

.page-cell.medium rect {
  fill: rgba(120, 53, 15, 0.2);
}

.page-cell.high rect {
  fill: rgba(127, 29, 29, 0.22);
}

.page-cell.critical rect {
  fill: rgba(159, 18, 57, 0.28);
}

.page-cell:nth-child(3n) rect {
  fill: rgba(30, 41, 59, 0.56);
}

.page-cell.medium:nth-child(3n) rect {
  fill: rgba(120, 53, 15, 0.32);
}

.page-cell.high:nth-child(3n) rect {
  fill: rgba(127, 29, 29, 0.34);
}

.page-cell.critical:nth-child(3n) rect {
  fill: rgba(159, 18, 57, 0.44);
}

.cluster-cell rect {
  fill: rgba(14, 116, 144, 0.32);
}

.cluster-cell.medium rect {
  fill: rgba(120, 53, 15, 0.28);
}

.cluster-cell.high rect,
.cluster-cell.critical rect {
  fill: rgba(127, 29, 29, 0.34);
}

.cluster-cell .detail {
  fill: #dbeafe;
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 13px;
  font-weight: 900;
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
  animation: byteSelectedPulse 1.8s ease-in-out infinite;
}

.byte-cell.active rect {
  fill: rgba(244, 63, 94, 0.36);
  stroke: rgba(251, 113, 133, 0.98);
  animation: byteDangerPulse 1.1s ease-in-out infinite;
}

.byte-cell text {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 42px;
}

.byte-cell.dense rect {
  stroke-width: 1.1;
}

.byte-cell.dense text {
  font-size: 16px;
  font-weight: 950;
}

.byte-cell .micro {
  font-size: 18px;
}

.die-hud {
  position: absolute;
  left: 50%;
  top: 1rem;
  z-index: 3;
  display: flex;
  align-items: center;
  gap: 0.85rem;
  max-width: calc(100% - 3rem);
  min-height: 2.1rem;
  padding: 0 0.85rem;
  border: 1px solid rgba(125, 211, 252, 0.18);
  border-radius: 2px;
  background:
    linear-gradient(90deg, rgba(2, 6, 23, 0.46), rgba(8, 47, 73, 0.28), rgba(2, 6, 23, 0.46)),
    repeating-linear-gradient(90deg, rgba(125, 211, 252, 0.08) 0 1px, transparent 1px 18px);
  color: #dbeafe;
  box-shadow: 0 0 30px rgba(14, 165, 233, 0.12), inset 0 0 20px rgba(125, 211, 252, 0.05);
  transform: translateX(-50%);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  pointer-events: none;
  clip-path: polygon(0 0, calc(100% - 12px) 0, 100% 50%, calc(100% - 12px) 100%, 0 100%, 10px 50%);
}

.die-coordinate-hud {
  position: absolute;
  right: 1.05rem;
  bottom: 0.76rem;
  z-index: 5;
  max-width: min(720px, calc(100% - 2.1rem));
  padding: 0.32rem 0.48rem;
  border: 1px solid rgba(125, 211, 252, 0.16);
  border-radius: 3px;
  background:
    linear-gradient(90deg, rgba(2, 6, 23, 0.42), rgba(8, 47, 73, 0.24)),
    repeating-linear-gradient(90deg, rgba(125, 211, 252, 0.05) 0 1px, transparent 1px 13px);
  color: rgba(219, 234, 254, 0.78);
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.59rem;
  font-weight: 900;
  line-height: 1.1;
  letter-spacing: 0;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  pointer-events: none;
  box-shadow: inset 0 0 18px rgba(125, 211, 252, 0.045);
}

.die-coordinate-hud.dragging {
  color: #e0f2fe;
  border-color: rgba(103, 232, 249, 0.36);
  box-shadow: 0 0 18px rgba(14, 165, 233, 0.14), inset 0 0 20px rgba(125, 211, 252, 0.07);
}

.die-page-context {
  position: absolute;
  left: 1.05rem;
  bottom: 2.58rem;
  z-index: 5;
  width: 6.8rem;
  padding: 0.34rem;
  border: 1px solid rgba(125, 211, 252, 0.18);
  border-radius: 3px;
  background:
    linear-gradient(135deg, rgba(2, 6, 23, 0.42), rgba(8, 47, 73, 0.2)),
    repeating-linear-gradient(90deg, rgba(125, 211, 252, 0.04) 0 1px, transparent 1px 12px);
  pointer-events: none;
  box-shadow: inset 0 0 18px rgba(125, 211, 252, 0.04);
}

.context-head {
  display: flex;
  justify-content: space-between;
  gap: 0.4rem;
  margin-bottom: 0.28rem;
  color: rgba(219, 234, 254, 0.76);
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.48rem;
  font-weight: 900;
  line-height: 1;
}

.context-head code {
  color: rgba(125, 211, 252, 0.82);
  font: inherit;
}

.context-grid {
  display: grid;
  grid-template-columns: repeat(16, 1fr);
  gap: 1px;
}

.context-grid span {
  aspect-ratio: 1;
  min-width: 0;
  background: rgba(15, 23, 42, 0.62);
  border: 1px solid rgba(125, 211, 252, 0.12);
}

.context-grid span.axis {
  background: rgba(8, 47, 73, 0.74);
  border-color: rgba(103, 232, 249, 0.26);
}

.context-grid span.active {
  background: rgba(244, 63, 94, 0.9);
  border-color: rgba(254, 205, 211, 0.95);
  box-shadow: 0 0 12px rgba(244, 63, 94, 0.68);
}

.die-hud.danger {
  border-color: rgba(251, 113, 133, 0.62);
  box-shadow: 0 0 36px rgba(244, 63, 94, 0.24), inset 0 0 18px rgba(251, 113, 133, 0.06);
}

.die-hud.armed {
  border-color: rgba(251, 113, 133, 0.72);
  box-shadow: 0 0 36px rgba(244, 63, 94, 0.3), inset 0 0 22px rgba(251, 113, 133, 0.08);
}

.die-hud span,
.die-hud code {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
}

.die-hud span {
  overflow: hidden;
  text-overflow: ellipsis;
  font-size: 0.78rem;
  font-weight: 900;
}

.die-hud .hud-chip {
  flex: 0 0 auto;
  display: inline-flex;
  align-items: center;
  min-height: 1.32rem;
  padding: 0 0.5rem;
  border: 1px solid rgba(103, 232, 249, 0.46);
  border-radius: 2px;
  background: rgba(14, 116, 144, 0.24);
  color: #e0f2fe;
  font-size: 0.64rem;
  letter-spacing: 0.04em;
}

.die-hud .hud-chip.level {
  color: #a5f3fc;
  text-shadow: 0 0 10px rgba(34, 211, 238, 0.55);
}

.die-hud .hud-chip.high,
.die-hud .hud-chip.critical {
  border-color: rgba(251, 113, 133, 0.42);
  background: rgba(127, 29, 29, 0.28);
  color: #fecdd3;
}

.die-hud .hud-chip.medium {
  border-color: rgba(251, 191, 36, 0.36);
  background: rgba(120, 53, 15, 0.22);
  color: #fde68a;
}

.die-hud code {
  flex: 0 0 auto;
  color: #67e8f9;
  font-size: 0.72rem;
}

.die-tooltip {
  position: fixed;
  z-index: 10000;
  width: 320px;
  max-width: calc(100vw - 36px);
  padding: 0.75rem 0.85rem;
  border: 1px solid rgba(125, 211, 252, 0.32);
  border-radius: 3px;
  background:
    linear-gradient(135deg, rgba(2, 6, 23, 0.92), rgba(8, 47, 73, 0.82)),
    repeating-linear-gradient(90deg, rgba(125, 211, 252, 0.05) 0 1px, transparent 1px 18px);
  color: #dbeafe;
  box-shadow: 0 18px 48px rgba(0, 0, 0, 0.42), 0 0 36px rgba(14, 165, 233, 0.18);
  pointer-events: none;
  backdrop-filter: blur(16px) saturate(1.15);
  -webkit-backdrop-filter: blur(16px) saturate(1.15);
}

.die-tooltip.high,
.die-tooltip.critical {
  border-color: rgba(251, 113, 133, 0.42);
  box-shadow: 0 18px 48px rgba(0, 0, 0, 0.42), 0 0 38px rgba(244, 63, 94, 0.2);
}

.tooltip-kicker,
.die-tooltip strong,
.die-tooltip p,
.die-tooltip code {
  display: block;
}

.tooltip-kicker {
  color: #67e8f9;
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.62rem;
  font-weight: 900;
  letter-spacing: 0.08em;
}

.die-tooltip.high .tooltip-kicker,
.die-tooltip.critical .tooltip-kicker {
  color: #fda4af;
}

.die-tooltip strong {
  margin-top: 0.22rem;
  color: #e0f2fe;
  font-size: 0.96rem;
  line-height: 1.18;
  text-shadow: 0 0 14px rgba(186, 230, 253, 0.18);
}

.die-tooltip p {
  margin: 0.45rem 0 0;
  color: #bfdbfe;
  font-size: 0.74rem;
  line-height: 1.45;
}

.die-tooltip code {
  margin-top: 0.55rem;
  color: #93c5fd;
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
  font-size: 0.68rem;
  font-weight: 900;
}

.die-meaning {
  position: absolute;
  right: 1.05rem;
  bottom: 2.62rem;
  z-index: 4;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  max-width: min(520px, calc(100% - 2.2rem));
  min-height: 2.16rem;
  padding: 0.32rem 0.54rem 0.32rem 0.68rem;
  border: 1px solid rgba(125, 211, 252, 0.44);
  border-radius: 3px;
  background:
    linear-gradient(135deg, rgba(2, 6, 23, 0.68), rgba(8, 47, 73, 0.46)),
    repeating-linear-gradient(90deg, rgba(125, 211, 252, 0.05) 0 1px, transparent 1px 18px);
  color: #e0f2fe;
  box-shadow: 0 0 30px rgba(14, 165, 233, 0.16), inset 0 0 22px rgba(125, 211, 252, 0.05);
  overflow: hidden;
  pointer-events: none;
  clip-path: polygon(10px 0, 100% 0, calc(100% - 10px) 100%, 0 100%);
}

.die-meaning.armed {
  border-color: rgba(251, 113, 133, 0.78);
  box-shadow: 0 0 34px rgba(244, 63, 94, 0.28), inset 0 0 24px rgba(251, 113, 133, 0.06);
  animation: armPulse 1.2s ease-in-out infinite;
}

.meaning-main {
  min-width: 0;
  max-width: 12rem;
}

.meaning-main,
.meaning-codes {
  display: flex;
}

.meaning-main {
  flex-direction: column;
}

.meaning-codes {
  flex: 0 0 auto;
  flex-direction: column;
  gap: 0.18rem;
}

.die-meaning span,
.die-meaning strong,
.die-meaning code,
.die-meaning p {
  font-family: 'JetBrains Mono', 'SFMono-Regular', Consolas, monospace;
}

.die-meaning span,
.die-meaning strong,
.die-meaning code {
  font-size: 0.62rem;
  font-weight: 900;
  white-space: nowrap;
}

.meaning-kicker {
  color: #67e8f9;
  font-size: 0.58rem;
  letter-spacing: 0.08em;
}

.die-meaning strong {
  color: #e0f2fe;
  overflow: hidden;
  text-overflow: ellipsis;
  text-shadow: 0 0 14px rgba(125, 211, 252, 0.18);
}

.die-meaning p {
  display: none;
  margin: 0;
  color: #bfdbfe;
  font-size: 0.66rem;
  font-weight: 800;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.die-meaning code {
  color: #bae6fd;
  opacity: 0.94;
  font-size: 0.58rem;
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

  .pixi-byte-layer {
    inset: 4rem 0.75rem 5.3rem;
    width: calc(100% - 1.5rem);
    height: calc(100% - 9.3rem);
  }

  .die-hud {
    left: 0.75rem;
    right: 0.75rem;
    transform: none;
    max-width: none;
    gap: 0.45rem;
  }

  .die-hud .hud-chip {
    display: none;
  }

  .die-meaning {
    left: 0.75rem;
    right: 0.75rem;
    bottom: 4rem;
    overflow-x: auto;
  }

  .die-page-context {
    display: none;
  }
}

@keyframes dieSweep {
  0%, 100% { transform: translateX(-12%); opacity: 0.18; }
  50% { transform: translateX(12%); opacity: 0.36; }
}

@keyframes traceFlow {
  to { stroke-dashoffset: -68; }
}

@keyframes entrySweep {
  0%, 100% { opacity: 0.12; transform: translateX(-10%); }
  50% { opacity: 0.32; transform: translateX(10%); }
}

@keyframes byteSelectedPulse {
  0%, 100% { stroke-width: 1.8; }
  50% { stroke-width: 3; }
}

@keyframes byteDangerPulse {
  0%, 100% { stroke-width: 2; filter: brightness(1); }
  50% { stroke-width: 3.4; filter: brightness(1.28); }
}

@keyframes armPulse {
  0%, 100% { box-shadow: 0 0 28px rgba(244, 63, 94, 0.22); }
  50% { box-shadow: 0 0 44px rgba(244, 63, 94, 0.42), 0 0 18px rgba(251, 113, 133, 0.22); }
}

@keyframes spin {
  to { transform: rotate(1turn); }
}
</style>
