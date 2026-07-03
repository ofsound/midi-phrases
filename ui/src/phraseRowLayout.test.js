import { describe, expect, it } from "vitest";
import {
  phraseGridBandHeightPx,
  phraseGridOriginLeftOffsetPx,
  phraseRowEndAddStepOverlayLeftPx,
  phraseRowsContentFitScale,
  phraseRowEndAddStepButtonWidthPx,
  phraseRowEndAddStepInsetPx,
  phraseRowEndAddStepReservePx,
  phraseRowEndStepTailPaddingPx,
  phraseRowLeadingControlsWidthPx,
  phraseRowScrollPaddingRightPx,
  seedModePanelMinHeightPx,
} from "./phraseRowLayout.js";
import {
  defaultRowTimingOffsetIndex,
  rowGridWidthPx,
  rowTimingOffsetShiftPx,
  stepCellPaddingPx,
  stepInsertZoneWidthPx,
  rowTimingOffsetIndexForQuarters,
} from "./stepCellLayout.js";

function phraseGridFieldWidthForContent(contentWidthPx) {
  return (
    phraseRowLeadingControlsWidthPx()
    + phraseGridOriginLeftOffsetPx()
    + phraseRowScrollPaddingRightPx()
    + phraseRowEndAddStepReservePx()
    + contentWidthPx
  );
}

describe("phrase grid band height", () => {
  it("matches the seed panel so opening seed mode does not resize the top content band", () => {
    expect(phraseGridBandHeightPx()).toBeGreaterThanOrEqual(seedModePanelMinHeightPx());
  });
});

describe("phraseRowsContentFitScale", () => {
  it("does not shrink a long unshifted row because another row has a positive timing offset", () => {
    const longUnshiftedRow = [6, 0, 2, 3, 3]; // 1.75, .25, .75, 1, 1 = 19 columns.
    const shorterShiftedRow = [1, 5, 3]; // .5, 1.5, 1 = 12 columns.
    const fieldWidth = phraseGridFieldWidthForContent(rowGridWidthPx(longUnshiftedRow));

    expect(
      phraseRowsContentFitScale(
        [longUnshiftedRow, [], [], shorterShiftedRow],
        [3, 3, 3, 5],
        fieldWidth,
        0,
        phraseRowEndAddStepReservePx(),
      ),
    ).toBe(1);
  });

  it("shrinks when a shifted row itself exceeds its available width", () => {
    const unshiftedRow = [6, 0, 2, 3, 3];
    const shiftedRow = [6, 6, 3]; // 1.75, 1.75, 1 = 18 columns, shifted by 2 columns.
    const fieldWidth = phraseGridFieldWidthForContent(rowGridWidthPx(unshiftedRow));
    const fixedAddControlWidth =
      phraseRowEndAddStepInsetPx() + phraseRowEndAddStepButtonWidthPx();
    const expectedScale = (
      rowGridWidthPx(unshiftedRow)
      + phraseRowEndStepTailPaddingPx()
    ) / (
      rowGridWidthPx(shiftedRow)
      + rowTimingOffsetShiftPx(rowTimingOffsetIndexForQuarters(0.5))
      - stepCellPaddingPx()
      + phraseRowEndStepTailPaddingPx()
    );

    expect(fixedAddControlWidth).toBe(
      phraseRowEndAddStepReservePx() - phraseRowEndStepTailPaddingPx(),
    );
    expect(
      phraseRowsContentFitScale(
        [unshiftedRow, shiftedRow],
        [defaultRowTimingOffsetIndex, rowTimingOffsetIndexForQuarters(0.5)],
        fieldWidth,
        0,
        phraseRowEndAddStepReservePx(),
      ),
    ).toBeCloseTo(expectedScale);
  });

  it("scales offset and row-end tail space instead of reserving them at full size", () => {
    const row = [6, 6, 6, 6];
    const offsetIndex = rowTimingOffsetIndexForQuarters(0.5);
    const desiredScale = 0.5;
    const fixedAddControlWidth =
      phraseRowEndAddStepInsetPx() + phraseRowEndAddStepButtonWidthPx();
    const scalableWidth =
      rowGridWidthPx(row)
      + rowTimingOffsetShiftPx(offsetIndex)
      - stepCellPaddingPx()
      + phraseRowEndStepTailPaddingPx();
    const fieldWidth =
      phraseRowLeadingControlsWidthPx()
      + phraseGridOriginLeftOffsetPx()
      + phraseRowScrollPaddingRightPx()
      + fixedAddControlWidth
      + scalableWidth * desiredScale;

    expect(
      phraseRowsContentFitScale(
        [row],
        [offsetIndex],
        fieldWidth,
        0,
        phraseRowEndAddStepReservePx(),
      ),
    ).toBeCloseTo(desiredScale);
  });

  it("keeps the default scale before the grid field has been measured", () => {
    expect(
      phraseRowsContentFitScale([[3]], [defaultRowTimingOffsetIndex], 0, 0, phraseRowEndAddStepReservePx()),
    ).toBe(1);
  });
});

describe("phraseRowEndAddStepOverlayLeftPx", () => {
  it("anchors the plus after the final rendered step", () => {
    expect(
      phraseRowEndAddStepOverlayLeftPx([
        { cellWidth: 100, gapBefore: false },
        { cellWidth: 50, gapBefore: true },
        { cellWidth: 150, gapBefore: true },
      ]),
    ).toBeCloseTo(
      stepCellPaddingPx()
      + 100
      + stepInsertZoneWidthPx()
      + 50
      + stepInsertZoneWidthPx()
      + 150
      + phraseRowEndStepTailPaddingPx(),
    );
  });

  it("ignores collapsed drag preview cells before the final rendered step", () => {
    expect(
      phraseRowEndAddStepOverlayLeftPx([
        { cellWidth: 100, gapBefore: false },
        { cellWidth: 0, gapBefore: true },
        { cellWidth: 150, gapBefore: true },
      ]),
    ).toBeCloseTo(
      stepCellPaddingPx()
      + 100
      + stepInsertZoneWidthPx()
      + 150
      + phraseRowEndStepTailPaddingPx(),
    );
  });

  it("places the first-step plus at the leading empty-row position", () => {
    expect(phraseRowEndAddStepOverlayLeftPx([])).toBeCloseTo(
      stepCellPaddingPx() - phraseRowEndAddStepInsetPx(),
    );
  });
});
