/**
 * Map a pointer position across a step's interior duration rail to 0...1.
 *
 * @param {number} clientX
 * @param {number} railLeftPx
 * @param {number} railWidthPx
 */
export function durationFractionFromRailX(clientX, railLeftPx, railWidthPx) {
  if (!Number.isFinite(railWidthPx) || railWidthPx <= 0) return 0;

  return Math.min(1, Math.max(0, (clientX - railLeftPx) / railWidthPx));
}
