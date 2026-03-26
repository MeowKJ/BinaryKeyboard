import { spawnSync } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const docsRoot = path.resolve(__dirname, "..");
const repoRoot = path.resolve(docsRoot, "..");
const outPath = path.join(docsRoot, "changelog.md");

const candidates =
  process.platform === "win32" ? ["python", "py"] : ["python3", "python"];

let success = false;
for (const command of candidates) {
  const result = spawnSync(
    command,
    [".github/scripts/releases_to_md.py", "--out", outPath],
    {
      cwd: repoRoot,
      stdio: "inherit",
      env: {
        ...process.env,
        PYTHONUTF8: "1",
      },
    },
  );
  if (!result.error && result.status === 0) {
    success = true;
    break;
  }
}

if (!success) {
  console.error("sync-changelog: failed to generate docs/changelog.md");
  process.exit(1);
}
