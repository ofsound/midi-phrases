import { describe, expect, it } from "vitest";
import {
  applyStepFlipChange,
  clearFlipOverrides,
  closeAllStepFlips,
  flipOverrideSyncToken,
  isStepFlipped,
  shouldResetFlipOverrides,
  syncFlipOverridesForGlobalView,
} from "./stepFlipState.js";

const fingerprint = "a|b|c";
const stepCount = 3;

describe("stepFlipState", () => {
  it("shows all fronts when global back-view is off", () => {
    const overrides = new Set();

    expect(isStepFlipped(false, overrides, fingerprint, 0)).toBe(false);
    expect(isStepFlipped(false, overrides, fingerprint, 1)).toBe(false);
    expect(isStepFlipped(false, overrides, fingerprint, 2)).toBe(false);
  });

  it("shows all backs when global back-view is on", () => {
    const overrides = new Set();

    expect(isStepFlipped(true, overrides, fingerprint, 0)).toBe(true);
    expect(isStepFlipped(true, overrides, fingerprint, 1)).toBe(true);
    expect(isStepFlipped(true, overrides, fingerprint, 2)).toBe(true);
  });

  it("tracks per-step back flips while global back-view is off", () => {
    const overrides = new Set();

    applyStepFlipChange(overrides, false, fingerprint, 1, true, stepCount);

    expect(isStepFlipped(false, overrides, fingerprint, 0)).toBe(false);
    expect(isStepFlipped(false, overrides, fingerprint, 1)).toBe(true);
    expect(isStepFlipped(false, overrides, fingerprint, 2)).toBe(false);
  });

  it("tracks per-step front flips while global back-view is on", () => {
    const overrides = new Set();

    applyStepFlipChange(overrides, true, fingerprint, 2, false, stepCount);

    expect(isStepFlipped(true, overrides, fingerprint, 0)).toBe(true);
    expect(isStepFlipped(true, overrides, fingerprint, 1)).toBe(true);
    expect(isStepFlipped(true, overrides, fingerprint, 2)).toBe(false);
  });

  it("clears stale overrides when leaving global back-view so every step returns to front", () => {
    const overrides = new Set();

    applyStepFlipChange(overrides, false, fingerprint, 1, true, stepCount);
    expect(isStepFlipped(false, overrides, fingerprint, 1)).toBe(true);

    syncFlipOverridesForGlobalView(overrides, 1, fingerprint, 0, fingerprint);
    expect(isStepFlipped(true, overrides, fingerprint, 1)).toBe(true);

    syncFlipOverridesForGlobalView(overrides, 2, fingerprint, 1, fingerprint);
    expect(isStepFlipped(false, overrides, fingerprint, 0)).toBe(false);
    expect(isStepFlipped(false, overrides, fingerprint, 1)).toBe(false);
    expect(isStepFlipped(false, overrides, fingerprint, 2)).toBe(false);
  });

  it("clears overrides when entering global back-view so previously flipped steps do not stay on back", () => {
    const overrides = new Set();

    applyStepFlipChange(overrides, false, fingerprint, 0, true, stepCount);
    applyStepFlipChange(overrides, false, fingerprint, 2, true, stepCount);

    syncFlipOverridesForGlobalView(overrides, 1, fingerprint, 0, fingerprint);

    expect(isStepFlipped(true, overrides, fingerprint, 0)).toBe(true);
    expect(isStepFlipped(true, overrides, fingerprint, 1)).toBe(true);
    expect(isStepFlipped(true, overrides, fingerprint, 2)).toBe(true);
  });

  it("resets overrides when the step fingerprint changes", () => {
    const overrides = new Set();

    applyStepFlipChange(overrides, false, fingerprint, 1, true, stepCount);

    const nextFingerprint = "a|b|c|d";
    syncFlipOverridesForGlobalView(overrides, 0, nextFingerprint, 0, fingerprint);

    expect(isStepFlipped(false, overrides, nextFingerprint, 1)).toBe(false);
  });

  it("closes all flips to the front while global back-view is on", () => {
    const overrides = new Set();

    closeAllStepFlips(overrides, true, fingerprint, stepCount);

    expect(isStepFlipped(true, overrides, fingerprint, 0)).toBe(false);
    expect(isStepFlipped(true, overrides, fingerprint, 1)).toBe(false);
    expect(isStepFlipped(true, overrides, fingerprint, 2)).toBe(false);
  });

  it("closes all flips while global back-view is off", () => {
    const overrides = new Set();

    applyStepFlipChange(overrides, false, fingerprint, 0, true, stepCount);
    applyStepFlipChange(overrides, false, fingerprint, 2, true, stepCount);
    closeAllStepFlips(overrides, false, fingerprint, stepCount);

    expect(overrides.size).toBe(0);
    expect(isStepFlipped(false, overrides, fingerprint, 0)).toBe(false);
    expect(isStepFlipped(false, overrides, fingerprint, 2)).toBe(false);
  });

  it("does not reset when sync token is unchanged", () => {
    const overrides = new Set();

    applyStepFlipChange(overrides, false, fingerprint, 1, true, stepCount);
    const token = flipOverrideSyncToken(0, fingerprint);

    expect(shouldResetFlipOverrides(0, fingerprint, 0, fingerprint)).toBe(false);
    expect(token).toBe("0:a|b|c");

    syncFlipOverridesForGlobalView(overrides, 0, fingerprint, 0, fingerprint);
    expect(isStepFlipped(false, overrides, fingerprint, 1)).toBe(true);
  });

  it("clearFlipOverrides removes every stored key", () => {
    const overrides = new Set(["front:a|b:c:0", "back:a|b:c:1"]);

    clearFlipOverrides(overrides);

    expect(overrides.size).toBe(0);
  });
});
