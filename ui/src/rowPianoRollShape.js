import { snapMidiToScale } from "./scaleUtils.js";

/**
 * @typedef {{ step: number, startQuarters: number, lengthQuarters: number }} RowRollStepSlot
 */

/**
 * Convert vertical pitch-drag movement to an immediately scale-constrained note.
 *
 * @param {number} baseMidi
 * @param {number} deltaRows
 * @param {number} scaleRoot
 * @param {number} scaleModeIndex
 */
export function midiFromPitchDragDelta(baseMidi, deltaRows, scaleRoot, scaleModeIndex) {
  const rawMidi = Math.min(127, Math.max(0, Math.round(baseMidi + deltaRows)));

  return snapMidiToScale(rawMidi, scaleRoot, scaleModeIndex);
}

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
 * @param {number} yPx
 * @param {number} rollHeightPx
 */
export function velocityFromRollY(yPx, rollHeightPx) {
  if (rollHeightPx <= 0) return 0;

  const percentFromTop = Math.min(1, Math.max(0, yPx / rollHeightPx));

  return Math.min(127, Math.max(0, Math.round((1 - percentFromTop) * 127)));
}

/**
 * @param {number} velocity
 * @param {number} rollHeightPx
 */
export function velocityYInRoll(velocity, rollHeightPx) {
  const clampedVelocity = Math.min(127, Math.max(0, Math.round(velocity)));

  return (1 - clampedVelocity / 127) * rollHeightPx;
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
 * Insert index for a new step from a piano-roll x position.
 * Clicks within an existing step's time span insert before that step;
 * clicks at or past the end of row content append at the phrase end.
 *
 * @param {number} xPx
 * @param {RowRollStepSlot[]} slots
 * @param {number} pxPerQuarter
 */
export function insertStepIndexFromRollX(xPx, slots, pxPerQuarter) {
  if (slots.length === 0 || pxPerQuarter <= 0) return 0;

  const quarters = Math.max(0, xPx / pxPerQuarter);
  const lastSlot = slots[slots.length - 1];
  const contentEndQuarters = lastSlot.startQuarters + lastSlot.lengthQuarters;

  if (quarters >= contentEndQuarters) return slots.length;

  for (let index = 0; index < slots.length; index += 1) {
    const slot = slots[index];
    const endQuarters = slot.startQuarters + slot.lengthQuarters;

    if (quarters < endQuarters) return slot.step;
  }

  return slots.length;
}

/**
 * @param {RowRollStepSlot} slot
 * @param {number} pxPerQuarter
 */
export function stepSlotCenterXPx(slot, pxPerQuarter) {
  return (slot.startQuarters + slot.lengthQuarters / 2) * pxPerQuarter;
}

/** Matches the mono-roll note pill label (`text-[10px]` + horizontal padding). */
export const pianoRollNoteLabelFontPx = 10;

export const pianoRollNoteLabelHorizontalPaddingPx = 8;

/** @param {string} label */
export function estimatePianoRollNoteLabelWidthPx(label) {
  return label.length * 6.5 + pianoRollNoteLabelHorizontalPaddingPx;
}

/**
 * Whether the in-note label is too small to read during a drag.
 *
 * @param {number} rowHeightPx
 * @param {number} noteWidthPx
 * @param {string} [label]
 */
export function pianoRollNoteDragTooltipVisible(rowHeightPx, noteWidthPx, label = "") {
  const noteBarHeightPx = Math.max(8, rowHeightPx - 2);

  if (noteBarHeightPx < pianoRollNoteLabelFontPx + 4) return true;
  if (noteWidthPx < 40) return true;

  return estimatePianoRollNoteLabelWidthPx(label) > noteWidthPx;
}

/**
 * Map a drawn stroke to per-step velocity updates for the horizontal span it covers.
 *
 * @param {{ x: number, y: number }[]} points - x/y in roll pixel coordinates
 * @param {RowRollStepSlot[]} slots
 * @param {number} pxPerQuarter
 * @param {number} rollHeightPx
 * @returns {{ step: number, velocity: number }[]}
 */
export function shapeVelocityUpdatesFromStroke(points, slots, pxPerQuarter, rollHeightPx) {
  if (points.length === 0 || slots.length === 0 || pxPerQuarter <= 0 || rollHeightPx <= 0) {
    return [];
  }

  const minX = Math.min(points[0].x, points[points.length - 1].x);
  const maxX = Math.max(points[0].x, points[points.length - 1].x);
  const firstStep = Math.max(0, stepAtRollX(minX, slots, pxPerQuarter));
  const lastStep = Math.min(slots.length - 1, stepAtRollX(maxX, slots, pxPerQuarter));
  /** @type {{ step: number, velocity: number }[]} */
  const updates = [];

  for (let step = firstStep; step <= lastStep; step += 1) {
    const slot = slots[step];
    const centerX = stepSlotCenterXPx(slot, pxPerQuarter);
    const y = firstStep === lastStep
      ? points[points.length - 1].y
      : interpolateShapeYAtX(points, centerX);

    if (y === null) continue;

    updates.push({
      step,
      velocity: velocityFromRollY(y, rollHeightPx),
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
