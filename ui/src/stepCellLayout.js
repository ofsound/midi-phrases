
export const stepTimingMultiplierQuarterStep = 0.25;
export const stepTimingMultiplierMin = 0.25;
export const stepTimingMultiplierMax = 4;

/** Index 0 = 0.25× … index 15 = 4× in 0.25 steps. */
export const stepTimingMultiplierCount = Math.round(
  (stepTimingMultiplierMax - stepTimingMultiplierMin) / stepTimingMultiplierQuarterStep,
) + 1;

export const timingMultiplierValues = Array.from(
  { length: stepTimingMultiplierCount },
  (_, index) => stepTimingMultiplierMin + index * stepTimingMultiplierQuarterStep,
);

/** Default index for 1× step length. */
export const defaultStepTimingMultiplierIndex = timingMultiplierValues.indexOf(1);

/** Upper bound for timing multiplier chosen via + drag-insert (0.25×–2×). */
export const insertStepTimingMultiplierMax = 2;

/** Matches PluginProcessor::maxPhraseStepsPerRow. */
export const maxPhraseStepsPerRow = 64;

/**
 * Width of one invisible 0.25× grid unit.
 * Sized so a 0.25× shell (N×W − 2P) fits “G#4 127” with note + velocity.
 */
export const stepCellQuarterGridWidthPx = 89;

/** Width of the insert divider control between cells (centered in the inter-step gap). */
export const stepInsertZoneWidthPx = 16;

/** Left edge for an insert slot centered on a quarter-grid boundary. */
export function insertSlotLeftPxAtGridBoundaryPx(boundaryPx) {
  return boundaryPx - stepInsertZoneWidthPx / 2;
}

/** Padding from a grid line to the step shell on each side (= half the insert zone). */
export const stepCellPaddingPx = stepInsertZoneWidthPx / 2;

/** Minimum shell width for the smallest (0.25×) step cell (1 column − 2 paddings). */
export const stepCellMinWidthPx = stepCellQuarterGridWidthPx - stepInsertZoneWidthPx;

/** Width of one skip / mute / gear slot in the 0.25× step footer (three equal columns). */
export const stepFooterActionSlotWidthPx = stepCellMinWidthPx / 3;

/** Base pixel width for a step with timing multiplier index at 1× (four grid columns). */
export const stepCellBaseWidthPx = stepCellQuarterGridWidthPx / stepTimingMultiplierQuarterStep;

/**
 * Row timing offset in step-timing-multiplier units on the fixed quarter grid
 * (e.g. 0.25 = one 0.25× step column); matches PluginProcessor::rowTimingOffsetValues.
 */
export const timingOffsetValues = [-0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75];

/** @param {number} durationQuarters */
export function durationToQuarterGridSteps(durationQuarters) {
  return Math.round(durationQuarters / stepTimingMultiplierQuarterStep);
}

/** @param {number} quarterGridSteps */
export function quarterGridStepsToWidthPx(quarterGridSteps) {
  return quarterGridSteps * stepCellQuarterGridWidthPx;
}

/**
 * Phrase-row horizontal shift for a timing offset index.
 * Uses the fixed step grid only (no pulse scaling): 0.25 aligns with one 0.25× step column.
 *
 * @param {number} offsetIndex
 */
export function rowTimingOffsetShiftPx(offsetIndex) {
  const offsetMultiplier = timingOffsetValues[offsetIndex] ?? 0;

  return quarterGridStepsToWidthPx(durationToQuarterGridSteps(offsetMultiplier));
}

/** @param {number} multiplierIndex */
export function timingMultiplierAtIndex(multiplierIndex) {
  return timingMultiplierValues[multiplierIndex] ?? 1;
}

/** @param {number} value */
export function formatTimingMultiplierLabel(value) {
  if (value < 1) {
    const text = value.toFixed(2).replace(/0+$/, "").replace(/\.$/, "");

    return text.startsWith("0.") ? text.slice(1) : text;
  }

  return Number.isInteger(value) ? String(value) : String(value);
}

export const timingMultiplierOptions = timingMultiplierValues.map((value, index) => ({
  index,
  label: formatTimingMultiplierLabel(value),
}));

/** @param {{ index: number, label: string }[]} [options] */
export function insertStepTimingMultiplierOptions(options = timingMultiplierOptions) {
  return options.filter((option) => {
    const value = timingMultiplierAtIndex(option.index);

    return value >= stepTimingMultiplierMin && value <= insertStepTimingMultiplierMax;
  });
}

/** @param {number} multiplierIndex */
export function quarterGridColumnsForMultiplierIndex(multiplierIndex) {
  const normalizedIndex = Number.isFinite(multiplierIndex)
    ? Math.round(multiplierIndex)
    : defaultStepTimingMultiplierIndex;
  const clampedIndex = Math.min(
    timingMultiplierValues.length - 1,
    Math.max(0, normalizedIndex),
  );

  return clampedIndex + 1;
}

/** Nominal grid span in px (N columns × W, before padding). */
export function stepCellGridSpanPx(multiplierIndex) {
  return (
    quarterGridColumnsForMultiplierIndex(multiplierIndex) * stepCellQuarterGridWidthPx
  );
}

/**
 * Step shell width on the quarter grid: N columns wide minus padding on both outer edges.
 * 0.25× → 1W − 2P, 0.5× → 2W − 2P, 1× → 4W − 2P, etc.
 *
 * @param {number} multiplierIndex
 */
export function stepDisplayWidthPx(multiplierIndex) {
  return stepCellGridSpanPx(multiplierIndex) - stepInsertZoneWidthPx;
}

/** @deprecated Use {@link stepDisplayWidthPx}; kept for call sites that mean shell width. */
export function stepCellWidthPx(multiplierIndex) {
  return stepDisplayWidthPx(multiplierIndex);
}

/** Total row span on the quarter grid in px (musical duration, ignores inter-step gaps). */
export function rowGridWidthPx(multiplierIndices) {
  const totalColumns = multiplierIndices.reduce(
    (sum, index) => sum + quarterGridColumnsForMultiplierIndex(index),
    0,
  );

  return totalColumns * stepCellQuarterGridWidthPx;
}

/**
 * Per-cell display widths (N×W − 2P). Gaps between cells are separate margins.
 *
 * @param {number[]} multiplierIndices
 */
export function rowCellDisplayWidthsPx(multiplierIndices) {
  return multiplierIndices.map((index) => stepDisplayWidthPx(index));
}

/**
 * @typedef {{
 *   leftPx: number,
 *   widthPx: number,
 *   boundaryBeforePx: number,
 *   gridColumns: number,
 * }} RowStepLayout
 */

/**
 * Absolute positions for each step shell on the row grid.
 * Inserts are centered on `boundaryBeforePx` and on the trailing grid edge.
 *
 * @param {number[]} multiplierIndices
 * @returns {{ layouts: RowStepLayout[], gridWidthPx: number }}
 */
export function rowStepLayoutsPx(multiplierIndices) {
  const W = stepCellQuarterGridWidthPx;
  const gapPx = stepInsertZoneWidthPx;
  const paddingPx = stepCellPaddingPx;

  let cumulativeColumns = 0;
  /** @type {RowStepLayout[]} */
  const layouts = [];

  for (let step = 0; step < multiplierIndices.length; step += 1) {
    const columns = quarterGridColumnsForMultiplierIndex(multiplierIndices[step]);
    const boundaryBeforePx = cumulativeColumns * W;
    const widthPx = columns * W - gapPx;

    layouts.push({
      leftPx: boundaryBeforePx + paddingPx,
      widthPx,
      boundaryBeforePx,
      gridColumns: columns,
    });

    cumulativeColumns += columns;
  }

  return {
    layouts,
    gridWidthPx: cumulativeColumns * W,
  };
}

export function minMultiplierCellWidthPx() {
  return stepDisplayWidthPx(0);
}

export function maxMultiplierCellWidthPx() {
  return stepDisplayWidthPx(timingMultiplierValues.length - 1);
}

/** @param {number} widthPx */
export function multiplierIndexFromWidth(widthPx) {
  return multiplierIndexFromSnapWidths(
    timingMultiplierValues.map((_, index) => stepDisplayWidthPx(index)),
    widthPx,
  );
}

/** @param {number[]} snapWidths @param {number} widthPx */
function multiplierIndexFromSnapWidths(snapWidths, widthPx) {
  const clamped = Math.min(snapWidths[snapWidths.length - 1], Math.max(snapWidths[0], widthPx));

  for (let index = 0; index < snapWidths.length - 1; index += 1) {
    const midpoint = (snapWidths[index] + snapWidths[index + 1]) / 2;

    if (clamped < midpoint) return index;
  }

  return snapWidths.length - 1;
}

/** @param {number} multiplierIndex @param {{ index: number, label: string }[]} options */
export function multiplierLabelForIndex(multiplierIndex, options) {
  return options.find((option) => option.index === multiplierIndex)?.label ?? "1";
}

/** @param {string[]} before @param {string[]} after */
export function findSingleMove(before, after) {
  if (before.length !== after.length) return null;

  for (let index = 0; index < before.length; index += 1) {
    if (before[index] !== after[index]) {
      return { from: index, to: after.indexOf(before[index]) };
    }
  }

  return null;
}
