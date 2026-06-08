import { defaultPulseIndex, pulseQuartersForIndex } from "./pulseLayout.js";
import { timingMultiplierAtIndex, timingOffsetValues } from "./stepCellLayout.js";

export const DEFAULT_PREVIEW_LENGTH_QUARTERS = 300;

const EPSILON = 1e-9;
const MAX_COMBINED_PREVIEW_NOTES = 4096;
export const combinationModes = [
  { index: 0, bit: 1, label: "W", name: "Weave" },
  { index: 1, bit: 2, label: "L", name: "Logic" },
  { index: 2, bit: 4, label: "X", name: "Cross-Mod" },
  { index: 3, bit: 8, label: "E", name: "Echo" },
];
export const swingSubdivisionValues = [0.25, 0.5, 1];

/** @type {{ index: number, label: string }[]} */
export const swingSubdivisionOptions = [
  { index: 0, label: ".25" },
  { index: 1, label: ".5" },
  { index: 2, label: "1" },
];

/** @param {number} triggerCount @param {number} cycle @param {number} cycleOffset */
export function cycleGatePasses(triggerCount, cycle, cycleOffset) {
  const length = Math.max(1, Math.round(cycle));

  return triggerCount % length === Math.min(Math.max(0, Math.round(cycleOffset)), length - 1);
}

/** @param {number} step @param {number} triggerCount @param {number} probability */
export function probabilityPasses(step, triggerCount, probability) {
  const chance = Math.min(100, Math.max(0, Math.round(probability)));

  if (chance >= 100) return true;
  if (chance <= 0) return false;

  const hash = (step * 2654435761 + triggerCount * 1597334677) >>> 0;

  return hash % 100 < chance;
}

/** @param {number} mask @param {number} modeIndex */
export function combinationModeEnabled(mask, modeIndex) {
  return (mask & (1 << modeIndex)) !== 0;
}

/** @param {number} row @param {number} step @param {number} ppq */
function deterministicEventHash(row, step, ppq) {
  let value =
    ((row + 1) * 0x9e3779b9) ^
    ((step + 1) * 0x85ebca6b) ^
    (Math.round(ppq * 960) * 0xc2b2ae35);

  value >>>= 0;
  value ^= value >>> 16;
  value = Math.imul(value, 0x7feb352d);
  value ^= value >>> 15;
  value = Math.imul(value, 0x846ca68b);
  value ^= value >>> 16;

  return value >>> 0;
}

/**
 * @typedef {{ start: number, end: number, midi: number, velocity: number, row: number, step: number }} ScheduledNote
 */

/**
 * Row timing offset in quarter notes for scheduling / piano-roll preview.
 * Pulse-scaled so 0.25 matches the length of a 0.25× step at the current pulse.
 *
 * @param {number} offsetIndex
 * @param {number} [pulseIndex]
 */
export function rowTimingOffsetQuarters(offsetIndex, pulseIndex = defaultPulseIndex) {
  return (timingOffsetValues[offsetIndex] ?? 0) * pulseQuartersForIndex(pulseIndex);
}

export function swingDelayQuartersForPpq(
  ppq,
  pulseIndex = defaultPulseIndex,
  swingPercent = 0,
  swingSubdivisionIndex = 1,
) {
  const swing = Math.min(100, Math.max(0, Math.round(swingPercent)));

  if (swing <= 0) return 0;

  const subdivisionValue =
    swingSubdivisionValues[
      Math.min(swingSubdivisionValues.length - 1, Math.max(0, Math.trunc(swingSubdivisionIndex)))
    ] ?? 0.5;
  const subdivisionQuarters = pulseQuartersForIndex(pulseIndex) * subdivisionValue;

  if (subdivisionQuarters <= 0) return 0;

  const subdivisionNumber = Math.floor((ppq + EPSILON) / subdivisionQuarters);

  return subdivisionNumber % 2 !== 0 ? subdivisionQuarters * 0.5 * (swing / 100) : 0;
}

/**
 * @param {number[]} timingMultiplierIndices
 * @param {number} [pulseIndex]
 */
export function rowStepLayout(
  timingMultiplierIndices,
  pulseIndex = defaultPulseIndex,
  stepSkipped = [],
) {
  const pulse = pulseQuartersForIndex(pulseIndex);
  /** @type {number[]} */
  const stepStartQuarters = [];
  /** @type {number[]} */
  const stepLengthQuarters = [];
  let cycleLengthQuarters = 0;

  for (let step = 0; step < timingMultiplierIndices.length; step += 1) {
    const index = timingMultiplierIndices[step];
    stepStartQuarters.push(cycleLengthQuarters);
    const length = timingMultiplierAtIndex(index) * pulse;
    stepLengthQuarters.push(length);

    if (!stepSkipped[step]) {
      cycleLengthQuarters += length;
    }
  }

  return { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters };
}

/**
 * Step trigger position within a cycle.
 *
 * @param {number[]} stepStartQuarters
 * @param {number} step
 */
export function stepStartInCycleForStep(stepStartQuarters, step) {
  return stepStartQuarters[step] ?? 0;
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
 * @param {boolean[][]} [params.stepMuted]
 * @param {boolean[][]} [params.stepSkipped]
 * @param {number[][]} [params.stepProbability]
 * @param {number[][]} [params.stepCycle]
 * @param {number[][]} [params.stepCycleOffset]
 * @param {number} [params.pulseIndex]
 * @param {number} [params.swingPercent]
 * @param {number} [params.swingSubdivisionIndex]
 * @param {number} [params.combinationModeMask]
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
  stepMuted = [],
  stepSkipped = [],
  stepProbability = [],
  stepCycle = [],
  stepCycleOffset = [],
  pulseIndex = defaultPulseIndex,
  swingPercent = 0,
  swingSubdivisionIndex = 1,
  combinationModeMask = 0,
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

    const rowSkipped = stepSkipped[row] ?? [];
    const rowProbability = stepProbability[row] ?? [];
    const rowCycle = stepCycle[row] ?? [];
    const rowCycleOffset = stepCycleOffset[row] ?? [];
    const { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters } = rowStepLayout(
      stepTimingMultiplier[row],
      pulseIndex,
      rowSkipped,
    );

    if (cycleLengthQuarters <= 0) continue;

    const offset = rowTimingOffsetQuarters(rowTimingOffset[row], pulseIndex);
    /** @type {{ ppq: number, step: number }[]} */
    const triggers = [];
    /** @type {number[]} */
    const stepTriggerCounts = [];

    for (let step = 0; step < stepCount; step += 1) {
      if (rowSkipped[step]) continue;

      const stepStartInCycle = stepStartInCycleForStep(stepStartQuarters, step);
      const nMin = Math.ceil(
        (ppqStart - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters,
      );
      const nMax = Math.floor(
        (ppqEnd - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters,
      );

      for (let cycleIndex = nMin; cycleIndex <= nMax; cycleIndex += 1) {
        const triggerPpq = cycleIndex * cycleLengthQuarters + stepStartInCycle + offset;

        if (triggerPpq < ppqStart - EPSILON || triggerPpq >= ppqEnd + EPSILON) continue;

        const triggerCount = stepTriggerCounts[step] ?? 0;
        stepTriggerCounts[step] = triggerCount + 1;

        const stepCycleLength = Math.max(1, rowCycle[step] ?? 1);
        const stepCyclePhase = Math.min(
          Math.max(0, rowCycleOffset[step] ?? 0),
          stepCycleLength - 1,
        );

        if (!cycleGatePasses(triggerCount, stepCycleLength, stepCyclePhase)) continue;

        const probability = rowProbability[step] ?? 100;

        if (!probabilityPasses(step, triggerCount, probability)) continue;

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

      const clippedEnd = Math.min(activeNote.end, endTime, ppqEnd);

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

      const step = trigger.step;
      const velocity = stepVelocity[row][step];
      const rowStepMuted = stepMuted[row] ?? [];
      const isStepMuted = rowStepMuted[step] ?? false;

      if (velocity <= 0 && !isStepMuted) continue;

      const durationFraction = stepDurationFraction[row][step];

      if (durationFraction <= 0) continue;

      const gateQuarters = stepLengthQuarters[step] * durationFraction;
      const noteStart =
        triggerPpq +
        swingDelayQuartersForPpq(triggerPpq, pulseIndex, swingPercent, swingSubdivisionIndex);

      flushActive(noteStart);

      activeNote = {
        start: noteStart,
        end: noteStart + gateQuarters,
        midi: rowNotes[step],
        velocity: isStepMuted ? 0 : velocity,
        row,
        step,
      };
    }

    flushActive(ppqEnd);
  }

  const sorted = scheduled.sort((a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row);

  return applyCombinationModes({
    scheduled: sorted,
    notes,
    rowMuted,
    stepTimingMultiplier,
    stepVelocity,
    stepDurationFraction,
    stepMuted,
    stepSkipped,
    pulseIndex,
    combinationModeMask,
    lengthQuarters,
  });
}

/** @param {ScheduledNote[]} events */
function groupByStart(events) {
  /** @type {ScheduledNote[][]} */
  const groups = [];

  for (const event of events) {
    const group = groups[groups.length - 1];

    if (group && Math.abs(group[0].start - event.start) <= EPSILON) {
      group.push(event);
    } else {
      groups.push([event]);
    }
  }

  return groups;
}

/**
 * @param {object} params
 * @param {ScheduledNote[]} params.scheduled
 * @param {number[][]} params.notes
 * @param {boolean[]} params.rowMuted
 * @param {number[][]} params.stepTimingMultiplier
 * @param {number[][]} params.stepVelocity
 * @param {number[][]} params.stepDurationFraction
 * @param {boolean[][]} params.stepMuted
 * @param {boolean[][]} params.stepSkipped
 * @param {number} params.pulseIndex
 * @param {number} params.combinationModeMask
 * @param {number} params.lengthQuarters
 * @returns {ScheduledNote[]}
 */
function applyCombinationModes({
  scheduled,
  notes,
  rowMuted,
  stepTimingMultiplier,
  stepVelocity,
  stepDurationFraction,
  stepMuted,
  stepSkipped,
  pulseIndex,
  combinationModeMask,
  lengthQuarters,
}) {
  if ((combinationModeMask & 0xf) === 0 || scheduled.length === 0) return scheduled;

  let events = scheduled.map((event) => ({ ...event }));
  const activeRows = notes
    .map((rowNotes, row) => ({ row, rowNotes }))
    .filter(({ row, rowNotes }) => !rowMuted[row] && rowNotes.length > 0)
    .map(({ row }) => row);

  if (activeRows.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 1)) {
    events = groupByStart(events)
      .filter((group) => group.length === 1)
      .map((group) => group[0]);
  }

  if (events.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 2) && activeRows.length > 1) {
    events = events.map((event) => {
      const activeIndex = Math.max(0, activeRows.indexOf(event.row));
      const pitchRow = activeRows[(activeIndex + 1) % activeRows.length];
      const velocityRow = activeRows[(activeIndex + 2) % activeRows.length];
      const durationRow = activeRows[(activeIndex + 3) % activeRows.length];
      const pitchStep = event.step % Math.max(1, notes[pitchRow]?.length ?? 1);
      const velocityStep = event.step % Math.max(1, stepVelocity[velocityRow]?.length ?? 1);
      const durationStep = event.step % Math.max(1, stepDurationFraction[durationRow]?.length ?? 1);
      const pitchBase = notes[pitchRow]?.[0] ?? 60;
      const interval = (notes[pitchRow]?.[pitchStep] ?? pitchBase) - pitchBase;
      const layout = rowStepLayout(
        stepTimingMultiplier[durationRow] ?? [],
        pulseIndex,
        stepSkipped[durationRow] ?? [],
      );
      const duration =
        (layout.stepLengthQuarters[durationStep] ?? event.end - event.start) *
        (stepDurationFraction[durationRow]?.[durationStep] ?? 1);

      return {
        ...event,
        midi: Math.min(127, Math.max(0, event.midi + interval)),
        velocity: Math.min(127, Math.max(1, stepVelocity[velocityRow]?.[velocityStep] ?? event.velocity)),
        end: event.start + (duration > EPSILON ? duration : event.end - event.start),
      };
    });
  }

  if (combinationModeEnabled(combinationModeMask, 3) && activeRows.length > 1) {
    /** @type {ScheduledNote[]} */
    const multiplied = [];

    for (const event of events) {
      const activeIndex = Math.max(0, activeRows.indexOf(event.row));
      const modRow = activeRows[(activeIndex + 1) % activeRows.length];
      const modNotes = notes[modRow] ?? [];
      const modBase = modNotes[0] ?? 60;
      const modLayout = rowStepLayout(
        stepTimingMultiplier[modRow] ?? [],
        pulseIndex,
        stepSkipped[modRow] ?? [],
      );

      for (let modStep = 0; modStep < modNotes.length; modStep += 1) {
        if (multiplied.length >= MAX_COMBINED_PREVIEW_NOTES) break;
        if (stepSkipped[modRow]?.[modStep] || stepMuted[modRow]?.[modStep]) continue;
        if ((stepVelocity[modRow]?.[modStep] ?? 0) <= 0) continue;

        const start = event.start + (modLayout.stepStartQuarters[modStep] ?? 0);

        if (start < -EPSILON || start >= lengthQuarters - EPSILON) continue;

        const modDuration =
          (modLayout.stepLengthQuarters[modStep] ?? event.end - event.start) *
          (stepDurationFraction[modRow]?.[modStep] ?? 1);
        const duration = Math.min(event.end - event.start, modDuration);

        if (duration <= EPSILON) continue;

        multiplied.push({
          ...event,
          start,
          end: start + duration,
          midi: Math.min(127, Math.max(0, event.midi + modNotes[modStep] - modBase)),
          velocity: Math.round((event.velocity + (stepVelocity[modRow]?.[modStep] ?? event.velocity)) / 2),
          step: modStep,
        });
      }
    }

    events = multiplied;
  }

  if (events.length === 0) return [];

  events = events.sort((a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row);

  if (combinationModeEnabled(combinationModeMask, 0)) {
    events = groupByStart(events).map((group) => {
      if (group.length === 1) return group[0];

      const totalWeight = group.reduce((total, event) => total + Math.max(1, event.velocity), 0);
      let pick = deterministicEventHash(group[0].row, group[0].step, group[0].start) % Math.max(1, totalWeight);

      for (const event of group) {
        pick -= Math.max(1, event.velocity);

        if (pick < 0) return event;
      }

      return group[group.length - 1];
    });
  }

  return events.sort((a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row);
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
 * @param {number} params.rowTimingOffset
 * @param {number[]} params.stepDurationFraction
 * @param {number[]} params.stepTimingMultiplier
 * @param {number[]} params.stepVelocity
 * @param {boolean[]} [params.stepMuted]
 * @param {boolean[]} [params.stepSkipped]
 * @param {number} [params.stepProbability]
 * @param {number} [params.stepCycle]
 * @param {number} [params.stepCycleOffset]
 * @param {number} [params.pulseIndex]
 * @param {number} [params.swingPercent]
 * @param {number} [params.swingSubdivisionIndex]
 */
export function isStepActiveAtBeat({
  beat,
  step,
  rowNotes,
  rowMuted,
  rowTimingOffset,
  stepDurationFraction,
  stepTimingMultiplier,
  stepVelocity,
  stepMuted = [],
  stepSkipped = [],
  stepProbability = [],
  stepCycle = [],
  stepCycleOffset = [],
  pulseIndex = defaultPulseIndex,
  swingPercent = 0,
  swingSubdivisionIndex = 1,
}) {
  if (beat < 0 || rowMuted || step < 0 || step >= rowNotes.length) return false;
  if (
    stepSkipped[step] ||
    stepMuted[step] ||
    (stepVelocity[step] ?? 0) <= 0 ||
    (stepDurationFraction[step] ?? 0) <= 0
  )
    return false;

  const cycle = Math.max(1, stepCycle[step] ?? 1);
  const cycleOffset = Math.min(Math.max(0, stepCycleOffset[step] ?? 0), cycle - 1);
  const probability = stepProbability[step] ?? 100;

  const { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters } = rowStepLayout(
    stepTimingMultiplier,
    pulseIndex,
    stepSkipped,
  );

  if (cycleLengthQuarters <= 0 || (stepLengthQuarters[step] ?? 0) <= 0) return false;

  const rowOffsetQuarters = rowTimingOffsetQuarters(rowTimingOffset, pulseIndex);
  const stepStartInCycle = stepStartInCycleForStep(stepStartQuarters, step);
  const relativeBeat = beat - stepStartInCycle - rowOffsetQuarters;

  if (relativeBeat < -EPSILON) return false;

  const cycleIndex = Math.floor((relativeBeat + EPSILON) / cycleLengthQuarters);

  if (cycle > 1 && !cycleGatePasses(cycleIndex, cycle, cycleOffset)) return false;

  if (!probabilityPasses(step, cycleIndex, probability)) return false;

  const triggerBeat = cycleIndex * cycleLengthQuarters + stepStartInCycle + rowOffsetQuarters;
  const noteStart =
    triggerBeat +
    swingDelayQuartersForPpq(triggerBeat, pulseIndex, swingPercent, swingSubdivisionIndex);
  const gateEnd = triggerBeat + stepLengthQuarters[step] * stepDurationFraction[step];

  return beat >= noteStart - EPSILON && beat < gateEnd - triggerBeat + noteStart - EPSILON;
}
