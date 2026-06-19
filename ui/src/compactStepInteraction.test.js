import { describe, expect, it } from "vitest";
import {
  compactPreviewStepOrder,
  compactStepMoveDominates,
  compactTimingMultipliersDuringMove,
  previewCompactStepBoundaryResize,
} from "./compactStepInteraction.js";

describe("compactStepMoveDominates", () => {
  it("prefers horizontal movement over vertical", () => {
    expect(compactStepMoveDominates(8, 2)).toBe(true);
    expect(compactStepMoveDominates(2, 8)).toBe(false);
  });
});

describe("compactPreviewStepOrder", () => {
  it("returns a reordered step index list during move preview", () => {
    expect(compactPreviewStepOrder(4, { mode: "move", step: 1, targetStep: 3 })).toEqual([
      0, 2, 3, 1,
    ]);
  });

  it("returns null when the target step matches the source", () => {
    expect(compactPreviewStepOrder(4, { mode: "move", step: 1, targetStep: 1 })).toBeNull();
  });
});

describe("compactTimingMultipliersDuringMove", () => {
  it("previews preceding-step resize without mutating the source array", () => {
    const source = [3, 7, 3];
    const preview = compactTimingMultipliersDuringMove(
      {
        mode: "move",
        previewPreviousStep: 0,
        previewPreviousMultiplierIndex: 2,
        originalPreviousMultiplierIndex: 3,
      },
      source,
    );

    expect(preview).toEqual([2, 7, 3]);
    expect(source).toEqual([3, 7, 3]);
  });
});

describe("previewCompactStepBoundaryResize", () => {
  it("snaps boundary resize gestures to timing-multiplier steps", () => {
    expect(previewCompactStepBoundaryResize(3, 0, 100, 1)).toBe(3);
    expect(previewCompactStepBoundaryResize(3, 13, 100, 1)).toBe(4);
  });
});
