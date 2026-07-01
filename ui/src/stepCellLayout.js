
import { scaledPx } from "./uiScale.svelte.js";

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
 * Sized so a 0.25× shell aligns “D#4 127” to the duration track's right edge.
 */
export const baseStepCellQuarterGridWidthPx = 76.5;

/** Width of the insert divider control between cells (centered in the inter-step gap). */
export const baseStepInsertZoneWidthPx = 16;

export function stepCellQuarterGridWidthPx() {
  return scaledPx(baseStepCellQuarterGridWidthPx);
}

export function stepInsertZoneWidthPx() {
  return scaledPx(baseStepInsertZoneWidthPx);
}

/** Width of the full-size step's trailing-edge resize affordance. */
export const baseStepBoundaryEndResizePx = 11;

/** Width of the leading-edge resize affordance on the following step. */
export const baseStepBoundaryStartResizePx = 12;

export function stepBoundaryEndResizePx() {
  return scaledPx(baseStepBoundaryEndResizePx);
}

export function stepBoundaryStartResizePx() {
  return scaledPx(baseStepBoundaryStartResizePx);
}

/** Combined boundary hit target: left step end + gap + right step start. */
export function stepBoundaryResizeZoneWidthPx() {
  return stepBoundaryEndResizePx() + stepInsertZoneWidthPx() + stepBoundaryStartResizePx();
}

/** Left edge for a boundary resize zone centered on a quarter-grid boundary. */
export function boundaryResizeZoneLeftPxAtGridBoundaryPx(boundaryPx) {
  return boundaryPx - stepBoundaryResizeZoneWidthPx() / 2;
}

/** Left edge for an insert slot centered on a quarter-grid boundary. */
export function insertSlotLeftPxAtGridBoundaryPx(boundaryPx) {
  return boundaryPx - stepInsertZoneWidthPx() / 2;
}

/** Padding from a grid line to the step shell on each side (= half the insert zone). */
export function stepCellPaddingPx() {
  return stepInsertZoneWidthPx() / 2;
}

/** Minimum shell width for the smallest (0.25×) step cell (1 column − 2 paddings). */
export function stepCellMinWidthPx() {
  return stepCellQuarterGridWidthPx() - stepInsertZoneWidthPx();
}

/** Width of one skip / mute / gear slot in the 0.25× step footer (three equal columns). */
export function stepFooterActionSlotWidthPx() {
  return stepCellMinWidthPx() / 3;
}

/** Base pixel width for a step with timing multiplier index at 1× (four grid columns). */
export function stepCellBaseWidthPx() {
  return stepCellQuarterGridWidthPx() / stepTimingMultiplierQuarterStep;
}

/**
 * Row timing offset in quarter notes on the fixed 0.25 grid
 * (matches PluginProcessor::rowTimingOffsetForIndex).
 */
export const rowTimingOffsetMinQuarters = -2;
export const rowTimingOffsetMaxQuarters = 2;
export const rowTimingOffsetQuarterStep = stepTimingMultiplierQuarterStep;

export const rowTimingOffsetCount = Math.round(
  (rowTimingOffsetMaxQuarters - rowTimingOffsetMinQuarters) / rowTimingOffsetQuarterStep,
) + 1;

/** @param {number} offsetIndex */
export function rowTimingOffsetQuartersAtIndex(offsetIndex) {
  const index = Math.min(rowTimingOffsetCount - 1, Math.max(0, offsetIndex));

  return rowTimingOffsetMinQuarters + index * rowTimingOffsetQuarterStep;
}

/** @param {number} quarters */
export function rowTimingOffsetIndexForQuarters(quarters) {
  const index = Math.round((quarters - rowTimingOffsetMinQuarters) / rowTimingOffsetQuarterStep);

  return Math.min(rowTimingOffsetCount - 1, Math.max(0, index));
}

/** Default index for 0 quarter-note offset. */
export const defaultRowTimingOffsetIndex = rowTimingOffsetIndexForQuarters(0);

export const timingOffsetValues = Array.from(
  { length: rowTimingOffsetCount },
  (_, index) => rowTimingOffsetQuartersAtIndex(index),
);

/** @param {number} durationQuarters */
export function durationToQuarterGridSteps(durationQuarters) {
  return Math.round(durationQuarters / stepTimingMultiplierQuarterStep);
}

/** @param {number} quarterGridSteps */
export function quarterGridStepsToWidthPx(quarterGridSteps) {
  return quarterGridSteps * stepCellQuarterGridWidthPx();
}

/**
 * Phrase-row horizontal shift for a timing offset index.
 * Uses the fixed step grid only (no pulse scaling): 0.25 aligns with one 0.25× step column.
 *
 * @param {number} offsetIndex
 */
export function rowTimingOffsetShiftPx(offsetIndex) {
  const offsetMultiplier = rowTimingOffsetQuartersAtIndex(offsetIndex);

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

/** @param {number} value */
export function clampTimingMultiplierValue(value) {
  if (!Number.isFinite(value)) return stepTimingMultiplierMin;

  const stepped = Math.round(value / stepTimingMultiplierQuarterStep) * stepTimingMultiplierQuarterStep;

  return Math.min(stepTimingMultiplierMax, Math.max(stepTimingMultiplierMin, stepped));
}

/** @param {number} value */
export function timingMultiplierIndexForValue(value) {
  const clamped = clampTimingMultiplierValue(value);

  return Math.round((clamped - stepTimingMultiplierMin) / stepTimingMultiplierQuarterStep);
}

/** @param {number} delta */
export function snapTimingMultiplierDelta(delta) {
  if (!Number.isFinite(delta)) return 0;

  return Math.round(delta / stepTimingMultiplierQuarterStep) * stepTimingMultiplierQuarterStep;
}

/**
 * @param {number} delta
 * @param {number[]} baselines
 */
export function clampTimingMultiplierDelta(delta, baselines) {
  const snapped = snapTimingMultiplierDelta(delta);

  if (baselines.length === 0) return 0;

  let minDelta = -stepTimingMultiplierMax;
  let maxDelta = stepTimingMultiplierMax;

  for (const baseline of baselines) {
    minDelta = Math.max(minDelta, stepTimingMultiplierMin - baseline);
    maxDelta = Math.min(maxDelta, stepTimingMultiplierMax - baseline);
  }

  return Math.min(maxDelta, Math.max(minDelta, snapped));
}

/** @param {number} delta */
export function formatSignedTimingMultiplierDelta(delta) {
  const stepped = snapTimingMultiplierDelta(delta);

  if (stepped === 0) return "0";

  const magnitude = formatTimingMultiplierLabel(Math.abs(stepped));

  return stepped > 0 ? `+${magnitude}` : `-${magnitude}`;
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

/**
 * Multiplier indices on the 0.25 grid from minIndex through maxIndex inclusive.
 *
 * @param {number} minIndex
 * @param {number} maxIndex
 */
export function timingMultiplierIndicesInRange(minIndex, maxIndex) {
  const start = Math.min(
    timingMultiplierValues.length - 1,
    Math.max(0, Math.round(Math.min(minIndex, maxIndex))),
  );
  const end = Math.min(
    timingMultiplierValues.length - 1,
    Math.max(0, Math.round(Math.max(minIndex, maxIndex))),
  );

  return Array.from({ length: end - start + 1 }, (_, offset) => start + offset);
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
    quarterGridColumnsForMultiplierIndex(multiplierIndex) * stepCellQuarterGridWidthPx()
  );
}

/**
 * Step shell width on the quarter grid: N columns wide minus padding on both outer edges.
 * 0.25× → 1W − 2P, 0.5× → 2W − 2P, 1× → 4W − 2P, etc.
 *
 * @param {number} multiplierIndex
 */
export function stepDisplayWidthPx(multiplierIndex) {
  return stepCellGridSpanPx(multiplierIndex) - stepInsertZoneWidthPx();
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

  return totalColumns * stepCellQuarterGridWidthPx();
}

/** Number of quarter-grid columns in the longest row. */
export function longestRowQuarterGridColumns(rows) {
  return Math.max(
    1,
    ...rows.map((row) =>
      row.reduce((sum, index) => sum + quarterGridColumnsForMultiplierIndex(index), 0),
    ),
  );
}

/** Timing offset expressed in the same quarter-grid columns used by step spans. */
export function rowTimingOffsetQuarterGridColumns(offsetIndex) {
  return durationToQuarterGridSteps(rowTimingOffsetQuartersAtIndex(offsetIndex));
}

/**
 * Shared compact-grid layout for every row. Timing offsets are normalized into
 * grid columns so all rows retain one horizontal scale. The row-end add
 * control sits just after the last step in each row.
 *
 * @param {number[][]} rows
 * @param {number[]} timingOffsetIndices
 */
export function compactPhraseGridLayout(rows, timingOffsetIndices) {
  const rawStartColumns = rows.map((row, index) =>
    row.length > 0
      ? rowTimingOffsetQuarterGridColumns(timingOffsetIndices[index])
      : 0,
  );
  const firstColumn = Math.min(0, ...rawStartColumns);
  const rowStartColumns = rawStartColumns.map((start) => start - firstColumn);
  const totalColumns = Math.max(
    1,
    ...rows.map((row, index) =>
      row.length > 0
        ? rowStartColumns[index] + rowQuarterGridColumns(row)
        : 0,
    ),
  );

  return { totalColumns, rowStartColumns };
}

/** Natural scroll width for the longest phrase row's scalable step content. */
export function phraseRowsScrollContentWidthPx(rows, timingOffsetIndices) {
  const { totalColumns } = compactPhraseGridLayout(rows, timingOffsetIndices);

  return totalColumns * stepCellQuarterGridWidthPx();
}

/** Total quarter-grid columns occupied by a row's steps. */
export function rowQuarterGridColumns(multiplierIndices) {
  return multiplierIndices.reduce(
    (sum, index) => sum + quarterGridColumnsForMultiplierIndex(index),
    0,
  );
}

/**
 * Horizontal inset for a stretch-to-fit step shell, as a percent of its grid span.
 * Matches {@link stepCellPaddingPx} on each side of {@link stepDisplayWidthPx}.
 *
 * @param {number} multiplierIndex
 */
export function compactStepShellPaddingPercent(multiplierIndex) {
  const columns = quarterGridColumnsForMultiplierIndex(multiplierIndex);
  const spanPx = columns * stepCellQuarterGridWidthPx();

  return (stepCellPaddingPx() / spanPx) * 100;
}

/**
 * Trailing inset after the last stretch-to-fit step shell.
 * Matches the phrase row end tail padding (two-thirds of {@link stepCellPaddingPx}).
 *
 * @param {number} multiplierIndex
 */
export function compactStepShellTrailingPaddingPercent(multiplierIndex) {
  const columns = quarterGridColumnsForMultiplierIndex(multiplierIndex);
  const spanPx = columns * stepCellQuarterGridWidthPx();
  const trailingPx = stepCellPaddingPx() * (2 / 3);

  return (trailingPx / spanPx) * 100;
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
  const W = stepCellQuarterGridWidthPx();
  const gapPx = stepInsertZoneWidthPx();
  const paddingPx = stepCellPaddingPx();

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
