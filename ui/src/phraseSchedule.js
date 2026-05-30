import { defaultPulseIndex, pulseQuartersForIndex } from "./pulseLayout.js";
import { timingMultiplierAtIndex, timingOffsetValues } from "./stepCellLayout.js";

export const DEFAULT_PREVIEW_LENGTH_QUARTERS = 300;

const EPSILON = 1e-9;

/**
 * @typedef {{ start: number, end: number, midi: number, velocity: number, row: number, step: number }} ScheduledNote
 */

/** @param {number} offsetIndex @param {number} [pulseIndex] */
export function rowTimingOffsetQuarters(offsetIndex, pulseIndex = defaultPulseIndex) {
  return (timingOffsetValues[offsetIndex] ?? 0) * pulseQuartersForIndex(pulseIndex);
}

/**
 * @param {number[]} timingMultiplierIndices
 * @param {number} [pulseIndex]
 */
export function rowStepLayout(timingMultiplierIndices, pulseIndex = defaultPulseIndex) {
  const pulse = pulseQuartersForIndex(pulseIndex);
  /** @type {number[]} */
  const stepStartQuarters = [];
  /** @type {number[]} */
  const stepLengthQuarters = [];
  let cycleLengthQuarters = 0;

  for (const index of timingMultiplierIndices) {
    stepStartQuarters.push(cycleLengthQuarters);
    const length = timingMultiplierAtIndex(index) * pulse;
    stepLengthQuarters.push(length);
    cycleLengthQuarters += length;
  }

  return { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters };
}

/**
 * Step trigger position within a cycle (forward or reverse playback order).
 *
 * @param {number[]} stepStartQuarters
 * @param {number[]} stepLengthQuarters
 * @param {number} step
 * @param {boolean} reversed
 */
export function stepStartInCycleForStep(stepStartQuarters, stepLengthQuarters, step, reversed) {
  if (!reversed) {
    return stepStartQuarters[step] ?? 0;
  }

  let start = 0;

  for (let index = step + 1; index < stepLengthQuarters.length; index += 1) {
    start += stepLengthQuarters[index] ?? 0;
  }

  return start;
}

/**
 * Deterministic MIDI schedule mirroring PluginProcessor::processBlock trigger logic.
 *
 * @param {object} params
 * @param {number[][]} params.notes
 * @param {boolean[]} params.rowMuted
 * @param {boolean[]} [params.rowReversed]
 * @param {number[]} params.rowTimingOffset
 * @param {number[][]} params.stepDurationFraction
 * @param {number[][]} params.stepTimingMultiplier
 * @param {number[][]} params.stepVelocity
 * @param {number} [params.pulseIndex]
 * @param {number} [params.lengthQuarters]
 * @returns {ScheduledNote[]}
 */
export function buildPhraseSchedule({
  notes,
  rowMuted,
  rowReversed = [],
  rowTimingOffset,
  stepDurationFraction,
  stepTimingMultiplier,
  stepVelocity,
  pulseIndex = defaultPulseIndex,
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
      pulseIndex,
    );

    if (cycleLengthQuarters <= 0) continue;

    const offset = rowTimingOffsetQuarters(rowTimingOffset[row], pulseIndex);
    const reversed = rowReversed[row] ?? false;

    /** @type {{ ppq: number, step: number }[]} */
    const triggers = [];

    for (let step = 0; step < stepCount; step += 1) {
      const stepStartInCycle = stepStartInCycleForStep(
        stepStartQuarters,
        stepLengthQuarters,
        step,
        reversed,
      );
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
        step,
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

/**
 * @param {ScheduledNote} note
 * @param {number} beat
 */
export function isScheduledNoteActiveAtBeat(note, beat) {
  if (beat < 0) return false;

  return beat >= note.start - EPSILON && beat < note.end - EPSILON;
}

/**
 * @param {object} params
 * @param {number} params.beat
 * @param {number} params.step
 * @param {number[]} params.rowNotes
 * @param {boolean} params.rowMuted
 * @param {boolean} [params.rowReversed]
 * @param {number} params.rowTimingOffset
 * @param {number[]} params.stepDurationFraction
 * @param {number[]} params.stepTimingMultiplier
 * @param {number[]} params.stepVelocity
 * @param {number} [params.pulseIndex]
 */
export function isStepActiveAtBeat({
  beat,
  step,
  rowNotes,
  rowMuted,
  rowReversed = false,
  rowTimingOffset,
  stepDurationFraction,
  stepTimingMultiplier,
  stepVelocity,
  pulseIndex = defaultPulseIndex,
}) {
  if (beat < 0 || rowMuted || step < 0 || step >= rowNotes.length) return false;
  if ((stepVelocity[step] ?? 0) <= 0 || (stepDurationFraction[step] ?? 0) <= 0) return false;

  const { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters } = rowStepLayout(
    stepTimingMultiplier,
    pulseIndex,
  );

  if (cycleLengthQuarters <= 0 || (stepLengthQuarters[step] ?? 0) <= 0) return false;

  const offset = rowTimingOffsetQuarters(rowTimingOffset, pulseIndex);
  const stepStartInCycle = stepStartInCycleForStep(
    stepStartQuarters,
    stepLengthQuarters,
    step,
    rowReversed,
  );
  const relativeBeat = beat - stepStartInCycle - offset;

  if (relativeBeat < -EPSILON) return false;

  const cycleIndex = Math.floor((relativeBeat + EPSILON) / cycleLengthQuarters);
  const triggerBeat = cycleIndex * cycleLengthQuarters + stepStartInCycle + offset;
  const gateEnd = triggerBeat + stepLengthQuarters[step] * stepDurationFraction[step];

  return beat >= triggerBeat - EPSILON && beat < gateEnd - EPSILON;
}
