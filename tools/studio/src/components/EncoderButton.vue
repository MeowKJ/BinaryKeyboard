<template>
    <div class="encoder-button" :class="{ selected: anySelected, disabled }" :style="gridStyle">
        <!-- 使用 SVG 绘制3个扇形区域和分割线 -->
        <svg class="encoder-svg" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
            <!-- 分割线 -->
            <line v-for="(angle, idx) in dividerAngles" :key="`divider-${idx}`"
                :x1="50 + 50 * Math.cos(angle * Math.PI / 180)" :y1="50 + 50 * Math.sin(angle * Math.PI / 180)" x2="50"
                y2="50" class="encoder-divider" stroke-width="2" stroke-linecap="round" />

            <!-- 3个扇形区域（作为可点击的 path） -->
            <path v-for="(part, idx) in encoderParts" :key="`sector-${idx}`" :d="getSectorPath(idx)"
                :class="['encoder-sector', `encoder-sector-${idx}`, { selected: selectedIndex === part.index }]"
                @click.stop="!disabled && handlePartClick(part.index)" />
        </svg>

        <!-- 标签和徽章（在 SVG 上方） -->
        <div v-for="(part, idx) in encoderParts" :key="`label-${idx}`" class="encoder-label-container"
            :class="{ selected: selectedIndex === part.index }" :style="getLabelContainerStyle(idx)"
            @click.stop="!disabled && handlePartClick(part.index)">
            <span class="encoder-part-label">{{ part.label }}</span>
            <span v-if="part.actionBadge" class="encoder-part-badge">{{ part.actionBadge }}</span>
        </div>
    </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { KeyDef } from '@/config/layouts';
import { ActionType, type KeyAction } from '@/types/protocol';
import { getKeycodeName } from '@/utils/keycodes';
import { getConsumerName } from '@/utils/consumer';

const props = defineProps<{
    /** 旋钮按键定义（按下按键） */
    keyDef: KeyDef;
    /** 3个旋钮操作的动作 */
    actions: {
        cw: KeyAction;    // 顺时针（右转）
        press: KeyAction; // 按下
        ccw: KeyAction;   // 逆时针（左转）
    };
    /** 选中的按键索引 */
    selectedIndex: number;
    /** 旋钮的3个按键索引 */
    encoderIndices: {
        cw: number;    // 顺时针索引
        press: number; // 按下索引
        ccw: number;   // 逆时针索引
    };
    /** 是否禁用 */
    disabled?: boolean;
}>();

const emit = defineEmits<{
    select: [index: number];
}>();

/** 网格定位样式 */
const gridStyle = computed(() => {
    const k = props.keyDef;
    const rowSpan = k.size === '2u-v' ? 2 : 1;
    const colSpan = k.size === '2u-h' ? 2 : 1;

    return {
        gridRow: `${Math.floor(k.row) + 1} / span ${rowSpan}`,
        gridColumn: `${k.col + 1} / span ${colSpan}`,
    };
});

/** 是否有任何部分被选中 */
const anySelected = computed(() => {
    return props.selectedIndex === props.encoderIndices.cw ||
        props.selectedIndex === props.encoderIndices.press ||
        props.selectedIndex === props.encoderIndices.ccw;
});

/** 旋钮的3个部分 */
const encoderParts = computed(() => {
    return [
        {
            index: props.encoderIndices.ccw,
            label: '↺',
            action: props.actions.ccw,
            actionBadge: getActionBadge(props.actions.ccw),
        },
        {
            index: props.encoderIndices.press,
            label: '●',
            action: props.actions.press,
            actionBadge: getActionBadge(props.actions.press),
        },
        {
            index: props.encoderIndices.cw,
            label: '↻',
            action: props.actions.cw,
            actionBadge: getActionBadge(props.actions.cw),
        },
    ];
});

/** 分割线角度 */
const dividerAngles = computed(() => {
    // 3条分割线，分别位于 -90度（顶部）、30度、150度
    return [-90, 30, 150];
});

/** 获取扇形 SVG path */
function getSectorPath(index: number): string {
    // 每个扇形120度
    // 第一个扇形（左转/逆时针）：从 -90度到 30度
    // 第二个扇形（按下）：从 30度到 150度
    // 第三个扇形（右转/顺时针）：从 150度到 270度（-90度）

    let startAngle = index * 120 - 90; // 起始角度（度）
    let endAngle = (index + 1) * 120 - 90; // 结束角度（度）

    // 标准化角度到 0-360 范围
    const normalizeAngle = (angle: number) => {
        while (angle < 0) angle += 360;
        while (angle >= 360) angle -= 360;
        return angle;
    };

    const normalizedStart = normalizeAngle(startAngle);
    const normalizedEnd = normalizeAngle(endAngle);

    const centerX = 50;
    const centerY = 50;
    const radius = 50;

    // 转换为弧度
    const startRad = startAngle * Math.PI / 180;
    const endRad = endAngle * Math.PI / 180;

    // 计算起始点和结束点
    const startX = centerX + radius * Math.cos(startRad);
    const startY = centerY + radius * Math.sin(startRad);
    const endX = centerX + radius * Math.cos(endRad);
    const endY = centerY + radius * Math.sin(endRad);

    // 计算角度差（考虑跨越0度的情况）
    let angleDiff = normalizedEnd - normalizedStart;
    if (angleDiff < 0) angleDiff += 360;
    if (angleDiff === 0) angleDiff = 360;

    // 大弧标志（如果角度差大于180度）
    const largeArcFlag = angleDiff > 180 ? 1 : 0;

    // 构建 SVG path
    // M = 移动到中心点
    // L = 画线到起始点
    // A = 画弧到结束点（rx ry x-axis-rotation large-arc-flag sweep-flag x y）
    // Z = 闭合路径
    return `M ${centerX} ${centerY} L ${startX} ${startY} A ${radius} ${radius} 0 ${largeArcFlag} 1 ${endX} ${endY} Z`;
}

/** 获取标签容器样式（根据扇形中心角度定位） */
function getLabelContainerStyle(index: number) {
    // 计算扇形的实际中心角度
    let startAngle = index * 120 - 90; // 起始角度（度）
    let endAngle = (index + 1) * 120 - 90; // 结束角度（度）

    // 计算中心角度（考虑跨越0度的情况）
    let centerAngle = (startAngle + endAngle) / 2;

    // 标准化角度
    while (centerAngle < 0) centerAngle += 360;
    while (centerAngle >= 360) centerAngle -= 360;

    // 转换为弧度
    const centerRad = centerAngle * Math.PI / 180;

    // 距离中心的距离（百分比）
    const radius = 32;

    // 计算位置（相对于100x100的viewBox）
    const x = 50 + radius * Math.cos(centerRad);
    const y = 50 + radius * Math.sin(centerRad);

    return {
        position: 'absolute' as const,
        left: `${x}%`,
        top: `${y}%`,
        transform: 'translate(-50%, -50%)',
        pointerEvents: 'none' as const,
        zIndex: 10,
    };
}

/** 处理部分点击 */
function handlePartClick(index: number) {
    emit('select', index);
}

/** 获取动作徽章 */
function getActionBadge(action: KeyAction): string | null {
    if (!action || action.type === ActionType.NONE) return null;

    switch (action.type) {
        case ActionType.KEYBOARD: return '⌨';
        case ActionType.MOUSE_BTN: return '🖱';
        case ActionType.MOUSE_WHEEL: return '⚙';
        case ActionType.CONSUMER: return '🎵';
        case ActionType.MACRO: return '📝';
        case ActionType.LAYER: return '📚';
        default: return null;
    }
}
</script>

<style scoped>
.encoder-button {
    position: relative;
    width: var(--key-unit);
    height: var(--key-unit);
    border-radius: 50%;
    background: var(--c-key-bg);
    border: 2px solid var(--c-key-border);
    overflow: hidden;
    cursor: pointer;
    transition: all var(--transition-fast);
}

.encoder-button:hover {
    border-color: var(--c-accent);
    box-shadow: 0 6px 20px var(--c-key-shadow);
}

.encoder-button.selected {
    border-color: var(--c-key-active-border);
    box-shadow: 0 0 0 3px var(--c-accent-soft), 0 8px 24px var(--c-key-shadow);
}

.encoder-button.disabled {
    cursor: not-allowed;
    opacity: 0.6;
    pointer-events: none;
}

.encoder-button.disabled:hover {
    border-color: var(--c-key-border);
    box-shadow: none;
}

.encoder-svg {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    z-index: 1;
    pointer-events: none;
}

.encoder-divider {
    stroke: var(--c-key-border);
    opacity: 0.6;
}

.encoder-sector {
    fill: rgba(255, 255, 255, 0.05);
    stroke: none;
    cursor: pointer;
    transition: fill 0.2s ease, opacity 0.2s ease;
    pointer-events: all;
    opacity: 0.8;
}

.encoder-button.disabled .encoder-sector {
    pointer-events: none;
    cursor: not-allowed;
}

.encoder-sector:hover {
    fill: rgba(59, 130, 246, 0.2);
    opacity: 1;
}

.encoder-sector.selected {
    fill: rgba(59, 130, 246, 0.35);
    opacity: 1;
}

.encoder-label-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 3px;
    color: var(--c-text-muted);
    transition: all var(--transition-fast);
    cursor: pointer;
    text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
}

.encoder-label-container.selected {
    color: var(--c-accent);
    text-shadow: 0 1px 3px rgba(59, 130, 246, 0.5);
}

.encoder-label-container:hover {
    color: var(--c-text-primary);
    transform: scale(1.1);
}

.encoder-part-label {
    font-size: 1.4rem;
    font-weight: 700;
    line-height: 1;
    user-select: none;
}

.encoder-part-badge {
    font-size: 0.7rem;
    opacity: 0.9;
    user-select: none;
    font-weight: 600;
}
</style>
