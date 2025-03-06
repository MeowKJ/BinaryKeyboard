<template>

    <Button :label="label" :class="getClassName()" severity="secondary" @click="seletorVisible = true" raised />
</template>

<script setup lang="ts">

import { ref } from "vue";

const seletorVisible = ref(false);
const props = defineProps<{
    label: string;
    type: "circle" | "square" | "vertical-bar" | "horizontal-bar";
    index: number
}>();

import KeySelector from "@/widgets/KeySelector.vue";

const getClassName = () => {
    switch (props.type) {
        case "circle":
            return "round-button";
        case "square":
            return "square-button";
        case "vertical-bar":
            return "tall-button";
        case "horizontal-bar":
            return "wide-button";
    }
};
</script>

<style scoped>
.square-button,
.round-button,
.tall-button {
    font-size: 1.5em;
}

.round-button {
    border-radius: 50%;
}

.tall-button {
    grid-row: span 2;
}

.wide-button {
    grid-column: span 2;
}
</style>