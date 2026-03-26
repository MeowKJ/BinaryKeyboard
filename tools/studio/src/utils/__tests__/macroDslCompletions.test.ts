/**
 * MeowMacro DSL Completions Test Suite
 *
 * Tests the auto-completion system to ensure:
 * 1. Each input state produces correct, non-redundant suggestions
 * 2. Deprecated suggestions (e.g. mouse down) are NOT shown
 * 3. Step-by-step flow: command → operand → suffix → value
 */

import { describe, it, expect } from "vitest";
import { getMacroDslCompletions, compileMacroDsl, type MacroDslCompletionItem } from "@/utils/macroDsl";
import { MacroActionType } from "@/types/protocol";

// ─── helpers ───────────────────────────────────────────────────────────────

/** Get completions at end of input string (simulates typing) */
function completionsAt(input: string): MacroDslCompletionItem[] {
  return getMacroDslCompletions(input, input.length);
}

/** Get just the labels from completions */
function labelsAt(input: string): string[] {
  return completionsAt(input).map((c) => c.label);
}

/** Check whether a specific label appears in completions */
function hasLabel(input: string, label: string): boolean {
  return labelsAt(input).includes(label);
}

/** Check whether ANY label containing `text` appears */
function hasLabelContaining(input: string, text: string): boolean {
  return labelsAt(input).some((l) => l.toLowerCase().includes(text.toLowerCase()));
}

/** Kinds of all completions */
function kindsAt(input: string): string[] {
  return completionsAt(input).map((c) => c.kind);
}

// ─── 1. Command-level completions ──────────────────────────────────────────

describe("command-level completions (empty / partial keyword)", () => {
  it("typing 't' should show 'tap' keyword", () => {
    const labels = labelsAt("t");
    expect(labels).toContain("tap");
  });

  it("typing 'd' should show 'delay' and 'down'", () => {
    const labels = labelsAt("d");
    expect(labels).toContain("delay");
    expect(labels).toContain("down");
  });

  it("typing 'w' should show 'wheel'", () => {
    expect(hasLabel("w", "wheel")).toBe(true);
  });

  it("typing 'c' should show 'consumer'", () => {
    expect(hasLabel("c", "consumer")).toBe(true);
  });

  it("typing 'u' should show 'up'", () => {
    expect(hasLabel("u", "up")).toBe(true);
  });

  it("typing 'm' should return no keyword matches (mouse is deprecated)", () => {
    const keywordItems = completionsAt("m").filter((c) => c.kind === "keyword");
    expect(keywordItems).toHaveLength(0);
  });

  it("snippet 'tap mouse:left' should appear when typing 'tap m'", () => {
    expect(hasLabel("tap m", "mouse:left")).toBe(true);
  });
});

// ─── 2. Deprecated items should NOT appear ─────────────────────────────────

describe("no deprecated suggestions", () => {
  it("should NOT have 'down mouse:left' in command snippets", () => {
    // At command level, typing 'd'
    expect(hasLabel("d", "down mouse:left")).toBe(false);
  });

  it("should NOT have 'up mouse:left' in command snippets", () => {
    expect(hasLabel("u", "up mouse:left")).toBe(false);
  });

  it("should NOT have 'down Ctrl+A' in command snippets", () => {
    expect(hasLabel("d", "down Ctrl+A")).toBe(false);
  });

  it("should NOT have 'up Ctrl+A' in command snippets", () => {
    expect(hasLabel("u", "up Ctrl+A")).toBe(false);
  });

  it("legacy 'mouse ' should NOT suggest 'down' sub-command", () => {
    const labels = labelsAt("mouse ");
    expect(labels).not.toContain("down");
    expect(labels).not.toContain("up");
  });

  it("legacy 'mouse ' should still suggest button names for compat", () => {
    const labels = labelsAt("mouse ");
    expect(labels).toContain("left");
    expect(labels).toContain("right");
  });
});

// ─── 3. tap command flow ───────────────────────────────────────────────────

describe("tap operand completions — step-by-step flow", () => {
  it("'tap ' should suggest key names and modifiers", () => {
    const items = completionsAt("tap ");
    expect(items.length).toBeGreaterThan(0);
    // Should include modifiers
    expect(hasLabel("tap ", "Ctrl")).toBe(true);
    expect(hasLabel("tap ", "Shift")).toBe(true);
    // Should include keys
    expect(hasLabel("tap ", "A")).toBe(true);
    // Should include mouse:left
    expect(hasLabel("tap ", "mouse:left")).toBe(true);
  });

  it("'tap A' (no trailing space) should suggest keys starting with A", () => {
    const labels = labelsAt("tap A");
    expect(labels).toContain("A");
    // Alt is a modifier that starts with A
    expect(labels).toContain("Alt");
  });

  it("'tap A ' (trailing space) should suggest hold/wait suffixes", () => {
    const labels = labelsAt("tap A ");
    expect(labels).toContain("hold 50ms");
    expect(labels).toContain("wait 100ms");
  });

  it("'tap Ctrl+C ' should suggest hold/wait", () => {
    const labels = labelsAt("tap Ctrl+C ");
    expect(labels).toContain("hold 50ms");
    expect(labels).toContain("wait 100ms");
  });

  it("'tap A h' should suggest 'hold 50ms'", () => {
    const labels = labelsAt("tap A h");
    expect(labels).toContain("hold 50ms");
  });

  it("'tap A w' should suggest 'wait 100ms'", () => {
    const labels = labelsAt("tap A w");
    expect(labels).toContain("wait 100ms");
  });

  it("'tap A hold ' should suggest time values", () => {
    const labels = labelsAt("tap A hold ");
    expect(labels).toContain("50ms");
    expect(labels).toContain("100ms");
    expect(labels).toContain("1s");
  });

  it("'tap A wait ' should suggest time values", () => {
    const labels = labelsAt("tap A wait ");
    expect(labels).toContain("50ms");
    expect(labels).toContain("100ms");
    expect(labels).toContain("1s");
  });

  it("'tap A hold 50ms ' should suggest wait (no duplicate hold)", () => {
    const labels = labelsAt("tap A hold 50ms ");
    expect(labels).toContain("wait 100ms");
    // hold already used, should not appear again
    expect(labels).not.toContain("hold 50ms");
  });

  it("'tap A * ' should still work after repeat (prefix matching)", () => {
    // After `tap A * 2 `, should suggest wait
    const labels = labelsAt("tap A * 2 ");
    expect(labels).toContain("wait 100ms");
  });

  it("'tap Ctrl+' should suggest keys after modifier", () => {
    const items = completionsAt("tap Ctrl+");
    expect(items.length).toBeGreaterThan(0);
    expect(hasLabel("tap Ctrl+", "C")).toBe(true);
    expect(hasLabel("tap Ctrl+", "A")).toBe(true);
  });

  it("'tap Shift+-' should not show duplicate minus suggestions", () => {
    const labels = labelsAt("tap Shift+-");
    expect(labels.filter((label) => label === "-")).toHaveLength(1);
  });
});

// ─── 4. down/up operand completions ────────────────────────────────────────

describe("down/up operand completions", () => {
  it("'down ' should suggest keys, modifiers, and mouse:xxx", () => {
    const items = completionsAt("down ");
    expect(items.length).toBeGreaterThan(0);
    expect(hasLabel("down ", "Ctrl")).toBe(true);
    expect(hasLabel("down ", "Shift")).toBe(true);
    expect(hasLabel("down ", "A")).toBe(true);
    expect(hasLabel("down ", "mouse:left")).toBe(true);
  });

  it("'up ' should suggest similar items as down", () => {
    const items = completionsAt("up ");
    expect(items.length).toBeGreaterThan(0);
    expect(hasLabel("up ", "mouse:left")).toBe(true);
  });

  it("'down m' should suggest mouse:xxx and modifier keys starting with m", () => {
    const labels = labelsAt("down m");
    expect(labels.some((l) => l.startsWith("mouse:"))).toBe(true);
  });
});

// ─── 5. delay completions ──────────────────────────────────────────────────

describe("delay completions", () => {
  it("'delay ' should suggest time values", () => {
    const labels = labelsAt("delay ");
    expect(labels).toContain("50ms");
    expect(labels).toContain("100ms");
    expect(labels).toContain("500ms");
  });

  it("'delay 5' should filter to '50ms' and '500ms'", () => {
    const labels = labelsAt("delay 5");
    expect(labels).toContain("50ms");
    expect(labels).toContain("500ms");
    expect(labels).not.toContain("100ms");
  });
});

// ─── 6. wheel completions ──────────────────────────────────────────────────

describe("wheel completions", () => {
  it("'wheel ' should suggest up and down", () => {
    const labels = labelsAt("wheel ");
    expect(labels).toContain("up");
    expect(labels).toContain("down");
  });

  it("'wheel u' should suggest 'up'", () => {
    const labels = labelsAt("wheel u");
    expect(labels).toContain("up");
    expect(labels).not.toContain("down");
  });
});

// ─── 7. consumer completions ───────────────────────────────────────────────

describe("consumer completions", () => {
  it("'consumer ' should suggest media keys", () => {
    const labels = labelsAt("consumer ");
    expect(labels.length).toBeGreaterThan(0);
    expect(labels).toContain("volume_up");
    expect(labels).toContain("play_pause");
  });

  it("'consumer v' should filter to volume-related keys", () => {
    const labels = labelsAt("consumer v");
    expect(labels).toContain("volume_up");
    expect(labels).toContain("volume_down");
    expect(labels).not.toContain("play_pause");
  });
});

// ─── 8. Edge cases ─────────────────────────────────────────────────────────

describe("edge cases", () => {
  it("empty string should return empty completions", () => {
    expect(completionsAt("")).toHaveLength(0);
  });

  it("comment line should return empty completions", () => {
    expect(completionsAt("# this is a comment")).toHaveLength(0);
  });

  it("second line completions should work independently", () => {
    const source = "tap A\n";
    const labels = getMacroDslCompletions(source, source.length).map((c) => c.label);
    // Empty second line, should return nothing (empty after trimming)
    expect(labels).toHaveLength(0);
  });

  it("second line with partial input should complete", () => {
    const source = "tap A\nd";
    const labels = getMacroDslCompletions(source, source.length).map((c) => c.label);
    expect(labels).toContain("delay");
    expect(labels).toContain("down");
  });

  it("insertText should replace from correct range", () => {
    const items = completionsAt("tap ");
    for (const item of items) {
      expect(item.replaceFrom).toBeLessThanOrEqual(item.replaceTo);
      expect(item.replaceFrom).toBeGreaterThanOrEqual(0);
    }
  });
});

// ─── 9. Snippet quality checks ─────────────────────────────────────────────

describe("snippet suggestions quality", () => {
  it("snippets at 't' should include useful templates", () => {
    const labels = labelsAt("t");
    expect(labels).toContain("tap Ctrl+C");
    expect(labels).toContain("tap A hold 50ms wait 100ms");
    expect(labels).toContain("tap A * 2 wait 50ms");
    expect(labels).toContain("tap mouse:left");
  });

  it("no duplicate concepts in snippets", () => {
    const items = completionsAt("t").filter((c) => c.kind === "snippet");
    const labels = items.map((c) => c.label);
    // Should not have both 'tap Ctrl+A' and 'tap Ctrl+C' — just one combo example
    const ctrlSnippets = labels.filter((l) => /^tap Ctrl\+[A-Z]$/.test(l));
    expect(ctrlSnippets.length).toBeLessThanOrEqual(1);
  });

  it("'delay' related snippet exists for 'd'", () => {
    expect(hasLabel("d", "delay 50ms")).toBe(true);
  });
});

// ─── 10. Completions count / noise check ────────────────────────────────────

describe("completion count reasonableness", () => {
  it("command-level should have ≤ 15 items (keywords + snippets)", () => {
    // Should not overwhelm with too many choices
    const items = completionsAt("t");
    expect(items.length).toBeLessThanOrEqual(15);
  });

  it("'tap A ' should have ≤ 3 suffix suggestions", () => {
    const items = completionsAt("tap A ");
    // hold, wait — at most these 2
    expect(items.length).toBeLessThanOrEqual(3);
  });

  it("hold/wait time values should have ≤ 6 options", () => {
    const holdItems = completionsAt("tap A hold ");
    expect(holdItems.length).toBeLessThanOrEqual(6);
    const waitItems = completionsAt("tap A wait ");
    expect(waitItems.length).toBeLessThanOrEqual(6);
  });
});

// ─── tap mouse:xxx 编译正确性 ──────────────────────────────────────────────

describe("tap mouse:xxx compiles correctly", () => {
  it("tap mouse:left produces MOUSE_DOWN + MOUSE_UP actions", () => {
    const { actions, diagnostics } = compileMacroDsl("tap mouse:left");
    expect(diagnostics).toHaveLength(0);
    expect(actions[0]).toMatchObject({ type: MacroActionType.MOUSE_DOWN, param: 0x01 });
    expect(actions[1]).toMatchObject({ type: MacroActionType.MOUSE_UP, param: 0x01 });
  });

  it("tap mouse:right hold 50ms produces MOUSE_DOWN + DELAY + MOUSE_UP", () => {
    const { actions, diagnostics } = compileMacroDsl("tap mouse:right hold 50ms");
    expect(diagnostics).toHaveLength(0);
    expect(actions[0]).toMatchObject({ type: MacroActionType.MOUSE_DOWN, param: 0x02 });
    expect(actions[1]).toMatchObject({ type: MacroActionType.DELAY, param: 5 });
    expect(actions[2]).toMatchObject({ type: MacroActionType.MOUSE_UP, param: 0x02 });
  });

  it("tap mouse:left * 2 produces two click pairs", () => {
    const { actions, diagnostics } = compileMacroDsl("tap mouse:left * 2");
    expect(diagnostics).toHaveLength(0);
    const types = actions.slice(0, 4).map((a) => a.type);
    expect(types).toEqual([
      MacroActionType.MOUSE_DOWN,
      MacroActionType.MOUSE_UP,
      MacroActionType.MOUSE_DOWN,
      MacroActionType.MOUSE_UP,
    ]);
  });
});

describe("symbol key parsing", () => {
  it("tap - compiles to the minus key", () => {
    const { actions, diagnostics } = compileMacroDsl("tap -");
    expect(diagnostics).toHaveLength(0);
    expect(actions[0]).toMatchObject({ type: MacroActionType.KEY_DOWN, param: 0x2d });
    expect(actions[1]).toMatchObject({ type: MacroActionType.KEY_UP, param: 0x2d });
  });

  it("tap Shift+- hold 50ms wait 20ms keeps minus as a key", () => {
    const { actions, diagnostics } = compileMacroDsl("tap Shift+- hold 50ms wait 20ms");
    expect(diagnostics).toHaveLength(0);
    expect(actions[0]).toMatchObject({ type: MacroActionType.MOD_DOWN, param: 0x02 });
    expect(actions[1]).toMatchObject({ type: MacroActionType.KEY_DOWN, param: 0x2d });
    expect(actions[2]).toMatchObject({ type: MacroActionType.DELAY, param: 5 });
    expect(actions[3]).toMatchObject({ type: MacroActionType.KEY_UP, param: 0x2d });
    expect(actions[4]).toMatchObject({ type: MacroActionType.MOD_UP, param: 0x02 });
    expect(actions[5]).toMatchObject({ type: MacroActionType.DELAY, param: 2 });
  });

  it("tap NumPlus compiles to keypad plus", () => {
    const { actions, diagnostics } = compileMacroDsl("tap NumPlus");
    expect(diagnostics).toHaveLength(0);
    expect(actions[0]).toMatchObject({ type: MacroActionType.KEY_DOWN, param: 0x57 });
    expect(actions[1]).toMatchObject({ type: MacroActionType.KEY_UP, param: 0x57 });
  });
});
