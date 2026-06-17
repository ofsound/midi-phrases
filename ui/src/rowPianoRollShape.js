/**
 * @typedef {{ step: number, startQuarters: number, lengthQuarters: number }} RowRollStepSlot
 */

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
 * @param {number} xPx
 * @param {RowRollStepSlot[]} slots
 * @param {number} pxPerQuarter
 */
export function stepAtRollX(xPx, slots, pxPerQuarter) {
  if (slots.length === 0 || pxPerQuarter <= 0) return -1;

  const quarters = xPx / pxPerQuarter;

  for (let index = 0; index < slots.length; index += 1) {
    const slot = slots[index];
    const endQuarters = slot.startQuarters + slot.lengthQuarters;

    if (quarters < endQuarters) return slot.step;
  }

  return slots[slots.length - 1].step;
}

/**
 * @param {RowRollStepSlot} slot
 * @param {number} pxPerQuarter
 */
export function stepSlotCenterXPx(slot, pxPerQuarter) {
  return (slot.startQuarters + slot.lengthQuarters / 2) * pxPerQuarter;
}

/**
 * Map a drawn stroke to per-step MIDI updates for the horizontal span it covers.
 *
 * @param {{ x: number, y: number }[]} points - x/y in roll pixel coordinates
 * @param {RowRollStepSlot[]} slots
 * @param {number} pxPerQuarter
 * @param {number} rowHeightPx
 * @param {number} maxMidi
 * @returns {{ step: number, midi: number }[]}
 */
export function shapeNoteUpdatesFromStroke(points, slots, pxPerQuarter, rowHeightPx, maxMidi) {
  if (points.length === 0 || slots.length === 0 || pxPerQuarter <= 0 || rowHeightPx <= 0) {
    return [];
  }

  const minX = Math.min(points[0].x, points[points.length - 1].x);
  const maxX = Math.max(points[0].x, points[points.length - 1].x);
  const firstStep = Math.max(0, stepAtRollX(minX, slots, pxPerQuarter));
  const lastStep = Math.min(slots.length - 1, stepAtRollX(maxX, slots, pxPerQuarter));
  /** @type {{ step: number, midi: number }[]} */
  const updates = [];

  for (let step = firstStep; step <= lastStep; step += 1) {
    const slot = slots[step];
    const centerX = stepSlotCenterXPx(slot, pxPerQuarter);
    const y = interpolateShapeYAtX(points, centerX);

    if (y === null) continue;

    updates.push({
      step,
      midi: midiFromRollY(y, rowHeightPx, maxMidi),
    });
  }

  return updates;
}

export const quartersPerMeasure = 4;

/**
 * @param {number} lengthQuarters
 */
export function rollLengthQuartersForCycle(lengthQuarters) {
  return Math.max(quartersPerMeasure, Math.ceil(lengthQuarters / quartersPerMeasure) * quartersPerMeasure);
}

/**
 * @param {number} lengthQuarters
 */
export function measureLineQuarters(lengthQuarters) {
  const paddedLength = rollLengthQuartersForCycle(lengthQuarters);

  return Array.from({ length: paddedLength / quartersPerMeasure + 1 }, (_, measure) => measure * quartersPerMeasure);
}

/**
 * @param {number} lengthQuarters
 */
export function beatLineQuarters(lengthQuarters) {
  const paddedLength = rollLengthQuartersForCycle(lengthQuarters);

  return Array.from({ length: paddedLength + 1 }, (_, quarter) => quarter);
}
