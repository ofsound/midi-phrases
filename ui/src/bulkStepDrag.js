import { maxPhraseStepsPerRow } from "./stepCellLayout.js";

/**
 * Selected step ids in their current row order.
 *
 * @param {string[]} rowIds
 * @param {string[]} selectedIds
 */
export function selectedIdsInRowOrder(rowIds, selectedIds) {
  const selected = new Set(selectedIds);

  return rowIds.filter((id) => selected.has(id));
}

/**
 * Reorder a row by moving a contiguous selection block to the drop shadow index.
 *
 * @param {string[]} beforeIds
 * @param {string[]} blockIds
 * @param {number} shadowIndex
 */
export function blockMoveOrder(beforeIds, blockIds, shadowIndex) {
  if (blockIds.length === 0 || beforeIds.length === 0) return [...beforeIds];

  const blockSet = new Set(blockIds);
  const block = beforeIds.filter((id) => blockSet.has(id));
  const rest = beforeIds.filter((id) => !blockSet.has(id));

  let restInsertIndex = 0;

  for (let index = 0; index < shadowIndex && index < beforeIds.length; index += 1) {
    if (!blockSet.has(beforeIds[index])) restInsertIndex += 1;
  }

  return [
    ...rest.slice(0, restInsertIndex),
    ...block,
    ...rest.slice(restInsertIndex),
  ];
}

/**
 * Insertion index for copying a block while the originals stay in place.
 *
 * @param {string[]} beforeIds
 * @param {string[]} blockIds
 * @param {number} shadowIndex
 */
/**
 * Maps a cross-row drag preview to a target-row insertion index.
 *
 * @param {string[]} targetBeforeIds
 * @param {string} movedStepId
 * @param {string[]} previewIds
 * @param {number} shadowIndex
 */
export function blockCrossRowInsertionIndex(targetBeforeIds, movedStepId, previewIds, shadowIndex) {
  const previewIndex = previewIds.indexOf(movedStepId);

  if (previewIndex >= 0) return previewIndex;

  return Math.min(Math.max(0, shadowIndex), targetBeforeIds.length);
}

export function blockDuplicateInsertionIndex(beforeIds, blockIds, shadowIndex) {
  const blockSet = new Set(blockIds);
  const blockIndices = beforeIds
    .map((id, index) => (blockSet.has(id) ? index : -1))
    .filter((index) => index >= 0);

  if (blockIndices.length === 0) return shadowIndex;

  const lastBlockIndex = blockIndices[blockIndices.length - 1];

  if (shadowIndex > lastBlockIndex) {
    return shadowIndex + 1 - (blockIds.length - 1);
  }

  return shadowIndex;
}

/**
 * Copy a same-row step block at `insertionIndex`.
 *
 * @param {Record<string, any[][]>} matrices
 * @param {string[][]} stepIds
 * @param {number} row
 * @param {string[]} blockIds
 * @param {number} insertionIndex
 * @param {() => string} createStepId
 */
export function duplicateBlockInRow(matrices, stepIds, row, blockIds, insertionIndex, createStepId) {
  const beforeIds = stepIds[row];
  const blockSet = new Set(blockIds);
  const block = beforeIds.filter((id) => blockSet.has(id));

  if (block.length === 0) return null;
  if (insertionIndex < 0 || insertionIndex > beforeIds.length) return null;
  if (beforeIds.length + block.length > maxPhraseStepsPerRow) return null;

  const newIds = block.map(() => createStepId());
  const nextStepIds = stepIds.map((ids) => [...ids]);
  nextStepIds[row] = [...beforeIds];
  nextStepIds[row].splice(insertionIndex, 0, ...newIds);

  /** @type {Record<string, any[][]>} */
  const nextMatrices = {};

  for (const [name, matrix] of Object.entries(matrices)) {
    const next = matrix.map((values) => [...values]);
    const rowValues = next[row];
    const blockValues = block.map((id) => rowValues[beforeIds.indexOf(id)]);

    next[row] = [...rowValues];
    next[row].splice(insertionIndex, 0, ...blockValues);
    nextMatrices[name] = next;
  }

  return { matrices: nextMatrices, stepIds: nextStepIds, newIds };
}
