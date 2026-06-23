import { describe, expect, it } from "vitest";
import {
  insertionIndexFromCellMidpoints,
  placementIndicatorLeftPx,
} from "./dndUtils.js";

describe("placementIndicatorLeftPx", () => {
  it("centers the indicator in the leading inset", () => {
    expect(placementIndicatorLeftPx([100, 80], 0, 12, 8)).toBe(6);
  });

  it("centers the indicator in a gap between steps", () => {
    expect(placementIndicatorLeftPx([100, 80], 1, 12, 8)).toBe(116);
  });

  it("supports the trailing insertion boundary", () => {
    expect(placementIndicatorLeftPx([100, 80], 2, 12, 8)).toBe(204);
  });
});

describe("insertionIndexFromCellMidpoints", () => {
  const cells = [
    { step: 0, left: 10, width: 100 },
    { step: 1, left: 130, width: 80 },
    { step: 2, left: 230, width: 60 },
  ];

  it("settles before a step on its left half", () => {
    expect(insertionIndexFromCellMidpoints(cells, 40, 3)).toBe(0);
    expect(insertionIndexFromCellMidpoints(cells, 150, 3)).toBe(1);
  });

  it("settles after a step on its right half", () => {
    expect(insertionIndexFromCellMidpoints(cells, 60, 3)).toBe(1);
    expect(insertionIndexFromCellMidpoints(cells, 280, 3)).toBe(3);
  });

  it("treats the midpoint as the right half", () => {
    expect(insertionIndexFromCellMidpoints(cells, 60, 3)).toBe(1);
  });
});
