import { describe, expect, it } from "vitest";
import {
  applySeedingRowSettingsUpdate,
  createDefaultSeedModeRowSettings,
  displaySeedingRowSettings,
  generateSeededPhraseRows,
  mergeSeededPhraseRows,
  normalizeSeedModeState,
  normalizeSeedingRowSettings,
  normalizeSeedingSettings,
  phraseRowsFromGridState,
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

describe("normalizeSeedingRowSettings", () => {
  it("clamps per-row settings without rhythm overlap", () => {
    const settings = normalizeSeedingRowSettings({
      phraseLength: 200,
      rangeSemitones: -5,
      repetition: 500,
      complexity: -1,
      randomness: Number.NaN,
      seed: -10,
    });

    expect(settings.phraseLength).toBe(16);
    expect(settings.rangeSemitones).toBe(2);
    expect(settings.repetition).toBe(100);
    expect(settings.complexity).toBe(0);
    expect(settings.randomness).toBe(0);
    expect(settings.seed).toBe(1);
  });
});

describe("normalizeSeedModeState", () => {
  it("normalizes rhythm, row settings, and targets together", () => {
    const state = normalizeSeedModeState({
      rhythmStep: 99,
      rowSettings: [{ phraseLength: 2 }, { phraseLength: 5 }],
      rowTargets: [true, false],
    });

    expect(state.rhythmStep).toBe(seedingRhythmStepMax);
    expect(state.rowSettings[0].phraseLength).toBe(2);
    expect(state.rowSettings[1].phraseLength).toBe(5);
    expect(state.rowTargets).toEqual([true, false, false, false]);
  });
});

describe("displaySeedingRowSettings", () => {
  it("shows the first targeted row settings", () => {
    const rowSettings = createDefaultSeedModeRowSettings();
    rowSettings[2].phraseLength = 9;

    expect(displaySeedingRowSettings(rowSettings, [false, false, true, true]).phraseLength).toBe(9);
  });
});

describe("applySeedingRowSettingsUpdate", () => {
  it("writes updates to every targeted row", () => {
    const rowSettings = createDefaultSeedModeRowSettings();
    const next = applySeedingRowSettingsUpdate(rowSettings, [true, false, true, false], {
      phraseLength: 7,
    });

    expect(next[0].phraseLength).toBe(7);
    expect(next[1].phraseLength).toBe(rowSettings[1].phraseLength);
    expect(next[2].phraseLength).toBe(7);
    expect(next[3].phraseLength).toBe(rowSettings[3].phraseLength);
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

  it("uses independent per-row settings when rowSettings are provided", () => {
    const rowSettings = createDefaultSeedModeRowSettings();
    rowSettings[0].seed = 11;
    rowSettings[1].seed = 22;
    rowSettings[0].phraseLength = 3;
    rowSettings[1].phraseLength = 6;

    const result = generateSeededPhraseRows({
      rhythmStep: seedingRhythmStepMin,
      rowSettings,
      root: 0,
      modeIndex: 0,
    });

    expect(result.notes[0]).toHaveLength(3);
    expect(result.notes[1]).toHaveLength(6);
    expect(result.notes[0]).not.toEqual(result.notes[1]);
  });
});

describe("phraseRowsFromGridState", () => {
  it("maps grid fields to generated phrase row shape", () => {
    const rows = phraseRowsFromGridState({
      grid: [[60, 62], [64, 65], [67, 69], [71, 72]],
      stepTimingMultiplier: [[3, 4], [3, 3], [3, 3], [3, 3]],
      rowTimingOffset: [3, 4, 5, 6],
    });

    expect(rows.notes).toEqual([[60, 62], [64, 65], [67, 69], [71, 72]]);
    expect(rows.stepTimingMultiplier[0]).toEqual([3, 4]);
    expect(rows.rowTimingOffset).toEqual([3, 4, 5, 6]);
  });
});

describe("mergeSeededPhraseRows", () => {
  const baseOptions = { phraseLength: 4, seed: 101, repetition: 0, randomness: 0, complexity: 0 };

  it("only replaces targeted rows", () => {
    const existing = generateSeededPhraseRows({ ...baseOptions, seed: 1 });
    const generated = generateSeededPhraseRows({ ...baseOptions, seed: 2 });
    const merged = mergeSeededPhraseRows(existing, generated, [false, true, false, false]);

    expect(merged.notes[0]).toEqual(existing.notes[0]);
    expect(merged.notes[1]).toEqual(generated.notes[1]);
    expect(merged.notes[2]).toEqual(existing.notes[2]);
    expect(merged.notes[3]).toEqual(existing.notes[3]);
    expect(merged.stepVelocity[1]).toEqual(generated.stepVelocity[1]);
    expect(merged.rowTimingOffset[1]).toEqual(generated.rowTimingOffset[1]);
  });

  it("returns full generated output when all rows are targeted", () => {
    const existing = generateSeededPhraseRows({ ...baseOptions, seed: 1 });
    const generated = generateSeededPhraseRows({ ...baseOptions, seed: 2 });
    const merged = mergeSeededPhraseRows(existing, generated, [true, true, true, true]);

    expect(merged).toEqual(generated);
  });

  it("preserves existing rows when none are targeted", () => {
    const existing = generateSeededPhraseRows({ ...baseOptions, seed: 1 });
    const generated = generateSeededPhraseRows({ ...baseOptions, seed: 2 });
    const merged = mergeSeededPhraseRows(existing, generated, [false, false, false, false]);

    expect(merged).toEqual(existing);
  });
});
