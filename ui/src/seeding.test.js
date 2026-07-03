import { describe, expect, it } from "vitest";
import {
  applySeedingRowSettingsUpdate,
  computeRhythmRowTimingOffsets,
  createDefaultSeedModeRowSettings,
  defaultSeedingCenterMidi,
  displaySeedingRowSettings,
  deriveRowGenerationLayout,
  generateSeededPhraseRows,
  mergeSeededPhraseRows,
  normalizeSeedModeState,
  normalizeSeedingRowSettings,
  normalizeSeedingSettings,
  phraseRowsFromGridState,
  reshuffleSeedingAspectSeedUpdate,
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
import { defaultStepProbabilityValue } from "./percentLimits.js";
import { defaultRowTimingOffsetIndex, timingOffsetValues } from "./stepCellLayout.js";

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
  it("defaults row targets to row 1 only", () => {
    expect(normalizeSeedModeState().rowTargets).toEqual([true, false, false, false]);
  });

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
      timingVariance: 50,
      rhythmStep: seedingRhythmStepMin,
      seed: 42,
    });
    const interleave = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      randomness: 0,
      complexity: 0,
      timingVariance: 50,
      rhythmStep: seedingRhythmStepMax,
      seed: 42,
    });

    expect(overlap.rowTimingOffset.map((index) => timingOffsetValues[index])).toEqual([0, 0, 0, 0]);
    expect(
      Math.max(...interleave.rowTimingOffset.map((index) => timingOffsetValues[index]))
      - Math.min(...interleave.rowTimingOffset.map((index) => timingOffsetValues[index]),
    )).toBeGreaterThan(0.5);
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

  it("shuffles contour roles across rows when the seed changes", () => {
    const shared = {
      phraseLength: 8,
      repetition: 100,
      complexity: 0,
      randomness: 0,
      rangeSemitones: 12,
      root: 0,
      modeIndex: 0,
    };
    const seedOne = generateSeededPhraseRows({ ...shared, seed: 1 });
    const seedTwo = generateSeededPhraseRows({ ...shared, seed: 2 });
    const pitchesOne = seedOne.notes.map((row) => row[0]);
    const pitchesTwo = seedTwo.notes.map((row) => row[0]);

    expect(pitchesOne).not.toEqual(pitchesTwo);
    expect(new Set([
      seedOne.notes[0][0],
      seedOne.notes[1][0],
      seedOne.notes[2][0],
      seedOne.notes[3][0],
    ]).size).toBe(4);
  });

  it("permutes row timing offsets with the contour layout", () => {
    const seedOne = generateSeededPhraseRows({
      phraseLength: 4,
      rhythmStep: seedingRhythmStepMax,
      timingVariance: 0,
      repetition: 0,
      randomness: 0,
      complexity: 0,
      seed: 11,
    });
    const seedTwo = generateSeededPhraseRows({
      phraseLength: 4,
      rhythmStep: seedingRhythmStepMax,
      timingVariance: 0,
      repetition: 0,
      randomness: 0,
      complexity: 0,
      seed: 29,
    });

    expect(seedOne.rowTimingOffset).not.toEqual(seedTwo.rowTimingOffset);
    expect(new Set(seedOne.rowTimingOffset).size).toBeGreaterThan(1);
  });

  it("scales row timing offsets to each row cycle length", () => {
    const shortCycles = computeRhythmRowTimingOffsets(
      seedingRhythmStepMax,
      [0, 1, 2, 3],
      [4, 4, 4, 4],
      1,
    );
    const longCycles = computeRhythmRowTimingOffsets(
      seedingRhythmStepMax,
      [0, 1, 2, 3],
      [8, 8, 8, 8],
      1,
    );

    const shortOffsets = shortCycles.map((index) => timingOffsetValues[index]);
    const longOffsets = longCycles.map((index) => timingOffsetValues[index]);

    expect(longOffsets[1]).toBeGreaterThan(shortOffsets[1]);
    expect(longOffsets[2]).toBeGreaterThan(shortOffsets[2]);
  });

  it("keeps one generated rhythm row anchored at zero offset", () => {
    for (let rhythmStep = seedingRhythmStepMin; rhythmStep <= seedingRhythmStepMax; rhythmStep += 1) {
      const offsets = computeRhythmRowTimingOffsets(
        rhythmStep,
        [1, 2, 3, 0],
        [4, 4, 4, 4],
        91,
      );

      expect(offsets).toContain(defaultRowTimingOffsetIndex);
    }
  });

  it("uses different offsets when rows have different phrase lengths", () => {
    const rowSettings = createDefaultSeedModeRowSettings();

    for (const settings of rowSettings) {
      settings.timingVariance = 0;
      settings.repetition = 0;
      settings.randomness = 0;
      settings.complexity = 0;
      settings.seed = 42;
    }

    rowSettings[0].phraseLength = 4;
    rowSettings[1].phraseLength = 8;
    rowSettings[2].phraseLength = 4;
    rowSettings[3].phraseLength = 8;

    const result = generateSeededPhraseRows({
      rhythmStep: seedingRhythmStepMax,
      rowSettings,
      root: 0,
      modeIndex: 0,
    });

    const offsets = result.rowTimingOffset.map((index) => timingOffsetValues[index]);

    expect(offsets[1]).toBeGreaterThan(offsets[0]);
    expect(offsets[3]).toBeGreaterThan(offsets[2]);
  });

  it("changes pitch chaos from randomness even when complexity is zero", () => {
    const structured = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      complexity: 0,
      randomness: 0,
      rangeSemitones: 12,
      seed: 91,
    });
    const chaotic = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      complexity: 0,
      randomness: 100,
      rangeSemitones: 12,
      seed: 91,
    });

    expect(chaotic.notes[0]).not.toEqual(structured.notes[0]);
    expect(new Set(structured.notes[0]).size).toBeLessThan(new Set(chaotic.notes[0]).size);
  });

  it("changes contour shape from complexity even when randomness is zero", () => {
    const flat = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      complexity: 0,
      randomness: 0,
      rangeSemitones: 12,
      seed: 91,
    });
    const shaped = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      complexity: 100,
      randomness: 0,
      rangeSemitones: 12,
      seed: 91,
    });

    expect(shaped.notes[0]).not.toEqual(flat.notes[0]);
    expect(new Set(shaped.notes[0]).size).toBeGreaterThan(1);
  });

  it("applies step probability only when randomness is above zero", () => {
    const none = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      complexity: 0,
      randomness: 0,
      seed: 91,
    });
    const some = generateSeededPhraseRows({
      phraseLength: 8,
      repetition: 0,
      complexity: 0,
      randomness: 100,
      seed: 91,
    });

    expect(none.stepProbability[0].every((value) => value === defaultStepProbabilityValue)).toBe(true);
    expect(some.stepProbability[0].some((value) => value < defaultStepProbabilityValue)).toBe(true);
  });
});

describe("phraseRowsFromGridState", () => {
  it("maps grid fields to generated phrase row shape", () => {
    const rows = phraseRowsFromGridState({
      grid: [[60, 62], [64, 65], [67, 69], [71, 72]],
      stepTimingMultiplier: [[3, 4], [3, 3], [3, 3], [3, 3]],
      rowTimingOffset: [8, 9, 10, 11],
    });

    expect(rows.notes).toEqual([[60, 62], [64, 65], [67, 69], [71, 72]]);
    expect(rows.stepTimingMultiplier[0]).toEqual([3, 4]);
    expect(rows.rowTimingOffset).toEqual([8, 9, 10, 11]);
  });
});

describe("deriveRowGenerationLayout", () => {
  it("is deterministic for a fixed master seed", () => {
    expect(deriveRowGenerationLayout(4242)).toEqual(deriveRowGenerationLayout(4242));
  });

  it("assigns each archetype exactly once across four rows", () => {
    const { archetypeIndices } = deriveRowGenerationLayout(91);

    expect(archetypeIndices).toHaveLength(4);
    expect([...archetypeIndices].sort((left, right) => left - right)).toEqual([0, 1, 2, 3]);
  });

  it("changes the row-to-archetype mapping for different master seeds", () => {
    const first = deriveRowGenerationLayout(1).archetypeIndices.join(",");
    const second = deriveRowGenerationLayout(2).archetypeIndices.join(",");

    expect(second).not.toBe(first);
  });

  it("applies hybrid jitter within expected bounds", () => {
    const { contours } = deriveRowGenerationLayout(77);

    for (const contour of contours) {
      expect(contour.centerSlot).toBeGreaterThanOrEqual(-1.85);
      expect(contour.centerSlot).toBeLessThanOrEqual(1.85);
      expect(contour.waveFactor).toBeGreaterThanOrEqual(1);
      expect(contour.waveFactor).toBeLessThanOrEqual(2.19);
      expect(contour.velocityBase).toBeGreaterThanOrEqual(74);
      expect(contour.velocityBase).toBeLessThanOrEqual(100);
    }
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

  it("preserves existing row timing offsets for non-targeted rows", () => {
    const existing = generateSeededPhraseRows({ ...baseOptions, seed: 1, rhythmStep: seedingRhythmStepMax });
    const generated = generateSeededPhraseRows({ ...baseOptions, seed: 2, rhythmStep: seedingRhythmStepMin });
    const merged = mergeSeededPhraseRows(existing, generated, [false, false, false, false]);

    expect(merged).toEqual(existing);
    expect(merged.rowTimingOffset).toEqual(existing.rowTimingOffset);
  });

  it("only replaces row timing offsets for targeted rows", () => {
    const existing = generateSeededPhraseRows({ ...baseOptions, seed: 1, rhythmStep: seedingRhythmStepMin });
    const generated = generateSeededPhraseRows({ ...baseOptions, seed: 2, rhythmStep: seedingRhythmStepMax });
    const merged = mergeSeededPhraseRows(existing, generated, [false, true, false, false]);

    expect(merged.rowTimingOffset[0]).toEqual(existing.rowTimingOffset[0]);
    expect(merged.rowTimingOffset[1]).toEqual(generated.rowTimingOffset[1]);
    expect(merged.rowTimingOffset[2]).toEqual(existing.rowTimingOffset[2]);
    expect(merged.rowTimingOffset[3]).toEqual(existing.rowTimingOffset[3]);
  });

  it("applies rhythm fields to all rows when rhythm is global", () => {
    const existing = generateSeededPhraseRows({ ...baseOptions, seed: 1, rhythmStep: seedingRhythmStepMin });
    const generated = generateSeededPhraseRows({ ...baseOptions, seed: 2, rhythmStep: seedingRhythmStepMax });
    const merged = mergeSeededPhraseRows(
      existing,
      generated,
      [false, true, false, false],
      { applyRhythmToAllRows: true },
    );

    for (let row = 0; row < 4; row += 1) {
      expect(merged.stepTimingMultiplier[row]).toEqual(generated.stepTimingMultiplier[row]);
      expect(merged.stepDurationFraction[row]).toEqual(generated.stepDurationFraction[row]);
      expect(merged.rowTimingOffset[row]).toEqual(generated.rowTimingOffset[row]);
    }

    expect(merged.notes[0]).toEqual(existing.notes[0]);
    expect(merged.notes[1]).toEqual(generated.notes[1]);
    expect(merged.notes[2]).toEqual(existing.notes[2]);
    expect(merged.notes[3]).toEqual(existing.notes[3]);
    expect(merged.stepVelocity[0]).toEqual(existing.stepVelocity[0]);
    expect(merged.stepVelocity[1]).toEqual(generated.stepVelocity[1]);
    expect(merged.stepVelocity[2]).toEqual(existing.stepVelocity[2]);
    expect(merged.stepVelocity[3]).toEqual(existing.stepVelocity[3]);
  });

  it("anchors global rhythm offsets to a non-empty row after selective merges", () => {
    const existing = generateSeededPhraseRows({ ...baseOptions, seed: 1, rhythmStep: seedingRhythmStepMin });
    const generated = generateSeededPhraseRows({ ...baseOptions, seed: 2, rhythmStep: seedingRhythmStepMax });

    existing.notes[2] = [];
    existing.notes[3] = [];
    generated.rowTimingOffset = [
      defaultRowTimingOffsetIndex + 3,
      defaultRowTimingOffsetIndex + 5,
      defaultRowTimingOffsetIndex,
      defaultRowTimingOffsetIndex + 6,
    ];

    const merged = mergeSeededPhraseRows(
      existing,
      generated,
      [false, false, false, false],
      { applyRhythmToAllRows: true },
    );

    expect(merged.notes[0]).toHaveLength(4);
    expect(merged.notes[1]).toHaveLength(4);
    expect(merged.notes[2]).toHaveLength(0);
    expect(merged.rowTimingOffset[0]).toBe(defaultRowTimingOffsetIndex);
    expect(merged.rowTimingOffset[1]).toBe(defaultRowTimingOffsetIndex + 2);
  });
});

describe("seeding aspect seeds", () => {
  const baseOptions = {
    phraseLength: 8,
    repetition: 55,
    complexity: 36,
    randomness: 45,
    timingMeanMultiplierIndex: 5,
    timingVariance: 52,
    seed: 4242,
  };

  it("derives aspect seeds from the main seed when unset", () => {
    const settings = normalizeSeedingRowSettings({ seed: 99 }, 2);

    expect(settings.repetitionSeed).toBeGreaterThan(0);
    expect(settings.complexitySeed).toBeGreaterThan(0);
    expect(settings.randomnessSeed).toBeGreaterThan(0);
    expect(settings.timingVarianceSeed).toBeGreaterThan(0);
  });

  it("re-shuffles only the targeted aspect when its seed changes", () => {
    const baseline = generateSeededPhraseRows({
      ...baseOptions,
      timingVarianceSeed: 1001,
      randomnessSeed: 2002,
      complexitySeed: 3003,
    });
    const timingReshuffle = generateSeededPhraseRows({
      ...baseOptions,
      timingVarianceSeed: 4004,
      randomnessSeed: 2002,
      complexitySeed: 3003,
    });
    const randomnessReshuffle = generateSeededPhraseRows({
      ...baseOptions,
      timingVarianceSeed: 1001,
      randomnessSeed: 5005,
      complexitySeed: 3003,
    });
    const complexityReshuffle = generateSeededPhraseRows({
      ...baseOptions,
      timingVarianceSeed: 1001,
      randomnessSeed: 2002,
      complexitySeed: 6006,
    });

    expect(timingReshuffle).not.toEqual(baseline);
    expect(randomnessReshuffle).not.toEqual(baseline);
    expect(complexityReshuffle).not.toEqual(baseline);
    expect(timingReshuffle.notes).toEqual(baseline.notes);
    expect(randomnessReshuffle.stepTimingMultiplier).toEqual(baseline.stepTimingMultiplier);
    expect(complexityReshuffle.stepTimingMultiplier).toEqual(baseline.stepTimingMultiplier);
    expect(complexityReshuffle.notes).not.toEqual(baseline.notes);
  });

  it("keeps repetition-driven notes stable when only randomness is re-shuffled", () => {
    const repetitionHeavy = generateSeededPhraseRows({
      ...baseOptions,
      repetition: 100,
      randomness: 0,
      complexity: 0,
    });
    const randomnessOnlyReshuffle = generateSeededPhraseRows({
      ...baseOptions,
      repetition: 100,
      randomness: 0,
      complexity: 0,
      ...reshuffleSeedingAspectSeedUpdate("randomness"),
    });

    expect(randomnessOnlyReshuffle.notes).toEqual(repetitionHeavy.notes);
  });
});
