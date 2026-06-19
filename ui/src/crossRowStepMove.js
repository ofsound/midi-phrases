/**
 * Moves one aligned entry between row-based matrices.
 *
 * @param {Record<string, any[][]>} matrices
 * @param {string[][]} stepIds
 * @param {number} sourceRow
 * @param {number} targetRow
 * @param {string} stepId
 * @param {string[]} orderedTargetIds
 * @returns {{ matrices: Record<string, any[][]>, stepIds: string[][] } | null}
 */
export function moveStepBetweenRows(
  matrices,
  stepIds,
  sourceRow,
  targetRow,
  stepId,
  orderedTargetIds,
) {
  if (sourceRow === targetRow || sourceRow < 0 || targetRow < 0) return null;
  if (!stepIds[sourceRow] || !stepIds[targetRow]) return null;

  const sourceIndex = stepIds[sourceRow].indexOf(stepId);
  const targetIndex = orderedTargetIds.indexOf(stepId);
  const expectedTargetIds = [...stepIds[targetRow], stepId];

  if (sourceIndex < 0 || targetIndex < 0) return null;
  if (orderedTargetIds.length !== expectedTargetIds.length) return null;
  if (new Set(orderedTargetIds).size !== orderedTargetIds.length) return null;
  if (!expectedTargetIds.every((id) => orderedTargetIds.includes(id))) return null;

  const nextStepIds = stepIds.map((ids) => [...ids]);
  nextStepIds[sourceRow].splice(sourceIndex, 1);
  nextStepIds[targetRow] = [...orderedTargetIds];

  /** @type {Record<string, any[][]>} */
  const nextMatrices = {};

  for (const [name, matrix] of Object.entries(matrices)) {
    const next = matrix.map((values) => [...values]);

    if (sourceIndex >= next[sourceRow].length) return null;

    const [value] = next[sourceRow].splice(sourceIndex, 1);
    next[targetRow].splice(targetIndex, 0, value);
    nextMatrices[name] = next;
  }

  return { matrices: nextMatrices, stepIds: nextStepIds };
}

/**
 * Copies one aligned entry into a target row. `orderedTargetIds` is the move-style
 * preview emitted by the drag library; the source id marks the requested drop slot.
 *
 * @param {Record<string, any[][]>} matrices
 * @param {string[][]} stepIds
 * @param {number} sourceRow
 * @param {number} targetRow
 * @param {string} stepId
 * @param {string[]} orderedTargetIds
 * @param {string} duplicateId
 * @param {number | null} [insertionIndexOverride]
 * @returns {{ matrices: Record<string, any[][]>, stepIds: string[][] } | null}
 */
export function duplicateStepBetweenRows(
  matrices,
  stepIds,
  sourceRow,
  targetRow,
  stepId,
  orderedTargetIds,
  duplicateId,
  insertionIndexOverride = null,
) {
  if (sourceRow < 0 || targetRow < 0 || !duplicateId) return null;
  if (!stepIds[sourceRow] || !stepIds[targetRow]) return null;
  if (stepIds.some((ids) => ids.includes(duplicateId))) return null;

  const sourceIndex = stepIds[sourceRow].indexOf(stepId);
  const previewIndex = orderedTargetIds.indexOf(stepId);

  if (sourceIndex < 0 || previewIndex < 0) return null;

  const expectedIds = sourceRow === targetRow
    ? stepIds[targetRow]
    : [...stepIds[targetRow], stepId];

  if (orderedTargetIds.length !== expectedIds.length) return null;
  if (new Set(orderedTargetIds).size !== orderedTargetIds.length) return null;
  if (!expectedIds.every((id) => orderedTargetIds.includes(id))) return null;

  const inferredInsertionIndex = sourceRow === targetRow && previewIndex > sourceIndex
    ? previewIndex + 1
    : previewIndex;
  const insertionIndex = insertionIndexOverride ?? inferredInsertionIndex;

  if (insertionIndex < 0 || insertionIndex > stepIds[targetRow].length) return null;
  const nextStepIds = stepIds.map((ids) => [...ids]);
  nextStepIds[targetRow].splice(insertionIndex, 0, duplicateId);

  /** @type {Record<string, any[][]>} */
  const nextMatrices = {};

  for (const [name, matrix] of Object.entries(matrices)) {
    const next = matrix.map((values) => [...values]);

    if (sourceIndex >= next[sourceRow].length) return null;

    next[targetRow].splice(insertionIndex, 0, next[sourceRow][sourceIndex]);
    nextMatrices[name] = next;
  }

  return { matrices: nextMatrices, stepIds: nextStepIds };
}
