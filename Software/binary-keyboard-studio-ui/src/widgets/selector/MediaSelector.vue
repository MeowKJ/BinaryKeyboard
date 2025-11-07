<template>
    <div class="card flex justify-center">
        <Select v-model="selected" :options="mediaOptions" optionLabel="label" class="w-full md:w-56" />
    </div>
</template>

<script setup lang="ts">
import { computed, ref, watch } from "vue";
import { type MediaConfig, MediaHIDCode } from "@/types";
import { mediaDescriptions } from "@/utils/hidConverters/mediaHIDConverter";

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


const mediaOptions = Object.entries(mediaDescriptions).map(([key, label]) => ({
    value: Number(key),
    label,
}));

</script>