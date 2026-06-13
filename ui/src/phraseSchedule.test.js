import { describe, expect, it } from "vitest";
import { buildPhraseScheduleBeforeBandpass } from "./phraseSchedule.js";
import { defaultStepTimingMultiplierIndex } from "./stepCellLayout.js";

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

describe("combination mode pulse-aware timing", () => {
  it("keeps Bloom ornaments on a half-note gesture clock at quarter pulse", () => {
    const quarterPulseStart = firstGeneratedStart(combinationSchedule(1, 1 << 4), 62);
    const halfPulseStart = firstGeneratedStart(combinationSchedule(2, 1 << 4), 62);

    expect(quarterPulseStart).toBeCloseTo(halfPulseStart, 9);
    expect(quarterPulseStart).toBeCloseTo(0.5, 9);
  });

  it("keeps Counter responses on a half-note gesture clock at quarter pulse", () => {
    const quarterPulseStart = firstGeneratedStart(combinationSchedule(1, 1 << 5), 65);
    const halfPulseStart = firstGeneratedStart(combinationSchedule(2, 1 << 5), 65);

    expect(halfPulseStart).toBeCloseTo(1, 9);
    expect(quarterPulseStart).toBeGreaterThanOrEqual(1);
  });
});
