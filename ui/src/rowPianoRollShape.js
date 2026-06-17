/**
 * @param {number} yPx
 * @param {number} rowHeightPx
 * @param {number} maxMidi
 */
export function midiFromRollY(yPx, rowHeightPx, maxMidi) {
  const row = Math.round(yPx / rowHeightPx);

  return Math.min(127, Math.max(0, maxMidi - row));
}

/**
 * @param {{ x: number, y: number }[]} points
 * @param {number} targetXPx
 * @returns {number | null}
 */
export function interpolateShapeYAtX(points, targetXPx) {
  if (points.length === 0) return null;
  if (points.length === 1) return points[0].y;

  const first = points[0];
  if (targetXPx <= first.x) return first.y;

  const last = points[points.length - 1];
  if (targetXPx >= last.x) return last.y;

  for (let index = 1; index < points.length; index += 1) {
    const previous = points[index - 1];
    const current = points[index];

    if (targetXPx >= previous.x && targetXPx <= current.x) {
      const span = current.x - previous.x;

      if (span <= 0) return current.y;

      const t = (targetXPx - previous.x) / span;
      return previous.y + t * (current.y - previous.y);
    }
  }

  return last.y;
}

/**
 * Map a drawn stroke to per-step MIDI updates for the horizontal span it covers.
 *
 * @param {{ x: number, y: number }[]} points - x/y in roll pixel coordinates
 * @param {number} stepCount
 * @param {number} pxPerStep
 * @param {number} rowHeightPx
 * @param {number} maxMidi
 * @returns {{ step: number, midi: number }[]}
 */
export function shapeNoteUpdatesFromStroke(points, stepCount, pxPerStep, rowHeightPx, maxMidi) {
  if (points.length === 0 || stepCount <= 0 || pxPerStep <= 0 || rowHeightPx <= 0) {
    return [];
  }

  const minX = Math.min(points[0].x, points[points.length - 1].x);
  const maxX = Math.max(points[0].x, points[points.length - 1].x);
  const firstStep = Math.max(0, Math.floor(minX / pxPerStep));
  const lastStep = Math.min(stepCount - 1, Math.floor(maxX / pxPerStep));
  /** @type {{ step: number, midi: number }[]} */
  const updates = [];

  for (let step = firstStep; step <= lastStep; step += 1) {
    const centerX = (step + 0.5) * pxPerStep;
    const y = interpolateShapeYAtX(points, centerX);

    if (y === null) continue;

    updates.push({
      step,
      midi: midiFromRollY(y, rowHeightPx, maxMidi),
    });
  }

  return updates;
}
