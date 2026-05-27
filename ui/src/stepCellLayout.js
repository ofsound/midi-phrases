/** Base pixel width for a step with timing multiplier index 2 (value 1). */
export const stepCellBaseWidthPx = 180;

/** Width of the insert divider control between cells. */
export const stepInsertZoneWidthPx = 16;

export const timingMultiplierValues = [0.25, 0.5, 1, 2, 4];

/** Row timing offset in quarter notes; matches PluginProcessor::rowTimingOffsetValues. */
export const timingOffsetValues = [-0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75];

/** @param {number} offsetIndex */
export function rowTimingOffsetShiftPx(offsetIndex) {
  const offset = timingOffsetValues[offsetIndex] ?? 0;

  return Math.round(stepCellBaseWidthPx * offset);
}

/** @param {number} multiplierIndex */
export function stepCellWidthPx(multiplierIndex) {
  const multiplier = timingMultiplierValues[multiplierIndex] ?? 1;

  return Math.round(stepCellBaseWidthPx * multiplier);
}

export function minMultiplierCellWidthPx() {
  return stepCellWidthPx(0);
}

export function maxMultiplierCellWidthPx() {
  return stepCellWidthPx(timingMultiplierValues.length - 1);
}

/** @param {number} widthPx */
export function multiplierIndexFromWidth(widthPx) {
  const snapWidths = timingMultiplierValues.map((_, index) => stepCellWidthPx(index));
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
