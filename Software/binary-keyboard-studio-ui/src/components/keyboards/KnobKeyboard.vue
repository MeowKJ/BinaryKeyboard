<template>
    <div class="keyboard-container">
        <div class="button-grid">
            <NormalButton label="1" type="square" />
        </div>
    </div>
</template>

<script setup lang="ts">
import Button from 'primevue/button';
import NormalButton from '../buttons/NormalButton.vue';
</script>

<style scoped>
.keyboard-container {
    display: flex;
    justify-content: center;
    align-items: center;
}

.button-grid {
    display: grid;
    grid-template-columns: repeat(3, 8em);
    grid-template-rows: repeat(2, 8em);
    gap: 10px;
}
</style>