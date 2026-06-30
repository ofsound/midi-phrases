import { describe, expect, it } from "vitest";
import {
  applySeedingRowSettingsUpdate,
  createDefaultSeedModeRowSettings,
  defaultSeedingCenterMidi,
  displaySeedingRowSettings,
  generateSeededPhraseRows,
  mergeSeededPhraseRows,
  normalizeSeedModeState,
  normalizeSeedingRowSettings,
  normalizeSeedingSettings,
  phraseRowsFromGridState,
  resolveSeedingCenterMidi,
  rhythmInterleaveRatio,
  seedingCenterMidiForIndex,
  seedingCenterNoteIndex,
  seedingRhythmStepMax,
  seedingRhythmStepMin,
  seedingTimingMultiplierMaxIndex,
  seedingTimingMultiplierMinIndex,
} from "./seeding.js";
import { isMidiInScale } from "./scaleUtils.js";
import { timingOffsetValues } from "./stepCellLayout.js";

/** @param {number[]} values */
function average(values) {
  return values.reduce((sum, value) => sum + value, 0) / Math.max(1, values.length);
}

/** @param {number[]} values */
function spread(values) {
  return Math.max(...values) - Math.min(...values);
}

describe("normalizeSeedingSettings", () => {
  it("clamps public settings to supported ranges", () => {
    const settings = normalizeSeedingSettings({
      phraseLength: 200,
      rangeSemitones: -5,
      repetition: 500,
      complexity: -1,
      randomness: Number.NaN,
      timingMeanMultiplierIndex: 99,
      timingVariance: Number.NaN,
      rhythmStep: 99,
      seed: -10,
    });

    expect(settings.phraseLength).toBe(16);
    expect(settings.centerMidi).toBe(defaultSeedingCenterMidi);
    expect(settings.rangeSemitones).toBe(2);
    expect(settings.repetition).toBe(100);
    expect(settings.complexity).toBe(0);
    expect(settings.randomness).toBe(0);
    expect(settings.timingMeanMultiplierIndex).toBe(seedingTimingMultiplierMaxIndex);
    expect(settings.timingVariance).toBe(0);
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

  it("preserves concrete center MIDI values", () => {
    expect(normalizeSeedingSettings({ centerMidi: 72 }).centerMidi).toBe(72);
    expect(normalizeSeedingSettings({ centerMidi: 200 }).centerMidi).toBe(127);
    expect(normalizeSeedingSettings({ centerMidi: -2 }).centerMidi).toBe(defaultSeedingCenterMidi);
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
      timingMeanMultiplierIndex: -99,
      timingVariance: 500,
      seed: -10,
    });

    expect(settings.phraseLength).toBe(16);
    expect(settings.centerMidi).toBe(defaultSeedingCenterMidi);
    expect(settings.rangeSemitones).toBe(2);
    expect(settings.repetition).toBe(100);
    expect(settings.complexity).toBe(0);
    expect(settings.randomness).toBe(0);
    expect(settings.timingMeanMultiplierIndex).toBe(seedingTimingMultiplierMinIndex);
    expect(settings.timingVariance).toBe(100);
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

  it("derives independent multiplier sequences for each row from the shared seed", () => {
    const result = generateSeededPhraseRows({
      phraseLength: 8,
      timingMeanMultiplierIndex: 5,
      timingVariance: 100,
      rhythmStep: seedingRhythmStepMin,
      seed: 91,
    });
    const signatures = result.stepTimingMultiplier.map((row) => row.join(","));

    expect(new Set(signatures).size).toBeGreaterThan(1);
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

  it("holds one pitch per row when repetition is at maximum", () => {
    const result = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 100,
      complexity: 41,
      randomness: 45,
      rangeSemitones: 9,
      seed: 1,
    });

    for (let row = 0; row < 4; row += 1) {
      expect(new Set(result.notes[row]).size).toBe(1);
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

  it("keeps legacy default center behavior when centerMidi is missing", () => {
    const options = { root: 0, modeIndex: 0, phraseLength: 8, seed: 91 };

    expect(generateSeededPhraseRows(options)).toEqual(generateSeededPhraseRows({
      ...options,
      centerMidi: defaultSeedingCenterMidi,
    }));
  });

  it("moves generated pitches around a concrete center note", () => {
    const lower = generateSeededPhraseRows({
      root: 0,
      modeIndex: 0,
      phraseLength: 8,
      repetition: 0,
      randomness: 0,
      complexity: 0,
      seed: 91,
      centerMidi: 60,
    });
    const higher = generateSeededPhraseRows({
      root: 0,
      modeIndex: 0,
      phraseLength: 8,
      repetition: 0,
      randomness: 0,
      complexity: 0,
      seed: 91,
      centerMidi: 72,
    });

    expect(higher.notes[0]).toEqual(lower.notes[0].map((midi) => midi + 12));
  });

  it("snaps concrete center notes to the active scale", () => {
    expect(resolveSeedingCenterMidi(61, 0, 1)).toBe(60);

    const centerIndex = seedingCenterNoteIndex(61, 0, 1);
    expect(seedingCenterMidiForIndex(centerIndex, 0, 1)).toBe(60);

    const result = generateSeededPhraseRows({
      root: 0,
      modeIndex: 1,
      phraseLength: 8,
      centerMidi: 61,
      seed: 91,
    });

    for (const row of result.notes) {
      for (const midi of row) {
        expect(isMidiInScale(midi, 0, 1)).toBe(true);
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
    expect(overlap.stepTimingMultiplier).not.toEqual(interleave.stepTimingMultiplier);
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

  it("uses length average to target row multiplier averages", () => {
    const short = generateSeededPhraseRows({
      phraseLength: 12,
      timingMeanMultiplierIndex: seedingTimingMultiplierMinIndex,
      timingVariance: 100,
      seed: 42,
    });
    const long = generateSeededPhraseRows({
      phraseLength: 12,
      timingMeanMultiplierIndex: seedingTimingMultiplierMaxIndex,
      timingVariance: 100,
      seed: 42,
    });

    expect(average(short.stepTimingMultiplier[0])).toBe(seedingTimingMultiplierMinIndex);
    expect(average(long.stepTimingMultiplier[0])).toBe(seedingTimingMultiplierMaxIndex);
  });

  it("keeps length variance zero uniform at the selected average", () => {
    const result = generateSeededPhraseRows({
      phraseLength: 8,
      timingMeanMultiplierIndex: 5,
      timingVariance: 0,
      seed: 91,
    });

    expect(result.stepTimingMultiplier[0]).toEqual(Array.from({ length: 8 }, () => 5));
  });

  it("widens multiplier spread as length variance rises", () => {
    const low = generateSeededPhraseRows({
      phraseLength: 16,
      timingMeanMultiplierIndex: 3,
      timingVariance: 10,
      rhythmStep: seedingRhythmStepMin,
      seed: 91,
    });
    const high = generateSeededPhraseRows({
      phraseLength: 16,
      timingMeanMultiplierIndex: 3,
      timingVariance: 100,
      rhythmStep: seedingRhythmStepMin,
      seed: 91,
    });

    expect(spread(high.stepTimingMultiplier[0])).toBeGreaterThan(spread(low.stepTimingMultiplier[0]));
  });

  it("places length variation below and above the selected average when possible", () => {
    const averageIndex = 7;
    const result = generateSeededPhraseRows({
      phraseLength: 12,
      timingMeanMultiplierIndex: averageIndex,
      timingVariance: 100,
      rhythmStep: seedingRhythmStepMin,
      seed: 91,
    });

    expect(result.stepTimingMultiplier[0].some((index) => index < averageIndex)).toBe(true);
    expect(result.stepTimingMultiplier[0].some((index) => index > averageIndex)).toBe(true);
  });

  it("does not let complexity alter generated step multipliers", () => {
    const simple = generateSeededPhraseRows({
      phraseLength: 12,
      complexity: 0,
      randomness: 0,
      repetition: 0,
      timingMeanMultiplierIndex: 3,
      timingVariance: 75,
      seed: 91,
    });
    const complex = generateSeededPhraseRows({
      phraseLength: 12,
      complexity: 100,
      randomness: 0,
      repetition: 0,
      timingMeanMultiplierIndex: 3,
      timingVariance: 75,
      seed: 91,
    });

    expect(complex.stepTimingMultiplier).toEqual(simple.stepTimingMultiplier);
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

  it("uses independent per-row center notes", () => {
    const rowSettings = createDefaultSeedModeRowSettings();
    rowSettings[0].centerMidi = 48;
    rowSettings[1].centerMidi = 84;

    const result = generateSeededPhraseRows({
      rhythmStep: seedingRhythmStepMin,
      rowSettings,
      root: 0,
      modeIndex: 0,
    });

    expect(Math.min(...result.notes[1])).toBeGreaterThan(Math.max(...result.notes[0]));
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
