import { describe, expect, it } from "vitest";
import {
  clampTimingMultiplierDelta,
  clampTimingMultiplierValue,
  compactPhraseGridLayout,
  compactStepShellPaddingPercent,
  compactStepShellTrailingPaddingPercent,
  formatSignedTimingMultiplierDelta,
  longestRowQuarterGridColumns,
  phraseRowsScrollContentWidthPx,
  quarterGridColumnsForMultiplierIndex,
  stepCellPaddingPx,
  stepCellQuarterGridWidthPx,
  stepDisplayWidthPx,
  timingMultiplierIndexForValue,
  timingMultiplierIndicesInRange,
  defaultRowTimingOffsetIndex,
  rowTimingOffsetIndexForQuarters,
} from "./stepCellLayout.js";

describe("timing multiplier bulk length helpers", () => {
  it("clamps applied values to the 0.25 grid", () => {
    expect(clampTimingMultiplierValue(0.6)).toBe(0.5);
    expect(timingMultiplierIndexForValue(0.6)).toBe(1);
  });

  it("blocks negative delta when a step is already at min", () => {
    expect(clampTimingMultiplierDelta(-0.25, [0.25, 1])).toBe(0);
  });

  it("allows shrinking steps above min", () => {
    expect(clampTimingMultiplierDelta(-0.5, [1, 2])).toBe(-0.5);
  });

  it("formats signed quarter-step deltas", () => {
    expect(formatSignedTimingMultiplierDelta(0)).toBe("0");
    expect(formatSignedTimingMultiplierDelta(0.25)).toBe("+.25");
    expect(formatSignedTimingMultiplierDelta(-0.5)).toBe("-.5");
  });
});

describe("timingMultiplierIndicesInRange", () => {
  it("returns every quarter-step index between min and max inclusive", () => {
    expect(timingMultiplierIndicesInRange(3, 7)).toEqual([3, 4, 5, 6, 7]);
    expect(timingMultiplierIndicesInRange(7, 3)).toEqual([3, 4, 5, 6, 7]);
  });

  it("returns a single index when min and max match", () => {
    expect(timingMultiplierIndicesInRange(5, 5)).toEqual([5]);
  });
});

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

describe("0.25x step text fit benchmark", () => {
  it("preserves enough full-step face budget for D#4 127 at 100% UI scale", () => {
    const quarterStepShellWidthPx = stepDisplayWidthPx(0);
    const cellBorderX = 4; // border-2 on both sides.
    const bodyPaddingX = 8; // px-1 on both sides.
    const noteVelocityGap = 4; // gap-1 between note and velocity labels.
    const dSharpOctaveLabelWidth = 24;
    const maxVelocityLabelWidth = 18;

    expect(
      quarterStepShellWidthPx - cellBorderX - bodyPaddingX - noteVelocityGap,
    ).toBeGreaterThanOrEqual(dSharpOctaveLabelWidth + maxVelocityLabelWidth);
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
      [defaultRowTimingOffsetIndex, rowTimingOffsetIndexForQuarters(0.25), defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
    );
    expect(layout.rowStartColumns).toEqual([0, 1, 0, 0]);
    expect(layout.totalColumns).toBe(20);
  });

  it("normalizes negative offsets without changing relative alignment", () => {
    const layout = compactPhraseGridLayout([[3], [3]], [rowTimingOffsetIndexForQuarters(-0.75), defaultRowTimingOffsetIndex]);
    expect(layout.rowStartColumns).toEqual([0, 3]);
    expect(layout.totalColumns).toBe(7);
  });
});
