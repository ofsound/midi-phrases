import { describe, expect, it } from "vitest";
import {
  buildPhraseSchedule,
  buildPhraseScheduleBeforeBandpass,
  buildPhraseScheduleWindow,
  combinationModes,
  buildPhraseScheduleWindowBeforeBandpass,
  applyWeaveMonophony,
  cleanupUnisonOverlaps,
  filterScheduleForNoteOnEmission,
  probabilityPasses,
  stepTriggerCountAtBeat,
  suppressHeldNoteRetriggers,
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
  it("places Tendril before Echo and Weave in the mode rail", () => {
    expect(combinationModes.map((mode) => mode.name)).toEqual([
      "Cross-Mod",
      "Canon",
      "Retro-Inv",
      "Hocket",
      "Tendril",
      "Echo",
      "Weave",
    ]);
    expect(combinationModes[1]).toMatchObject({index: 7, bit: 128});
    expect(combinationModes[2]).toMatchObject({index: 8, bit: 256});
    expect(combinationModes[3]).toMatchObject({index: 6, bit: 64});
    expect(combinationModes[4]).toMatchObject({index: 1, bit: 2});
    expect(combinationModes[6]).toMatchObject({index: 4, bit: 16});
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

  it("builds Cross-Mod + Hocket long previews without scanning slices outside the emit window", () => {
    const params = fourRowOffsetHocketParams({
      combinationModeMask: (1 << 0) | (1 << 6),
      lengthQuarters: 300,
    });
    const startedAt = performance.now();
    const schedule = buildPhraseScheduleBeforeBandpass(params);
    const elapsedMs = performance.now() - startedAt;

    expect(schedule.length).toBeGreaterThan(0);
    expect(elapsedMs).toBeLessThan(500);

    const windowed = buildPhraseScheduleWindowBeforeBandpass({
      ...params,
      windowStartQuarters: 32,
      windowEndQuarters: 64,
      windowLookbackQuarters: 0,
    });

    expect(comparableNotes(windowed)).toEqual(
      comparableNotes(clippedToWindow(schedule, 32, 64)),
    );
  });

  it("keeps source gates for Hocket when Cross-Mod also borrows duration", () => {
    const baseParams = {
      notes: [[60], [67], [72], []],
      rowMuted: [false, false, false, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      stepDurationFraction: [[0.5], [1], [1], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
      ],
      stepVelocity: [[100], [100], [100], []],
      stepMuted: [[false], [false], [false], []],
      stepSkipped: [[false], [false], [false], []],
      pulseIndex: 1,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    };
    const hocketOnly = buildPhraseScheduleBeforeBandpass({
      ...baseParams,
      combinationModeMask: 1 << 6,
    });
    const crossModHocket = buildPhraseScheduleBeforeBandpass({
      ...baseParams,
      combinationModeMask: (1 << 0) | (1 << 6),
    });

    expect(hocketOnly.length).toBeGreaterThan(0);
    expect(crossModHocket.length).toBe(hocketOnly.length);
    expect(crossModHocket.map((note) => [note.start, note.end - note.start])).toEqual(
      hocketOnly.map((note) => [note.start, note.end - note.start]),
    );
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

  it("keeps Tendril curls on sparse gesture anchors at quarter pulse", () => {
    const quarterPulseStarts = generatedStarts(combinationSchedule(1, 1 << 1), 59);
    const halfPulseStart = firstGeneratedStart(combinationSchedule(2, 1 << 1), 59);

    expect(quarterPulseStarts).toEqual([0.5, 2.5]);
    expect(halfPulseStart).toBeCloseTo(0.5, 9);
  });

  it("adds Tendril answers from the next row contour", () => {
    const quarterPulseStart = firstGeneratedStart(combinationSchedule(1, 1 << 1), 65);
    const halfPulseStart = firstGeneratedStart(combinationSchedule(2, 1 << 1), 65);

    expect(quarterPulseStart).toBeCloseTo(1.5, 9);
    expect(halfPulseStart).toBeCloseTo(1, 9);
  });

  it("folds wide Tendril answers into the source register", () => {
    const schedule = buildPhraseScheduleBeforeBandpass({
      notes: [[60], [72, 91], [], []],
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
      pulseIndex: 1,
      combinationModeMask: 1 << 1,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(schedule).toEqual(expect.arrayContaining([
      expect.objectContaining({start: 1.5, midi: 55, row: 1, step: 1}),
    ]));
    expect(schedule).not.toEqual(expect.arrayContaining([
      expect.objectContaining({start: 1.5, midi: 79, row: 1, step: 1}),
    ]));
  });

  it("keeps Tendril note-ons on quarter-gesture slots at whole-note pulse", () => {
    const schedule = combinationSchedule(3, 1 << 1);
    const tendrilStarts = schedule
      .filter((note) => note.start > 0)
      .map((note) => note.start);

    expect(tendrilStarts).toEqual(expect.arrayContaining([1, 2, 3]));
    expect(tendrilStarts.every((start) => Math.abs(start - Math.round(start)) < 1e-9)).toBe(true);
  });

  it("ignores retired combination bits", () => {
    const base = combinationSchedule(1, 0);

    expect(combinationSchedule(1, 1 << 2)).toEqual(base);
    expect(combinationSchedule(1, 1 << 5)).toEqual(base);
  });

  it("does not merge combination attacks again after the processing rail", () => {
    const schedule = buildPhraseSchedule({
      notes: [[60], [60], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      rowMidiChannel: [1, 1, 3, 4],
      stepDurationFraction: [[1], [1], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      stepVelocity: [[72], [104], [], []],
      stepMuted: [[false], [false], [], []],
      stepSkipped: [[false], [false], [], []],
      pulseIndex: 1,
      combinationModeMask: 1,
      lengthQuarters: 1,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });
    const firstAttack = schedule.filter((note) => note.start === 0 && note.midi === 60 && note.channel === 1);

    expect(firstAttack).toHaveLength(1);
    expect(firstAttack[0]).toMatchObject({velocity: 72, row: 0, step: 0});
  });

  it("merges near same-channel same-pitch attacks inside the unison cleanup window", () => {
    const cleaned = cleanupUnisonOverlaps([
      {start: 0, end: 0.25, midi: 60, velocity: 72, row: 0, step: 0, channel: 1},
      {start: 1 / 192, end: 0.4, midi: 60, velocity: 104, row: 1, step: 0, channel: 1},
    ]);

    expect(cleaned).toHaveLength(1);
    expect(cleaned[0]).toMatchObject({start: 0, end: 0.4, midi: 60, velocity: 104, row: 0, step: 0, channel: 1});
  });

  it("keeps different pitches and later retriggers during unison cleanup", () => {
    expect(cleanupUnisonOverlaps([
      {start: 0, end: 0.25, midi: 60, velocity: 90, row: 0, step: 0, channel: 1},
      {start: 0, end: 0.25, midi: 62, velocity: 90, row: 1, step: 0, channel: 1},
    ])).toHaveLength(2);

    const crossChannelUnison = cleanupUnisonOverlaps([
      {start: 0, end: 0.25, midi: 60, velocity: 90, row: 0, step: 0, channel: 1},
      {start: 0, end: 0.25, midi: 60, velocity: 104, row: 1, step: 0, channel: 2},
    ]);

    expect(crossChannelUnison).toHaveLength(1);
    expect(crossChannelUnison[0]).toMatchObject({midi: 60, channel: 1, velocity: 104});

    expect(cleanupUnisonOverlaps([
      {start: 0, end: 0.25, midi: 60, velocity: 90, row: 0, step: 0, channel: 1},
      {start: 0.25, end: 0.5, midi: 60, velocity: 90, row: 1, step: 0, channel: 1},
    ])).toHaveLength(2);
  });

  it("merges overlapping same-channel same-pitch notes into one held note", () => {
    const cleaned = suppressHeldNoteRetriggers([
      {start: 0, end: 1, midi: 60, velocity: 72, row: 0, step: 0, channel: 1},
      {start: 0.5, end: 1.5, midi: 60, velocity: 104, row: 1, step: 0, channel: 1},
      {start: 1.25, end: 2, midi: 60, velocity: 96, row: 2, step: 0, channel: 1},
    ]);

    expect(cleaned).toEqual([
      {start: 0, end: 2, midi: 60, velocity: 72, row: 0, step: 0, channel: 1, extendedByHeldOverlap: true},
    ]);
  });

  it("keeps adjacent same-pitch notes and different pitch overlaps", () => {
    const cleaned = suppressHeldNoteRetriggers([
      {start: 0, end: 0.5, midi: 60, velocity: 72, row: 0, step: 0, channel: 1},
      {start: 0.5, end: 1, midi: 60, velocity: 104, row: 1, step: 0, channel: 1},
      {start: 0.25, end: 0.75, midi: 62, velocity: 96, row: 2, step: 0, channel: 1},
    ]);

    expect(cleaned).toHaveLength(3);
    expect(cleaned.map((note) => [note.start, note.end, note.midi, note.channel])).toEqual([
      [0, 0.5, 60, 1],
      [0.25, 0.75, 62, 1],
      [0.5, 1, 60, 1],
    ]);
  });

  it("merges overlapping same-pitch notes across channels", () => {
    const cleaned = suppressHeldNoteRetriggers([
      {start: 0, end: 0.5, midi: 60, velocity: 72, row: 0, step: 0, channel: 1},
      {start: 0.25, end: 0.75, midi: 60, velocity: 96, row: 3, step: 0, channel: 2},
    ]);

    expect(cleaned).toEqual([
      {start: 0, end: 0.75, midi: 60, velocity: 72, row: 0, step: 0, channel: 1, extendedByHeldOverlap: true},
    ]);
  });

  it("suppresses retriggers created by final octave transforms", () => {
    const schedule = buildPhraseSchedule({
      notes: [[60], [72], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, rowTimingOffsetIndexForQuarters(0.5), defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      rowMidiChannel: [1, 1, 3, 4],
      stepDurationFraction: [[1], [1], [], []],
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      stepVelocity: [[100], [80], [], []],
      stepMuted: [[false], [false], [], []],
      stepSkipped: [[false], [false], [], []],
      stepCycle: [[2], [2], [], []],
      stepCycleOffset: [[1], [1], [], []],
      pulseIndex: 1,
      combinationModeMask: 0,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
      octavizerDown8vaEnabled: true,
      octavizerDown8vaRelativeVelocity: 0,
      noteBandpassLowMidi: 60,
      noteBandpassHighMidi: 60,
    });

    expect(schedule.map((note) => [note.start, note.end, note.midi, note.channel, note.velocity])).toEqual([
      [0, 1.5, 60, 1, 100],
    ]);
  });

  it("trims Weave preview notes to the next selected attack", () => {
    const schedule = buildPhraseSchedule({
      notes: [[60], [64], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, rowTimingOffsetIndexForQuarters(0.5), defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      rowMidiChannel: [1, 2, 3, 4],
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
      combinationModeMask: 1 << 4,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(schedule.map((note) => [note.start, note.end, note.midi])).toEqual([
      [0, 0.5, 60],
      [0.5, 1, 64],
      [1, 1.5, 60],
      [1.5, 2, 64],
    ]);
  });

  it("selects one final event per start when applying Weave monophony", () => {
    const woven = applyWeaveMonophony([
      {start: 0, end: 1, midi: 60, velocity: 100, row: 0, step: 0, channel: 1},
      {start: 0, end: 1, midi: 64, velocity: 100, row: 1, step: 0, channel: 2},
      {start: 0.5, end: 1.5, midi: 67, velocity: 100, row: 2, step: 0, channel: 3},
    ]);

    expect(woven).toHaveLength(2);
    expect(woven[0].end).toBe(0.5);
    expect(woven[1]).toMatchObject({start: 0.5, midi: 67});
  });

  it("keeps Weave choices stable across phrase repeats", () => {
    const schedule = buildPhraseSchedule({
      notes: [[60], [64], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      rowMidiChannel: [1, 2, 3, 4],
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
      combinationModeMask: 1 << 4,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
    });

    expect(schedule.map((note) => [note.start, note.midi])).toEqual([
      [0, 64],
      [1, 64],
    ]);
  });

  it("keeps octavizer copies after Weave thins multi-row collisions", () => {
    const schedule = buildPhraseSchedule({
      notes: [[60], [64], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      rowMidiChannel: [1, 2, 3, 4],
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
      combinationModeMask: 1 << 4,
      lengthQuarters: 2,
      scaleRoot: 0,
      scaleModeIndex: 1,
      octavizerDown8vaEnabled: true,
      octavizerUp8vaEnabled: true,
    });

    const sameStart = schedule.filter((note) => note.start === 0);

    expect(sameStart.map((note) => note.midi).sort((a, b) => a - b)).toEqual([52, 64, 76]);
  });

  it("keeps shimmer taps after Weave thins multi-row collisions", () => {
    const schedule = buildPhraseSchedule({
      notes: [[60], [64], [], []],
      rowMuted: [false, false, true, true],
      rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
      rowMidiChannel: [1, 2, 3, 4],
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
      combinationModeMask: 1 << 4,
      lengthQuarters: 4,
      scaleRoot: 0,
      scaleModeIndex: 1,
      shimmerEnabled: true,
      shimmerDelayMultiplierIndex: defaultStepTimingMultiplierIndex,
      shimmerFeedbackPercent: 80,
      shimmerMixPercent: 100,
    });

    expect(schedule.some((note) => note.midi === 76 && note.start > 0)).toBe(true);
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

  it("collects Hocket history from loop start when loop output bounds are active", () => {
    const params = fourRowOffsetHocketParams();
    const loopStart = 8;
    const loopEnd = 16;
    const loopOutput = buildPhraseScheduleBeforeBandpass({
      ...params,
      loopOutputStartQuarters: loopStart,
      loopOutputEndQuarters: loopEnd,
      lengthQuarters: loopEnd,
    });
    const naiveClip = clippedToWindow(buildPhraseScheduleBeforeBandpass(params), loopStart, loopEnd);

    expect(loopOutput.every((note) => note.start >= loopStart && note.start < loopEnd)).toBe(true);
    expect(comparableNotes(loopOutput)).not.toEqual(comparableNotes(naiveClip));
    expect(loopOutput[0]).toMatchObject({ start: loopStart, midi: 55, step: 2 });
  });

  it("matches loop-output Hocket windows to the loop-output schedule", () => {
    const params = fourRowOffsetHocketParams();
    const loopStart = 8;
    const loopEnd = 16;
    const loopOutput = buildPhraseScheduleBeforeBandpass({
      ...params,
      loopOutputStartQuarters: loopStart,
      loopOutputEndQuarters: loopEnd,
      lengthQuarters: loopEnd,
    });
    const windowed = buildPhraseScheduleWindowBeforeBandpass({
      ...params,
      windowStartQuarters: loopStart,
      windowEndQuarters: loopEnd,
      windowLookbackQuarters: 0,
      loopOutputStartQuarters: loopStart,
      loopOutputEndQuarters: loopEnd,
      lengthQuarters: loopEnd,
    });

    expect(comparableNotes(windowed)).toEqual(comparableNotes(loopOutput));
  });

  it("does not emit Hocket slices outside loop output bounds", () => {
    const loopStart = 8;
    const loopEnd = 16;
    const loopOutput = buildPhraseSchedule({
      ...fourRowOffsetHocketParams(),
      loopOutputStartQuarters: loopStart,
      loopOutputEndQuarters: loopEnd,
      lengthQuarters: loopEnd,
    });

    expect(loopOutput.length).toBeGreaterThan(0);
    expect(loopOutput.every((note) => note.start >= loopStart - 1e-6 && note.start < loopEnd - 1e-6)).toBe(true);
    expect(loopOutput.every((note) => note.end <= loopEnd + 1e-6)).toBe(true);
  });
});

function userLoopHocketParams(overrides = {}) {
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
      [48, 48, 48, 43],
      [43, 43, 46],
      [55, 51, 53, 58],
      [],
    ],
    rowMuted: [false, false, false, true],
    rowTimingOffset: [
      defaultRowTimingOffsetIndex,
      defaultRowTimingOffsetIndex + 1,
      defaultRowTimingOffsetIndex + 2,
      defaultRowTimingOffsetIndex,
    ],
    stepDurationFraction: [
      [1, 1, 1, 1],
      [1, 1, 1],
      [1, 1, 1, 1],
      [],
    ],
    stepTimingMultiplier: [timing4, timing3, timing4, []],
    stepVelocity: [
      [100, 100, 100, 100],
      [100, 100, 100],
      [100, 100, 100, 100],
      [],
    ],
    stepMuted: [
      [false, false, false, false],
      [false, false, false],
      [false, false, false, false],
      [],
    ],
    stepSkipped: [
      [false, false, false, false],
      [false, false, false],
      [false, false, false, false],
      [],
    ],
    pulseIndex: 1,
    combinationModeMask: 1 << 6,
    scaleRoot: 0,
    scaleModeIndex: 1,
    ...overrides,
  };
}

describe("hocket and echo combination parity", () => {
  const twoRowEchoParams = () => ({
    notes: [[60, 64, 67, 72], [48, 52, 55, 58], [], []],
    rowMuted: [false, false, true, true],
    rowTimingOffset: [defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex, defaultRowTimingOffsetIndex],
    stepDurationFraction: [
      [1, 1, 1, 1],
      [1, 1, 1, 1],
      [],
      [],
    ],
    stepTimingMultiplier: [
      [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex],
      [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex],
      [],
      [],
    ],
    stepVelocity: [
      [100, 100, 100, 100],
      [100, 100, 100, 100],
      [],
      [],
    ],
    stepMuted: [
      [false, false, false, false],
      [false, false, false, false],
      [],
      [],
    ],
    stepSkipped: [
      [false, false, false, false],
      [false, false, false, false],
      [],
      [],
    ],
    pulseIndex: 1,
    scaleRoot: 0,
    scaleModeIndex: 1,
    lengthQuarters: 8,
  });

  it("applies Echo after Hocket thinning, not to the full row schedule", () => {
    const params = twoRowEchoParams();
    const hocketOnly = buildPhraseSchedule({...params, combinationModeMask: 1 << 6});
    const echoOnly = buildPhraseSchedule({...params, combinationModeMask: 1 << 3});
    const hocketEcho = buildPhraseSchedule({...params, combinationModeMask: (1 << 6) | (1 << 3)});

    expect(hocketOnly.length).toBeGreaterThan(0);
    expect(echoOnly.length).toBeGreaterThan(hocketOnly.length);
    expect(hocketEcho.length).toBeGreaterThan(hocketOnly.length);
    expect(hocketEcho).not.toEqual(hocketOnly);
  });
});

describe("loop hocket emission parity", () => {
  it("drops held-overlap extensions that start before the loop window", () => {
    const filtered = filterScheduleForNoteOnEmission([
      {start: 3.5, end: 4.5, midi: 60, velocity: 100, row: 0, step: 0, channel: 1, extendedByHeldOverlap: true},
      {start: 4.25, end: 5, midi: 60, velocity: 100, row: 1, step: 0, channel: 1},
    ], 4, 8);

    expect(filtered).toEqual([
      {start: 4.25, end: 5, midi: 60, velocity: 100, row: 1, step: 0, channel: 1},
    ]);
  });

  it("matches the C++ loop 4-8 note-on list for the user hocket pattern", () => {
    const loopStart = 4;
    const loopEnd = 8;
    const schedule = buildPhraseSchedule({
      ...userLoopHocketParams(),
      loopOutputStartQuarters: loopStart,
      loopOutputEndQuarters: loopEnd,
      lengthQuarters: loopEnd,
    });

    expect(schedule).toHaveLength(12);
    expect(schedule.filter((note) => note.midi === 46)).toHaveLength(1);
    expect(schedule.map((note) => [Number(note.start.toFixed(3)), note.midi])).toEqual([
      [4, 48],
      [4.5, 55],
      [4.667, 48],
      [5, 43],
      [5.333, 48],
      [5.667, 46],
      [6, 51],
      [6.5, 53],
      [6.667, 53],
      [7, 53],
      [7.333, 43],
      [7.667, 43],
    ]);
  });

  it("uses the same loop-output schedule for windowed preview requests", () => {
    const loopStart = 4;
    const loopEnd = 8;
    const loopOutput = buildPhraseSchedule({
      ...userLoopHocketParams(),
      loopOutputStartQuarters: loopStart,
      loopOutputEndQuarters: loopEnd,
      lengthQuarters: loopEnd,
    });
    const windowed = buildPhraseScheduleWindow({
      ...userLoopHocketParams(),
      loopOutputStartQuarters: loopStart,
      loopOutputEndQuarters: loopEnd,
      lengthQuarters: loopEnd,
      windowStartQuarters: loopStart,
      windowEndQuarters: loopEnd,
    });

    expect(windowed).toEqual(loopOutput);
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
