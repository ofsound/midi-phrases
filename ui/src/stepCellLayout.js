/** Base pixel width for a step with timing multiplier index 2 (value 1). */
export const stepCellBaseWidthPx = 180;

/** Width of the insert divider control between cells. */
export const stepInsertZoneWidthPx = 16;

export const timingMultiplierValues = [0.25, 0.5, 1, 2, 4];

/** @param {number} multiplierIndex */
export function stepCellWidthPx(multiplierIndex) {
  const multiplier = timingMultiplierValues[multiplierIndex] ?? 1;

  return Math.round(stepCellBaseWidthPx * multiplier);
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
