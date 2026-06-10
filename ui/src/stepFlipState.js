/**
 * Pure flip-override state for phrase step cards.
 *
 * Global back-view shows every step's settings face by default; overrides mark
 * steps flipped the other way. Overrides must be cleared when the global toggle
 * or step fingerprint changes so stale keys cannot resurface.
 */

/**
 * @param {boolean} globalStepBackView
 * @param {string} fingerprint
 * @param {number} step
 */
export function flipOverrideKey(globalStepBackView, fingerprint, step) {
  const mode = globalStepBackView ? "back" : "front";

  return `${mode}:${fingerprint}:${step}`;
}

/**
 * @param {Set<string>} flipOverrides
 */
export function clearFlipOverrides(flipOverrides) {
  flipOverrides.clear();
}

/**
 * @param {number} command
 * @param {string} fingerprint
 */
export function flipOverrideSyncToken(command, fingerprint) {
  return `${command}:${fingerprint}`;
}

/**
 * @param {number} command
 * @param {string} fingerprint
 * @param {number} lastCommand
 * @param {string} lastFingerprint
 */
export function shouldResetFlipOverrides(command, fingerprint, lastCommand, lastFingerprint) {
  return command !== lastCommand || fingerprint !== lastFingerprint;
}

/**
 * @param {boolean} globalStepBackView
 * @param {Set<string>} flipOverrides
 * @param {string} fingerprint
 * @param {number} step
 */
export function isStepFlipped(globalStepBackView, flipOverrides, fingerprint, step) {
  const hasOverride = flipOverrides.has(flipOverrideKey(globalStepBackView, fingerprint, step));

  return globalStepBackView ? !hasOverride : hasOverride;
}

/**
 * @param {Set<string>} flipOverrides
 * @param {boolean} globalStepBackView
 * @param {string} fingerprint
 * @param {number} step
 * @param {boolean} flipped
 * @param {number} stepCount
 */
export function applyStepFlipChange(
  flipOverrides,
  globalStepBackView,
  fingerprint,
  step,
  flipped,
  stepCount,
) {
  if (step < 0 || step >= stepCount) {
    return;
  }

  const key = flipOverrideKey(globalStepBackView, fingerprint, step);

  if (globalStepBackView) {
    if (flipped) {
      flipOverrides.delete(key);
    } else {
      flipOverrides.add(key);
    }
  } else if (flipped) {
    flipOverrides.add(key);
  } else {
    flipOverrides.delete(key);
  }
}

/**
 * @param {Set<string>} flipOverrides
 * @param {boolean} globalStepBackView
 * @param {string} fingerprint
 * @param {number} stepCount
 */
export function closeAllStepFlips(flipOverrides, globalStepBackView, fingerprint, stepCount) {
  if (globalStepBackView) {
    for (let step = 0; step < stepCount; step += 1) {
      flipOverrides.add(flipOverrideKey(globalStepBackView, fingerprint, step));
    }
  } else {
    flipOverrides.clear();
  }
}

/**
 * @param {Set<string>} flipOverrides
 * @param {number} command
 * @param {string} fingerprint
 * @param {number} lastCommand
 * @param {string} lastFingerprint
 */
export function syncFlipOverridesForGlobalView(
  flipOverrides,
  command,
  fingerprint,
  lastCommand,
  lastFingerprint,
) {
  if (!shouldResetFlipOverrides(command, fingerprint, lastCommand, lastFingerprint)) {
    return;
  }

  clearFlipOverrides(flipOverrides);
}
