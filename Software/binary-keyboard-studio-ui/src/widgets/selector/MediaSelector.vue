<template>
    <div class="card flex justify-center">
        <Select v-model="selected" :options="mediaOptions" optionLabel="label" class="w-full md:w-56" />
    </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from "vue";
import { type MediaConfig, MediaHIDCode } from "@/types";

interface MediaOption {
    value: MediaHIDCode;
    label: string;
}

const selected = computed({
    get: () => {
        return {
            value: mediaConfig.value.key,
            label: mediaDescriptions[mediaConfig.value.key],
        };
    },
    set: (value: MediaOption) => {
        mediaConfig.value.key = value.value;
    },
});
const props = defineProps<{ mediaConfig: MediaConfig }>();
const emit = defineEmits(["update:mediaConfig"]);

const mediaConfig = computed({
    get: () => props.mediaConfig,
    set: (value) => {
        console.log(value);
        emit("update:mediaConfig", value);

    },
});


// 说明映射
const mediaDescriptions: Record<MediaHIDCode, string> = {
    [MediaHIDCode.None]: "无",
    [MediaHIDCode.Play]: "播放",
    [MediaHIDCode.Pause]: "暂停",
    [MediaHIDCode.PlayPause]: "播放/暂停",
    [MediaHIDCode.Stop]: "停止",
    [MediaHIDCode.NextTrack]: "下一曲",
    [MediaHIDCode.PrevTrack]: "上一曲",
    [MediaHIDCode.FastForward]: "快进",
    [MediaHIDCode.Rewind]: "倒带",
    [MediaHIDCode.Mute]: "静音",
    [MediaHIDCode.VolumeUp]: "音量增加",
    [MediaHIDCode.VolumeDown]: "音量减少",
    [MediaHIDCode.ChannelUp]: "频道增加",
    [MediaHIDCode.ChannelDown]: "频道减少",
    [MediaHIDCode.Power]: "设备电源",
    [MediaHIDCode.Sleep]: "休眠",
    [MediaHIDCode.WakeUp]: "唤醒",
    [MediaHIDCode.Menu]: "进入菜单",
    [MediaHIDCode.Home]: "主页",
    [MediaHIDCode.Back]: "返回",
    [MediaHIDCode.Exit]: "退出",
    [MediaHIDCode.Select]: "确认/选择",
    [MediaHIDCode.Up]: "向上",
    [MediaHIDCode.Down]: "向下",
    [MediaHIDCode.Left]: "向左",
    [MediaHIDCode.Right]: "向右",
    [MediaHIDCode.Calculator]: "计算器",
    [MediaHIDCode.FileExplorer]: "文件资源管理器",
    [MediaHIDCode.BrowserForward]: "浏览器前进",
    [MediaHIDCode.BrowserRefresh]: "浏览器刷新",
    [MediaHIDCode.BrowserFavorites]: "浏览器收藏夹",
    [MediaHIDCode.Email]: "电子邮件应用",

};

const mediaOptions = Object.entries(mediaDescriptions).map(([key, label]) => ({
    value: Number(key),
    label,
}));

</script>