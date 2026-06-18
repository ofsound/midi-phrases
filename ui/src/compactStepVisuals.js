export const compactStepMinimumVelocityOpacity = 0.22;
export const compactStepSkippedOpacity = 0.12;

/**
 * Accent-layer opacity for a compact step. The non-zero floor keeps velocity-zero
 * steps visible, while skipped steps use a separate, unmistakably faint state.
 *
 * @param {number} velocity
 * @param {boolean} [skipped]
 */
export function compactStepVelocityOpacity(velocity, skipped = false) {
  if (skipped) return compactStepSkippedOpacity;

  const normalizedVelocity = Math.min(127, Math.max(0, Number(velocity) || 0)) / 127;

  return (
    compactStepMinimumVelocityOpacity +
    (1 - compactStepMinimumVelocityOpacity) * normalizedVelocity
  );
}
