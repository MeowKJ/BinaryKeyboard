/**
 * Vercel 一体化构建脚本
 *
 * 产物布局：
 *   dist/           ← docs (VitePress, base = /)
 *   dist/studio/    ← studio (Vite SPA, base = /studio/)
 */

import { execSync } from 'node:child_process';
import { cpSync, mkdirSync, rmSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const run = (cmd, cwd, opts = {}) => execSync(cmd, { cwd, stdio: 'inherit', ...opts });

// ── 清理 ──────────────────────────────────────────────────────────────────
rmSync(resolve(root, 'dist'), { recursive: true, force: true });
mkdirSync(resolve(root, 'dist/studio'), { recursive: true });

// ── 构建 Docs（base = /，覆盖 config 里的 /BinaryKeyboard/）─────────────
const docsDir = resolve(root, 'docs');
run('pnpm install --frozen-lockfile', docsDir);
run('pnpm exec vitepress build --base /', docsDir);
cpSync(resolve(docsDir, '.vitepress/dist'), resolve(root, 'dist'), { recursive: true });

// ── 构建 Studio（base = /studio/）────────────────────────────────────────
const studioDir = resolve(root, 'tools/studio');
run('pnpm install --frozen-lockfile', studioDir);
run('pnpm run build-only', studioDir, { env: { ...process.env, VITE_BASE_URL: '/studio/' } });
cpSync(resolve(studioDir, 'dist'), resolve(root, 'dist/studio'), { recursive: true });

console.log('✓ Vercel build complete → dist/');
