import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const studioRoot = path.resolve(__dirname, '..');
const repoRoot = path.resolve(studioRoot, '..', '..');
const outPath = path.join(studioRoot, 'src', 'generated', 'versionConfig.ts');

const candidates = process.platform === 'win32'
  ? ['python', 'py']
  : ['python3', 'python'];

let lastStatus = null;

for (const command of candidates) {
  const result = spawnSync(command, ['tools/scripts/versioning.py', 'emit-ts', '--out', outPath], {
    cwd: repoRoot,
    stdio: 'inherit',
  });

  if (!result.error && result.status === 0) {
    process.exit(0);
  }

  lastStatus = result;
}

if (lastStatus?.error) {
  throw lastStatus.error;
}

process.exit(lastStatus?.status ?? 1);
