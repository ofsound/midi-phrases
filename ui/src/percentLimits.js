/** Max value for 0–100 style percent controls (swing, humanize, probability, duration, etc.). */
export const maxPercentValue = 99;

export const maxSignedRelativePercentValue = 99;
export const minSignedRelativePercentValue = -99;

/** @param {number} value @param {{ fallback?: number }} [options] */
export function clampHundredScalePercent(value, { fallback = 0 } = {}) {
  const parsed = Number.parseInt(String(value), 10);

  return Number.isNaN(parsed) ? fallback : Math.min(maxPercentValue, Math.max(0, parsed));
}

/** @param {number} value @param {{ fallback?: number }} [options] */
export function clampSignedRelativePercent(value, { fallback = 0 } = {}) {
  const parsed = Number.parseInt(String(value), 10);

  return Number.isNaN(parsed)
    ? fallback
    : Math.min(maxSignedRelativePercentValue, Math.max(minSignedRelativePercentValue, parsed));
}
