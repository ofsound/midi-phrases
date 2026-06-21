import { describe, expect, it } from "vitest";
import {
  compactStepShellPaddingPercent,
  compactStepShellTrailingPaddingPercent,
  compactPhraseGridLayout,
  longestRowQuarterGridColumns,
  phraseRowsScrollContentWidthPx,
  quarterGridColumnsForMultiplierIndex,
  stepCellPaddingPx,
  stepCellQuarterGridWidthPx,
  stepDisplayWidthPx,
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

describe("compact stretch-to-fit shell spacing", () => {
  it("matches full-size shell insets for a 1x step", () => {
    const multiplierIndex = 3;
    const columns = quarterGridColumnsForMultiplierIndex(multiplierIndex);
    const spanPx = columns * stepCellQuarterGridWidthPx();
    const paddingPx = stepCellPaddingPx();

    expect(compactStepShellPaddingPercent(multiplierIndex)).toBeCloseTo(
      (paddingPx / spanPx) * 100,
    );
    expect(100 - compactStepShellPaddingPercent(multiplierIndex) * 2).toBeCloseTo(
      (stepDisplayWidthPx(multiplierIndex) / spanPx) * 100,
    );
  });

  it("uses the shorter trailing inset on the last step shell", () => {
    const multiplierIndex = 1;
    const spanPx =
      quarterGridColumnsForMultiplierIndex(multiplierIndex) * stepCellQuarterGridWidthPx();

    expect(compactStepShellTrailingPaddingPercent(multiplierIndex)).toBeCloseTo(
      ((stepCellPaddingPx() * 2) / 3 / spanPx) * 100,
    );
  });
});

describe("phraseRowsScrollContentWidthPx", () => {
  it("matches the shared compact grid span in px", () => {
    const rows = [
      [4, 3, 2],
      [0, 3, 5, 7],
      [4],
      [],
    ];
    const offsets = [3, 4, 3, 3];
    const layout = compactPhraseGridLayout(rows, offsets);

    expect(phraseRowsScrollContentWidthPx(rows, offsets)).toBe(
      layout.totalColumns * stepCellQuarterGridWidthPx(),
    );
  });
});

describe("compactPhraseGridLayout", () => {
  it("uses one scale for row spans and positive timing offsets", () => {
    const layout = compactPhraseGridLayout(
      [
        [4, 3, 2],
        [0, 3, 5, 7],
        [4],
        [],
      ],
      [3, 4, 3, 3],
    );
    expect(layout.rowStartColumns).toEqual([0, 1, 0, 0]);
    expect(layout.totalColumns).toBe(20);
  });

  it("normalizes negative offsets without changing relative alignment", () => {
    const layout = compactPhraseGridLayout([[3], [3]], [0, 3]);
    expect(layout.rowStartColumns).toEqual([0, 3]);
    expect(layout.totalColumns).toBe(7);
  });
});
