import { rowTimingOffsetShiftPx, stepCellPaddingPx } from "./stepCellLayout.js";
import { scaledPx } from "./uiScale.svelte.js";

/** Leading inset before the quarter grid (half of PhraseRow’s scroll clip gutter). */
export const basePhraseRowScrollPaddingLeftPx = 8;

/** Matches PhraseRow scroll container `pt-2` + `pb-2` (16px root). */
export const basePhraseRowScrollPaddingVerticalPx = 16;

/** Matches PhraseRow scroll container `pt-2`. */
export function phraseRowScrollPaddingLeftPx() {
  return scaledPx(basePhraseRowScrollPaddingLeftPx);
}

export function phraseRowScrollPaddingVerticalPx() {
  return scaledPx(basePhraseRowScrollPaddingVerticalPx);
}

/** Matches PhraseRow scroll container `pt-2`. */
export function phraseRowScrollPaddingTopPx() {
  return phraseRowScrollPaddingVerticalPx() / 2;
}

/** Matches PhraseRow scroll container `pb-2`. */
export function phraseRowScrollPaddingBottomPx() {
  return phraseRowScrollPaddingVerticalPx() / 2;
}

/** Matches PhraseRow scroll container `pr-2`. */
export function phraseRowScrollPaddingRightPx() {
  return phraseRowScrollPaddingVerticalPx() / 2;
}

/** Matches App row gap spacers (`h-2`). */
export const basePhraseRowInterRowGapPx = 8;

export function phraseRowInterRowGapPx() {
  return scaledPx(basePhraseRowInterRowGapPx);
}

/** Matches App phrase-grid top/bottom marquee zones (`h-6`). */
export const basePhraseGridOuterPaddingPx = 24;

export function phraseGridOuterPaddingPx() {
  return scaledPx(basePhraseGridOuterPaddingPx);
}

/**
 * Minimum front-face step cell height (header h-5, body, footer h-5, border-2).
 * Keeps zero-step rows the same band height as rows with steps.
 */
export const basePhraseStepCellMinHeightPx = 88;

export function phraseStepCellMinHeightPx() {
  return scaledPx(basePhraseStepCellMinHeightPx);
}

/** PhraseRow scroll container min-height including vertical padding. */
export function phraseRowMinHeightPx() {
  return phraseStepCellMinHeightPx() + phraseRowScrollPaddingVerticalPx();
}

/** Matches App row header `gap-1` between controls and before PhraseRow. */
export const basePhraseRowHeaderGapPx = 4;

export function phraseRowHeaderGapPx() {
  return scaledPx(basePhraseRowHeaderGapPx);
}

/** Matches row record icon button `h-6 w-6`. */
export const basePhraseRowActionIconControlWidthPx = 24;

export function phraseRowActionIconControlWidthPx() {
  return scaledPx(basePhraseRowActionIconControlWidthPx);
}

/** Matches row on/off toggle `h-[1.6875rem] w-[1.6875rem]`. */
export const basePhraseRowMuteControlWidthPx = 27;

export function phraseRowMuteControlWidthPx() {
  return scaledPx(basePhraseRowMuteControlWidthPx);
}

/** Matches BipolarKnob `h-6 w-6` in the row header. */
export const basePhraseRowOffsetKnobWidthPx = 24;

export function phraseRowOffsetKnobWidthPx() {
  return scaledPx(basePhraseRowOffsetKnobWidthPx);
}

/** Inset from PhraseRow left edge to the row’s quarter-grid origin (offset 0). */
export function phraseGridOriginLeftOffsetPx() {
  return phraseRowScrollPaddingLeftPx();
}

/**
 * Inset from PhraseRow left edge to step 0’s shell when row offset is 0
 * (scroll clip gutter + step 0’s `margin-left` inside the scroll area).
 */
export function phraseFirstStepLeftOffsetPx() {
  return phraseGridOriginLeftOffsetPx() + stepCellPaddingPx();
}

/** Horizontal gap from the last step shell to the row-end add button (one insert-zone width). */
export const basePhraseRowEndAddStepInsetPx = 16;

/** Matches PlusDragButton `variant="large"` (`size-[54px]`). */
export const basePhraseRowEndAddStepButtonWidthPx = 54;

export function phraseRowEndAddStepInsetPx() {
  return scaledPx(basePhraseRowEndAddStepInsetPx);
}

export function phraseRowEndAddStepButtonWidthPx() {
  return scaledPx(basePhraseRowEndAddStepButtonWidthPx);
}

/** Right inset after the last step shell before the row-end + (⅔ of {@link stepCellPaddingPx}). */
export function phraseRowEndStepTailPaddingPx() {
  return stepCellPaddingPx() * (2 / 3);
}

/** Matches row header `pl-6 pr-6` (inset around mute / record / offset controls). */
export const basePhraseRowHeaderHorizontalPaddingPx = 48;

export function phraseRowHeaderHorizontalPaddingPx() {
  return scaledPx(basePhraseRowHeaderHorizontalPaddingPx);
}

/** Inset plus trailing add button width reserved after the last step. */
export function phraseRowEndAddStepReservePx() {
  return (
    phraseRowEndStepTailPaddingPx() +
    phraseRowEndAddStepInsetPx() +
    phraseRowEndAddStepButtonWidthPx()
  );
}

/**
 * Row header order in App.svelte: mute → record → offset knob → PhraseRow.
 * Trailing edge of the offset knob measured from the row container’s left edge.
 */
export function phraseRowOffsetKnobTrailingEdgePx() {
  return (
    phraseRowMuteControlWidthPx() +
    phraseRowHeaderGapPx() +
    phraseRowActionIconControlWidthPx() +
    phraseRowHeaderGapPx() +
    phraseRowOffsetKnobWidthPx()
  );
}

/**
 * Fixed width from the row container’s left edge to PhraseRow’s left edge.
 * Includes row header `pl-6 pr-6` and the trailing `gap-1` after the offset knob.
 */
export function phraseRowLeadingControlsWidthPx() {
  return (
    phraseRowHeaderHorizontalPaddingPx() +
    phraseRowOffsetKnobTrailingEdgePx() +
    phraseRowHeaderGapPx()
  );
}

/** Row leading controls after the mute button (record, knob + gaps). */
export function phraseRowLeadingControlsAfterMuteWidthPx() {
  return phraseRowLeadingControlsWidthPx() - phraseRowMuteControlWidthPx();
}

/** Global X of step 0’s shell when that row’s timing offset is 0 (before compensation). */
export function phraseFirstStepShellGlobalLeftPx() {
  return phraseRowLeadingControlsWidthPx() + phraseFirstStepLeftOffsetPx();
}

/**
 * Beat guide sits midway between the offset knob and step 0 so spacing is even
 * on both sides of the line (offset from PhraseRow’s left edge).
 */
export function phraseBeatGuideOffsetPx() {
  return (
    (phraseRowOffsetKnobTrailingEdgePx() + phraseFirstStepShellGlobalLeftPx()) / 2 -
    phraseRowLeadingControlsWidthPx()
  );
}

/**
 * UI-only horizontal shift for the phrase grid and beat guide.
 * When any row has a negative timing offset, returns a positive px value so the
 * furthest-left row lines up with the default offset-0 layout; the beat-one guide
 * moves right by the same amount. Does not affect scheduling or native state.
 *
 * @param {number[]} rowTimingOffsetIndices
 */
export function phraseGridVisualOffsetCompensationPx(rowTimingOffsetIndices) {
  let minShiftPx = 0;

  for (const offsetIndex of rowTimingOffsetIndices) {
    const shiftPx = rowTimingOffsetShiftPx(offsetIndex);

    if (shiftPx < minShiftPx) {
      minShiftPx = shiftPx;
    }
  }

  return -minShiftPx;
}

/**
 * Global X for the beat-one guide (even gap from offset knob and step 0).
 * @param {number} [visualCompensationPx] UI shift from {@link phraseGridVisualOffsetCompensationPx}
 */
export function phraseBeatGuideGlobalLeftPx(visualCompensationPx = 0) {
  return phraseRowLeadingControlsWidthPx() + phraseBeatGuideOffsetPx() + visualCompensationPx;
}

/**
 * How far the beat-one guide extends past the top/bottom of step cells.
 * Matches PhraseRow vertical scroll padding (`pt-2` / `pb-2`).
 */
export function phraseBeatGuideVerticalOverhangPx() {
  return phraseRowScrollPaddingBottomPx();
}

/**
 * Top inset for the beat-one guide within the phrase grid column.
 * One overhang above the first row's step cells (below the top inter-row gap).
 */
export function phraseBeatGuideTopPx() {
  return (
    phraseGridOuterPaddingPx() +
    phraseRowScrollPaddingTopPx() -
    phraseBeatGuideVerticalOverhangPx()
  );
}

/**
 * Global X for the left edge of step 0’s shell when that row’s timing offset is 0.
 * @param {number} [visualCompensationPx] UI shift from {@link phraseGridVisualOffsetCompensationPx}
 */
export function phraseFirstStepGlobalLeftPx(visualCompensationPx = 0) {
  return phraseFirstStepShellGlobalLeftPx() + visualCompensationPx;
}
