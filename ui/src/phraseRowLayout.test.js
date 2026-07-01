import { describe, expect, it } from "vitest";
import {
  phraseGridOriginLeftOffsetPx,
  phraseRowsContentFitScale,
  phraseRowEndAddStepButtonWidthPx,
  phraseRowEndAddStepInsetPx,
  phraseRowEndAddStepReservePx,
  phraseRowEndStepTailPaddingPx,
  phraseRowLeadingControlsWidthPx,
  phraseRowScrollPaddingRightPx,
} from "./phraseRowLayout.js";
import { rowGridWidthPx, rowTimingOffsetShiftPx, stepCellPaddingPx } from "./stepCellLayout.js";

function phraseGridFieldWidthForContent(contentWidthPx) {
  return (
    phraseRowLeadingControlsWidthPx()
    + phraseGridOriginLeftOffsetPx()
    + phraseRowScrollPaddingRightPx()
    + phraseRowEndAddStepReservePx()
    + contentWidthPx
  );
}

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
      + rowTimingOffsetShiftPx(5)
      - stepCellPaddingPx()
      + phraseRowEndStepTailPaddingPx()
    );

    expect(fixedAddControlWidth).toBe(
      phraseRowEndAddStepReservePx() - phraseRowEndStepTailPaddingPx(),
    );
    expect(
      phraseRowsContentFitScale(
        [unshiftedRow, shiftedRow],
        [3, 5],
        fieldWidth,
        0,
        phraseRowEndAddStepReservePx(),
      ),
    ).toBeCloseTo(expectedScale);
  });

  it("scales offset and row-end tail space instead of reserving them at full size", () => {
    const row = [6, 6, 6, 6];
    const offsetIndex = 5;
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
      phraseRowsContentFitScale([[3]], [3], 0, 0, phraseRowEndAddStepReservePx()),
    ).toBe(1);
  });
});
