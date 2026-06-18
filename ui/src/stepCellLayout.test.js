import { describe, expect, it } from "vitest";
import {
  longestRowQuarterGridColumns,
  quarterGridColumnsForMultiplierIndex,
} from "./stepCellLayout.js";

describe("longestRowQuarterGridColumns", () => {
  it("uses the proportional timing span of the longest row", () => {
    const rows = [
      [0, 0, 0],
      [3, 3],
      [1],
    ];

    expect(longestRowQuarterGridColumns(rows)).toBe(8);
  });

  it("keeps an empty grid valid for CSS layout", () => {
    expect(longestRowQuarterGridColumns([[], [], [], []])).toBe(1);
  });

  it("normalizes invalid multiplier indices consistently", () => {
    expect(longestRowQuarterGridColumns([[Number.NaN]])).toBe(
      quarterGridColumnsForMultiplierIndex(Number.NaN),
    );
  });
});
