import { defaultPulseIndex, pulseQuartersForIndex } from "./pulseLayout.js";

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

/** Matches PluginProcessor::maxPhraseStepsPerRow. */
export const maxPhraseStepsPerRow = 64;

/** Width of one invisible 0.25× grid unit — fits D#4 + 127 aligned with bar/header. */
export const stepCellQuarterGridWidthPx = 73;

/** Minimum shell width for the smallest (0.25×) step cell. */
export const stepCellMinWidthPx = stepCellQuarterGridWidthPx;

/** Width of one skip / mute / gear slot in the 0.25× step footer (three equal columns). */
export const stepFooterActionSlotWidthPx = stepCellMinWidthPx / 3;

/** Base pixel width for a step with timing multiplier index at 1×. */
export const stepCellBaseWidthPx = stepCellQuarterGridWidthPx / stepTimingMultiplierQuarterStep;

/** Width of the insert divider control between cells. */
export const stepInsertZoneWidthPx = 16;

/** Row timing offset in quarter notes; matches PluginProcessor::rowTimingOffsetValues. */
export const timingOffsetValues = [-0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75];

/** @param {number} offsetIndex @param {number} [pulseIndex] */
export function rowTimingOffsetShiftPx(offsetIndex, pulseIndex = defaultPulseIndex) {
  const offset = (timingOffsetValues[offsetIndex] ?? 0) * pulseQuartersForIndex(pulseIndex);

  return Math.round(stepCellBaseWidthPx * offset);
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

/** @param {number} multiplierIndex */
export function stepCellWidthPx(multiplierIndex) {
  const normalizedIndex = Number.isFinite(multiplierIndex)
    ? Math.round(multiplierIndex)
    : defaultStepTimingMultiplierIndex;
  const clampedIndex = Math.min(
    timingMultiplierValues.length - 1,
    Math.max(0, normalizedIndex),
  );

  return stepCellQuarterGridWidthPx * (clampedIndex + 1);
}

/**
 * Per-cell display widths on the fixed 0.25× grid.
 * Insert zone spacing is a separate margin between cells, never subtracted from a cell.
 *
 * @param {number[]} multiplierIndices
 */
export function rowCellDisplayWidthsPx(multiplierIndices) {
  return multiplierIndices.map((index) => stepCellWidthPx(index));
}

export function minMultiplierCellWidthPx() {
  return stepCellWidthPx(0);
}

export function maxMultiplierCellWidthPx() {
  return stepCellWidthPx(timingMultiplierValues.length - 1);
}

/** @param {number} widthPx */
export function multiplierIndexFromWidth(widthPx) {
  return multiplierIndexFromSnapWidths(
    timingMultiplierValues.map((_, index) => stepCellWidthPx(index)),
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
