/* eslint-env node */
require('@eslint/js')
const tseslint = require('typescript-eslint')
const vuePlugin = require('eslint-plugin-vue')
const vueConfigs = require('eslint-plugin-vue/configs/flat/recommended')

module.exports = tseslint.config(
  {
    ignores: ['dist', 'node_modules', '*.config.*'],
  },
  {
    files: ['**/*.{js,mjs,cjs,ts,vue}'],
    languageOptions: {
      ecmaVersion: 2022,
      sourceType: 'module',
      parserOptions: {
        ecmaVersion: 'latest',
        parser: '@typescript-eslint/parser',
      },
    },
    plugins: {
      vue: vuePlugin,
    },
    rules: {
      ...vueConfigs.rules,
      'vue/multi-word-component-names': 'warn',
      'vue/no-unused-vars': 'warn',
      '@typescript-eslint/no-unused-vars': ['warn', { argsIgnorePattern: '^_' }],
      'no-console': ['warn', { allow: ['warn', 'error'] }],
      'no-debugger': 'warn',
    },
  },
)
