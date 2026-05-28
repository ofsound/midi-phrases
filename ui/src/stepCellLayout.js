export const timingMultiplierValues = [0.25, 0.5, 1, 2, 4];

/** Minimum shell width for the smallest (0.25×) step cell — fits G3 + 127 aligned with bar/header. */
export const stepCellMinWidthPx = 64;

/** Base pixel width for a step with timing multiplier index 2 (value 1). */
export const stepCellBaseWidthPx = stepCellMinWidthPx / timingMultiplierValues[0];

/** Width of the insert divider control between cells. */
export const stepInsertZoneWidthPx = 16;

/** Row timing offset in quarter notes; matches PluginProcessor::rowTimingOffsetValues. */
export const timingOffsetValues = [-0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75];

/** @param {number} offsetIndex */
export function rowTimingOffsetShiftPx(offsetIndex) {
  const offset = timingOffsetValues[offsetIndex] ?? 0;

  return Math.round(stepCellBaseWidthPx * offset);
}

/** @param {number} multiplierIndex */
export function timingMultiplierAtIndex(multiplierIndex) {
  return timingMultiplierValues[multiplierIndex] ?? 1;
}

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
 * Per-cell display widths so Σ widths + (n−1)×gap equals the canonical span for Σ durations.
 * During resize, pass resizeStep + resizeDisplayWidth (compensated px) so siblings re-compensate live.
 *
 * @param {number[]} multiplierIndices
 * @param {{ gapPx?: number, resizeStep?: number, resizeDisplayWidth?: number }} [options]
 */
export function rowCellDisplayWidthsPx(multiplierIndices, options = {}) {
  const gapPx = options.gapPx ?? stepInsertZoneWidthPx;
  const resizeStep = options.resizeStep ?? -1;
  const resizeDisplayWidth = options.resizeDisplayWidth ?? 0;
  const n = multiplierIndices.length;

  if (n === 0) {
    return [];
  }

  if (n === 1) {
    if (resizeStep === 0) {
      return [Math.round(resizeDisplayWidth)];
    }

    return [stepCellWidthPx(multiplierIndices[0])];
  }

  const durations = multiplierIndices.map((index, step) => {
    if (step === resizeStep) {
      return timingMultiplierAtIndex(
        multiplierIndexFromCompensatedWidth(multiplierIndices, resizeStep, resizeDisplayWidth),
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

  const exactWidths = durations.map((duration) => (duration / totalDuration) * cellBudget);
  let rounded = exactWidths.map((width) => Math.round(width));
  const targetSum = Math.round(cellBudget);
  let sum = rounded.reduce((total, width) => total + width, 0);
  const diff = targetSum - sum;

  if (diff !== 0) {
    rounded[rounded.length - 1] += diff;
  }

  rounded = rounded.map((width) => Math.max(stepCellMinWidthPx, width));
  sum = rounded.reduce((total, width) => total + width, 0);

  let overflow = sum - targetSum;

  while (overflow > 0) {
    let shrinkStep = -1;
    let shrinkHeadroom = 0;

    for (let step = 0; step < rounded.length; step += 1) {
      const headroom = rounded[step] - stepCellMinWidthPx;

      if (headroom > shrinkHeadroom) {
        shrinkHeadroom = headroom;
        shrinkStep = step;
      }
    }

    if (shrinkStep < 0 || shrinkHeadroom <= 0) {
      break;
    }

    const delta = Math.min(overflow, shrinkHeadroom);
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
 */
export function compensatedCellWidthPx(multiplierIndices, step, multiplierIndex) {
  const preview = multiplierIndices.slice();
  preview[step] = multiplierIndex;

  return rowCellDisplayWidthsPx(preview)[step];
}

/**
 * Compensated min/max for resize drag so pointer-down does not jump to naive width.
 * @param {number[]} multiplierIndices
 * @param {number} step
 */
export function compensatedResizeBoundsPx(multiplierIndices, step) {
  const snapWidths = timingMultiplierValues.map((_, index) =>
    compensatedCellWidthPx(multiplierIndices, step, index),
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
 */
export function multiplierIndexFromCompensatedWidth(multiplierIndices, step, widthPx) {
  const snapWidths = timingMultiplierValues.map((_, index) =>
    compensatedCellWidthPx(multiplierIndices, step, index),
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
