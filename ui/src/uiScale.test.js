import { describe, expect, it } from "vitest";
import {
  computeUiScale,
  currentUiScaleMinimumSize,
  normalizeUiScalePercent,
  resolveInitialUiScalePercent,
  setUiScalePercent,
  uiDesignHeightPluginPx,
  uiDesignHeightStandalonePx,
  uiDesignWidthPx,
  uiMinScale,
  uiScaleStorageKey,
} from "./uiScale.svelte.js";

describe("computeUiScale", () => {
  it("returns the selected percentage at the plugin design size", () => {
    setUiScalePercent(100, { persist: false });

    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(1);
  });

  it("returns the selected percentage at the standalone design size", () => {
    setUiScalePercent(90, { persist: false });

    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: uiDesignHeightStandalonePx,
      standaloneTransportAvailable: true,
    })).toBe(0.9);
  });

  it("does not change scale for windows wider than design", () => {
    setUiScalePercent(80, { persist: false });

    expect(computeUiScale({
      widthPx: 2000,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(0.8);
  });

  it("does not scale down for narrow windows", () => {
    setUiScalePercent(100, { persist: false });

    expect(computeUiScale({
      widthPx: 1200,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(1);
  });

  it("does not scale down for short windows", () => {
    setUiScalePercent(50, { persist: false });

    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: 760,
    })).toBe(uiMinScale);
  });

  it("returns scale-specific minimum editor sizes", () => {
    setUiScalePercent(73, { persist: false });

    expect(currentUiScaleMinimumSize()).toEqual({
      widthPx: 1234,
      heightPx: 721,
    });
  });

  it("uses a taller standalone minimum", () => {
    setUiScalePercent(73, { persist: false });

    expect(currentUiScaleMinimumSize({ standaloneTransportAvailable: true })).toEqual({
      widthPx: 1234,
      heightPx: 765,
    });
  });

  it("supports every integer percentage and clamps values to 50–100", () => {
    setUiScalePercent(67, { persist: false });
    expect(computeUiScale({ widthPx: 1, heightPx: 1 })).toBe(0.67);
    expect(normalizeUiScalePercent(49)).toBe(50);
    expect(normalizeUiScalePercent(101)).toBe(100);
    expect(normalizeUiScalePercent(null)).toBe(100);
  });
});

describe("resolveInitialUiScalePercent", () => {
  it("defaults to 100% when no stored preference exists", () => {
    expect(resolveInitialUiScalePercent(70)).toBe(100);
  });

  it("prefers the stored UI scale over project state", () => {
    const storage = new Map([["midiPhrasesUiScalePreset", "85"]]);
    const previous = globalThis.localStorage;

    Object.defineProperty(globalThis, "localStorage", {
      configurable: true,
      value: {
        getItem: (key) => storage.get(key) ?? null,
        setItem: (key, value) => {
          storage.set(key, value);
        },
      },
    });

    try {
      expect(resolveInitialUiScalePercent(70)).toBe(85);
      expect(uiScaleStorageKey).toBe("midiPhrasesUiScalePreset");
    } finally {
      Object.defineProperty(globalThis, "localStorage", {
        configurable: true,
        value: previous,
      });
    }
  });
});
