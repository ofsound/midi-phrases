import { describe, expect, it } from "vitest";
import {
  buildPhraseSchedule,
  buildPhraseScheduleBeforeBandpass,
  buildPhraseScheduleWindowBeforeBandpass,
  stepTriggerCountAtBeat,
} from "./phraseSchedule.js";
import { defaultStepTimingMultiplierIndex } from "./stepCellLayout.js";

describe("stepTriggerCountAtBeat", () => {
  const timing = [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex];

  it("advances once per full row cycle at the selected step position", () => {
    expect(stepTriggerCountAtBeat({
      beat: 0,
      step: 0,
      rowTimingOffset: 3,
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(0);
    expect(stepTriggerCountAtBeat({
      beat: 2,
      step: 0,
      rowTimingOffset: 3,
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(1);
    expect(stepTriggerCountAtBeat({
      beat: 5,
      step: 1,
      rowTimingOffset: 3,
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(2);
  });

  it("waits for a delayed row offset and ignores skipped steps", () => {
    expect(stepTriggerCountAtBeat({
      beat: 0,
      step: 0,
      rowTimingOffset: 4,
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(-1);
    expect(stepTriggerCountAtBeat({
      beat: 4,
      step: 1,
      rowTimingOffset: 3,
      stepTimingMultiplier: timing,
      stepSkipped: [false, true],
      pulseIndex: 1,
    })).toBe(-1);
  });
});

function combinationSchedule(pulseIndex, combinationModeMask) {
  return buildPhraseScheduleBeforeBandpass({
    notes: [[60], [62, 67], [], []],
    rowMuted: [false, false, true, true],
    rowTimingOffset: [3, 3, 3, 3],
    stepDurationFraction: [[1], [1, 1], [], []],
    stepTimingMultiplier: [
      [defaultStepTimingMultiplierIndex],
      [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex],
      [],
      [],
    ],
    stepVelocity: [[100], [100, 100], [], []],
    stepMuted: [[false], [false, false], [], []],
    stepSkipped: [[false], [false, false], [], []],
    pulseIndex,
    combinationModeMask,
    lengthQuarters: 4,
    scaleRoot: 0,
    scaleModeIndex: 1,
  });
}

function firstGeneratedStart(schedule, midi) {
  return schedule.find((note) => note.midi === midi && note.start > 0)?.start;
}

function generatedStarts(schedule, midi) {
  return schedule
    .filter((note) => note.midi === midi && note.start > 0)
    .map((note) => note.start);
}

describe("combination mode pulse-aware timing", () => {
  it("keeps Bloom ornaments on sparse half-note anchors at quarter pulse", () => {
    const quarterPulseStarts = generatedStarts(combinationSchedule(1, 1 << 1), 59);
    const halfPulseStart = firstGeneratedStart(combinationSchedule(2, 1 << 1), 59);

    expect(quarterPulseStarts).toEqual([0.5, 2.5]);
    expect(halfPulseStart).toBeCloseTo(0.5, 9);
  });

  it("keeps Counter responses on a half-note gesture clock at quarter pulse", () => {
    const quarterPulseStart = firstGeneratedStart(combinationSchedule(1, 1 << 2), 65);
    const halfPulseStart = firstGeneratedStart(combinationSchedule(2, 1 << 2), 65);

    expect(halfPulseStart).toBeCloseTo(1, 9);
    expect(quarterPulseStart).toBeGreaterThanOrEqual(1);
  });
});

function baseWindowScheduleParams(overrides = {}) {
  return {
    notes: [[60], [], [], []],
    rowMuted: [false, true, true, true],
    rowTimingOffset: [3, 3, 3, 3],
    stepDurationFraction: [[1], [], [], []],
    stepTimingMultiplier: [[defaultStepTimingMultiplierIndex], [], [], []],
    stepVelocity: [[100], [], [], []],
    stepMuted: [[false], [], [], []],
    stepSkipped: [[false], [], [], []],
    stepProbability: [[100], [], [], []],
    stepCycle: [[1], [], [], []],
    stepCycleOffset: [[1], [], [], []],
    pulseIndex: 1,
    combinationModeMask: 0,
    lengthQuarters: 128,
    scaleRoot: 0,
    scaleModeIndex: 1,
    ...overrides,
  };
}

function clippedToWindow(schedule, start, end) {
  return schedule
    .filter((note) => note.end > start && note.start < end)
    .map((note) => ({
      ...note,
      start: Math.max(start, note.start),
      end: Math.min(end, note.end),
    }))
    .filter((note) => note.end > note.start);
}

function comparableNotes(schedule) {
  return schedule.map(({ start, end, midi, velocity, row, step }) => ({
    start,
    end,
    midi,
    velocity,
    row,
    step,
  }));
}

describe("windowed phrase schedule preview", () => {
  it("matches the full schedule for an initial simple window", () => {
    const params = baseWindowScheduleParams();
    const full = buildPhraseScheduleBeforeBandpass(params);
    const windowed = buildPhraseScheduleWindowBeforeBandpass({
      ...params,
      windowStartQuarters: 0,
      windowEndQuarters: 16,
    });

    expect(comparableNotes(windowed)).toEqual(comparableNotes(clippedToWindow(full, 0, 16)));
  });

  it("seeds step cycle and deterministic probability phases for later windows", () => {
    const params = baseWindowScheduleParams({
      notes: [[60, 62], [], [], []],
      stepDurationFraction: [[1, 1], [], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex],
        [],
        [],
        [],
      ],
      stepVelocity: [[100, 100], [], [], []],
      stepMuted: [[false, false], [], [], []],
      stepSkipped: [[false, false], [], [], []],
      stepProbability: [[50, 50], [], [], []],
      stepCycle: [[3, 4], [], [], []],
      stepCycleOffset: [[2, 4], [], [], []],
    });
    const full = buildPhraseScheduleBeforeBandpass(params);
    const windowed = buildPhraseScheduleWindowBeforeBandpass({
      ...params,
      windowStartQuarters: 96,
      windowEndQuarters: 112,
    });

    expect(comparableNotes(windowed)).toEqual(comparableNotes(clippedToWindow(full, 96, 112)));
  });

  it("clips notes that cross the requested window boundary", () => {
    const params = baseWindowScheduleParams({
      stepTimingMultiplier: [[defaultStepTimingMultiplierIndex + 12], [], [], []],
      lengthQuarters: 16,
    });
    const windowed = buildPhraseScheduleWindowBeforeBandpass({
      ...params,
      windowStartQuarters: 2,
      windowEndQuarters: 3,
    });

    expect(windowed).toHaveLength(1);
    expect(windowed[0].start).toBe(2);
    expect(windowed[0].end).toBe(3);
  });
});

describe("velocity tilt schedule preview", () => {
  it("tilts filtered notes around the pivot in velocity per octave", () => {
    const schedule = buildPhraseSchedule({
      ...baseWindowScheduleParams({
        notes: [[48, 60, 72], [], [], []],
        stepDurationFraction: [[1, 1, 1], [], [], []],
        stepTimingMultiplier: [
          [
            defaultStepTimingMultiplierIndex,
            defaultStepTimingMultiplierIndex,
            defaultStepTimingMultiplierIndex,
          ],
          [],
          [],
          [],
        ],
        stepVelocity: [[100, 100, 100], [], [], []],
        stepMuted: [[false, false, false], [], [], []],
        stepSkipped: [[false, false, false], [], [], []],
        stepProbability: [[100, 100, 100], [], [], []],
        stepCycle: [[1, 1, 1], [], [], []],
        stepCycleOffset: [[1, 1, 1], [], [], []],
        lengthQuarters: 3,
        noteBandpassLowMidi: 60,
        noteBandpassHighMidi: 72,
        velocityTiltPivotMidi: 60,
        velocityTiltAmount: 12,
      }),
    });
    const velocitiesByMidi = Object.fromEntries(
      schedule.map((note) => [note.midi, note.velocity]),
    );

    expect(velocitiesByMidi[48]).toBeUndefined();
    expect(velocitiesByMidi[60]).toBe(100);
    expect(velocitiesByMidi[72]).toBe(112);
  });
});

describe("global transpose schedule preview", () => {
  it("transposes after bandpass and remembers the requested semitone offset", () => {
    const schedule = buildPhraseSchedule({
      ...baseWindowScheduleParams({
        notes: [[60], [], [], []],
        lengthQuarters: 1,
        noteBandpassLowMidi: 60,
        noteBandpassHighMidi: 60,
        globalTransposeSemitones: 5,
      }),
    });

    expect(schedule).toHaveLength(1);
    expect(schedule[0].midi).toBe(65);
  });
});
