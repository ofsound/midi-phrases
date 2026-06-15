import { rowTimingOffsetShiftPx, stepCellPaddingPx } from "./stepCellLayout.js";

/** Leading inset before the quarter grid (half of PhraseRow’s scroll clip gutter). */
export const phraseRowScrollPaddingLeftPx = 8;

/** Matches PhraseRow scroll container `pt-2` + `pb-2` (16px root). */
export const phraseRowScrollPaddingVerticalPx = 16;

/** Matches PhraseRow scroll container `pt-2`. */
export const phraseRowScrollPaddingTopPx = phraseRowScrollPaddingVerticalPx / 2;

/** Matches PhraseRow scroll container `pb-2`. */
export const phraseRowScrollPaddingBottomPx = phraseRowScrollPaddingVerticalPx / 2;

/** Matches App row gap spacers (`h-3`). */
export const phraseRowInterRowGapPx = 12;

/**
 * Minimum front-face step cell height (header h-5, body, footer h-5, border-2).
 * Keeps zero-step rows the same band height as rows with steps.
 */
export const phraseStepCellMinHeightPx = 92;

/** PhraseRow scroll container min-height including vertical padding. */
export const phraseRowMinHeightPx =
  phraseStepCellMinHeightPx + phraseRowScrollPaddingVerticalPx;

/** Matches App row header `gap-1` between controls and before PhraseRow. */
export const phraseRowHeaderGapPx = 4;

/** Matches row record icon button `h-6 w-6`. */
export const phraseRowActionIconControlWidthPx = 24;

/** Matches row on/off toggle `h-[1.6875rem] w-[1.6875rem]`. */
export const phraseRowMuteControlWidthPx = 27;

/** Matches BipolarKnob `h-6 w-6` in the row header. */
export const phraseRowOffsetKnobWidthPx = 24;

/** Inset from PhraseRow left edge to the row’s quarter-grid origin (offset 0). */
export const phraseGridOriginLeftOffsetPx = phraseRowScrollPaddingLeftPx;

/**
 * Inset from PhraseRow left edge to step 0’s shell when row offset is 0
 * (scroll clip gutter + step 0’s `margin-left` inside the scroll area).
 */
export const phraseFirstStepLeftOffsetPx =
  phraseGridOriginLeftOffsetPx + stepCellPaddingPx;

/** Horizontal gap after the last step cell before the row-end add button (`px-3`). */
export const phraseRowEndAddStepInsetPx = 12;

/**
 * Row header order in App.svelte: mute → record → offset knob → PhraseRow.
 * Trailing edge of the offset knob measured from the row container’s left edge.
 */
export const phraseRowOffsetKnobTrailingEdgePx =
  phraseRowMuteControlWidthPx +
  phraseRowHeaderGapPx +
  phraseRowActionIconControlWidthPx +
  phraseRowHeaderGapPx +
  phraseRowOffsetKnobWidthPx;

/**
 * Fixed width from the row container’s left edge to PhraseRow’s left edge.
 * Includes the trailing `gap-1` after the offset knob.
 */
export const phraseRowLeadingControlsWidthPx =
  phraseRowOffsetKnobTrailingEdgePx + phraseRowHeaderGapPx;

/** Row leading controls after the mute button (record, knob + gaps). */
export const phraseRowLeadingControlsAfterMuteWidthPx =
  phraseRowLeadingControlsWidthPx - phraseRowMuteControlWidthPx;

/** Global X of step 0’s shell when that row’s timing offset is 0 (before compensation). */
export const phraseFirstStepShellGlobalLeftPx =
  phraseRowLeadingControlsWidthPx + phraseFirstStepLeftOffsetPx;

/**
 * Beat guide sits midway between the offset knob and step 0 so spacing is even
 * on both sides of the line (offset from PhraseRow’s left edge).
 */
export const phraseBeatGuideOffsetPx =
  (phraseRowOffsetKnobTrailingEdgePx + phraseFirstStepShellGlobalLeftPx) / 2 -
  phraseRowLeadingControlsWidthPx;

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
  return phraseRowLeadingControlsWidthPx + phraseBeatGuideOffsetPx + visualCompensationPx;
}

/**
 * How far the beat-one guide extends past the top/bottom of step cells.
 * Matches PhraseRow vertical scroll padding (`pt-2` / `pb-2`).
 */
export const phraseBeatGuideVerticalOverhangPx = phraseRowScrollPaddingBottomPx;

/**
 * Top inset for the beat-one guide within the phrase grid column.
 * One overhang above the first row's step cells (below the top inter-row gap).
 */
export const phraseBeatGuideTopPx =
  phraseRowInterRowGapPx +
  phraseRowScrollPaddingTopPx -
  phraseBeatGuideVerticalOverhangPx;

/**
 * Global X for the left edge of step 0’s shell when that row’s timing offset is 0.
 * @param {number} [visualCompensationPx] UI shift from {@link phraseGridVisualOffsetCompensationPx}
 */
export function phraseFirstStepGlobalLeftPx(visualCompensationPx = 0) {
  return phraseFirstStepShellGlobalLeftPx + visualCompensationPx;
}
