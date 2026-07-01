export const compactStepMinimumVelocityOpacity = 0.22;
export const compactStepSkippedOpacity = 0.12;
export const durationBarMinimumVelocityOpacity = 0.2;

/**
 * Opacity for duration-bar fill and monophonic piano-roll notes.
 * Matches DurationBar: 20% floor at velocity 0, full strength at 127.
 *
 * @param {number} velocity
 */
export function durationBarFillOpacity(velocity) {
  const normalizedVelocity = Math.min(127, Math.max(0, Number(velocity) || 0)) / 127;

  return (
    durationBarMinimumVelocityOpacity +
    (1 - durationBarMinimumVelocityOpacity) * normalizedVelocity
  );
}

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

/**
 * Width percent for the read-only duration overlay in compact step cells.
 * Matches DurationBar: muted steps show no fill.
 *
 * @param {number} durationFraction
 * @param {boolean} [stepMuted]
 */
export function compactStepDurationFillPercent(durationFraction, stepMuted = false) {
  if (stepMuted) return 0;

  const raw = Number(durationFraction);
  const fraction = Number.isFinite(raw)
    ? Math.min(1, Math.max(0, raw))
    : 1;
  if (fraction <= 0) return 0;

  return fraction * 100;
}
