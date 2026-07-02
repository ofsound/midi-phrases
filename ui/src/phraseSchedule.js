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
const ROUND_ROBIN_OVERLAP_FRACTION = 0.25;
const HOCKET_MINIMUM_SLICE_OVERLAP_FRACTION = 0.5;
const UNISON_OVERLAP_WINDOW_QUARTERS = 1 / 96;
const DEFAULT_PREVIEW_WINDOW_LOOKBACK_QUARTERS = 64;
export const combinationModeMaskBits = 0x1ff;
/** Display order matches processing order. Weave keeps its legacy bit and runs last. */
export const combinationModes = [
  {index: 0, bit: 1, icon: "crossMod", name: "Cross-Mod"},
  {index: 7, bit: 128, icon: "canon", name: "Canon"},
  {index: 8, bit: 256, icon: "retroInv", name: "Retro-Inv"},
  {index: 6, bit: 64, icon: "hocket", name: "Hocket"},
  {index: 5, bit: 32, icon: "roundRobin", name: "Round Robin"},
  {index: 1, bit: 2, icon: "bloom", name: "Bloom"},
  {index: 2, bit: 4, icon: "counter", name: "Counter"},
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
function isBloomGestureAnchor(ppq, duration, gesturePulse) {
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

  /** @type {ScheduledNote[]} */
  const monophonic = [];

  for (let index = 0; index < selected.length; index += 1) {
    const event = selected[index];
    const next = selected[index + 1];
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
  });

  const octavized = applyOctavizer(combined, {
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
  const repeatLengthQuarters = patternRepeatLengthQuarters({
    stepTimingMultiplier: params.stepTimingMultiplier ?? [],
    rowMuted: params.rowMuted ?? [],
    stepSkipped: params.stepSkipped ?? [],
    stepCycle: params.stepCycle ?? [],
    pulseIndex: params.pulseIndex ?? defaultPulseIndex,
  });
  const combinationModesActive = (params.combinationModeMask ?? 0) !== 0;
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

  return suppressHeldNoteRetriggers(
    applyWeaveMonophony(
      cleanupUnisonOverlaps(finalTransformed, combinationModesActive),
      combinationModeEnabled(params.combinationModeMask ?? 0, 4),
      repeatLengthQuarters,
    ),
    true,
  );
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
    ...scheduleParams
  } = params;

  return buildPhraseScheduleCore(scheduleParams);
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
    ...scheduleParams
  } = params;
  const timelineEnd = Math.max(0, scheduleParams.lengthQuarters ?? DEFAULT_PREVIEW_LENGTH_QUARTERS);
  const windowStart = Math.min(timelineEnd, Math.max(0, windowStartQuarters));
  const windowEnd = Math.min(timelineEnd, Math.max(windowStart, windowEndQuarters));
  const hocketWindowNeedsFullHistory = combinationModeEnabled(scheduleParams.combinationModeMask ?? 0, 6);
  const scheduleStart = hocketWindowNeedsFullHistory
    ? 0
    : Math.max(0, windowStart - Math.max(0, windowLookbackQuarters));

  if (windowEnd <= windowStart) return [];

  return buildPhraseScheduleCore({
    ...scheduleParams,
    lengthQuarters: windowEnd,
    scheduleStartQuarters: scheduleStart,
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
 * @returns {ScheduledNote[]}
 */
function hocketEvents(events, activeRows, rowMidiChannel, stepVelocity, pulseQuarters, lengthQuarters) {
  if (activeRows.length <= 1 || events.length === 0) return events;

  const sliceQuarters = pulseQuarters / activeRows.length;

  if (sliceQuarters <= EPSILON) return events;

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

      if (sliceStart >= lengthQuarters - EPSILON) continue;

      const start = Math.max(event.start, sliceStart);
      const end = Math.min(eventEnd, sliceEnd, lengthQuarters);
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

  if (candidatesBySlice.size === 0) return [];

  /** @type {ScheduledNote[]} */
  const hocketed = [];

  for (const [slice, candidates] of [...candidatesBySlice.entries()].sort((a, b) => a[0] - b[0])) {
    if (hocketed.length >= MAX_COMBINED_PREVIEW_NOTES) break;

    const sliceTargetRow = activeRows[((slice % activeRows.length) + activeRows.length) % activeRows.length];

    if (candidates.length === 1) {
      hocketed.push({
        ...candidates[0],
        row: sliceTargetRow,
        channel: midiChannelForRow(rowMidiChannel, sliceTargetRow),
      });
      continue;
    }

    candidates.sort(
      (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
    );

    const candidateWeight = (event) => {
      const sourceStepVelocity = stepVelocity[event.row]?.[event.step % Math.max(1, stepVelocity[event.row]?.length ?? 1)];

      return Math.max(1, sourceStepVelocity > 0 ? sourceStepVelocity : event.velocity);
    };
    const totalWeight = candidates.reduce((total, event) => total + candidateWeight(event), 0);
    let pick = deterministicEventHash(sliceTargetRow, slice, slice * sliceQuarters) % Math.max(1, totalWeight);

    for (const event of candidates) {
      pick -= candidateWeight(event);

      if (pick < 0) {
        hocketed.push({
          ...event,
          row: sliceTargetRow,
          channel: midiChannelForRow(rowMidiChannel, sliceTargetRow),
        });
        break;
      }
    }
  }

  return hocketed.sort(
    (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
  );
}

/**
 * @param {number} ppq
 * @param {number[]} activeRows
 * @param {number} gesturePulse
 */
function roundRobinWindowForPpq(ppq, activeRows, gesturePulse) {
  const segmentLength = Math.max(EPSILON, gesturePulse);
  const segmentIndex = Math.floor((ppq + EPSILON) / segmentLength);
  const currentIndex = positiveMod(segmentIndex, activeRows.length);
  const phase = positiveMod(ppq, segmentLength);
  const currentRow = activeRows[currentIndex];
  const overlapLength = segmentLength * ROUND_ROBIN_OVERLAP_FRACTION;
  const inOverlap = overlapLength > EPSILON && phase >= segmentLength - overlapLength - EPSILON;

  if (!inOverlap) {
    return {overlap: false, currentRow, nextRow: currentRow};
  }

  return {
    overlap: true,
    currentRow,
    nextRow: activeRows[(currentIndex + 1) % activeRows.length],
  };
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
function applyCombinationModes({scheduled, notes, rowMuted, stepTimingMultiplier, stepVelocity, rowMidiChannel, stepDurationFraction, stepMuted, stepSkipped, pulseIndex, combinationModeMask, lengthQuarters, scaleRoot, scaleModeIndex}) {
  if ((combinationModeMask & combinationModeMaskBits) === 0 || scheduled.length === 0) return scheduled;

  let events = scheduled.map((event) => ({...event}));
  const weaveRepeatLengthQuarters = patternRepeatLengthQuarters({
    stepTimingMultiplier,
    rowMuted,
    stepSkipped,
    pulseIndex,
  });
  const weaveHashPpq = (ppq) => (
    weaveRepeatLengthQuarters > EPSILON ? positiveMod(ppq, weaveRepeatLengthQuarters) : ppq
  );
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
    events = hocketEvents(events, activeRows, rowMidiChannel, stepVelocity, pulseQuartersForIndex(pulseIndex), lengthQuarters);
  }

  if (events.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 5) && activeRows.length > 1) {
    /** @type {ScheduledNote[]} */
    const roundRobin = [];
    const pulseQuarters = pulseQuartersForIndex(pulseIndex);
    const combinationGesturePulse = Math.max(pulseQuarters, COMBINATION_GESTURE_PULSE_QUARTERS_FLOOR);

    for (const group of groupByStart(events)) {
      const window = roundRobinWindowForPpq(group[0].start, activeRows, combinationGesturePulse);
      const eligible = group.filter((event) => {
        if (event.row === window.currentRow) return true;

        return window.overlap && event.row === window.nextRow;
      });

      if (eligible.length === 0) continue;

      if (!window.overlap || eligible.length === 1) {
        roundRobin.push(...eligible);
        continue;
      }

      const totalWeight = eligible.reduce((total, event) => total + Math.max(1, event.velocity), 0);
      let pick = deterministicEventHash(eligible[0].row, eligible[0].step, eligible[0].start) % Math.max(1, totalWeight);

      for (const event of eligible) {
        pick -= Math.max(1, event.velocity);

        if (pick < 0) {
          roundRobin.push(event);
          break;
        }
      }
    }

    events = roundRobin;
  }

  if (events.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 1) && activeRows.length > 1) {
    /** @type {ScheduledNote[]} */
    const bloomed = [];
    const pulseQuarters = pulseQuartersForIndex(pulseIndex);
    const combinationGesturePulse = Math.max(pulseQuarters, COMBINATION_GESTURE_PULSE_QUARTERS_FLOOR);

    for (const event of events) {
      if (bloomed.length >= MAX_COMBINED_PREVIEW_NOTES) break;

      bloomed.push(event);

      const activeIndex = Math.max(0, activeRows.indexOf(event.row));
      const modRow = activeRows[(activeIndex + 1) % activeRows.length];
      const modNotes = notes[modRow] ?? [];

      if (modNotes.length <= 0) continue;

      const modStep = event.step % Math.max(1, modNotes.length);
      const previousModStep = (modStep + modNotes.length - 1) % modNotes.length;
      const movement = scaleDegreeDelta(modNotes[previousModStep], modNotes[modStep], scaleRoot, scaleModeIndex);
      const direction = movement < 0 ? -1 : 1;
      const duration = event.end - event.start;
      const sourceSupportsReturnBloom = duration >= combinationGesturePulse - EPSILON;

      if (!isBloomGestureAnchor(event.start, duration, combinationGesturePulse)) continue;

      const ornamentGate = Math.min(duration * 0.375, combinationGesturePulse * 0.25);

      if (ornamentGate <= EPSILON) continue;

      const firstDelay = combinationGesturePulse * 0.25;
      const secondDelay = combinationGesturePulse * 0.5;

      const appendBloom = (degreeDelta, delay, velocityScale) => {
        if (bloomed.length >= MAX_COMBINED_PREVIEW_NOTES) return;

        const start = event.start + delay;

        if (start >= lengthQuarters - EPSILON) return;

        const midi = transposeMidiByScaleDegrees(event.midi, degreeDelta, scaleRoot, scaleModeIndex);

        if (midi === event.midi) return;

        bloomed.push({
          ...event,
          start,
          end: start + ornamentGate,
          midi,
          velocity: Math.min(127, Math.max(1, Math.round(event.velocity * velocityScale))),
        });
      };

      appendBloom(direction, firstDelay, 0.65);

      if (sourceSupportsReturnBloom && Math.abs(movement) >= 2) {
        appendBloom(-direction, secondDelay, 0.5);
      }
    }

    events = bloomed.sort(
      (a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step,
    );
  }

  if (events.length === 0) return [];

  if (combinationModeEnabled(combinationModeMask, 2) && activeRows.length > 1) {
    /** @type {ScheduledNote[]} */
    const countered = [];
    const pulseQuarters = pulseQuartersForIndex(pulseIndex);
    const combinationGesturePulse = Math.max(pulseQuarters, COMBINATION_GESTURE_PULSE_QUARTERS_FLOOR);

    for (const event of events) {
      if (countered.length >= MAX_COMBINED_PREVIEW_NOTES) break;

      countered.push(event);

      const activeIndex = Math.max(0, activeRows.indexOf(event.row));
      const modRow = activeRows[(activeIndex + 1) % activeRows.length];
      const modNotes = notes[modRow] ?? [];

      if (modNotes.length <= 0) continue;

      const modStep = (event.step + 1) % modNotes.length;

      if (stepSkipped[modRow]?.[modStep] || stepMuted[modRow]?.[modStep]) continue;
      if ((stepVelocity[modRow]?.[modStep] ?? 0) <= 0) continue;

      let counterDelay = combinationGesturePulse * 0.5;
      let start = event.start + counterDelay;

      if (eventStartCollides(events, start)) {
        counterDelay += combinationGesturePulse * 0.125;
        start = event.start + counterDelay;

        if (eventStartCollides(events, start)) continue;
      }

      if (start >= lengthQuarters - EPSILON) continue;

      const duration = Math.min((event.end - event.start) * 0.5, combinationGesturePulse * 0.375);

      if (duration <= EPSILON) continue;

      countered.push({
        ...event,
        start,
        end: start + duration,
        midi: echoNoteFromModStep(event.midi, modNotes[0] ?? 60, modNotes[modStep] ?? modNotes[0] ?? 60, scaleRoot, scaleModeIndex),
        velocity: Math.min(127, Math.max(1, Math.round((event.velocity + (stepVelocity[modRow]?.[modStep] ?? event.velocity)) * 0.31))),
        step: modStep,
      });
    }

    events = countered.sort(
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
