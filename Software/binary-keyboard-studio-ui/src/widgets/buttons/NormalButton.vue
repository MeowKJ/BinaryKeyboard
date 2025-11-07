<template>
    <Button :label="label" :class="[getClassName(), { 'active-button': index === currentIndex }]" severity="secondary"
        raised />
</template>

<script setup lang="ts">
const props = defineProps<{
    label: string;
    type: "circle" | "square" | "vertical-bar" | "horizontal-bar";
    index: number;
    currentIndex: number;
}>();

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
.tall-button,
.wide-button {
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

.active-button {
    border: 3px solid var(--p-button-outlined-primary-border-color);
}

.active-button:hover {
    border: 3px solid var(--p-button-outlined-primary-border-color) !important;
}
</style>
