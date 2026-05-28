import { timingMultiplierAtIndex, timingOffsetValues } from "./stepCellLayout.js";

export const DEFAULT_PREVIEW_LENGTH_QUARTERS = 300;

const EPSILON = 1e-9;

/**
 * @typedef {{ start: number, end: number, midi: number, velocity: number, row: number }} ScheduledNote
 */

/** @param {number} offsetIndex */
export function rowTimingOffsetQuarters(offsetIndex) {
  return timingOffsetValues[offsetIndex] ?? 0;
}

/**
 * @param {number[]} timingMultiplierIndices
 */
export function rowStepLayout(timingMultiplierIndices) {
  /** @type {number[]} */
  const stepStartQuarters = [];
  /** @type {number[]} */
  const stepLengthQuarters = [];
  let cycleLengthQuarters = 0;

  for (const index of timingMultiplierIndices) {
    stepStartQuarters.push(cycleLengthQuarters);
    const length = timingMultiplierAtIndex(index);
    stepLengthQuarters.push(length);
    cycleLengthQuarters += length;
  }

  return { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters };
}

/**
 * Deterministic MIDI schedule mirroring PluginProcessor::processBlock trigger logic.
 *
 * @param {object} params
 * @param {number[][]} params.notes
 * @param {boolean[]} params.rowMuted
 * @param {number[]} params.rowTimingOffset
 * @param {number[][]} params.stepDurationFraction
 * @param {number[][]} params.stepTimingMultiplier
 * @param {number[][]} params.stepVelocity
 * @param {number} [params.lengthQuarters]
 * @returns {ScheduledNote[]}
 */
export function buildPhraseSchedule({
  notes,
  rowMuted,
  rowTimingOffset,
  stepDurationFraction,
  stepTimingMultiplier,
  stepVelocity,
  lengthQuarters = DEFAULT_PREVIEW_LENGTH_QUARTERS,
}) {
  const ppqStart = 0;
  const ppqEnd = lengthQuarters;
  /** @type {ScheduledNote[]} */
  const scheduled = [];

  for (let row = 0; row < notes.length; row += 1) {
    if (rowMuted[row]) continue;

    const rowNotes = notes[row];
    const stepCount = rowNotes.length;

    if (stepCount <= 0) continue;

    const { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters } = rowStepLayout(
      stepTimingMultiplier[row],
    );

    if (cycleLengthQuarters <= 0) continue;

    const offset = rowTimingOffsetQuarters(rowTimingOffset[row]);

    /** @type {{ ppq: number, step: number }[]} */
    const triggers = [];

    for (let step = 0; step < stepCount; step += 1) {
      const stepStartInCycle = stepStartQuarters[step];
      const nMin = Math.ceil(
        (ppqStart - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters,
      );
      const nMax = Math.floor(
        (ppqEnd - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters,
      );

      for (let cycle = nMin; cycle <= nMax; cycle += 1) {
        const triggerPpq = cycle * cycleLengthQuarters + stepStartInCycle + offset;

        if (triggerPpq < ppqStart - EPSILON || triggerPpq >= ppqEnd + EPSILON) continue;

        triggers.push({ ppq: triggerPpq, step });
      }
    }

    if (triggers.length === 0) continue;

    triggers.sort((a, b) => a.ppq - b.ppq);

    let lastTrigger = -Infinity;
    /** @type {ScheduledNote | null} */
    let activeNote = null;

    /** @param {number} endTime */
    const flushActive = (endTime) => {
      if (!activeNote) return;

      if (activeNote.start >= ppqEnd - EPSILON) {
        activeNote = null;
        return;
      }

      const clippedEnd = Math.min(endTime, ppqEnd);

      if (clippedEnd > activeNote.start + EPSILON) {
        scheduled.push({
          ...activeNote,
          end: clippedEnd,
        });
      }

      activeNote = null;
    };

    for (const trigger of triggers) {
      const triggerPpq = trigger.ppq;

      if (triggerPpq <= lastTrigger + EPSILON) continue;

      lastTrigger = triggerPpq;
      flushActive(triggerPpq);

      const step = trigger.step;
      const velocity = stepVelocity[row][step];

      if (velocity <= 0) continue;

      const durationFraction = stepDurationFraction[row][step];

      if (durationFraction <= 0) continue;

      const gateQuarters = stepLengthQuarters[step] * durationFraction;

      activeNote = {
        start: triggerPpq,
        end: triggerPpq + gateQuarters,
        midi: rowNotes[step],
        velocity,
        row,
      };
    }

    flushActive(ppqEnd);
  }

  return scheduled.sort((a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row);
}

/** @param {ScheduledNote[]} scheduled @param {number} [paddingSemitones] */
export function pitchRangeForSchedule(scheduled, paddingSemitones = 2) {
  if (scheduled.length === 0) {
    return { minMidi: 48, maxMidi: 72 };
  }

  let minMidi = scheduled[0].midi;
  let maxMidi = scheduled[0].midi;

  for (const note of scheduled) {
    minMidi = Math.min(minMidi, note.midi);
    maxMidi = Math.max(maxMidi, note.midi);
  }

  minMidi = Math.max(0, minMidi - paddingSemitones);
  maxMidi = Math.min(127, maxMidi + paddingSemitones);

  minMidi -= minMidi % 12;
  maxMidi += 11 - (maxMidi % 12);

  return { minMidi, maxMidi };
}

/** @param {number} midi */
export function isBlackKey(midi) {
  return [1, 3, 6, 8, 10].includes(midi % 12);
}
