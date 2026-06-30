import { describe, expect, it } from "vitest";
import {
  phraseGridOriginLeftOffsetPx,
  phraseRowsContentFitScale,
  phraseRowEndAddStepReservePx,
  phraseRowLeadingControlsWidthPx,
  phraseRowScrollPaddingRightPx,
} from "./phraseRowLayout.js";
import { rowGridWidthPx } from "./stepCellLayout.js";

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
    const expectedScale = (
      rowGridWidthPx(unshiftedRow)
      - rowGridWidthPx([1])
    ) / rowGridWidthPx(shiftedRow);

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

  it("keeps the default scale before the grid field has been measured", () => {
    expect(
      phraseRowsContentFitScale([[3]], [3], 0, 0, phraseRowEndAddStepReservePx()),
    ).toBe(1);
  });
});
