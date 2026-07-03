import {loopBraceSnapQuarters} from "./loopBraceLayout.js";
import {defaultPulseIndex, pulseQuartersForIndex} from "./pulseLayout.js";
import {applyNoteBandpass, defaultNoteBandpassHighMidi, defaultNoteBandpassLowMidi} from "./noteBandpass.js";
import {applyOctavizer, defaultOctavizerRelativeVelocity} from "./octavizer.js";
import {defaultStepProbabilityValue, maxPercentValue, maxStepProbabilityValue} from "./percentLimits.js";
import {applyShimmer, defaultShimmerDelayMultiplierIndex, defaultShimmerFeedbackPercent, defaultShimmerMixPercent} from "./shimmer.js";
import {applyVelocityTilt, defaultVelocityTiltAmount, defaultVelocityTiltPivotMidi} from "./velocityTilt.js";
import {applyGlobalTranspose, defaultGlobalTransposeSemitones} from "./globalTranspose.js";
import {
  defaultScaleModeIndex,
  defaultScaleRoot,
  echoNoteFromModStep,
  scaleDegreeDelta,
  transposeMidiByScaleDegrees,
} from "./scaleUtils.js";
import {cycleGatePasses} from "./cyclePattern.js";
import {timingMultiplierAtIndex, timingOffsetValues} from "./stepCellLayout.js";

export const DEFAULT_PREVIEW_LENGTH_QUARTERS = 300;

const EPSILON = 1e-9;
const MAX_COMBINED_PREVIEW_NOTES = 4096;
const COMBINATION_GESTURE_PULSE_QUARTERS_FLOOR = 2;
const HOCKET_MINIMUM_SLICE_OVERLAP_FRACTION = 0.5;
const UNISON_OVERLAP_WINDOW_QUARTERS = 1 / 96;
const DEFAULT_PREVIEW_WINDOW_LOOKBACK_QUARTERS = 64;
export const combinationModeMaskBits = 0x1db;
/** Display order matches processing order. Weave keeps its legacy bit and runs last. */
export const combinationModes = [
  {index: 0, bit: 1, icon: "crossMod", name: "Cross-Mod"},
  {index: 7, bit: 128, icon: "canon", name: "Canon"},
  {index: 8, bit: 256, icon: "retroInv", name: "Retro-Inv"},
  {index: 6, bit: 64, icon: "hocket", name: "Hocket"},
  {index: 1, bit: 2, icon: "tendril", name: "Tendril"},
  {index: 3, bit: 8, icon: "echo", name: "Echo"},
  {index: 4, bit: 16, icon: "weave", name: "Weave"},
];
export const swingSubdivisionValues = [0.25, 0.5, 1];

/** @type {{ index: number, label: string }[]} */
export const swingSubdivisionOptions = [
  {index: 0, label: ".25"},
  {index: 1, label: ".5"},
  {index: 2, label: "1"},
];

export {cycleGatePasses} from "./cyclePattern.js";

/** @param {number} step @param {number} triggerCount @param {number} probability */
export function probabilityPasses(step, triggerCount, probability) {
  const chance = Math.min(maxStepProbabilityValue, Math.max(0, Math.round(probability)));

  if (chance >= maxStepProbabilityValue) return true;
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
  let value = ((row + 1) * 0x9e3779b9) ^ ((step + 1) * 0x85ebca6b) ^ (Math.round(ppq * 960) * 0xc2b2ae35);

  value >>>= 0;
  value ^= value >>> 16;
  value = Math.imul(value, 0x7feb352d);
  value ^= value >>> 15;
  value = Math.imul(value, 0x846ca68b);
  value ^= value >>> 16;

  return value >>> 0;
}

/**
 * @param {number} ppq
 * @param {number} duration
 * @param {number} gesturePulse
 */
function isCombinationGestureAnchor(ppq, duration, gesturePulse) {
  if (gesturePulse <= EPSILON) return true;

  let phase = ppq - Math.floor((ppq + EPSILON) / gesturePulse) * gesturePulse;

  if (phase < 0) phase += gesturePulse;
  if (phase >= gesturePulse - EPSILON) phase = 0;

  const anchorWindow = Math.min(
    gesturePulse * 0.125,
    Math.max(duration * 0.25, gesturePulse * 0.0625),
  );

  return phase <= anchorWindow + EPSILON;
}

/**
 * @typedef {{ start: number, end: number, midi: number, velocity: number, row: number, step: number, channel: number }} ScheduledNote
 */

/** @param {number} channel */
function clampMidiChannel(channel) {
  return Math.min(16, Math.max(1, Math.round(channel)));
}

/** @param {number[]} rowMidiChannel @param {number} row */
function midiChannelForRow(rowMidiChannel, row) {
  return clampMidiChannel(rowMidiChannel[row] ?? row + 1);
}

function foldMidiToNearestRegister(midi, centerMidi) {
  const maxDistanceSemitones = 7;
  let best = Math.min(127, Math.max(0, Math.round(midi)));
  const center = Math.min(127, Math.max(0, Math.round(centerMidi)));
  let bestDistance = Math.abs(best - center);

  for (let octaveOffset = -10; octaveOffset <= 10; octaveOffset += 1) {
    const candidate = Math.round(midi) + octaveOffset * 12;

    if (candidate < 0 || candidate > 127) continue;

    const distance = Math.abs(candidate - center);

    if (distance < bestDistance || (distance === bestDistance && candidate < best)) {
      best = candidate;
      bestDistance = distance;
    }
  }

  if (bestDistance <= maxDistanceSemitones) return best;

  return Math.min(
    Math.min(127, center + maxDistanceSemitones),
    Math.max(Math.max(0, center - maxDistanceSemitones), best),
  );
}

/**
 * @param {ScheduledNote[]} events
 * @param {boolean} enabled
 * @returns {ScheduledNote[]}
 */
export function cleanupUnisonOverlaps(events, enabled = true) {
  if (!enabled || events.length <= 1) return events;

  const sorted = [...events].sort((a, b) => (
    a.midi - b.midi
    || a.start - b.start
    || (a.channel ?? 1) - (b.channel ?? 1)
    || a.row - b.row
    || a.step - b.step
  ));
  /** @type {ScheduledNote[]} */
  const mergedEvents = [];

  for (let index = 0; index < sorted.length;) {
    const merged = {...sorted[index]};
    let mergedEnd = merged.end;
    let next = index + 1;

    while (
      next < sorted.length
      && sorted[next].midi === merged.midi
      && sorted[next].start <= merged.start + UNISON_OVERLAP_WINDOW_QUARTERS + EPSILON
    ) {
      merged.velocity = Math.max(merged.velocity, sorted[next].velocity);
      mergedEnd = Math.max(mergedEnd, sorted[next].end);
      next += 1;
    }

    merged.end = Math.max(merged.start, mergedEnd);
    mergedEvents.push(merged);
    index = next;
  }

  return mergedEvents.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
}

/**
 * @param {ScheduledNote[]} events
 * @param {boolean} enabled
 * @returns {ScheduledNote[]}
 */
export function suppressHeldNoteRetriggers(events, enabled = true) {
  if (!enabled || events.length <= 1) return events;

  const sorted = [...events].sort((a, b) => (
    a.midi - b.midi
    || a.start - b.start
    || (a.channel ?? 1) - (b.channel ?? 1)
    || a.row - b.row
    || a.step - b.step
  ));
  /** @type {ScheduledNote[]} */
  const mergedEvents = [];

  for (let index = 0; index < sorted.length;) {
    const merged = {...sorted[index]};
    let mergedEnd = merged.end;
    let next = index + 1;

    while (
      next < sorted.length
      && sorted[next].midi === merged.midi
      && sorted[next].start < mergedEnd - EPSILON
    ) {
      mergedEnd = Math.max(mergedEnd, sorted[next].end);
      merged.extendedByHeldOverlap = true;
      next += 1;
    }

    merged.end = Math.max(merged.start, mergedEnd);
    mergedEvents.push(merged);
    index = next;
  }

  return mergedEvents.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
}

/**
 * Keep only notes that produce a note-on inside the emission window.
 * Mirrors PluginProcessor::processCombinedScheduledRange note-on gating.
 *
 * @param {ScheduledNote[]} events
 * @param {number} emitStartQuarters
 * @param {number} emitEndQuarters
 * @returns {ScheduledNote[]}
 */
export function filterScheduleForNoteOnEmission(events, emitStartQuarters, emitEndQuarters) {
  if (emitEndQuarters <= emitStartQuarters + EPSILON) return [];

  /** @type {ScheduledNote[]} */
  const emitted = [];

  for (const event of events) {
    if (event.velocity <= 0) continue;

    const gate = event.end - event.start;

    if (gate <= EPSILON) continue;

    if (event.start >= emitEndQuarters - EPSILON) continue;

    if (event.start < emitStartQuarters - EPSILON) continue;

    const clippedStart = Math.max(event.start, emitStartQuarters);
    const clippedEnd = Math.min(event.end, emitEndQuarters);

    if (clippedEnd <= clippedStart + EPSILON) continue;

    emitted.push({
      ...event,
      start: clippedStart,
      end: clippedEnd,
    });
  }

  return emitted;
}

/**
 * @param {ScheduledNote[]} events
 * @param {number} windowStart
 * @param {number} windowEnd
 * @returns {ScheduledNote[]}
 */
function clipScheduleToWindow(events, windowStart, windowEnd) {
  if (windowEnd <= windowStart + EPSILON) return [];

  return events
    .filter((note) => note.end > windowStart + EPSILON && note.start < windowEnd - EPSILON)
    .map((note) => ({
      ...note,
      start: Math.max(windowStart, note.start),
      end: Math.min(windowEnd, note.end),
    }))
    .filter((note) => note.end > note.start + EPSILON);
}

/**
 * Trim woven carrier gates to the next attack. Collision picking stays in applyCombinationModes.
 *
 * @param {ScheduledNote[]} events
 * @param {boolean} [enabled]
 * @returns {ScheduledNote[]}
 */
export function applyWeaveGateTrimming(events, enabled = true) {
  if (!enabled || events.length <= 1) return events;

  const sorted = [...events].sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
  /** @type {ScheduledNote[]} */
  const monophonic = [];

  for (let index = 0; index < sorted.length; index += 1) {
    const event = sorted[index];
    const next = sorted[index + 1];
    const end = next ? Math.min(event.end, next.start) : event.end;

    if (end > event.start + EPSILON) {
      monophonic.push({
        ...event,
        end,
      });
    }
  }

  return monophonic;
}

/**
 * Collision pick plus gate trim — mirrors the full pre-rail Weave stage for tests.
 *
 * @param {ScheduledNote[]} events
 * @param {boolean} enabled
 * @param {number} repeatLengthQuarters
 * @returns {ScheduledNote[]}
 */
export function applyWeaveMonophony(events, enabled = true, repeatLengthQuarters = 0) {
  if (!enabled || events.length <= 1) return events;

  const hashPpq = (ppq) => (
    repeatLengthQuarters > EPSILON ? positiveMod(ppq, repeatLengthQuarters) : ppq
  );

  const selected = groupByStart([...events].sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  )).map((group) => {
    if (group.length === 1) return {...group[0]};

    const totalWeight = group.reduce((total, event) => total + Math.max(1, event.velocity), 0);
    let pick = deterministicEventHash(group[0].row, group[0].step, hashPpq(group[0].start)) % Math.max(1, totalWeight);

    for (const event of group) {
      pick -= Math.max(1, event.velocity);

      if (pick < 0) return {...event};
    }

    return {...group[group.length - 1]};
  });

  return applyWeaveGateTrimming(selected, true);
}

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

export function swingDelayQuartersForPpq(ppq, pulseIndex = defaultPulseIndex, swingPercent = 0, swingSubdivisionIndex = 1) {
  const swing = Math.min(maxPercentValue, Math.max(0, Math.round(swingPercent)));

  if (swing <= 0) return 0;

  const subdivisionValue = swingSubdivisionValues[Math.min(swingSubdivisionValues.length - 1, Math.max(0, Math.trunc(swingSubdivisionIndex)))] ?? 0.5;
  const subdivisionQuarters = pulseQuartersForIndex(pulseIndex) * subdivisionValue;

  if (subdivisionQuarters <= 0) return 0;

  const subdivisionNumber = Math.floor((ppq + EPSILON) / subdivisionQuarters);

  return subdivisionNumber % 2 !== 0 ? subdivisionQuarters * 0.5 * (swing / 100) : 0;
}

/**
 * @param {number[]} timingMultiplierIndices
 * @param {number} [pulseIndex]
 */
export function rowStepLayout(timingMultiplierIndices, pulseIndex = defaultPulseIndex, stepSkipped = []) {
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

  return {stepStartQuarters, stepLengthQuarters, cycleLengthQuarters};
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
 * Zero-based occurrence count for a step at the current transport beat.
 * Mirrors the trigger-count seed used by the scheduler so cycle-pattern UI can
 * follow playback without maintaining an independent clock.
 *
 * @param {object} params
 * @param {number} params.beat
 * @param {number} params.step
 * @param {number} params.rowTimingOffset
 * @param {number[]} params.stepTimingMultiplier
 * @param {boolean[]} [params.stepSkipped]
 * @param {number} [params.pulseIndex]
 */
export function stepTriggerCountAtBeat({
  beat,
  step,
  rowTimingOffset,
  stepTimingMultiplier,
  stepSkipped = [],
  pulseIndex = defaultPulseIndex,
}) {
  if (beat < 0 || step < 0 || step >= stepTimingMultiplier.length || stepSkipped[step]) {
    return -1;
  }

  const {stepStartQuarters, cycleLengthQuarters} = rowStepLayout(
    stepTimingMultiplier,
    pulseIndex,
    stepSkipped,
  );

  if (cycleLengthQuarters <= 0) return -1;

  const rowOffsetQuarters = rowTimingOffsetQuarters(rowTimingOffset, pulseIndex);
  const stepStartInCycle = stepStartInCycleForStep(stepStartQuarters, step);
  const relativeBeat = beat - stepStartInCycle - rowOffsetQuarters;

  if (relativeBeat < -EPSILON) return -1;

  const firstGlobalTrigger = Math.ceil(
    (0 - stepStartInCycle - rowOffsetQuarters - EPSILON) / cycleLengthQuarters,
  );
  const cycleIndex = Math.floor((relativeBeat + EPSILON) / cycleLengthQuarters);
  const triggerCount = cycleIndex - firstGlobalTrigger;

  return triggerCount < 0 ? -1 : triggerCount;
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
 * @param {number[]} [params.rowMidiChannel]
 * @param {boolean[][]} [params.stepMuted]
 * @param {boolean[][]} [params.stepSkipped]
 * @param {number[][]} [params.stepProbability]
 * @param {number[][]} [params.stepCycle]
 * @param {number[][]} [params.stepCycleMask]
 * @param {number[][]} [params.stepCycleOffset] - Legacy alias for stepCycleMask.
 * @param {number} [params.pulseIndex]
 * @param {number} [params.swingPercent]
 * @param {number} [params.swingSubdivisionIndex]
 * @param {number} [params.combinationModeMask]
 * @param {number} [params.lengthQuarters]
 * @param {number} [params.scaleRoot]
 * @param {number} [params.scaleModeIndex]
 * @param {number} [params.noteBandpassLowMidi]
 * @param {number} [params.noteBandpassHighMidi]
 * @param {boolean} [params.octavizerDown8vaEnabled]
 * @param {boolean} [params.octavizerUp8vaEnabled]
 * @param {number} [params.octavizerDown8vaRelativeVelocity]
 * @param {number} [params.octavizerUp8vaRelativeVelocity]
 * @param {boolean} [params.shimmerEnabled]
 * @param {number} [params.shimmerDelayMultiplierIndex]
 * @param {number} [params.shimmerFeedbackPercent]
 * @param {number} [params.shimmerMixPercent]
 * @returns {ScheduledNote[]}
 */
/**
 * @param {number | undefined} start
 * @param {number | undefined} end
 */
export function loopOutputBoundsActive(start, end) {
  return start !== undefined
    && end !== undefined
    && end > start + EPSILON;
}

function buildPhraseScheduleCore({
  notes,
  rowMuted,
  rowTimingOffset,
  stepDurationFraction,
  stepTimingMultiplier,
  stepVelocity,
  rowMidiChannel = [],
  stepMuted = [],
  stepSkipped = [],
  stepProbability = [],
  stepCycle = [],
  stepCycleMask = [],
  stepCycleOffset = [],
  pulseIndex = defaultPulseIndex,
  swingPercent = 0,
  swingSubdivisionIndex = 1,
  combinationModeMask = 0,
  lengthQuarters = DEFAULT_PREVIEW_LENGTH_QUARTERS,
  scaleRoot = defaultScaleRoot,
  scaleModeIndex = defaultScaleModeIndex,
  octavizerDown8vaEnabled = false,
  octavizerUp8vaEnabled = false,
  octavizerDown8vaRelativeVelocity = defaultOctavizerRelativeVelocity,
  octavizerUp8vaRelativeVelocity = defaultOctavizerRelativeVelocity,
  shimmerEnabled = false,
  shimmerDelayMultiplierIndex = defaultShimmerDelayMultiplierIndex,
  shimmerFeedbackPercent = defaultShimmerFeedbackPercent,
  shimmerMixPercent = defaultShimmerMixPercent,
  scheduleStartQuarters = 0,
  loopOutputStartQuarters,
  loopOutputEndQuarters,
}) {
  const ppqStart = Math.max(0, scheduleStartQuarters);
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
    const rowCycleMask = stepCycleMask[row] ?? stepCycleOffset[row] ?? [];
    const {stepStartQuarters, stepLengthQuarters, cycleLengthQuarters} = rowStepLayout(stepTimingMultiplier[row], pulseIndex, rowSkipped);

    if (cycleLengthQuarters <= 0) continue;

    const offset = rowTimingOffsetQuarters(rowTimingOffset[row], pulseIndex);
    /** @type {{ ppq: number, step: number }[]} */
    const triggers = [];
    /** @type {number[]} */
    const stepTriggerCounts = [];

    for (let step = 0; step < stepCount; step += 1) {
      if (rowSkipped[step]) continue;

      const stepStartInCycle = stepStartInCycleForStep(stepStartQuarters, step);
      const nMin = Math.ceil((ppqStart - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters);
      const nMax = Math.floor((ppqEnd - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters);
      const firstGlobalTrigger = Math.ceil((0 - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters);
      const previousTrigger = Math.floor((ppqStart - stepStartInCycle - offset - EPSILON) / cycleLengthQuarters);

      stepTriggerCounts[step] = Math.max(0, previousTrigger - firstGlobalTrigger + 1);

      for (let cycleIndex = nMin; cycleIndex <= nMax; cycleIndex += 1) {
        const triggerPpq = cycleIndex * cycleLengthQuarters + stepStartInCycle + offset;

        if (triggerPpq < ppqStart - EPSILON || triggerPpq >= ppqEnd + EPSILON) continue;

        const triggerCount = stepTriggerCounts[step] ?? 0;
        stepTriggerCounts[step] = triggerCount + 1;

        const stepCycleLength = Math.max(1, rowCycle[step] ?? 1);
        const stepCyclePatternMask = rowCycleMask[step] ?? 1;

        if (!cycleGatePasses(triggerCount, stepCycleLength, stepCyclePatternMask)) continue;

        const probability = rowProbability[step] ?? defaultStepProbabilityValue;

        if (!probabilityPasses(step, triggerCount, probability)) continue;

        triggers.push({ppq: triggerPpq, step});
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

      if (velocity <= 0 || isStepMuted) continue;

      const durationFraction = stepDurationFraction[row][step];

      if (durationFraction <= 0) continue;

      const gateQuarters = stepLengthQuarters[step] * durationFraction;
      const noteStart = triggerPpq + swingDelayQuartersForPpq(triggerPpq, pulseIndex, swingPercent, swingSubdivisionIndex);

      flushActive(noteStart);

      activeNote = {
        start: noteStart,
        end: noteStart + gateQuarters,
        midi: rowNotes[step],
        velocity,
        row,
        step,
        channel: midiChannelForRow(rowMidiChannel, row),
      };
    }

    flushActive(ppqEnd);
  }

  const sorted = scheduled.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );

  const combined = applyCombinationModes({
    scheduled: sorted,
    notes,
    rowMuted,
    stepTimingMultiplier,
    stepVelocity,
    rowMidiChannel,
    stepDurationFraction,
    stepMuted,
    stepSkipped,
    pulseIndex,
    combinationModeMask,
    lengthQuarters,
    scaleRoot,
    scaleModeIndex,
    loopOutputStartQuarters,
    loopOutputEndQuarters,
    scheduleStartQuarters: ppqStart,
  });

  const woven = applyWeaveGateTrimming(
    combined,
    combinationModeEnabled(combinationModeMask, 4),
  );

  const octavized = applyOctavizer(woven, {
    down8vaEnabled: octavizerDown8vaEnabled,
    up8vaEnabled: octavizerUp8vaEnabled,
    down8vaRelativeVelocity: octavizerDown8vaRelativeVelocity,
    up8vaRelativeVelocity: octavizerUp8vaRelativeVelocity,
  });

  return applyShimmer(octavized, {
    enabled: shimmerEnabled,
    delayMultiplierIndex: shimmerDelayMultiplierIndex,
    feedbackPercent: shimmerFeedbackPercent,
    mixPercent: shimmerMixPercent,
    pulseIndex,
  });
}

/**
 * Full preview schedule including note bandpass.
 * @param {Parameters<typeof buildPhraseScheduleBeforeBandpass>[0]} params
 * @returns {ScheduledNote[]}
 */
export function buildPhraseSchedule(params) {
  const loopOutputActive = loopOutputBoundsActive(
    params.loopOutputStartQuarters,
    params.loopOutputEndQuarters,
  );
  const finalTransformed = applyGlobalTranspose(
    applyVelocityTilt(
      applyNoteBandpass(
        buildPhraseScheduleBeforeBandpass(params),
        params.noteBandpassLowMidi ?? defaultNoteBandpassLowMidi,
        params.noteBandpassHighMidi ?? defaultNoteBandpassHighMidi,
      ),
      params.velocityTiltPivotMidi ?? defaultVelocityTiltPivotMidi,
      params.velocityTiltAmount ?? defaultVelocityTiltAmount,
    ),
    params.globalTransposeSemitones ?? defaultGlobalTransposeSemitones,
  );

  const postProcessed = suppressHeldNoteRetriggers(finalTransformed, true);

  if (loopOutputActive) {
    return filterScheduleForNoteOnEmission(
      postProcessed,
      params.loopOutputStartQuarters,
      params.loopOutputEndQuarters,
    );
  }

  return postProcessed;
}

/**
 * Full preview schedule for a beat window, including overlap corrections on lookback.
 *
 * @param {Parameters<typeof buildPhraseSchedule>[0] & {
 *   windowStartQuarters?: number,
 *   windowEndQuarters?: number,
 *   windowLookbackQuarters?: number,
 * }} params
 * @returns {ScheduledNote[]}
 */
export function buildPhraseScheduleWindow(params) {
  const {
    windowStartQuarters = 0,
    windowEndQuarters = params.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS,
    windowLookbackQuarters = DEFAULT_PREVIEW_WINDOW_LOOKBACK_QUARTERS,
    loopOutputStartQuarters,
    loopOutputEndQuarters,
    ...scheduleParams
  } = params;
  const loopOutputActive = loopOutputBoundsActive(loopOutputStartQuarters, loopOutputEndQuarters);

  if (loopOutputActive) {
    return buildPhraseSchedule({
      ...scheduleParams,
      loopOutputStartQuarters,
      loopOutputEndQuarters,
      lengthQuarters: loopOutputEndQuarters,
    });
  }

  const timelineEnd = Math.max(0, scheduleParams.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS);
  const windowStart = Math.min(timelineEnd, Math.max(0, windowStartQuarters));
  const windowEnd = Math.min(timelineEnd, Math.max(windowStart, windowEndQuarters));

  if (windowEnd <= windowStart) return [];

  const schedule = buildPhraseSchedule({
    ...scheduleParams,
    lengthQuarters: windowEnd,
    scheduleStartQuarters: Math.max(0, windowStart - Math.max(0, windowLookbackQuarters)),
  });

  return clipScheduleToWindow(schedule, windowStart, windowEnd);
}

/**
 * Preview schedule through shimmer/octavizer but before note bandpass.
 * Bandpass-only updates can filter this result instead of rebuilding from scratch.
 * @param {Parameters<typeof buildPhraseSchedule>[0]} params
 * @returns {ScheduledNote[]}
 */
export function buildPhraseScheduleBeforeBandpass(params) {
  const {
    noteBandpassLowMidi: _noteBandpassLowMidi,
    noteBandpassHighMidi: _noteBandpassHighMidi,
    velocityTiltPivotMidi: _velocityTiltPivotMidi,
    velocityTiltAmount: _velocityTiltAmount,
    globalTransposeSemitones: _globalTransposeSemitones,
    loopOutputStartQuarters,
    loopOutputEndQuarters,
    scheduleStartQuarters,
    ...scheduleParams
  } = params;
  const loopOutputActive = loopOutputBoundsActive(loopOutputStartQuarters, loopOutputEndQuarters);
  const outputStart = loopOutputActive ? loopOutputStartQuarters : 0;
  const outputEnd = loopOutputActive
    ? loopOutputEndQuarters
    : (scheduleParams.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS);
  const hocketModeEnabled = combinationModeEnabled(scheduleParams.combinationModeMask ?? 0, 6);
  const resolvedScheduleStart = scheduleStartQuarters ?? (
    hocketModeEnabled
      ? (loopOutputActive ? outputStart : 0)
      : (loopOutputActive ? outputStart : 0)
  );

  return buildPhraseScheduleCore({
    ...scheduleParams,
    lengthQuarters: loopOutputActive ? outputEnd : (scheduleParams.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS),
    scheduleStartQuarters: resolvedScheduleStart,
    loopOutputStartQuarters: loopOutputActive ? outputStart : undefined,
    loopOutputEndQuarters: loopOutputActive ? outputEnd : undefined,
  });
}

/**
 * Preview schedule through shimmer/octavizer before note bandpass for a beat window.
 * Output is clipped to the requested window while step-cycle and deterministic
 * probability phases are seeded as if the full schedule began at beat zero.
 *
 * @param {Parameters<typeof buildPhraseSchedule>[0] & {
 *   windowStartQuarters?: number,
 *   windowEndQuarters?: number,
 *   windowLookbackQuarters?: number,
 * }} params
 * @returns {ScheduledNote[]}
 */
export function buildPhraseScheduleWindowBeforeBandpass(params) {
  const {
    noteBandpassLowMidi: _noteBandpassLowMidi,
    noteBandpassHighMidi: _noteBandpassHighMidi,
    velocityTiltPivotMidi: _velocityTiltPivotMidi,
    velocityTiltAmount: _velocityTiltAmount,
    globalTransposeSemitones: _globalTransposeSemitones,
    windowStartQuarters = 0,
    windowEndQuarters = params.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS,
    windowLookbackQuarters = DEFAULT_PREVIEW_WINDOW_LOOKBACK_QUARTERS,
    loopOutputStartQuarters,
    loopOutputEndQuarters,
    ...scheduleParams
  } = params;
  const loopOutputActive = loopOutputBoundsActive(loopOutputStartQuarters, loopOutputEndQuarters);
  const outputStart = loopOutputActive ? loopOutputStartQuarters : 0;
  const outputEnd = loopOutputActive
    ? loopOutputEndQuarters
    : Math.max(0, scheduleParams.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS);
  const timelineEnd = loopOutputActive
    ? outputEnd
    : Math.max(0, scheduleParams.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS);
  const windowStart = loopOutputActive
    ? outputStart
    : Math.min(timelineEnd, Math.max(0, windowStartQuarters));
  const windowEnd = loopOutputActive
    ? outputEnd
    : Math.min(timelineEnd, Math.max(windowStart, windowEndQuarters));
  const hocketModeEnabled = combinationModeEnabled(scheduleParams.combinationModeMask ?? 0, 6);
  const scheduleStart = hocketModeEnabled
    ? (loopOutputActive ? outputStart : 0)
    : Math.max(loopOutputActive ? outputStart : 0, windowStart - Math.max(0, windowLookbackQuarters));

  if (windowEnd <= windowStart) return [];

  return buildPhraseScheduleCore({
    ...scheduleParams,
    lengthQuarters: windowEnd,
    scheduleStartQuarters: scheduleStart,
    loopOutputStartQuarters: loopOutputActive ? outputStart : undefined,
    loopOutputEndQuarters: loopOutputActive ? outputEnd : undefined,
  })
    .filter((note) => note.end > windowStart + EPSILON && note.start < windowEnd - EPSILON)
    .map((note) => ({
      ...note,
      start: Math.max(windowStart, note.start),
      end: Math.min(windowEnd, note.end),
    }))
    .filter((note) => note.end > note.start + EPSILON);
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

/** @param {ScheduledNote[]} events @param {number} start */
function eventStartCollides(events, start) {
  return events.some((event) => Math.abs(event.start - start) <= EPSILON);
}

/** @param {number[]} activeRows @param {number} row */
function activeRowPosition(activeRows, row) {
  return Math.max(0, activeRows.indexOf(row));
}

/**
 * @param {ScheduledNote[]} events
 * @param {number[]} activeRows
 * @param {number[][]} notes
 * @param {number[]} rowMidiChannel
 * @param {number} scaleRoot
 * @param {number} scaleModeIndex
 * @param {number} delayQuarters
 * @param {number} lengthQuarters
 * @returns {ScheduledNote[]}
 */
function addCanonFollowers(events, activeRows, notes, rowMidiChannel, scaleRoot, scaleModeIndex, delayQuarters, lengthQuarters) {
  if (activeRows.length <= 1 || events.length === 0) return events;

  const original = events.map((event) => ({...event}));
  const combined = [...original];

  for (const event of original) {
    if (combined.length >= MAX_COMBINED_PREVIEW_NOTES) break;

    const sourceRow = event.row;
    const position = activeRowPosition(activeRows, sourceRow);
    const targetRow = activeRows[(position + 1) % activeRows.length];
    const targetNotes = notes[targetRow] ?? [];

    if (targetNotes.length <= 0) continue;

    const targetStep = event.step % targetNotes.length;
    const sourceBase = notes[sourceRow]?.[0] ?? event.midi;
    const targetBase = targetNotes[0] ?? event.midi;
    const degreeDelta = scaleDegreeDelta(sourceBase, event.midi, scaleRoot, scaleModeIndex);
    const start = event.start + delayQuarters;

    if (start >= lengthQuarters - EPSILON) continue;

    combined.push({
      ...event,
      start,
      end: start + Math.max(0, event.end - event.start),
      midi: transposeMidiByScaleDegrees(targetBase, degreeDelta, scaleRoot, scaleModeIndex),
      velocity: Math.min(127, Math.max(1, Math.round(event.velocity * 0.78))),
      row: targetRow,
      step: targetStep,
      channel: midiChannelForRow(rowMidiChannel, targetRow),
    });
  }

  return combined.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
}

/**
 * @param {ScheduledNote[]} events
 * @param {number[]} activeRows
 * @param {number[][]} notes
 * @param {number[]} rowMidiChannel
 * @param {number} scaleRoot
 * @param {number} scaleModeIndex
 * @param {number} delayQuarters
 * @param {number} lengthQuarters
 * @returns {ScheduledNote[]}
 */
function addRetroInversionFollowers(events, activeRows, notes, rowMidiChannel, scaleRoot, scaleModeIndex, delayQuarters, lengthQuarters) {
  if (activeRows.length <= 1 || events.length === 0) return events;

  const original = events.map((event) => ({...event}));
  const combined = [...original];

  for (const event of original) {
    if (combined.length >= MAX_COMBINED_PREVIEW_NOTES) break;

    const sourceRow = event.row;
    const sourceNotes = notes[sourceRow] ?? [];

    if (sourceNotes.length <= 0) continue;

    const position = activeRowPosition(activeRows, sourceRow);
    const targetRow = activeRows[(position + 1) % activeRows.length];
    const targetNotes = notes[targetRow] ?? [];

    if (targetNotes.length <= 0) continue;

    const mirroredStep = sourceNotes.length - 1 - (event.step % sourceNotes.length);
    const targetStep = mirroredStep % targetNotes.length;
    const sourceBase = sourceNotes[0] ?? event.midi;
    const targetBase = targetNotes[0] ?? event.midi;
    const mirroredSourceNote = sourceNotes[mirroredStep] ?? sourceBase;
    const mirroredDelta = scaleDegreeDelta(sourceBase, mirroredSourceNote, scaleRoot, scaleModeIndex);
    const start = event.start + delayQuarters;

    if (start >= lengthQuarters - EPSILON) continue;

    const gate = Math.max(0, (event.end - event.start) * 0.7);

    if (gate <= EPSILON) continue;

    combined.push({
      ...event,
      start,
      end: start + gate,
      midi: transposeMidiByScaleDegrees(targetBase, -mirroredDelta, scaleRoot, scaleModeIndex),
      velocity: Math.min(127, Math.max(1, Math.round(event.velocity * 0.68))),
      row: targetRow,
      step: targetStep,
      channel: midiChannelForRow(rowMidiChannel, targetRow),
    });
  }

  return combined.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
}

/**
 * @param {ScheduledNote[]} events
 * @param {number[]} activeRows
 * @param {number[]} rowMidiChannel
 * @param {number[][]} stepVelocity
 * @param {number} pulseQuarters
 * @param {number} lengthQuarters
 * @param {{
 *   emitStartQuarters?: number,
 *   emitEndQuarters?: number,
 *   hocketLengthQuarters?: number,
 * }} [options]
 * @returns {ScheduledNote[]}
 */
function hocketEvents(events, activeRows, rowMidiChannel, stepVelocity, pulseQuarters, lengthQuarters, options = {}) {
  if (activeRows.length <= 1 || events.length === 0) return events;

  const sliceQuarters = pulseQuarters / activeRows.length;

  if (sliceQuarters <= EPSILON) return events;

  const emitStart = options.emitStartQuarters ?? 0;
  const emitEnd = options.emitEndQuarters ?? lengthQuarters;
  const hocketLength = options.hocketLengthQuarters ?? lengthQuarters;
  const minimumSliceOverlap = sliceQuarters * HOCKET_MINIMUM_SLICE_OVERLAP_FRACTION;
  /** @type {Map<number, ScheduledNote[]>} */
  const candidatesBySlice = new Map();

  for (const event of events) {
    const eventEnd = event.end;

    if (eventEnd <= event.start + EPSILON) continue;

    const firstSlice = Math.floor((event.start + EPSILON) / sliceQuarters);
    const lastSlice = Math.ceil((eventEnd - EPSILON) / sliceQuarters) - 1;

    for (let slice = firstSlice; slice <= lastSlice; slice += 1) {
      const sliceStart = slice * sliceQuarters;
      const sliceEnd = sliceStart + sliceQuarters;

      if (sliceStart >= hocketLength - EPSILON) continue;

      const start = Math.max(event.start, sliceStart);
      const end = Math.min(eventEnd, sliceEnd, hocketLength);
      const duration = end - start;

      if (duration <= EPSILON || duration + EPSILON < minimumSliceOverlap) continue;

      const gate = Math.min(duration, sliceQuarters * 0.85);

      if (gate <= EPSILON) continue;

      const candidate = {
        ...event,
        start,
        end: start + gate,
      };
      const bucket = candidatesBySlice.get(slice);

      if (bucket) {
        bucket.push(candidate);
      } else {
        candidatesBySlice.set(slice, [candidate]);
      }
    }
  }

  let minSlice = Number.POSITIVE_INFINITY;
  let maxSlice = Number.NEGATIVE_INFINITY;

  for (const slice of candidatesBySlice.keys()) {
    minSlice = Math.min(minSlice, slice);
    maxSlice = Math.max(maxSlice, slice);
  }

  const scheduleFirstSlice = Math.floor((emitStart + EPSILON) / sliceQuarters);
  const scheduleLastSlice = Math.ceil((emitEnd - EPSILON) / sliceQuarters) - 1;

  if (!Number.isFinite(minSlice) || minSlice > maxSlice) {
    minSlice = scheduleFirstSlice;
    maxSlice = scheduleLastSlice;
  } else {
    minSlice = Math.min(minSlice, scheduleFirstSlice);
    maxSlice = Math.max(maxSlice, scheduleLastSlice);
  }

  if (minSlice > maxSlice) return [];

  const candidateWeight = (event) => {
    const sourceStepVelocity = stepVelocity[event.row]?.[event.step % Math.max(1, stepVelocity[event.row]?.length ?? 1)];

    return Math.max(1, sourceStepVelocity > 0 ? sourceStepVelocity : event.velocity);
  };

  /** @type {ScheduledNote[]} */
  const hocketed = [];

  for (let slice = minSlice; slice <= maxSlice; slice += 1) {
    if (hocketed.length >= MAX_COMBINED_PREVIEW_NOTES) break;

    const sliceStart = slice * sliceQuarters;
    const sliceEnd = sliceStart + sliceQuarters;

    if (sliceStart < emitStart - EPSILON) continue;
    if (sliceStart >= emitEnd - EPSILON) continue;
    if (sliceStart >= hocketLength - EPSILON) continue;

    const candidates = candidatesBySlice.get(slice);

    if (!candidates || candidates.length === 0) continue;

    const sliceTargetRow = activeRows[((slice % activeRows.length) + activeRows.length) % activeRows.length];

    if (candidates.length === 1) {
      const source = candidates[0];
      const sourceEnd = source.end;
      const start = Math.max(source.start, sliceStart);
      const end = Math.min(sourceEnd, sliceEnd, hocketLength);
      const gate = Math.min(end - start, sliceQuarters * 0.85);

      if (gate <= EPSILON) continue;

      hocketed.push({
        ...source,
        start,
        end: start + gate,
        row: sliceTargetRow,
        channel: midiChannelForRow(rowMidiChannel, sliceTargetRow),
      });
      continue;
    }

    candidates.sort(
      (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
    );

    const totalWeight = candidates.reduce((total, event) => total + candidateWeight(event), 0);
    let pick = deterministicEventHash(sliceTargetRow, slice, sliceStart) % Math.max(1, totalWeight);
    let selected = candidates[0];
    let found = false;

    for (const event of candidates) {
      pick -= candidateWeight(event);

      if (pick < 0) {
        selected = event;
        found = true;
        break;
      }
    }

    if (!found) continue;

    const sourceEnd = selected.end;
    const start = Math.max(selected.start, sliceStart);
    const end = Math.min(sourceEnd, sliceEnd, hocketLength);
    const gate = Math.min(end - start, sliceQuarters * 0.85);

    if (gate <= EPSILON) continue;

    hocketed.push({
      ...selected,
      start,
      end: start + gate,
      row: sliceTargetRow,
      channel: midiChannelForRow(rowMidiChannel, sliceTargetRow),
    });
  }

  return hocketed.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
}

/**
 * @param {object} params
 * @param {ScheduledNote[]} params.scheduled
 * @param {number[][]} params.notes
 * @param {boolean[]} params.rowMuted
 * @param {number[][]} params.stepTimingMultiplier
 * @param {number[][]} params.stepVelocity
 * @param {number[]} params.rowMidiChannel
 * @param {number[][]} params.stepDurationFraction
 * @param {boolean[][]} params.stepMuted
 * @param {boolean[][]} params.stepSkipped
 * @param {number} params.pulseIndex
 * @param {number} params.combinationModeMask
 * @param {number} params.lengthQuarters
 * @param {number} params.scaleRoot
 * @param {number} params.scaleModeIndex
 * @returns {ScheduledNote[]}
 */
function applyCombinationModes({
  scheduled,
  notes,
  rowMuted,
  stepTimingMultiplier,
  stepVelocity,
  rowMidiChannel,
  stepDurationFraction,
  stepMuted,
  stepSkipped,
  pulseIndex,
  combinationModeMask,
  lengthQuarters,
  scaleRoot,
  scaleModeIndex,
  loopOutputStartQuarters,
  loopOutputEndQuarters,
  scheduleStartQuarters = 0,
}) {
  if ((combinationModeMask & combinationModeMaskBits) === 0 || scheduled.length === 0) return scheduled;

  let events = scheduled.map((event) => ({...event}));
  const loopOutputActive = loopOutputBoundsActive(loopOutputStartQuarters, loopOutputEndQuarters);
  const emitStartQuarters = loopOutputActive ? loopOutputStartQuarters : scheduleStartQuarters;
  const emitEndQuarters = lengthQuarters;
  const hocketLengthQuarters = loopOutputActive ? loopOutputEndQuarters : lengthQuarters;
  const weaveRepeatLengthQuarters = patternRepeatLengthQuarters({
    stepTimingMultiplier,
    rowMuted,
    stepSkipped,
    pulseIndex,
  });
  const weaveHashPpq = (ppq) => {
    if (loopOutputActive) {
      const loopLength = loopOutputEndQuarters - loopOutputStartQuarters;

      return loopOutputStartQuarters + positiveMod(ppq - loopOutputStartQuarters, loopLength);
    }

    return weaveRepeatLengthQuarters > EPSILON ? positiveMod(ppq, weaveRepeatLengthQuarters) : ppq;
  };
  const activeRows = notes
    .map((rowNotes, row) => {
      const layout = rowStepLayout(stepTimingMultiplier[row] ?? [], pulseIndex, stepSkipped[row] ?? []);

      return {row, rowNotes, cycleLengthQuarters: layout.cycleLengthQuarters};
    })
    .filter(({row, rowNotes, cycleLengthQuarters}) => !rowMuted[row] && rowNotes.length > 0 && cycleLengthQuarters > EPSILON)
    .map(({row}) => row);

  if (activeRows.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 0) && activeRows.length > 1) {
    events = events.map((event) => {
      const activeIndex = activeRowPosition(activeRows, event.row);
      const pitchRow = activeRows[(activeIndex + 1) % activeRows.length];
      const velocityRow = activeRows[(activeIndex + 2) % activeRows.length];
      const durationRow = activeRows[(activeIndex + 3) % activeRows.length];
      const pitchStep = event.step % Math.max(1, notes[pitchRow]?.length ?? 1);
      const velocityStep = event.step % Math.max(1, stepVelocity[velocityRow]?.length ?? 1);
      const durationStep = event.step % Math.max(1, stepDurationFraction[durationRow]?.length ?? 1);
      const pitchBase = notes[pitchRow]?.[0] ?? 60;
      const layout = rowStepLayout(stepTimingMultiplier[durationRow] ?? [], pulseIndex, stepSkipped[durationRow] ?? []);
      const duration = (layout.stepLengthQuarters[durationStep] ?? event.end - event.start) * (stepDurationFraction[durationRow]?.[durationStep] ?? 1);

      return {
        ...event,
        midi: echoNoteFromModStep(event.midi, pitchBase, notes[pitchRow]?.[pitchStep] ?? pitchBase, scaleRoot, scaleModeIndex),
        velocity: Math.min(127, Math.max(1, stepVelocity[velocityRow]?.[velocityStep] ?? event.velocity)),
        end: event.start + (duration > EPSILON ? duration : event.end - event.start),
      };
    });
  }

  if (combinationModeEnabled(combinationModeMask, 7) && activeRows.length > 1) {
    const combinationGesturePulse = Math.max(pulseQuartersForIndex(pulseIndex), COMBINATION_GESTURE_PULSE_QUARTERS_FLOOR);
    events = addCanonFollowers(
      events,
      activeRows,
      notes,
      rowMidiChannel,
      scaleRoot,
      scaleModeIndex,
      combinationGesturePulse / activeRows.length,
      lengthQuarters,
    );
  }

  if (events.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 8) && activeRows.length > 1) {
    const combinationGesturePulse = Math.max(pulseQuartersForIndex(pulseIndex), COMBINATION_GESTURE_PULSE_QUARTERS_FLOOR);
    events = addRetroInversionFollowers(
      events,
      activeRows,
      notes,
      rowMidiChannel,
      scaleRoot,
      scaleModeIndex,
      combinationGesturePulse * 0.25,
      lengthQuarters,
    );
  }

  if (combinationModeEnabled(combinationModeMask, 6) && activeRows.length > 1) {
    events = hocketEvents(
      events,
      activeRows,
      rowMidiChannel,
      stepVelocity,
      pulseQuartersForIndex(pulseIndex),
      lengthQuarters,
      {
        emitStartQuarters,
        emitEndQuarters,
        hocketLengthQuarters,
      },
    );
  }

  if (events.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 1) && activeRows.length > 1) {
    /** @type {ScheduledNote[]} */
    const tendriled = [];
    const pulseQuarters = pulseQuartersForIndex(pulseIndex);
    const combinationGesturePulse = Math.max(pulseQuarters, COMBINATION_GESTURE_PULSE_QUARTERS_FLOOR);

    for (const event of events) {
      if (tendriled.length >= MAX_COMBINED_PREVIEW_NOTES) break;

      tendriled.push(event);

      const activeIndex = Math.max(0, activeRows.indexOf(event.row));
      const modRow = activeRows[(activeIndex + 1) % activeRows.length];
      const modNotes = notes[modRow] ?? [];
      const duration = event.end - event.start;

      if (modNotes.length <= 0) continue;
      if (!isCombinationGestureAnchor(event.start, duration, combinationGesturePulse)) continue;

      const hash = deterministicEventHash(event.row, event.step, event.start);
      const modStep = event.step % Math.max(1, modNotes.length);
      const previousModStep = (modStep + modNotes.length - 1) % modNotes.length;
      const nextModStep = (modStep + 1) % modNotes.length;
      let movement = scaleDegreeDelta(modNotes[previousModStep], modNotes[modStep], scaleRoot, scaleModeIndex);

      if (movement === 0) {
        movement = scaleDegreeDelta(modNotes[modStep], modNotes[nextModStep], scaleRoot, scaleModeIndex);
      }

      const direction = movement < 0 ? -1 : movement > 0 ? 1 : (hash & 1) !== 0 ? 1 : -1;
      const tendrilGrid = combinationGesturePulse * 0.25;
      const curlDelay = tendrilGrid;
      const answerBaseDelay = tendrilGrid * 2;
      const resolutionDelay = tendrilGrid * 3;
      const curlGate = Math.min(duration * 0.3, tendrilGrid * 0.75);
      const answerGate = Math.min(duration * 0.45, tendrilGrid * 0.875);
      const resolutionGate = Math.min(duration * 0.25, tendrilGrid * 0.75);

      if (curlGate <= EPSILON && answerGate <= EPSILON) continue;

      const stepCanSpeak = (row, step) => {
        const count = notes[row]?.length ?? 0;
        if (count <= 0) return false;
        const index = step % count;
        return !stepSkipped[row]?.[index] && !stepMuted[row]?.[index] && (stepVelocity[row]?.[index] ?? 0) > 0;
      };

      const choosePlayableStep = (row, preferredStep) => {
        const count = notes[row]?.length ?? 0;
        if (count <= 0) return -1;

        for (let offset = 0; offset < count; offset += 1) {
          const step = (preferredStep + offset) % count;

          if (stepCanSpeak(row, step)) return step;
        }

        return -1;
      };

      const appendTendril = (row, step, midi, delay, gate, velocity) => {
        if (tendriled.length >= MAX_COMBINED_PREVIEW_NOTES) return;
        if (gate <= EPSILON) return;
        const start = event.start + delay;

        if (start >= lengthQuarters - EPSILON) return;

        tendriled.push({
          ...event,
          row,
          step,
          channel: midiChannelForRow(rowMidiChannel, row),
          start,
          end: start + gate,
          midi,
          velocity: Math.min(127, Math.max(1, Math.round(velocity))),
        });
      };

      const curlMidi = transposeMidiByScaleDegrees(event.midi, direction, scaleRoot, scaleModeIndex);

      if (curlMidi !== event.midi) {
        appendTendril(
          event.row,
          event.step,
          curlMidi,
          curlDelay,
          curlGate,
          event.velocity * 0.58,
        );
      }

      const answerStep = choosePlayableStep(modRow, nextModStep);

      if (answerStep >= 0 && answerGate > EPSILON) {
        let answerDelay = answerBaseDelay;
        let start = event.start + answerDelay;

        if (eventStartCollides(events, start)) {
          answerDelay += tendrilGrid;
          start = event.start + answerDelay;

          if (eventStartCollides(events, start)) {
            answerDelay += tendrilGrid;
          }
        }

        let answerMidi = echoNoteFromModStep(
          event.midi,
          modNotes[0] ?? 60,
          modNotes[answerStep] ?? modNotes[0] ?? 60,
          scaleRoot,
          scaleModeIndex,
        );

        if (answerMidi === event.midi) {
          answerMidi = transposeMidiByScaleDegrees(answerMidi, direction, scaleRoot, scaleModeIndex);
        }

        answerMidi = foldMidiToNearestRegister(answerMidi, event.midi);

        if (answerMidi !== event.midi) {
          appendTendril(
            modRow,
            answerStep,
            answerMidi,
            answerDelay,
            answerGate,
            event.velocity * 0.42 + (stepVelocity[modRow]?.[answerStep] ?? event.velocity) * 0.36,
          );
        }
      }

      if (duration >= combinationGesturePulse * 0.75 - EPSILON && Math.abs(movement) >= 2 && resolutionGate > EPSILON) {
        let resolutionRow = event.row;
        let resolutionStep = event.step;
        let resolutionMidi = transposeMidiByScaleDegrees(event.midi, -direction, scaleRoot, scaleModeIndex);
        let resolutionVelocity = event.velocity * 0.42;

        if (activeRows.length > 2) {
          const resolutionCandidateRow = activeRows[(activeIndex + 2) % activeRows.length];
          const resolutionNotes = notes[resolutionCandidateRow] ?? [];
          const preferredResolutionStep = (event.step + ((hash >> 3) & 3)) % Math.max(1, resolutionNotes.length);
          const candidateStep = choosePlayableStep(resolutionCandidateRow, preferredResolutionStep);

          if (candidateStep >= 0) {
            resolutionRow = resolutionCandidateRow;
            resolutionStep = candidateStep;
            resolutionMidi = echoNoteFromModStep(
              event.midi,
              resolutionNotes[0] ?? 60,
              resolutionNotes[candidateStep] ?? resolutionNotes[0] ?? 60,
              scaleRoot,
              scaleModeIndex,
            );
            resolutionMidi = transposeMidiByScaleDegrees(resolutionMidi, -direction, scaleRoot, scaleModeIndex);
            resolutionVelocity = event.velocity * 0.26 + (stepVelocity[resolutionCandidateRow]?.[candidateStep] ?? event.velocity) * 0.28;
          }
        }

        resolutionMidi = foldMidiToNearestRegister(resolutionMidi, event.midi);

        if (resolutionMidi !== event.midi) {
          appendTendril(
            resolutionRow,
            resolutionStep,
            resolutionMidi,
            resolutionDelay,
            resolutionGate,
            resolutionVelocity,
          );
        }
      }
    }

    events = tendriled.sort(
      (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
    );
  }

  if (events.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 3) && activeRows.length > 1) {
    /** @type {ScheduledNote[]} */
    const multiplied = [];

    for (const event of events) {
      const activeIndex = Math.max(0, activeRows.indexOf(event.row));
      const modRow = activeRows[(activeIndex + 1) % activeRows.length];
      const modNotes = notes[modRow] ?? [];
      const modBase = modNotes[0] ?? 60;
      const modLayout = rowStepLayout(stepTimingMultiplier[modRow] ?? [], pulseIndex, stepSkipped[modRow] ?? []);

      for (let modStep = 0; modStep < modNotes.length; modStep += 1) {
        if (multiplied.length >= MAX_COMBINED_PREVIEW_NOTES) break;
        if (stepSkipped[modRow]?.[modStep] || stepMuted[modRow]?.[modStep]) continue;
        if ((stepVelocity[modRow]?.[modStep] ?? 0) <= 0) continue;

        const start = event.start + (modLayout.stepStartQuarters[modStep] ?? 0);

        if (start < -EPSILON || start >= lengthQuarters - EPSILON) continue;

        const modDuration = (modLayout.stepLengthQuarters[modStep] ?? event.end - event.start) * (stepDurationFraction[modRow]?.[modStep] ?? 1);
        const duration = Math.min(event.end - event.start, modDuration);

        if (duration <= EPSILON) continue;

        multiplied.push({
          ...event,
          start,
          end: start + duration,
          midi: echoNoteFromModStep(event.midi, modBase, modNotes[modStep] ?? modBase, scaleRoot, scaleModeIndex),
          velocity: Math.round((event.velocity + (stepVelocity[modRow]?.[modStep] ?? event.velocity)) / 2),
          step: modStep,
        });
      }
    }

    events = multiplied;
  }

  if (events.length === 0) return [];

  events = events.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );

  if (combinationModeEnabled(combinationModeMask, 4)) {
    events = groupByStart(events).map((group) => {
      if (group.length === 1) return group[0];

      const totalWeight = group.reduce((total, event) => total + Math.max(1, event.velocity), 0);
      let pick = deterministicEventHash(group[0].row, group[0].step, weaveHashPpq(group[0].start)) % Math.max(1, totalWeight);

      for (const event of group) {
        pick -= Math.max(1, event.velocity);

        if (pick < 0) return event;
      }

      return group[group.length - 1];
    });
  }

  return events.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
}

/** @param {ScheduledNote[]} scheduled @param {number} [paddingSemitones] */
export function pitchRangeForSchedule(scheduled, paddingSemitones = 2) {
  if (scheduled.length === 0) {
    return {minMidi: 48, maxMidi: 72};
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

  return {minMidi, maxMidi};
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
  stepCycleMask = [],
  stepCycleOffset = [],
  pulseIndex = defaultPulseIndex,
  swingPercent = 0,
  swingSubdivisionIndex = 1,
}) {
  if (beat < 0 || rowMuted || step < 0 || step >= rowNotes.length) return false;
  if (stepSkipped[step] || stepMuted[step] || (stepVelocity[step] ?? 0) <= 0 || (stepDurationFraction[step] ?? 0) <= 0) return false;

  const cycle = Math.max(1, stepCycle[step] ?? 1);
  const cyclePatternMask = (stepCycleMask[step] ?? stepCycleOffset[step] ?? 1);
  const probability = stepProbability[step] ?? defaultStepProbabilityValue;

  const {stepStartQuarters, stepLengthQuarters, cycleLengthQuarters} = rowStepLayout(stepTimingMultiplier, pulseIndex, stepSkipped);

  if (cycleLengthQuarters <= 0 || (stepLengthQuarters[step] ?? 0) <= 0) return false;

  const rowOffsetQuarters = rowTimingOffsetQuarters(rowTimingOffset, pulseIndex);
  const stepStartInCycle = stepStartInCycleForStep(stepStartQuarters, step);
  const relativeBeat = beat - stepStartInCycle - rowOffsetQuarters;

  if (relativeBeat < -EPSILON) return false;

  const firstGlobalTrigger = Math.ceil((0 - stepStartInCycle - rowOffsetQuarters - EPSILON) / cycleLengthQuarters);
  const cycleIndex = Math.floor((relativeBeat + EPSILON) / cycleLengthQuarters);
  const triggerCount = cycleIndex - firstGlobalTrigger;

  if (cycle > 1 && !cycleGatePasses(triggerCount, cycle, cyclePatternMask)) return false;

  if (!probabilityPasses(step, triggerCount, probability)) return false;

  const triggerBeat = cycleIndex * cycleLengthQuarters + stepStartInCycle + rowOffsetQuarters;
  const noteStart = triggerBeat + swingDelayQuartersForPpq(triggerBeat, pulseIndex, swingPercent, swingSubdivisionIndex);
  const gateEnd = triggerBeat + stepLengthQuarters[step] * stepDurationFraction[step];

  return beat >= noteStart - EPSILON && beat < gateEnd - triggerBeat + noteStart - EPSILON;
}

const PATTERN_REPEAT_GRID_QUARTERS = loopBraceSnapQuarters;

/** @param {number} value @param {number} modulus */
export function positiveMod(value, modulus) {
  if (modulus <= EPSILON) return value;

  const remainder = value % modulus;

  return remainder < 0 ? remainder + modulus : remainder;
}

/** @param {number} a @param {number} b */
function gcdInt(a, b) {
  let x = Math.abs(Math.trunc(a));
  let y = Math.abs(Math.trunc(b));

  while (y !== 0) {
    const next = x % y;
    x = y;
    y = next;
  }

  return x;
}

/** @param {number} a @param {number} b */
function lcmInt(a, b) {
  if (a === 0 || b === 0) return 0;

  return Math.abs(a / gcdInt(a, b) * b);
}

/** @param {number} quarters */
function patternRepeatUnits(quarters) {
  return Math.max(1, Math.round(quarters / PATTERN_REPEAT_GRID_QUARTERS));
}

/** @param {number} units */
function quartersFromPatternRepeatUnits(units) {
  return units * PATTERN_REPEAT_GRID_QUARTERS;
}

/**
 * Quarter-note repeat length for one row, including per-step cycle patterns.
 *
 * @param {number[]} stepTimingMultiplier
 * @param {boolean[]} [stepSkipped]
 * @param {number[]} [stepCycle]
 * @param {number} [pulseIndex]
 */
export function rowPatternRepeatLengthQuarters(
  stepTimingMultiplier,
  stepSkipped = [],
  stepCycle = [],
  pulseIndex = defaultPulseIndex,
) {
  const {cycleLengthQuarters} = rowStepLayout(stepTimingMultiplier, pulseIndex, stepSkipped);

  if (cycleLengthQuarters <= EPSILON) return 0;

  let triggerCycleUnits = 1;

  for (let step = 0; step < stepTimingMultiplier.length; step += 1) {
    if (stepSkipped[step]) continue;

    const cycle = Math.max(1, Math.trunc(stepCycle[step] ?? 1));
    triggerCycleUnits = lcmInt(triggerCycleUnits, cycle);
  }

  return quartersFromPatternRepeatUnits(
    patternRepeatUnits(cycleLengthQuarters) * triggerCycleUnits,
  );
}

/**
 * Smallest clean loop length where every active row returns to the same phase.
 *
 * @param {object} params
 * @param {number[][]} params.stepTimingMultiplier
 * @param {boolean[]} params.rowMuted
 * @param {boolean[][]} [params.stepSkipped]
 * @param {number[][]} [params.stepCycle]
 * @param {number} [params.pulseIndex]
 */
export function patternRepeatLengthQuarters({
  stepTimingMultiplier,
  rowMuted,
  stepSkipped = [],
  stepCycle = [],
  pulseIndex = defaultPulseIndex,
}) {
  let repeatUnits = 0;

  for (let row = 0; row < stepTimingMultiplier.length; row += 1) {
    if (rowMuted[row]) continue;

    const rowRepeatQuarters = rowPatternRepeatLengthQuarters(
      stepTimingMultiplier[row] ?? [],
      stepSkipped[row] ?? [],
      stepCycle[row] ?? [],
      pulseIndex,
    );

    if (rowRepeatQuarters <= EPSILON) continue;

    const rowRepeatUnits = patternRepeatUnits(rowRepeatQuarters);
    repeatUnits = repeatUnits <= 0 ? rowRepeatUnits : lcmInt(repeatUnits, rowRepeatUnits);
  }

  return repeatUnits > 0 ? quartersFromPatternRepeatUnits(repeatUnits) : 0;
}

/** @param {number} beat */
export function mapPlaybackBeatForPianoRoll(
  beat,
  { loopEnabled = false, patternLengthQuarters = 0 } = {},
) {
  if (beat < 0) return -1;

  if (loopEnabled) return beat;

  if (patternLengthQuarters > EPSILON) {
    return positiveMod(beat, patternLengthQuarters);
  }

  return beat;
}

/**
 * @param {ScheduledNote} note
 * @param {number} beat
 * @param {{ loopEnabled?: boolean, patternLengthQuarters?: number }} [options]
 */
export function isScheduledNoteActiveAtPlaybackBeat(
  note,
  beat,
  _options = {},
) {
  if (beat < 0) return false;

  return isScheduledNoteActiveAtBeat(note, beat);
}

/**
 * @param {ScheduledNote} note
 * @param {number} beat
 * @param {number} patternLength
 */
export function isScheduledNoteActiveAtPatternBeat(note, beat, patternLength) {
  if (beat < 0) return false;
  if (patternLength <= EPSILON) return isScheduledNoteActiveAtBeat(note, beat);

  const duration = note.end - note.start;

  if (duration <= EPSILON) return false;

  const startInPattern = positiveMod(note.start, patternLength);
  const beatInPattern = positiveMod(beat, patternLength);

  if (startInPattern + duration <= patternLength + EPSILON) {
    return beatInPattern >= startInPattern - EPSILON
      && beatInPattern < startInPattern + duration - EPSILON;
  }

  const wrappedEnd = startInPattern + duration - patternLength;

  return beatInPattern >= startInPattern - EPSILON || beatInPattern < wrappedEnd - EPSILON;
}
