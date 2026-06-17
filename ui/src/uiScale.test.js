import { describe, expect, it } from "vitest";
import {
  computeUiScale,
  currentUiScaleMinimumSize,
  setUiScalePreset,
  uiDesignHeightPluginPx,
  uiDesignHeightStandalonePx,
  uiDesignWidthPx,
  uiMinScale,
} from "./uiScale.svelte.js";

describe("computeUiScale", () => {
  it("returns the selected preset at the plugin design size", () => {
    setUiScalePreset("100", { persist: false });

    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(1);
  });

  it("returns the selected preset at the standalone design size", () => {
    setUiScalePreset("90", { persist: false });

    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: uiDesignHeightStandalonePx,
      standaloneTransportAvailable: true,
    })).toBe(0.9);
  });

  it("does not change scale for windows wider than design", () => {
    setUiScalePreset("80", { persist: false });

    expect(computeUiScale({
      widthPx: 2000,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(0.8);
  });

  it("does not scale down for narrow windows", () => {
    setUiScalePreset("100", { persist: false });

    expect(computeUiScale({
      widthPx: 1200,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(1);
  });

  it("does not scale down for short windows", () => {
    setUiScalePreset("70", { persist: false });

    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: 760,
    })).toBe(uiMinScale);
  });

  it("returns scale-specific minimum editor sizes", () => {
    setUiScalePreset("90", { persist: false });

    expect(currentUiScaleMinimumSize()).toEqual({
      widthPx: 1350,
      heightPx: 790,
    });
  });

  it("uses a taller standalone minimum", () => {
    setUiScalePreset("90", { persist: false });

    expect(currentUiScaleMinimumSize({ standaloneTransportAvailable: true })).toEqual({
      widthPx: 1350,
      heightPx: 836,
    });
  });
});
