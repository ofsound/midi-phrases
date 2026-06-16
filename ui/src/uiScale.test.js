import { describe, expect, it } from "vitest";
import {
  computeUiScale,
  uiDesignHeightPluginPx,
  uiDesignHeightStandalonePx,
  uiDesignWidthPx,
  uiMinScale,
} from "./uiScale.svelte.js";

describe("computeUiScale", () => {
  it("returns 1 at the plugin design size", () => {
    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(1);
  });

  it("returns 1 at the standalone design size", () => {
    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: uiDesignHeightStandalonePx,
      standaloneTransportAvailable: true,
    })).toBe(1);
  });

  it("does not scale above 1 for windows wider than design", () => {
    expect(computeUiScale({
      widthPx: 2000,
      heightPx: uiDesignHeightPluginPx,
    })).toBe(1);
  });

  it("scales down for narrow windows", () => {
    expect(computeUiScale({
      widthPx: 1200,
      heightPx: uiDesignHeightPluginPx,
    })).toBeCloseTo(1200 / uiDesignWidthPx);
  });

  it("scales down for short windows", () => {
    expect(computeUiScale({
      widthPx: uiDesignWidthPx,
      heightPx: 760,
    })).toBeCloseTo(760 / uiDesignHeightPluginPx);
  });

  it("uses height for wide-but-short windows", () => {
    expect(computeUiScale({
      widthPx: 2000,
      heightPx: 620,
    })).toBeCloseTo(620 / uiDesignHeightPluginPx);
  });

  it("clamps the minimum scale", () => {
    expect(computeUiScale({
      widthPx: 1000,
      heightPx: 480,
    })).toBe(uiMinScale);
  });
});
