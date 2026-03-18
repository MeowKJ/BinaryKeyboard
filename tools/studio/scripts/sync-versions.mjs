import path from 'node:path';
import fs from 'node:fs';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const studioRoot = path.resolve(__dirname, '..');
const repoRoot = path.resolve(studioRoot, '..', '..');
const outPath = path.join(studioRoot, 'src', 'generated', 'versionConfig.ts');

const candidates = process.platform === 'win32'
  ? ['python', 'py']
  : ['python3', 'python'];

function runPython(args) {
  for (const command of candidates) {
    const result = spawnSync(command, args, { cwd: repoRoot, encoding: 'utf8' });
    if (!result.error && result.status === 0) {
      return result.stdout.trim();
    }
  }
  return null;
}

// 1. 生成 versionConfig.ts
let emitted = false;
for (const command of candidates) {
  const result = spawnSync(command, ['tools/scripts/versioning.py', 'emit-ts', '--out', outPath], {
    cwd: repoRoot,
    stdio: 'inherit',
  });
  if (!result.error && result.status === 0) {
    emitted = true;
    break;
  }
}
if (!emitted) {
  console.error('sync-versions: failed to emit versionConfig.ts');
  process.exit(1);
}

// 2. 用 studio 版本号同步 package.json version（供 electron-builder 生成带版本的文件名）
const raw = runPython(['tools/scripts/versioning.py', 'show', '--component', 'studio']);
if (raw) {
  try {
    const { version } = JSON.parse(raw);
    const pkgPath = path.join(studioRoot, 'package.json');
    const pkg = JSON.parse(fs.readFileSync(pkgPath, 'utf8'));
    if (pkg.version !== version) {
      pkg.version = version;
      fs.writeFileSync(pkgPath, JSON.stringify(pkg, null, 2) + '\n');
      console.log(`sync-versions: package.json version -> ${version}`);
    }
  } catch (e) {
    console.warn('sync-versions: failed to update package.json version:', e.message);
  }
}
