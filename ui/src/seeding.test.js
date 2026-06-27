import { describe, expect, it } from "vitest";
import {
  generateSeededPhraseRows,
  normalizeSeedingSettings,
  rhythmInterleaveRatio,
  seedingRhythmStepMax,
  seedingRhythmStepMin,
} from "./seeding.js";
import { isMidiInScale } from "./scaleUtils.js";
import { timingOffsetValues } from "./stepCellLayout.js";

describe("normalizeSeedingSettings", () => {
  it("clamps public settings to supported ranges", () => {
    const settings = normalizeSeedingSettings({
      phraseLength: 200,
      rangeSemitones: -5,
      repetition: 500,
      complexity: -1,
      randomness: Number.NaN,
      rhythmStep: 99,
      seed: -10,
    });

    expect(settings.phraseLength).toBe(16);
    expect(settings.rangeSemitones).toBe(2);
    expect(settings.repetition).toBe(100);
    expect(settings.complexity).toBe(0);
    expect(settings.randomness).toBe(0);
    expect(settings.rhythmStep).toBe(seedingRhythmStepMax);
    expect(settings.seed).toBe(1);
  });

  it("maps legacy rhythm modes to stepped values", () => {
    expect(normalizeSeedingSettings({ rhythmMode: "overlap" }).rhythmStep).toBe(seedingRhythmStepMin);
    expect(normalizeSeedingSettings({ rhythmMode: "interleave" }).rhythmStep).toBe(seedingRhythmStepMax);
  });

  it("maps legacy range presets to semitone spans", () => {
    expect(normalizeSeedingSettings({ rangeIndex: 0 }).rangeSemitones).toBe(5);
    expect(normalizeSeedingSettings({ rangeIndex: 1 }).rangeSemitones).toBe(8);
    expect(normalizeSeedingSettings({ rangeIndex: 2 }).rangeSemitones).toBe(12);
    expect(normalizeSeedingSettings({ rangeIndex: 3 }).rangeSemitones).toBe(16);
  });

  it("clamps range semitones to four octaves", () => {
    expect(normalizeSeedingSettings({ rangeSemitones: 200 }).rangeSemitones).toBe(48);
  });

  it("allows short two-step phrases", () => {
    const settings = normalizeSeedingSettings({ phraseLength: -10 });

    expect(settings.phraseLength).toBe(2);
  });

});

describe("generateSeededPhraseRows", () => {
  it("is deterministic for a fixed seed", () => {
    const options = { root: 2, modeIndex: 1, phraseLength: 16, seed: 4242 };

    expect(generateSeededPhraseRows(options)).toEqual(generateSeededPhraseRows(options));
  });

  it("returns four monophonic rows with matching step arrays", () => {
    const result = generateSeededPhraseRows({ phraseLength: 2, seed: 7 });

    expect(result.notes).toHaveLength(4);
    for (let row = 0; row < 4; row += 1) {
      expect(result.notes[row]).toHaveLength(2);
      expect(result.stepTimingMultiplier[row]).toHaveLength(2);
      expect(result.stepDurationFraction[row]).toHaveLength(2);
      expect(result.stepVelocity[row]).toHaveLength(2);
      expect(result.stepMuted[row]).toHaveLength(2);
      expect(result.stepSkipped[row]).toHaveLength(2);
      expect(result.stepProbability[row]).toHaveLength(2);
      expect(result.stepCycle[row]).toHaveLength(2);
      expect(result.stepCycleOffset[row]).toHaveLength(2);
    }
  });

  it("keeps generated pitches inside the chosen scale", () => {
    const root = 5;
    const modeIndex = 10;
    const result = generateSeededPhraseRows({ root, modeIndex, phraseLength: 24, seed: 99 });

    for (const row of result.notes) {
      for (const midi of row) {
        expect(isMidiInScale(midi, root, modeIndex)).toBe(true);
      }
    }
  });

  it("mirrors each row when symmetry is enabled", () => {
    const result = generateSeededPhraseRows({
      phraseLength: 8,
      symmetry: true,
      repetition: 0,
      seed: 23,
    });

    for (const row of result.notes) {
      expect(row.slice(4)).toEqual(row.slice(0, 4).reverse());
    }
  });

  it("matches legacy overlap and interleave presets at the slider extremes", () => {
    const overlap = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      randomness: 0,
      complexity: 0,
      rhythmStep: seedingRhythmStepMin,
      seed: 42,
    });
    const interleave = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      randomness: 0,
      complexity: 0,
      rhythmStep: seedingRhythmStepMax,
      seed: 42,
    });

    expect(overlap.rowTimingOffset.map((index) => timingOffsetValues[index])).toEqual([0, 0, 0, 0]);
    expect(interleave.rowTimingOffset.map((index) => timingOffsetValues[index])).toEqual([0, 0.25, -0.25, 0.5]);
    expect(overlap.stepDurationFraction[0][0]).toBeGreaterThan(interleave.stepDurationFraction[0][0]);
    expect(overlap.stepTimingMultiplier[0][0]).not.toBe(interleave.stepTimingMultiplier[0][0]);
  });

  it("ramps interleave amount across the slider", () => {
    expect(rhythmInterleaveRatio(seedingRhythmStepMin)).toBe(0);
    expect(rhythmInterleaveRatio(seedingRhythmStepMax)).toBe(1);
    expect(rhythmInterleaveRatio(3)).toBeCloseTo(3 / 7, 5);
  });

  it("produces distinct rhythm output for every slider step at short phrase length", () => {
    const base = { phraseLength: 3, repetition: 0, randomness: 0, complexity: 0, seed: 42 };
    const signatures = [];

    for (let rhythmStep = seedingRhythmStepMin; rhythmStep <= seedingRhythmStepMax; rhythmStep += 1) {
      const result = generateSeededPhraseRows({ ...base, rhythmStep });
      signatures.push(JSON.stringify({
        offsets: result.rowTimingOffset,
        durations: result.stepDurationFraction,
        multipliers: result.stepTimingMultiplier,
      }));
    }

    expect(new Set(signatures).size).toBe(seedingRhythmStepMax + 1);
  });
});
