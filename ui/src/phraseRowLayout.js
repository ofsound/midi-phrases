import { rowTimingOffsetShiftPx, stepInsertZoneWidthPx } from "./stepCellLayout.js";

/** Matches PhraseRow scroll container `pl-2`. */
export const phraseRowScrollPaddingLeftPx = 8;

/** Inset from PhraseRow left edge to the left edge of step 0 when row offset is 0. */
export const phraseFirstStepLeftOffsetPx =
  phraseRowScrollPaddingLeftPx + stepInsertZoneWidthPx;

/**
 * Vertical beat guide: centered in the inter-step gap (leading insert zone before step 0).
 * Sits stepInsertZoneWidthPx / 2 left of the first step edges when all offsets are 0.
 */
export const phraseBeatGuideOffsetPx =
  phraseFirstStepLeftOffsetPx - stepInsertZoneWidthPx / 2;

/** Matches row reverse toggle `min-w-[3.75rem]` + horizontal padding. */
export const phraseRowReverseControlWidthPx = 60;

/** Matches row on/off toggle `h-10 w-10`. */
export const phraseRowMuteControlWidthPx = 40;

/** Matches App row header `gap-1` between controls and before PhraseRow. */
export const phraseRowHeaderGapPx = 4;

/** MIDI channel slot in App (`w-10`). */
export const phraseRowMidiChannelWidthPx = 40;

/** Matches BipolarKnob `h-9 w-9` in the row header. */
export const phraseRowOffsetKnobWidthPx = 36;

/**
 * Fixed width from the row container’s left edge to PhraseRow’s left edge.
 * Must stay in sync with App.svelte row header controls (includes trailing gap-1).
 */
export const phraseRowLeadingControlsWidthPx =
  phraseRowMuteControlWidthPx +
  phraseRowHeaderGapPx +
  phraseRowMidiChannelWidthPx +
  phraseRowHeaderGapPx +
  phraseRowOffsetKnobWidthPx +
  phraseRowHeaderGapPx +
  phraseRowReverseControlWidthPx +
  phraseRowHeaderGapPx;

/**
 * UI-only horizontal shift for the phrase grid and beat guide.
 * When any row has a negative timing offset, returns a positive px value so the
 * furthest-left row lines up with the default offset-0 layout; the beat-one guide
 * moves right by the same amount. Does not affect scheduling or native state.
 *
 * @param {number[]} rowTimingOffsetIndices
 * @param {number} [pulseIndex]
 */
export function phraseGridVisualOffsetCompensationPx(rowTimingOffsetIndices, pulseIndex) {
  let minShiftPx = 0;

  for (const offsetIndex of rowTimingOffsetIndices) {
    const shiftPx = rowTimingOffsetShiftPx(offsetIndex, pulseIndex);

    if (shiftPx < minShiftPx) {
      minShiftPx = shiftPx;
    }
  }

  return -minShiftPx;
}

/**
 * Global X for the beat-one guide (center of the leading inter-step gap).
 * @param {number} [visualCompensationPx] UI shift from {@link phraseGridVisualOffsetCompensationPx}
 */
export function phraseBeatGuideGlobalLeftPx(visualCompensationPx = 0) {
  return phraseRowLeadingControlsWidthPx + phraseBeatGuideOffsetPx + visualCompensationPx;
}
