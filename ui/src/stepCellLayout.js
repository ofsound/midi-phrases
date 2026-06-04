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

/** Minimum shell width for the smallest (0.25×) step cell — fits D#4 + 127 aligned with bar/header. */
export const stepCellMinWidthPx = 73;

/** Width of one skip / mute / gear slot in the 0.25× step footer (three equal columns). */
export const stepFooterActionSlotWidthPx = stepCellMinWidthPx / 3;

/** Base pixel width for a step with timing multiplier index at 1×. */
export const stepCellBaseWidthPx = stepCellMinWidthPx / timingMultiplierValues[0];

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
  return Math.round(stepCellBaseWidthPx * timingMultiplierAtIndex(multiplierIndex));
}

/**
 * Canonical row span for a total duration (single cell, no gaps).
 * @param {number} totalDuration
 */
export function rowCanonicalWidthPx(totalDuration) {
  return Math.round(stepCellBaseWidthPx * totalDuration);
}

/**
 * @typedef {{
 *   totalDuration: number,
 *   stepCount: number,
 *   multiplierIndices: number[],
 * }} PhraseReferenceRow
 */

/** @param {number[]} multiplierIndices */
export function rowTotalDurationFromMultiplierIndices(multiplierIndices) {
  return multiplierIndices.reduce(
    (sum, index) => sum + timingMultiplierAtIndex(index),
    0,
  );
}

/**
 * Virtual fullest row when every phrase row has a single step (blank/default pattern).
 * Matches gap-carved 1× width once any row grows past one step.
 */
export const defaultSoloLayoutReferenceStepCount = 4;

/** @returns {PhraseReferenceRow} */
export function defaultPhraseLayoutReferenceRow() {
  const multiplierIndices = Array.from(
    { length: defaultSoloLayoutReferenceStepCount },
    () => defaultStepTimingMultiplierIndex,
  );

  return {
    totalDuration: rowTotalDurationFromMultiplierIndices(multiplierIndices),
    stepCount: multiplierIndices.length,
    multiplierIndices,
  };
}

/**
 * Fullest row in the phrase: max total step duration; ties break toward more steps (more gaps).
 * When every row has only one step, uses {@link defaultPhraseLayoutReferenceRow}.
 *
 * @param {number[][]} allRowMultipliers
 * @returns {PhraseReferenceRow | null}
 */
export function phraseFullestRowReference(allRowMultipliers) {
  /** @type {PhraseReferenceRow | null} */
  let fullest = null;
  let maxStepCount = 0;

  for (const row of allRowMultipliers) {
    if (!row?.length) {
      continue;
    }

    maxStepCount = Math.max(maxStepCount, row.length);

    const totalDuration = rowTotalDurationFromMultiplierIndices(row);
    const stepCount = row.length;

    if (totalDuration <= 0) {
      continue;
    }

    if (
      !fullest ||
      totalDuration > fullest.totalDuration ||
      (totalDuration === fullest.totalDuration && stepCount > fullest.stepCount)
    ) {
      fullest = {
        totalDuration,
        stepCount,
        multiplierIndices: row.slice(),
      };
    }
  }

  if (maxStepCount <= 1) {
    return defaultPhraseLayoutReferenceRow();
  }

  return fullest;
}

/**
 * Solo cell width using the same gap-carving layout as the phrase's fullest row
 * (simulates that row with this cell in slot 0 — matches adding a sibling on the sparse row).
 *
 * @param {number} multiplierIndex
 * @param {PhraseReferenceRow} reference
 * @param {number} gapPx
 */
export function soloCellDisplayWidthPx(multiplierIndex, reference, gapPx = stepInsertZoneWidthPx) {
  const { stepCount, multiplierIndices } = reference;

  if (stepCount <= 1) {
    return stepCellWidthPx(multiplierIndex);
  }

  const synthetic = multiplierIndices.slice();
  synthetic[0] = multiplierIndex;

  return rowCellDisplayWidthsPx(synthetic, { gapPx })[0];
}

/**
 * Integer pixel boundaries at cumulative musical durations (start + after each cell).
 * End is pinned to canonicalWidth so shared durations align across rows with different step counts.
 *
 * @param {number[]} durations
 * @param {number} totalDuration
 * @param {number} canonicalWidth
 */
function musicalBoundariesPx(durations, totalDuration, canonicalWidth) {
  const boundaries = [0];
  let cumulative = 0;

  for (const duration of durations) {
    cumulative += duration;
    boundaries.push((cumulative / totalDuration) * canonicalWidth);
  }

  boundaries[boundaries.length - 1] = canonicalWidth;

  const rounded = [0];

  for (let index = 1; index < boundaries.length - 1; index += 1) {
    rounded.push(Math.round(boundaries[index]));
  }

  rounded.push(canonicalWidth);

  for (let index = 1; index < rounded.length; index += 1) {
    if (rounded[index] < rounded[index - 1]) {
      rounded[index] = rounded[index - 1];
    }
  }

  return rounded;
}

/**
 * Cell widths from musical boundaries; each non-final cell yields gapPx to the insert zone after it.
 *
 * @param {number[]} boundaries
 * @param {number} gapPx
 */
function rowCellWidthsFromBoundariesPx(boundaries, gapPx) {
  const cellCount = boundaries.length - 1;
  const widths = [];

  for (let index = 0; index < cellCount; index += 1) {
    const gapAfter = index < cellCount - 1 ? gapPx : 0;
    widths.push(boundaries[index + 1] - boundaries[index] - gapAfter);
  }

  return widths;
}

/**
 * Per-cell display widths so Σ widths + (n−1)×gap equals the canonical span for Σ durations.
 * Boundaries at cumulative musical durations align across rows; gaps are carved from trailing edges.
 * During resize, pass resizeStep + resizeDisplayWidth (compensated px) so siblings re-compensate live.
 *
 * @param {number[]} multiplierIndices
 * @param {{ gapPx?: number, resizeStep?: number, resizeDisplayWidth?: number, phraseReferenceRow?: PhraseReferenceRow | null }} [options]
 */
export function rowCellDisplayWidthsPx(multiplierIndices, options = {}) {
  const gapPx = options.gapPx ?? stepInsertZoneWidthPx;
  const resizeStep = options.resizeStep ?? -1;
  const resizeDisplayWidth = options.resizeDisplayWidth ?? 0;
  const phraseReferenceRow = options.phraseReferenceRow ?? null;
  const n = multiplierIndices.length;

  if (n === 0) {
    return [];
  }

  if (n === 1) {
    if (phraseReferenceRow) {
      return [soloCellDisplayWidthPx(multiplierIndices[0], phraseReferenceRow, gapPx)];
    }

    if (resizeStep === 0) {
      return [stepCellWidthPx(multiplierIndexFromWidth(resizeDisplayWidth))];
    }

    return [stepCellWidthPx(multiplierIndices[0])];
  }

  const durations = multiplierIndices.map((index, step) => {
    if (step === resizeStep) {
      return timingMultiplierAtIndex(
        multiplierIndexFromCompensatedWidth(
          multiplierIndices,
          resizeStep,
          resizeDisplayWidth,
          options,
        ),
      );
    }

    return timingMultiplierAtIndex(index);
  });

  const totalDuration = durations.reduce((sum, duration) => sum + duration, 0);

  if (totalDuration <= 0) {
    return multiplierIndices.map(() => stepCellMinWidthPx);
  }

  const canonicalWidth = rowCanonicalWidthPx(totalDuration);
  const cellBudget = canonicalWidth - (n - 1) * gapPx;

  if (cellBudget <= 0) {
    return multiplierIndices.map((index) => stepCellWidthPx(index));
  }

  const boundaries = musicalBoundariesPx(durations, totalDuration, canonicalWidth);
  let rounded = rowCellWidthsFromBoundariesPx(boundaries, gapPx);

  for (let index = 0; index < rounded.length; index += 1) {
    if (rounded[index] >= stepCellMinWidthPx) {
      continue;
    }

    let deficit = stepCellMinWidthPx - rounded[index];
    rounded[index] = stepCellMinWidthPx;

    for (let donor = rounded.length - 1; donor > index && deficit > 0; donor -= 1) {
      const headroom = rounded[donor] - stepCellMinWidthPx;

      if (headroom <= 0) {
        continue;
      }

      const take = Math.min(deficit, headroom);
      rounded[donor] -= take;
      deficit -= take;
    }

    for (let donor = index - 1; donor >= 0 && deficit > 0; donor -= 1) {
      const headroom = rounded[donor] - stepCellMinWidthPx;

      if (headroom <= 0) {
        continue;
      }

      const take = Math.min(deficit, headroom);
      rounded[donor] -= take;
      deficit -= take;
    }
  }

  let sum = rounded.reduce((total, width) => total + width, 0);
  const targetSum = cellBudget;
  let overflow = sum - targetSum;

  while (overflow > 0) {
    let shrinkStep = -1;

    for (let step = rounded.length - 1; step >= 0; step -= 1) {
      if (rounded[step] - stepCellMinWidthPx > 0) {
        shrinkStep = step;
        break;
      }
    }

    if (shrinkStep < 0) {
      break;
    }

    const headroom = rounded[shrinkStep] - stepCellMinWidthPx;
    const delta = Math.min(overflow, headroom);
    rounded[shrinkStep] -= delta;
    overflow -= delta;
  }

  if (resizeStep >= 0 && resizeStep < n) {
    rounded[resizeStep] = Math.round(resizeDisplayWidth);
  }

  return rounded;
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

/**
 * Compensated display width for one cell at a multiplier index (row-aware).
 * @param {number[]} multiplierIndices
 * @param {number} step
 * @param {number} multiplierIndex
 * @param {{ gapPx?: number, phraseReferenceRow?: PhraseReferenceRow | null }} [layoutOptions]
 */
export function compensatedCellWidthPx(
  multiplierIndices,
  step,
  multiplierIndex,
  layoutOptions = {},
) {
  const preview = multiplierIndices.slice();
  preview[step] = multiplierIndex;

  return rowCellDisplayWidthsPx(preview, layoutOptions)[step];
}

/**
 * Compensated min/max for resize drag so pointer-down does not jump to naive width.
 * @param {number[]} multiplierIndices
 * @param {number} step
 * @param {{ gapPx?: number, phraseReferenceRow?: PhraseReferenceRow | null }} [layoutOptions]
 */
export function compensatedResizeBoundsPx(multiplierIndices, step, layoutOptions = {}) {
  const snapWidths = timingMultiplierValues.map((_, index) =>
    compensatedCellWidthPx(multiplierIndices, step, index, layoutOptions),
  );

  return {
    min: Math.min(...snapWidths),
    max: Math.max(...snapWidths),
  };
}

/**
 * Snap multiplier from compensated pixel width for a cell in this row.
 * @param {number[]} multiplierIndices
 * @param {number} step
 * @param {number} widthPx
 * @param {{ gapPx?: number, phraseReferenceRow?: PhraseReferenceRow | null }} [layoutOptions]
 */
export function multiplierIndexFromCompensatedWidth(
  multiplierIndices,
  step,
  widthPx,
  layoutOptions = {},
) {
  const snapWidths = timingMultiplierValues.map((_, index) =>
    compensatedCellWidthPx(multiplierIndices, step, index, layoutOptions),
  );

  return multiplierIndexFromSnapWidths(snapWidths, widthPx);
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
