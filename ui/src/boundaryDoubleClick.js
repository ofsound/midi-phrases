/**
 * Resolve a full-size step-boundary double-click into an insertion command.
 * The source for duplication is always the step immediately left of insertStep.
 *
 * @param {{ altKey: boolean, insertStep: number, stepCount: number, maxStepCount: number }} options
 * @returns {{ type: "insert" | "duplicate", insertStep: number } | null}
 */
export function boundaryDoubleClickCommand({ altKey, insertStep, stepCount, maxStepCount }) {
  if (
    insertStep <= 0
    || insertStep > stepCount
    || stepCount >= maxStepCount
  ) {
    return null;
  }

  return {
    type: altKey ? "duplicate" : "insert",
    insertStep,
  };
}
