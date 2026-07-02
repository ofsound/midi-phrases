import { describe, expect, it } from "vitest";
import {
  buildPhraseSchedule,
  buildPhraseScheduleBeforeBandpass,
  combinationModes,
  buildPhraseScheduleWindowBeforeBandpass,
  probabilityPasses,
  stepTriggerCountAtBeat,
} from "./phraseSchedule.js";
import { defaultRowTimingOffsetIndex, defaultStepTimingMultiplierIndex, rowTimingOffsetIndexForQuarters } from "./stepCellLayout.js";

describe("stepTriggerCountAtBeat", () => {
  const timing = [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex];

  it("advances once per full row cycle at the selected step position", () => {
    expect(stepTriggerCountAtBeat({
      beat: 0,
      step: 0,
      rowTimingOffset: defaultRowTimingOffsetIndex,
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(0);
    expect(stepTriggerCountAtBeat({
      beat: 2,
      step: 0,
      rowTimingOffset: defaultRowTimingOffsetIndex,
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(1);
    expect(stepTriggerCountAtBeat({
      beat: 5,
      step: 1,
      rowTimingOffset: defaultRowTimingOffsetIndex,
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(2);
  });

  it("waits for a delayed row offset and ignores skipped steps", () => {
    expect(stepTriggerCountAtBeat({
      beat: 0,
      step: 0,
      rowTimingOffset: rowTimingOffsetIndexForQuarters(0.25),
      stepTimingMultiplier: timing,
      pulseIndex: 1,
    })).toBe(-1);
    expect(stepTriggerCountAtBeat({
      beat: 4,
      step: 1,
      rowTimingOffset: defaultRowTimingOffsetIndex,
      stepTimingMultiplier: timing,
      stepSkipped: [false, true],
      pulseIndex: 1,
    })).toBe(-1);
  });
});

describe("step probability", () => {
  it("treats 100 as always on", () => {
    for (let triggerCount = 0; triggerCount < 128; triggerCount += 1) {
      expect(probabilityPasses(0, triggerCount, 100)).toBe(true);
    }
  });

  it("defaults missing probability values to always on", () => {
    const schedule = buildPhraseScheduleBeforeBandpass({
      notes: [[60], [], [], []],
      rowMuted: [false, true, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1], [], [], []],
      stepTimingMultiplier: [[defaultStepTimingMultiplierIndex], [], [], []],
      stepVelocity: [[100], [], [], []],
      stepMuted: [[false], [], [], []],
      stepSkipped: [[false], [], [], []],
      lengthQuarters: 1,
    });

    expect(schedule.some((note) => note.midi === 60 && note.start === 0)).toBe(true);
  });
});

function combinationSchedule(pulseIndex, combinationModeMask) {
  return buildPhraseScheduleBeforeBandpass({
    notes: [[60], [62, 67], [], []],
    rowMuted: [false, false, true, true],
    rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
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

function fourRowOffsetHocketParams(overrides = {}) {
  const timing3 = [
    defaultStepTimingMultiplierIndex,
    defaultStepTimingMultiplierIndex,
    defaultStepTimingMultiplierIndex,
  ];
  const timing4 = [
    defaultStepTimingMultiplierIndex,
    defaultStepTimingMultiplierIndex,
    defaultStepTimingMultiplierIndex,
    defaultStepTimingMultiplierIndex,
  ];

  return {
    notes: [
      [53, 55, 55],
      [57, 53, 58],
      [55, 62, 57, 57],
      [50, 48, 48],
    ],
    rowMuted: [false, false, false, false],
    rowTimingOffset: [
      defaultRowTimingOffsetIndex,
      defaultRowTimingOffsetIndex + 1,
      defaultRowTimingOffsetIndex + 2,
      defaultRowTimingOffsetIndex + 3,
    ],
    stepDurationFraction: [
      [1, 1, 1],
      [1, 1, 1],
      [1, 1, 1, 1],
      [1, 1, 1],
    ],
    stepTimingMultiplier: [timing3, timing3, timing4, timing3],
    stepVelocity: [
      [100, 100, 100],
      [100, 100, 100],
      [100, 100, 100, 100],
      [100, 100, 100],
    ],
    stepMuted: [
      [false, false, false],
      [false, false, false],
      [false, false, false, false],
      [false, false, false],
    ],
    stepSkipped: [
      [false, false, false],
      [false, false, false],
      [false, false, false, false],
      [false, false, false],
    ],
    pulseIndex: 1,
    combinationModeMask: 1 << 6,
    lengthQuarters: 32,
    scaleRoot: 0,
    scaleModeIndex: 1,
    ...overrides,
  };
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
  it("places Round Robin after Cross-Mod and before Bloom in the mode rail", () => {
    expect(combinationModes.map((mode) => mode.name)).toEqual([
      "Cross-Mod",
      "Canon",
      "Retro-Inv",
      "Hocket",
      "Round Robin",
      "Bloom",
      "Counter",
      "Echo",
      "Weave",
    ]);
    expect(combinationModes[1]).toMatchObject({index: 7, bit: 128});
    expect(combinationModes[2]).toMatchObject({index: 8, bit: 256});
    expect(combinationModes[3]).toMatchObject({index: 6, bit: 64});
    expect(combinationModes[4]).toMatchObject({index: 5, bit: 32});
    expect(combinationModes[8]).toMatchObject({index: 4, bit: 16});
  });

  it("adds scale-aware delayed Canon followers on the next active row", () => {
    const schedule = buildPhraseScheduleBeforeBandpass({
      notes: [[60, 64], [67], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1, 1], [1], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      stepVelocity: [[100, 100], [100], [], []],
      stepMuted: [[false, false], [false], [], []],
      stepSkipped: [[false, false], [false], [], []],
      pulseIndex: 1,
      combinationModeMask: 1 << 7,
      lengthQuarters: 3,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(schedule).toEqual(expect.arrayContaining([
      expect.objectContaining({start: 1, midi: 67, row: 1, step: 0, velocity: 78}),
      expect.objectContaining({start: 2, midi: 71, row: 1, step: 0, velocity: 78}),
    ]));
  });

  it("adds reversed scale-degree inversion followers for Retro-Inv", () => {
    const schedule = buildPhraseScheduleBeforeBandpass({
      notes: [[60, 64, 67], [72], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1, 1, 1], [1], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      stepVelocity: [[100, 100, 100], [100], [], []],
      stepMuted: [[false, false, false], [false], [], []],
      stepSkipped: [[false, false, false], [false], [], []],
      pulseIndex: 1,
      combinationModeMask: 1 << 8,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(schedule).toEqual(expect.arrayContaining([
      expect.objectContaining({start: 0.5, midi: 65, row: 1, step: 0, velocity: 68}),
    ]));
  });

  it("leaves a single active row unchanged in Canon and Retro-Inv modes", () => {
    const baseParams = {
      notes: [[60, 64], [], [], []],
      rowMuted: [false, true, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1, 1], [], [], []],
      stepTimingMultiplier: [[defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex], [], [], []],
      stepVelocity: [[100, 100], [], [], []],
      stepMuted: [[false, false], [], [], []],
      stepSkipped: [[false, false], [], [], []],
      pulseIndex: 1,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    };
    const base = buildPhraseScheduleBeforeBandpass({...baseParams, combinationModeMask: 0});
    const imitated = buildPhraseScheduleBeforeBandpass({
      ...baseParams,
      combinationModeMask: (1 << 7) | (1 << 8),
    });

    expect(imitated).toEqual(base);
  });

  it("hockets overlapping rows into one short handoff per pulse slice", () => {
    const schedule = buildPhraseScheduleBeforeBandpass({
      notes: [[60], [67], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1], [1], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      stepVelocity: [[100], [100], [], []],
      stepMuted: [[false], [false], [], []],
      stepSkipped: [[false], [false], [], []],
      pulseIndex: 1,
      combinationModeMask: 1 << 6,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(schedule.map((note) => note.start)).toEqual([0, 0.5, 1, 1.5]);
    expect(schedule.map((note) => note.row)).toEqual([0, 1, 0, 1]);
    expect(schedule.every((note) => note.end - note.start <= 0.425 + 1e-9)).toBe(true);

    for (const start of [0, 0.5, 1, 1.5]) {
      expect(schedule.filter((note) => note.start === start)).toHaveLength(1);
    }
  });

  it("drops tiny Hocket slice overlaps instead of rendering blips", () => {
    const schedule = buildPhraseScheduleBeforeBandpass({
      notes: [[60], [67], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[0.2], [1], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      stepVelocity: [[100], [0], [], []],
      stepMuted: [[false], [false], [], []],
      stepSkipped: [[false], [false], [], []],
      pulseIndex: 1,
      combinationModeMask: 1 << 6,
      lengthQuarters: 1,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(schedule).toEqual([]);
  });

  it("leaves a single active row unchanged in Hocket mode", () => {
    const base = buildPhraseScheduleBeforeBandpass({
      notes: [[60], [], [], []],
      rowMuted: [false, true, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1], [], [], []],
      stepTimingMultiplier: [[defaultStepTimingMultiplierIndex], [], [], []],
      stepVelocity: [[100], [], [], []],
      stepMuted: [[false], [], [], []],
      stepSkipped: [[false], [], [], []],
      pulseIndex: 1,
      combinationModeMask: 0,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });
    const hocketed = buildPhraseScheduleBeforeBandpass({
      notes: [[60], [], [], []],
      rowMuted: [false, true, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1], [], [], []],
      stepTimingMultiplier: [[defaultStepTimingMultiplierIndex], [], [], []],
      stepVelocity: [[100], [], [], []],
      stepMuted: [[false], [], [], []],
      stepSkipped: [[false], [], [], []],
      pulseIndex: 1,
      combinationModeMask: 1 << 6,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(hocketed).toEqual(base);
  });

  it("continues deterministic Hocket choices after the first phrase repeat", () => {
    const schedule = buildPhraseScheduleBeforeBandpass(fourRowOffsetHocketParams());
    const firstCycle = schedule
      .filter((note) => note.start >= 0 && note.start < 8)
      .map((note) => [note.start, note.midi]);
    const secondCycle = schedule
      .filter((note) => note.start >= 8 && note.start < 16)
      .map((note) => [note.start - 8, note.midi]);

    expect(firstCycle.length).toBeGreaterThan(0);
    expect(secondCycle.length).toBeGreaterThan(0);
    expect(secondCycle).not.toEqual(firstCycle);
  });

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

  it("round-robins active rows and chooses one note during transition overlap", () => {
    const schedule = buildPhraseScheduleBeforeBandpass({
      notes: [[60], [67], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [rowTimingOffsetIndexForQuarters(0.5), rowTimingOffsetIndexForQuarters(0.5), defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[1], [1], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      stepVelocity: [[100], [100], [], []],
      stepMuted: [[false], [false], [], []],
      stepSkipped: [[false], [false], [], []],
      pulseIndex: 1,
      combinationModeMask: 1 << 5,
      lengthQuarters: 4,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });
    const startsByBeat = new Map();

    for (const note of schedule) {
      startsByBeat.set(note.start, [...(startsByBeat.get(note.start) ?? []), note]);
    }

    expect(startsByBeat.get(0.5)?.map((note) => note.midi)).toEqual([60]);
    expect(startsByBeat.get(1.5)).toHaveLength(1);
    expect([60, 67]).toContain(startsByBeat.get(1.5)?.[0].midi);
    expect(startsByBeat.get(2.5)?.map((note) => note.midi)).toEqual([67]);
    expect(schedule).toHaveLength(4);
  });
});

function baseWindowScheduleParams(overrides = {}) {
  return {
    notes: [[60], [], [], []],
    rowMuted: [false, true, true, true],
    rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
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

  it("matches the full Hocket schedule for later windows without relying on lookback", () => {
    const params = fourRowOffsetHocketParams();
    const full = buildPhraseScheduleBeforeBandpass(params);
    const windowed = buildPhraseScheduleWindowBeforeBandpass({
      ...params,
      windowStartQuarters: 8,
      windowEndQuarters: 16,
      windowLookbackQuarters: 0,
    });

    expect(comparableNotes(windowed)).toEqual(comparableNotes(clippedToWindow(full, 8, 16)));
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
