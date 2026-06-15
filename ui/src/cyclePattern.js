/** @typedef {{ cycle: number, mask: number }} CyclePattern */

export const minStepCycle = 1;
export const maxStepCycle = 64;
/** Maximum cycle length exposed in the step-inspector pattern editor. */
export const maxCyclePatternCells = 8;
export const defaultStepCycle = 1;
export const defaultStepCycleMask = 1;

/** @param {number} cycle */
export function clampStepCycle(cycle) {
  return Math.min(maxStepCycle, Math.max(minStepCycle, Math.round(cycle)));
}

/** @param {number} cycle */
export function clampEditorStepCycle(cycle) {
  return Math.min(maxCyclePatternCells, Math.max(minStepCycle, Math.round(cycle)));
}

/** @param {number} cycle */
export function maskForCycleLength(cycle) {
  const bits = Math.min(clampStepCycle(cycle), 30);

  return bits <= 0 ? 1 : (1 << bits) - 1;
}

/** @param {number} mask @param {number} cycle */
export function clampStepCycleMask(mask, cycle) {
  return Math.max(0, Math.round(mask)) & maskForCycleLength(cycle);
}

/**
 * Convert a legacy single-offset value to a one-hot mask.
 *
 * @param {number} offset
 * @param {number} cycle
 */
export function cycleMaskFromLegacyOffset(offset, cycle) {
  const length = clampStepCycle(cycle);

  if (length <= 1) return defaultStepCycleMask;

  const phase = Math.min(Math.max(0, Math.round(offset)), length - 1);

  return 1 << phase;
}

/**
 * @param {number} triggerCount
 * @param {number} cycle
 * @param {number} mask
 */
export function cycleGatePasses(triggerCount, cycle, mask) {
  const length = clampStepCycle(cycle);
  const pattern = clampStepCycleMask(mask, length) || defaultStepCycleMask;

  if (length <= 1) return (pattern & 1) !== 0;

  const phase = ((Math.round(triggerCount) % length) + length) % length;

  return ((pattern >> phase) & 1) !== 0;
}

/** @param {number} cycle @param {number} mask */
export function normalizeCyclePattern(cycle, mask) {
  const nextCycle = clampStepCycle(cycle);
  let nextMask = clampStepCycleMask(mask, nextCycle);

  if (nextMask === 0) {
    nextMask = defaultStepCycleMask;
  }

  return {cycle: nextCycle, mask: nextMask};
}

/** @param {number} cycle @param {number} mask */
export function normalizeEditorCyclePattern(cycle, mask) {
  const nextCycle = clampEditorStepCycle(cycle);
  let nextMask = clampStepCycleMask(mask, nextCycle);

  if (nextMask === 0) {
    nextMask = defaultStepCycleMask;
  }

  return {cycle: nextCycle, mask: nextMask};
}

/**
 * @param {number} cycle
 * @param {number} mask
 * @param {number} index
 */
export function isCycleCellActive(cycle, mask, index) {
  if (index < 0 || index >= clampEditorStepCycle(cycle)) return false;

  return (clampStepCycleMask(mask, cycle) & (1 << index)) !== 0;
}

/**
 * @param {number} cycle
 * @param {number} mask
 * @param {number} index
 */
export function toggleCycleCell(cycle, mask, index) {
  const nextCycle = clampEditorStepCycle(cycle);

  if (index < 0 || index >= nextCycle) {
    return normalizeEditorCyclePattern(nextCycle, mask);
  }

  const bit = 1 << index;
  const nextMask = clampStepCycleMask(mask, nextCycle) ^ bit;

  return normalizeEditorCyclePattern(nextCycle, nextMask);
}

/**
 * @param {number} cycle
 * @param {number} mask
 * @param {number} nextCycle
 */
export function resizeCyclePattern(cycle, mask, nextCycle) {
  const normalized = normalizeEditorCyclePattern(cycle, mask);
  const length = clampEditorStepCycle(nextCycle);
  let nextMask = clampStepCycleMask(normalized.mask, length);

  if (nextMask === 0) {
    nextMask = defaultStepCycleMask;
  }

  return {cycle: length, mask: nextMask};
}

/**
 * @param {number} clientX
 * @param {Array<HTMLElement | null | undefined>} cellElements
 */
export function cycleLengthFromCellElements(clientX, cellElements) {
  for (let i = cellElements.length - 1; i >= 0; i -= 1) {
    const cell = cellElements[i];

    if (!cell) continue;

    const {left} = cell.getBoundingClientRect();

    if (clientX >= left) {
      return clampEditorStepCycle(i + 1);
    }
  }

  return 1;
}

/**
 * @param {number} clientX
 * @param {DOMRect} rect
 * @param {number} [cellCount]
 */
export function cycleLengthFromClientX(clientX, rect, cellCount = maxCyclePatternCells) {
  const x = Math.min(rect.right, Math.max(rect.left, clientX));
  const cellWidth = rect.width / cellCount;
  const index = Math.floor((x - rect.left) / cellWidth);

  return clampEditorStepCycle(index + 1);
}
