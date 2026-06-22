const fs = require('fs');
const path = require('path');
const { createRequire } = require('module');
const Module = require('module');
const { spawn } = require('child_process');

function requirePlaywright() {
  try {
    return require('playwright');
  } catch (error) {
    const home = process.env.USERPROFILE || process.env.HOME || '';
    const bundled = path.join(home, '.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules');
    if (!fs.existsSync(bundled)) throw error;
    const playwrightRoot = path.join(bundled, '.pnpm/playwright@1.60.0/node_modules');
    const playwrightCoreRoot = path.join(bundled, '.pnpm/playwright-core@1.60.0/node_modules');
    process.env.NODE_PATH = [bundled, playwrightRoot, playwrightCoreRoot, process.env.NODE_PATH].filter(Boolean).join(path.delimiter);
    Module._initPaths();
    return createRequire(path.join(playwrightRoot, 'playwright/package.json'))('playwright');
  }
}

const { chromium } = requirePlaywright();

const baseUrl = process.argv[2] || 'http://127.0.0.1:5173/';
const outDir = process.argv[3] || 'C:/Users/ijink/Desktop/codex/BinaryKeyboard/tools/studio';
const minScore = Number(process.env.DATAFLASH_SCORE_MIN || 99);

async function canReach(url) {
  try {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 800);
    const response = await fetch(url, { signal: controller.signal });
    clearTimeout(timeout);
    return response.ok || response.status < 500;
  } catch {
    return false;
  }
}

async function waitForUrl(url, timeoutMs = 20000) {
  const start = Date.now();
  while (Date.now() - start < timeoutMs) {
    if (await canReach(url)) return true;
    await new Promise((resolve) => setTimeout(resolve, 350));
  }
  return false;
}

async function ensureDevServer(url) {
  if (await canReach(url)) return null;
  const target = new URL(url);
  if (!['127.0.0.1', 'localhost'].includes(target.hostname)) {
    throw new Error(`Dev server is not reachable: ${url}`);
  }
  const pnpm = process.platform === 'win32' ? 'pnpm.cmd' : 'pnpm';
  const child = spawn(pnpm, ['run', 'dev', '--', '--host', '127.0.0.1', '--port', String(target.port || 5173)], {
    cwd: path.resolve(__dirname, '..'),
    env: { ...process.env, BROWSER: 'none' },
    stdio: 'ignore',
    windowsHide: true,
  });
  const ready = await waitForUrl(url);
  if (!ready) {
    child.kill();
    throw new Error(`Timed out waiting for Vite dev server at ${url}`);
  }
  return child;
}

function withParams(url, params) {
  const target = new URL(url);
  for (const [key, value] of Object.entries(params)) target.searchParams.set(key, value);
  target.searchParams.set('t', String(Date.now()));
  return target.toString();
}

function scoreAll({ overview, zoom, anchor, interaction, perf }) {
  const areaScore = overview.maxRenderedAreaError <= 0.002 ? 18 : Math.max(0, 18 - overview.maxRenderedAreaError * 2400);
  const semanticScore =
    overview.hasOrderedSlots && overview.hasOrderedRegions && overview.hasAddressOrder && !overview.hasMisleadingMeowfs && !overview.hasLockTail ? 18 : 8;
  const zoomScore =
    zoom.byteCells >= 256 && zoom.bitCells >= 8 && zoom.hugeTextCount === 0 ? 15 : 7;
  const anchorScore = anchor.maxError <= 0.05 ? 14 : Math.max(0, 14 - anchor.maxError * 8);
  const interactionScore =
    interaction.wheelMaxError <= 0.35 && interaction.dragDistance > 80 && interaction.selectionStable && interaction.meaningNonBlocking ? 12 : 4;
  const pixiPixelsHealthy =
    perf.pixiPixels.litRatio > 0.02 &&
    perf.pixiPixels.cyanRatio > 0.01 &&
    perf.pixiPixels.redRatio > 0.0005 &&
    perf.pixiPixels.activeLitRatio > 0.08 &&
    perf.pixiPixels.activeRedRatio > 0.02;
  const perfHealthy =
    perf.fps >= 45 && perf.p95 <= 28 && perf.p99 <= 45 && perf.longTasks <= 3 && perf.heapMb < 240 && pixiPixelsHealthy && perf.pixiByteRenderDelta === 0;
  const perfScore =
    perfHealthy ? 16 :
      perf.relativeFps >= 0.85 && perf.longTasks <= 3 && perf.heapMb < 240 && pixiPixelsHealthy && perf.pixiByteRenderDelta <= 1 ? 12 : 5;
  const visualScore =
    overview.hasMaterialPatterns && overview.lowUiRadius && overview.hasDieHardware && overview.hasChargePackets && overview.dieOccupancy > 0.36 && overview.lineDensity >= 8 ? 7 : 2;
  const occlusionPenalty = overview.toastVisible ? 8 : 0;
  return Math.round(areaScore + semanticScore + zoomScore + anchorScore + interactionScore + perfScore + visualScore - occlusionPenalty);
}

async function collectOverview(page) {
  return page.evaluate(() => {
    const explorer = document.querySelector('.die-explorer');
    const audit = JSON.parse(explorer?.getAttribute('data-area-audit') || '[]');
    const bytesByRegion = Object.fromEntries(audit.map((row) => [row.id, row.bytes]));
    const totalBytes = audit.reduce((sum, row) => sum + row.bytes, 0);
    const regionAreas = {};
    for (const rect of document.querySelectorAll('.region-cell > rect')) {
      const regionId = rect.closest('[data-region-id]')?.getAttribute('data-region-id');
      if (!regionId) continue;
      const box = rect.getBoundingClientRect();
      regionAreas[regionId] = (regionAreas[regionId] || 0) + box.width * box.height;
    }
    const totalArea = Object.values(regionAreas).reduce((sum, area) => sum + area, 0);
    const renderedAreaAudit = Object.entries(bytesByRegion).map(([id, bytes]) => {
      const byteRatio = bytes / totalBytes;
      const areaRatio = (regionAreas[id] || 0) / Math.max(1, totalArea);
      return { id, bytes, byteRatio, areaRatio, error: areaRatio - byteRatio };
    });
    const texts = [...document.querySelectorAll('text')].map((el) => {
      const r = el.getBoundingClientRect();
      return { text: (el.textContent || '').trim(), w: r.width, h: r.height };
    });
    const labels = texts.filter((t) => t.text).map((t) => t.text);
    const macroFill = getComputedStyle(document.querySelector('.region-cell[data-region-id="macro"] rect')).fill;
    const reservedFill = getComputedStyle(document.querySelector('.region-cell[data-region-id="reserved"] rect')).fill;
    const hudRadius = parseFloat(getComputedStyle(document.querySelector('.die-hud')).borderTopLeftRadius);
    const dialog = document.querySelector('.storm-dataflash-dialog');
    const dialogRadius = dialog ? parseFloat(getComputedStyle(dialog).borderTopLeftRadius) : 0;
    const orderedRegionLabels = ['CONFIG', 'RUNTIME', 'MeowFS', 'RESERVED'];
    const regionBoxes = Object.fromEntries([...document.querySelectorAll('.region-cell')].map((node) => {
      const id = node.getAttribute('data-region-id');
      const rect = node.querySelector('rect')?.getBoundingClientRect();
      return [id, rect ? { x: rect.x, y: rect.y, width: rect.width, height: rect.height } : null];
    }).filter(([id, rect]) => id && rect));
    const boxes = Object.values(regionBoxes);
    const minX = Math.min(...boxes.map((box) => box.x));
    const minY = Math.min(...boxes.map((box) => box.y));
    const maxX = Math.max(...boxes.map((box) => box.x + box.width));
    const maxY = Math.max(...boxes.map((box) => box.y + box.height));
    const canvasBox = document.querySelector('.die-canvas')?.getBoundingClientRect();
    const dieOccupancy = canvasBox ? ((maxX - minX) * (maxY - minY)) / Math.max(1, canvasBox.width * canvasBox.height) : 0;
    const lineDensity =
      document.querySelectorAll('.pad-ring rect').length / 12 +
      document.querySelectorAll('.decoder-ribs path').length / 3 +
      document.querySelectorAll('.sense-amps rect').length / 3 +
      document.querySelectorAll('.metal-bus, .static-flow-lanes path').length;
    return {
      audit,
      renderedAreaAudit,
      maxRenderedAreaError: Math.max(...renderedAreaAudit.map((row) => Math.abs(row.error))),
      hasOrderedSlots: labels.indexOf('SLOT 0') >= 0 && labels.indexOf('SLOT 0') < labels.indexOf('SLOT 1') && labels.indexOf('SLOT 1') < labels.indexOf('SLOT 2'),
      hasOrderedRegions: orderedRegionLabels.every((label) => labels.includes(label)),
      hasAddressOrder: regionBoxes.config.y <= regionBoxes.runtime.y && regionBoxes.runtime.y <= regionBoxes.macro.y && regionBoxes.macro.y <= regionBoxes.reserved.y && regionBoxes.reserved.y <= regionBoxes.ble.y && regionBoxes.ble.x <= regionBoxes.tail.x,
      hasMisleadingMeowfs: labels.some((label) => /LOG HEAD|TAIL PAGE/.test(label)),
      hasLockTail: labels.some((label) => /LOCK/.test(label)),
      hasMaterialPatterns: macroFill.includes('url') && reservedFill.includes('url'),
      hasDieHardware: document.querySelectorAll('.pad-ring rect').length >= 80 && document.querySelectorAll('.die-seal').length >= 2 && document.querySelectorAll('.sense-amps rect').length >= 12,
      hasChargePackets: document.querySelectorAll('.charge-packets circle').length >= 6 && document.querySelectorAll('.danger-thermal-field rect').length >= 1,
      dieOccupancy,
      lineDensity,
      lowUiRadius: hudRadius <= 4 && dialogRadius <= 4,
      pageCells: document.querySelectorAll('.page-cell').length,
      frames: document.querySelectorAll('.map-frame').length,
      hugeTextCount: texts.filter((t) => t.w > 260 || t.h > 60).length,
      toastVisible: [...document.querySelectorAll('.p-toast')].some((el) => {
        const r = el.getBoundingClientRect();
        const style = getComputedStyle(el);
        return r.width > 0 && r.height > 0 && style.display !== 'none' && style.visibility !== 'hidden';
      }),
      hud: document.querySelector('.die-coordinate-hud')?.textContent?.trim() || '',
      labels: labels.slice(0, 140),
    };
  });
}

async function collectZoom(page) {
  await page.evaluate(async () => {
    await window.stormDataFlashCamera.focusAddress(0x1000, 128, 0);
  });
  await page.waitForTimeout(250);
  return page.evaluate(() => {
    const texts = [...document.querySelectorAll('text')].map((el) => {
      const r = el.getBoundingClientRect();
      return { text: (el.textContent || '').trim(), w: r.width, h: r.height };
    });
    return {
      camera: document.querySelector('.die-explorer')?.getAttribute('data-camera-state'),
      hugeTextCount: texts.filter((t) => t.w > 260 || t.h > 60).length,
      byteCells: document.querySelectorAll('.svg-byte-fallback .byte-cell').length,
      bitCells: document.querySelectorAll('.selected-bit-scope rect').length,
      hud: document.querySelector('.die-coordinate-hud')?.textContent?.trim() || '',
    };
  });
}

async function collectAnchor(page) {
  return page.evaluate(async () => {
    const api = window.stormDataFlashCamera;
    const svg = document.querySelector('.die-canvas');
    const rect = svg.getBoundingClientRect();
    const points = [
      { x: rect.left + rect.width * 0.28, y: rect.top + rect.height * 0.34, scale: 2.7 },
      { x: rect.left + rect.width * 0.63, y: rect.top + rect.height * 0.52, scale: 7.5 },
      { x: rect.left + rect.width * 0.42, y: rect.top + rect.height * 0.72, scale: 1.1 },
    ];
    const errors = [];
    api.reset();
    await new Promise((resolve) => setTimeout(resolve, 80));
    for (const point of points) {
      const before = api.screenToWorld(point.x, point.y);
      api.zoomAtClient(point.x, point.y, point.scale, 0);
      const after = api.screenToWorld(point.x, point.y);
      errors.push(Math.hypot(before.x - after.x, before.y - after.y));
    }
    return { errors, maxError: Math.max(...errors) };
  });
}

async function collectInteraction(page) {
  const svgBox = await page.locator('.die-canvas').boundingBox();
  const point = { x: svgBox.x + svgBox.width * 0.62, y: svgBox.y + svgBox.height * 0.48 };
  const wheelErrors = [];
  await page.evaluate(() => window.stormDataFlashCamera.reset());
  await page.waitForTimeout(700);
  for (const deltaY of [-420, 260, -360]) {
    const before = await page.evaluate(({ x, y }) => window.stormDataFlashCamera.screenToWorld(x, y), point);
    await page.mouse.move(point.x, point.y);
    await page.mouse.wheel(0, deltaY);
    await page.waitForTimeout(80);
    const after = await page.evaluate(({ x, y }) => window.stormDataFlashCamera.screenToWorld(x, y), point);
    wheelErrors.push(Math.hypot(before.x - after.x, before.y - after.y));
  }

  const beforeDrag = await page.evaluate(() => window.stormDataFlashCamera.get());
  await page.mouse.move(svgBox.x + svgBox.width * 0.35, svgBox.y + svgBox.height * 0.44);
  await page.mouse.down({ button: 'right' });
  await page.mouse.move(svgBox.x + svgBox.width * 0.52, svgBox.y + svgBox.height * 0.61, { steps: 8 });
  await page.mouse.up({ button: 'right' });
  await page.waitForTimeout(180);
  const afterDrag = await page.evaluate(() => window.stormDataFlashCamera.get());
  const selectedBeforeClick = await page.evaluate(() => document.querySelector('.die-hud .hud-chip.level')?.textContent?.trim());
  await page.mouse.click(svgBox.x + svgBox.width * 0.52, svgBox.y + svgBox.height * 0.61);
  await page.waitForTimeout(80);
  const selectedAfterClick = await page.evaluate(() => document.querySelector('.die-hud .hud-chip.level')?.textContent?.trim());
  const meaningNonBlocking = await page.evaluate(() => getComputedStyle(document.querySelector('.die-meaning') || document.body).pointerEvents === 'none');

  return {
    wheelErrors,
    wheelMaxError: Math.max(...wheelErrors),
    dragDistance: Math.hypot(afterDrag.tx - beforeDrag.tx, afterDrag.ty - beforeDrag.ty),
    selectionStable: selectedBeforeClick === selectedAfterClick,
    meaningNonBlocking,
  };
}

async function collectPerf(browser) {
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 }, deviceScaleFactor: 1 });
  await page.goto(withParams(baseUrl, { stormDataFlashTest: '1', stormDataFlashPerf: '1' }), { waitUntil: 'domcontentloaded' });
  await page.waitForSelector('.die-explorer', { state: 'visible', timeout: 15000 });
  await page.waitForTimeout(700);
  await page.evaluate(() => window.dispatchEvent(new CustomEvent('storm-dataflash-camera', { detail: { address: 0x1000, scale: 14, duration: 0 } })));
  await page.waitForTimeout(500);
  const perf = await page.evaluate(async () => {
    function percentile(values, pct) {
      const sorted = [...values].sort((a, b) => a - b);
      return sorted[Math.min(sorted.length - 1, Math.max(0, Math.floor(sorted.length * pct)))] || 0;
    }

    async function samplePixiPixels() {
      const canvas = document.querySelector('.pixi-byte-layer canvas');
      if (!canvas) return { litRatio: 0, redRatio: 0, cyanRatio: 0, activeLitRatio: 0, activeRedRatio: 0, width: 0, height: 0 };
      const url = canvas.toDataURL('image/png');
      const image = new Image();
      image.src = url;
      await image.decode();
      const probe = document.createElement('canvas');
      probe.width = Math.min(320, image.width);
      probe.height = Math.min(180, image.height);
      const ctx = probe.getContext('2d', { willReadFrequently: true });
      ctx.drawImage(image, 0, 0, probe.width, probe.height);
      const data = ctx.getImageData(0, 0, probe.width, probe.height).data;
      let lit = 0;
      let red = 0;
      let cyan = 0;
      function classifyAt(i) {
        const r = data[i];
        const g = data[i + 1];
        const b = data[i + 2];
        const a = data[i + 3];
        return {
          lit: a > 18 && r + g + b > 34,
          red: a > 18 && r > g * 1.15 && r > b * 1.15,
          cyan: a > 18 && b > r * 1.05 && g > r * 1.05,
        };
      }
      for (let i = 0; i < data.length; i += 4) {
        const px = classifyAt(i);
        if (px.lit) lit++;
        if (px.red) red++;
        if (px.cyan) cyan++;
      }
      const total = data.length / 4;
      const api = window.stormDataFlashCamera;
      const stats = api?.pixiStats?.();
      const camera = api?.get?.();
      let activeLit = 0;
      let activeRed = 0;
      let activeTotal = 0;
      if (stats?.activeByte && camera) {
        const fit = Math.min(image.width / 1000, image.height / 1000);
        const ox = (image.width - 1000 * fit) / 2;
        const oy = (image.height - 1000 * fit) / 2;
        const sx = probe.width / image.width;
        const sy = probe.height / image.height;
        const left = Math.max(0, Math.floor((ox + (camera.tx + stats.activeByte.x * camera.scale) * fit) * sx));
        const top = Math.max(0, Math.floor((oy + (camera.ty + stats.activeByte.y * camera.scale) * fit) * sy));
        const right = Math.min(probe.width, Math.ceil((ox + (camera.tx + (stats.activeByte.x + stats.activeByte.width) * camera.scale) * fit) * sx));
        const bottom = Math.min(probe.height, Math.ceil((oy + (camera.ty + (stats.activeByte.y + stats.activeByte.height) * camera.scale) * fit) * sy));
        for (let y = top; y < bottom; y++) {
          for (let x = left; x < right; x++) {
            const px = classifyAt((y * probe.width + x) * 4);
            activeTotal++;
            if (px.lit) activeLit++;
            if (px.red) activeRed++;
          }
        }
      }
      return {
        litRatio: lit / total,
        redRatio: red / total,
        cyanRatio: cyan / total,
        activeLitRatio: activeTotal ? activeLit / activeTotal : 0,
        activeRedRatio: activeTotal ? activeRed / activeTotal : 0,
        width: image.width,
        height: image.height,
      };
    }

    async function sampleFrames(durationMs, animate) {
      const frames = [];
      let last = performance.now();
      const end = last + durationMs;
      function frame(now) {
        frames.push(now - last);
        last = now;
        if (animate) animate(now);
        if (now < end) requestAnimationFrame(frame);
      }
      requestAnimationFrame(frame);
      await new Promise((resolve) => setTimeout(resolve, durationMs + 120));
      const avg = frames.reduce((sum, value) => sum + value, 0) / Math.max(1, frames.length);
      return {
        fps: avg > 0 ? 1000 / avg : 0,
        frameCount: frames.length,
        p95: percentile(frames, 0.95),
        p99: percentile(frames, 0.99),
        dropped: frames.filter((frame) => frame > 34).length,
      };
    }

    const heapStart = performance.memory ? performance.memory.usedJSHeapSize / 1024 / 1024 : 0;
    const pixiPixels = await samplePixiPixels();
    const pixiStatsBefore = window.stormDataFlashCamera?.pixiStats?.() || { byteRenders: -1, weatherRenders: -1, pixiPresent: false };
    const longTasks = [];
    let observer;
    if ('PerformanceObserver' in window) {
      try {
        observer = new PerformanceObserver((list) => longTasks.push(...list.getEntries()));
        observer.observe({ entryTypes: ['longtask'] });
      } catch {}
    }
    const baseline = await sampleFrames(1200);
    const moving = await sampleFrames(3600, (now) => {
        window.dispatchEvent(new CustomEvent('storm-dataflash-camera', {
          detail: { raw: true, tx: -1420 + Math.sin(now / 120) * 120, ty: -4850 + Math.cos(now / 160) * 120, scale: 14, duration: 0 },
        }));
    });
    observer?.disconnect?.();
    const pixiStatsAfter = window.stormDataFlashCamera?.pixiStats?.() || { byteRenders: -1, weatherRenders: -1, pixiPresent: false };
    const longTaskTotalMs = longTasks.reduce((sum, task) => sum + task.duration, 0);
    const heapEnd = performance.memory ? performance.memory.usedJSHeapSize / 1024 / 1024 : heapStart;
    return {
      baselineFps: baseline.fps,
      fps: moving.fps,
      relativeFps: baseline.fps > 0 ? moving.fps / baseline.fps : 0,
      frameCount: moving.frameCount,
      p95: moving.p95,
      p99: moving.p99,
      dropped: moving.dropped,
      longTasks: longTasks.length,
      longTaskTotalMs,
      heapMb: heapEnd,
      heapDeltaMb: heapEnd - heapStart,
      pixiPixels,
      pixiStatsBefore,
      pixiStatsAfter,
      pixiByteRenderDelta: pixiStatsAfter.byteRenders - pixiStatsBefore.byteRenders,
      pixiHost: !!document.querySelector('.pixi-byte-layer'),
      pixiPresent: !!document.querySelector('.pixi-byte-layer canvas'),
    };
  });
  await page.close();
  return perf;
}

(async () => {
  let devServer = null;
  let browser = null;
  try {
  devServer = await ensureDevServer(baseUrl);
  browser = await chromium.launch({
    headless: true,
    args: [
      '--disable-background-timer-throttling',
      '--disable-backgrounding-occluded-windows',
      '--disable-renderer-backgrounding',
      '--disable-frame-rate-limit',
    ],
  });
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 }, deviceScaleFactor: 1 });
  await page.goto(withParams(baseUrl, { stormDataFlashTest: '1' }), { waitUntil: 'domcontentloaded' });
  await page.waitForSelector('.die-explorer', { state: 'visible', timeout: 15000 });
  await page.waitForTimeout(700);

  const overview = await collectOverview(page);
  await page.screenshot({ path: `${outDir}/dataflash-atlas-overview-v8.png` });
  const zoom = await collectZoom(page);
  await page.screenshot({ path: `${outDir}/dataflash-atlas-zoom-v8.png` });
  const anchor = await collectAnchor(page);
  const interaction = await collectInteraction(page);
  const perf = await collectPerf(browser);
  await browser.close();
  browser = null;

  const score = scoreAll({ overview, zoom, anchor, interaction, perf });
  console.log(JSON.stringify({ overview, zoom, anchor, interaction, perf, score, minScore }, null, 2));
  if (score < minScore) process.exitCode = 1;
  } finally {
    await browser?.close?.().catch?.(() => {});
    devServer?.kill();
  }
})().catch((error) => {
  console.error(error);
  process.exit(1);
});
