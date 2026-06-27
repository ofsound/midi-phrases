import { defaultStepCycle, defaultStepCycleMask } from "./cyclePattern.js";
import { defaultStepNoteForScaleRoot } from "./midiNoteNames.js";
import { maxPercentValue } from "./percentLimits.js";
import { timingOffsetValues } from "./stepCellLayout.js";
import {
  clampScaleModeIndex,
  clampScaleRoot,
  snapMidiToScale,
} from "./scaleUtils.js";

export const seedingPhraseLengthMin = 2;
export const seedingPhraseLengthMax = 16;

/** Total pitch spread for seeded motifs (±half span from each row center). */
export const seedingRangeSemitonesMin = 2;
export const seedingRangeSemitonesMax = 48;
export const defaultSeedingRangeSemitones = 8;

/** Former preset labels mapped to chromatic span for legacy `rangeIndex` values. */
const legacySeedingRangeSemitonesByIndex = [5, 8, 12, 16];

/** Stepped overlap ↔ interleave blend (0 = dense overlap, 7 = full interleave). */
export const seedingRhythmStepMin = 0;
export const seedingRhythmStepMax = 7;
export const defaultSeedingRhythmStep = 7;

const rhythmRowTimingOffsetProfiles = [
  [3, 2, 4, 3],
  [3, 2, 4, 4],
  [3, 2, 3, 4],
  [3, 3, 4, 4],
  [3, 3, 3, 5],
  [3, 4, 3, 5],
  [3, 4, 2, 4],
  [3, 4, 2, 5],
];

/** Per-step gate-length penalty (quarters of the accent value). */
const rhythmDurationPenalties = [0, 0.02, 0.05, 0.07, 0.09, 0.12, 0.14, 0.16];

/** Per-step accent timing multiplier at full interleave end of the slider. */
const rhythmAccentTimingMultipliers = [1, 0.9375, 0.875, 0.8125, 0.75, 0.6875, 0.5625, 0.5];

export const defaultSeedingSettings = {
  phraseLength: 3,
  rangeSemitones: defaultSeedingRangeSemitones,
  repetition: 45,
  complexity: 50,
  randomness: 45,
  symmetry: false,
  rhythmStep: defaultSeedingRhythmStep,
  seed: 1,
};

/** @param {number} value @param {number} min @param {number} max */
function clamp(value, min, max) {
  return Math.min(max, Math.max(min, Number.isFinite(value) ? value : min));
}

/** @param {number} value */
function clampPercent(value) {
  return clamp(Math.round(value), 0, 100);
}

/** @param {number} value */
function clampPhraseLength(value) {
  return Math.round(clamp(value, seedingPhraseLengthMin, seedingPhraseLengthMax));
}

/** @param {number} seed */
function mulberry32(seed) {
  let state = seed >>> 0;

  return () => {
    state += 0x6D2B79F5;
    let value = state;
    value = Math.imul(value ^ (value >>> 15), value | 1);
    value ^= value + Math.imul(value ^ (value >>> 7), value | 61);
    return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
  };
}

/** @param {() => number} random @param {number} min @param {number} max */
function randomInt(random, min, max) {
  return Math.floor(random() * (max - min + 1)) + min;
}

/** @param {number} value */
function timingMultiplierIndexForValue(value) {
  return Math.min(15, Math.max(0, Math.round((value - 0.25) / 0.25)));
}

/** @param {number} step */
function clampRhythmStep(step) {
  return Math.round(clamp(step, seedingRhythmStepMin, seedingRhythmStepMax));
}

/**
 * @param {number} step
 * @returns {number} 0 = overlap, 1 = interleave
 */
export function rhythmInterleaveRatio(step) {
  const span = seedingRhythmStepMax - seedingRhythmStepMin;
  return span > 0 ? clampRhythmStep(step) / span : 0;
}

/** @param {number} step */
function rhythmRowTimingOffsets(step) {
  const index = clampRhythmStep(step);
  const profile = rhythmRowTimingOffsetProfiles[index] ?? rhythmRowTimingOffsetProfiles[0];

  return profile.map((offsetIndex) => (
    timingOffsetValues[offsetIndex] === undefined ? 3 : offsetIndex
  ));
}

/**
 * How strongly a step should receive interleave timing/gate shaping.
 * Short phrases spread the effect across every step so the slider stays audible.
 *
 * @param {number} step
 * @param {number} row
 * @param {number} phraseLength
 */
function stepRhythmWeight(step, row, phraseLength) {
  if ((step + row) % 4 === 0) {
    return 1;
  }

  if (phraseLength <= 3) {
    return 0.35 + 0.3 * ((step + row * 2) % 3) / 2;
  }

  return 0.2;
}

/** @param {number} rhythmStep */
function rhythmDurationPenalty(rhythmStep) {
  return rhythmDurationPenalties[clampRhythmStep(rhythmStep)] ?? 0;
}

/** @param {number} rhythmStep */
function rhythmAccentTimingMultiplier(rhythmStep) {
  return rhythmAccentTimingMultipliers[clampRhythmStep(rhythmStep)] ?? 1;
}

/**
 * @param {Partial<typeof defaultSeedingSettings> & { rhythmMode?: string }>} settings
 */
function resolveRhythmStep(settings) {
  if (settings.rhythmStep !== undefined) {
    return clampRhythmStep(settings.rhythmStep);
  }

  if (settings.rhythmMode === "overlap") {
    return seedingRhythmStepMin;
  }

  if (settings.rhythmMode === "interleave") {
    return seedingRhythmStepMax;
  }

  return defaultSeedingRhythmStep;
}

/** @param {number} value */
function clampRangeSemitones(value) {
  return Math.round(clamp(value, seedingRangeSemitonesMin, seedingRangeSemitonesMax));
}

/**
 * @param {Partial<typeof defaultSeedingSettings>} settings
 */
function resolveRangeSemitones(settings) {
  if (settings.rangeSemitones !== undefined) {
    return clampRangeSemitones(settings.rangeSemitones);
  }

  if (settings.rangeIndex !== undefined) {
    const legacyIndex = Math.round(settings.rangeIndex);
    const legacyValue = legacySeedingRangeSemitonesByIndex[legacyIndex];
    return clampRangeSemitones(legacyValue ?? defaultSeedingRangeSemitones);
  }

  return clampRangeSemitones(defaultSeedingRangeSemitones);
}

/** @param {number} root @param {number} modeIndex @param {number} semitoneOffset */
function midiForSeedingOffset(root, modeIndex, semitoneOffset) {
  const base = snapMidiToScale(defaultStepNoteForScaleRoot(root), root, modeIndex);
  return snapMidiToScale(base + Math.round(semitoneOffset), root, modeIndex);
}

/**
 * @param {Partial<typeof defaultSeedingSettings> & { root?: number, modeIndex?: number }} settings
 */
export function normalizeSeedingSettings(settings = {}) {
  const merged = {
    ...defaultSeedingSettings,
    ...settings,
  };
  return {
    root: clampScaleRoot(merged.root ?? 0),
    modeIndex: clampScaleModeIndex(merged.modeIndex ?? 0),
    phraseLength: clampPhraseLength(merged.phraseLength),
    rangeSemitones: resolveRangeSemitones(settings),
    repetition: clampPercent(merged.repetition),
    complexity: clampPercent(merged.complexity),
    randomness: clampPercent(merged.randomness),
    symmetry: Boolean(merged.symmetry),
    rhythmStep: resolveRhythmStep(settings),
    seed: Math.max(1, Math.round(clamp(merged.seed, 1, 2147483647))),
  };
}

/**
 * Generate four independent monophonic phrase rows.
 *
 * @param {Partial<typeof defaultSeedingSettings> & { root?: number, modeIndex?: number }} settings
 */
export function generateSeededPhraseRows(settings = {}) {
  const options = normalizeSeedingSettings(settings);
  const random = mulberry32(options.seed);
  const span = options.rangeSemitones;
  const halfSpan = Math.max(2, Math.floor(span / 2));
  const complexityRatio = options.complexity / 100;
  const repetitionRatio = options.repetition / 100;
  const randomnessRatio = options.randomness / 100;
  const rhythmBlend = rhythmInterleaveRatio(options.rhythmStep);
  const rowTimingOffset = rhythmRowTimingOffsets(options.rhythmStep);
  const durationPenalty = rhythmDurationPenalty(options.rhythmStep);
  const accentTimingMultiplier = rhythmAccentTimingMultiplier(options.rhythmStep);
  const rows = {
    notes: [],
    stepTimingMultiplier: [],
    stepDurationFraction: [],
    stepVelocity: [],
    stepMuted: [],
    stepSkipped: [],
    stepProbability: [],
    stepCycle: [],
    stepCycleOffset: [],
    rowTimingOffset: rowTimingOffset.map((index) => (
      timingOffsetValues[index] === undefined ? 3 : index
    )),
  };

  for (let row = 0; row < 4; row += 1) {
    const motifLength = options.symmetry ? Math.ceil(options.phraseLength / 2) : options.phraseLength;
    const motif = [];
    const center = Math.round((row - 1.5) * Math.max(1, span / 6));
    let previous = center;

    for (let step = 0; step < motifLength; step += 1) {
      const reusePrevious = step > 0 && random() < repetitionRatio * 0.54;
      const reuseEarlier = step > 3 && random() < repetitionRatio * 0.28;

      if (reusePrevious) {
        motif.push(previous);
        continue;
      }

      if (reuseEarlier) {
        previous = motif[Math.max(0, step - randomInt(random, 2, Math.min(6, step)))] ?? previous;
        motif.push(previous);
        continue;
      }

      const periodic = Math.sin((step / Math.max(1, motifLength - 1)) * Math.PI * (1.25 + row * 0.28));
      const directed = Math.round(periodic * halfSpan * (0.35 + complexityRatio * 0.45));
      const leap = randomInt(
        random,
        -Math.max(1, Math.round(1 + complexityRatio * halfSpan)),
        Math.max(1, Math.round(1 + complexityRatio * halfSpan)),
      );
      const randomPush = random() < randomnessRatio ? leap : Math.sign(leap);

      previous = Math.round(clamp(center + directed + randomPush, -halfSpan, halfSpan));
      motif.push(previous);
    }

    const degrees = options.symmetry
      ? [...motif, ...motif.slice(0, options.phraseLength - motif.length).reverse()]
      : motif;
    const timingPool = complexityRatio > 0.65
      ? [0.5, 0.75, 1, 1.25, 1.5]
      : complexityRatio > 0.35
        ? [0.75, 1, 1, 1.25]
        : [1, 1, 1, 1.25];
    const velocityBase = 78 + row * 6;
    const velocitySwing = Math.round(10 + complexityRatio * 28);

    rows.notes[row] = degrees.map((semitoneOffset) => (
      midiForSeedingOffset(options.root, options.modeIndex, semitoneOffset)
    ));
    rows.stepTimingMultiplier[row] = degrees.map((_, step) => {
      const poolValue = timingPool[randomInt(random, 0, timingPool.length - 1)];

      if (rhythmBlend <= 0) {
        return timingMultiplierIndexForValue(poolValue);
      }

      const weight = stepRhythmWeight(step, row, options.phraseLength);
      const blendedMultiplier = poolValue + (accentTimingMultiplier - poolValue) * weight;

      return timingMultiplierIndexForValue(blendedMultiplier);
    });
    rows.stepDurationFraction[row] = degrees.map((_, step) => {
      const accent = (step + row) % 4 === 0 ? 0.95 : 0.72 + random() * 0.2;
      const weight = stepRhythmWeight(step, row, options.phraseLength);
      const stepPenalty = durationPenalty * weight;

      return Number(clamp(accent - stepPenalty, 0.35, 1).toFixed(2));
    });
    rows.stepVelocity[row] = degrees.map((_, step) => (
      Math.round(clamp(
        velocityBase + Math.sin((step + row) * 1.7) * velocitySwing + randomInt(random, -9, 9),
        28,
        127,
      ))
    ));
    rows.stepMuted[row] = degrees.map(() => false);
    rows.stepSkipped[row] = degrees.map((_, step) => (
      complexityRatio < 0.3 && random() < 0.08 && step % 4 !== row % 4
    ));
    rows.stepProbability[row] = degrees.map((_, step) => (
      random() < randomnessRatio * 0.24 && step % 4 !== 0
        ? Math.round(clamp(100 - randomnessRatio * randomInt(random, 18, 42), 20, 100))
        : maxPercentValue
    ));
    rows.stepCycle[row] = degrees.map(() => defaultStepCycle);
    rows.stepCycleOffset[row] = degrees.map(() => defaultStepCycleMask);
  }

  return rows;
}
