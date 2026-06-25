import { describe, expect, it } from "vitest";
import { generateSeededPhraseRows, normalizeSeedingSettings } from "./seeding.js";
import { isMidiInScale } from "./scaleUtils.js";

describe("normalizeSeedingSettings", () => {
  it("clamps public settings to supported ranges", () => {
    const settings = normalizeSeedingSettings({
      phraseLength: 200,
      rangeIndex: -5,
      repetition: 500,
      complexity: -1,
      randomness: Number.NaN,
      rhythmMode: "unknown",
      seed: -10,
    });

    expect(settings.phraseLength).toBe(32);
    expect(settings.rangeIndex).toBe(0);
    expect(settings.repetition).toBe(100);
    expect(settings.complexity).toBe(0);
    expect(settings.randomness).toBe(0);
    expect(settings.rhythmMode).toBe("interleave");
    expect(settings.seed).toBe(1);
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
});
