import { defaultStepCycle, defaultStepCycleMask } from "./cyclePattern.js";
import { defaultStepNoteForScaleRoot, midiToNoteName } from "./midiNoteNames.js";
import { defaultStepProbabilityValue } from "./percentLimits.js";
import {
  defaultRowTimingOffsetIndex,
  defaultStepTimingMultiplierIndex,
  timingMultiplierIndexForValue,
  timingOffsetValues,
} from "./stepCellLayout.js";
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

/** Legacy/default center: resolve from the current scale root at generation time. */
export const defaultSeedingCenterMidi = -1;

/** Former preset labels mapped to chromatic span for legacy `rangeIndex` values. */
const legacySeedingRangeSemitonesByIndex = [5, 8, 12, 16];

/** Stepped overlap ↔ interleave blend (0 = dense overlap, 7 = full interleave). */
export const seedingRhythmStepMin = 0;
export const seedingRhythmStepMax = 7;
export const defaultSeedingRhythmStep = 7;

export const seedingTimingMultiplierMinIndex = timingMultiplierIndexForValue(0.5);
export const seedingTimingMultiplierMaxIndex = timingMultiplierIndexForValue(4);
export const defaultSeedingTimingMeanMultiplierIndex = defaultStepTimingMultiplierIndex;
export const defaultSeedingTimingVariance = 50;

const rhythmRowTimingOffsetProfiles = [
  [8, 8, 8, 8],
  [8, 7, 9, 9],
  [8, 7, 8, 9],
  [8, 8, 9, 9],
  [8, 8, 8, 10],
  [8, 9, 8, 10],
  [8, 9, 7, 9],
  [8, 9, 7, 10],
];

/** Per-step gate-length penalty (quarters of the accent value). */
const rhythmDurationPenalties = [0, 0.02, 0.05, 0.07, 0.09, 0.12, 0.14, 0.16];

/** Per-step accent timing multiplier at full interleave end of the slider. */
const rhythmAccentTimingMultipliers = [1, 0.9375, 0.875, 0.8125, 0.75, 0.6875, 0.5625, 0.5];

export const defaultSeedingSettings = {
  phraseLength: 3,
  centerMidi: defaultSeedingCenterMidi,
  rangeSemitones: defaultSeedingRangeSemitones,
  repetition: 45,
  complexity: 50,
  randomness: 45,
  timingMeanMultiplierIndex: defaultSeedingTimingMeanMultiplierIndex,
  timingVariance: defaultSeedingTimingVariance,
  symmetry: false,
  rhythmStep: defaultSeedingRhythmStep,
  seed: 1,
};

/** Per-row seeding parameters (overlap / rhythmStep is pattern-global). */
export const defaultSeedingRowSettings = {
  phraseLength: defaultSeedingSettings.phraseLength,
  centerMidi: defaultSeedingSettings.centerMidi,
  rangeSemitones: defaultSeedingSettings.rangeSemitones,
  repetition: defaultSeedingSettings.repetition,
  complexity: defaultSeedingSettings.complexity,
  randomness: defaultSeedingSettings.randomness,
  timingMeanMultiplierIndex: defaultSeedingSettings.timingMeanMultiplierIndex,
  timingVariance: defaultSeedingSettings.timingVariance,
  symmetry: defaultSeedingSettings.symmetry,
  seed: defaultSeedingSettings.seed,
};

/** @typedef {typeof defaultSeedingRowSettings} SeedingRowSettings */

export const defaultSeedModeRowTargets = [true, false, false, false];

/** @returns {SeedingRowSettings[]} */
export function createDefaultSeedModeRowSettings() {
  return Array.from({ length: 4 }, () => ({ ...defaultSeedingRowSettings }));
}

export const defaultSeedModeState = {
  rhythmStep: defaultSeedingRhythmStep,
  rowSettings: createDefaultSeedModeRowSettings(),
  rowTargets: [...defaultSeedModeRowTargets],
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

/** @param {number} seed @param {number} row */
function seedForSeedingRow(seed, row) {
  return (seed + Math.imul(row + 1, 0x9E3779B9)) >>> 0;
}

/** Fixed contour roles shuffled across physical rows each generation. */
const rowContourArchetypes = [
  { centerSlot: -1.5, waveFactor: 1.25, velocityBase: 78, accentPhase: 0 },
  { centerSlot: -0.5, waveFactor: 1.53, velocityBase: 84, accentPhase: 1 },
  { centerSlot: 0.5, waveFactor: 1.81, velocityBase: 90, accentPhase: 2 },
  { centerSlot: 1.5, waveFactor: 2.09, velocityBase: 96, accentPhase: 3 },
];

/**
 * @typedef {{
 *   centerSlot: number,
 *   waveFactor: number,
 *   velocityBase: number,
 *   accentPhase: number,
 * }} RowContour
 */

/**
 * @param {number[]} items
 * @param {() => number} random
 */
function fisherYatesShuffle(items, random) {
  const shuffled = [...items];

  for (let index = shuffled.length - 1; index > 0; index -= 1) {
    const swapIndex = Math.floor(random() * (index + 1));
    const current = shuffled[index];
    shuffled[index] = shuffled[swapIndex];
    shuffled[swapIndex] = current;
  }

  return shuffled;
}

/**
 * @param {typeof rowContourArchetypes[number]} archetype
 * @param {() => number} random
 * @returns {RowContour}
 */
function applyHybridContourJitter(archetype, random) {
  return {
    centerSlot: archetype.centerSlot + (random() * 2 - 1) * 0.35,
    waveFactor: Math.max(1, archetype.waveFactor + (random() * 2 - 1) * 0.1),
    velocityBase: Math.round(clamp(archetype.velocityBase + (random() * 2 - 1) * 4, 28, 127)),
    accentPhase: archetype.accentPhase,
  };
}

/**
 * @param {ReturnType<typeof normalizeSeedingRowSettings>[]} rowSettings
 */
function resolveMasterSeed(rowSettings) {
  const mixed = rowSettings.reduce(
    (hash, settings, row) => (hash ^ seedForSeedingRow(settings.seed, row)) >>> 0,
    0,
  );

  return mixed || 1;
}

/**
 * Shuffle four contour archetypes across physical rows, with small seed-derived jitter.
 *
 * @param {number} masterSeed
 * @returns {{ contours: RowContour[], archetypeIndices: number[] }}
 */
export function deriveRowGenerationLayout(masterSeed) {
  const random = mulberry32(masterSeed >>> 0);
  const archetypeIndices = fisherYatesShuffle([0, 1, 2, 3], random);
  const contours = archetypeIndices.map((archetypeIndex) => (
    applyHybridContourJitter(rowContourArchetypes[archetypeIndex], random)
  ));

  return { contours, archetypeIndices };
}

/** @param {() => number} random @param {number} min @param {number} max */
function randomInt(random, min, max) {
  return Math.floor(random() * (max - min + 1)) + min;
}

/** @param {number} step */
function clampRhythmStep(step) {
  return Math.round(clamp(step, seedingRhythmStepMin, seedingRhythmStepMax));
}

/** @param {number} multiplierIndex */
function clampSeedTimingMultiplierIndex(multiplierIndex) {
  return Math.round(clamp(
    multiplierIndex,
    seedingTimingMultiplierMinIndex,
    seedingTimingMultiplierMaxIndex,
  ));
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
    timingOffsetValues[offsetIndex] === undefined ? defaultRowTimingOffsetIndex : offsetIndex
  ));
}

/** @param {number} rhythmStep @param {number[]} archetypeIndices */
function rhythmRowTimingOffsetsForLayout(rhythmStep, archetypeIndices) {
  const baseOffsets = rhythmRowTimingOffsets(rhythmStep);

  return archetypeIndices.map((archetypeIndex) => baseOffsets[archetypeIndex]);
}

/**
 * How strongly a step should receive interleave timing/gate shaping.
 * Short phrases spread the effect across every step so the slider stays audible.
 *
 * @param {number} step
 * @param {number} accentPhase
 * @param {number} phraseLength
 */
function stepRhythmWeight(step, accentPhase, phraseLength) {
  if ((step + accentPhase) % 4 === 0) {
    return 1;
  }

  if (phraseLength <= 3) {
    return 0.35 + 0.3 * ((step + accentPhase * 2) % 3) / 2;
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

/** @param {number | undefined} value */
function clampCenterMidi(value) {
  if (value === undefined) {
    return defaultSeedingCenterMidi;
  }

  const parsed = Number.parseInt(String(value), 10);

  if (Number.isNaN(parsed) || parsed < 0) {
    return defaultSeedingCenterMidi;
  }

  return Math.min(127, Math.max(0, parsed));
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

/**
 * @param {number} centerMidi
 * @param {number} root
 * @param {number} modeIndex
 */
export function resolveSeedingCenterMidi(centerMidi, root, modeIndex) {
  const clamped = clampCenterMidi(centerMidi);
  const fallback = defaultStepNoteForScaleRoot(root);
  const center = clamped === defaultSeedingCenterMidi ? fallback : clamped;

  return snapMidiToScale(center, root, modeIndex);
}

/**
 * @param {number} root
 * @param {number} modeIndex
 */
export function seedingCenterNoteOptions(root, modeIndex) {
  const options = [];

  for (let midi = 0; midi <= 127; midi += 1) {
    if (snapMidiToScale(midi, root, modeIndex) === midi) {
      options.push({
        index: options.length,
        midi,
        label: midiToNoteName(midi),
      });
    }
  }

  return options;
}

/**
 * @param {number} centerMidi
 * @param {number} root
 * @param {number} modeIndex
 */
export function seedingCenterNoteIndex(centerMidi, root, modeIndex) {
  const resolved = resolveSeedingCenterMidi(centerMidi, root, modeIndex);
  const options = seedingCenterNoteOptions(root, modeIndex);
  const index = options.findIndex((option) => option.midi === resolved);

  return Math.max(0, index);
}

/**
 * @param {number} centerMidi
 * @param {number} root
 * @param {number} modeIndex
 */
export function seedingCenterNoteLabel(centerMidi, root, modeIndex) {
  return midiToNoteName(resolveSeedingCenterMidi(centerMidi, root, modeIndex));
}

/**
 * @param {number} index
 * @param {number} root
 * @param {number} modeIndex
 */
export function seedingCenterMidiForIndex(index, root, modeIndex) {
  const options = seedingCenterNoteOptions(root, modeIndex);
  const clampedIndex = Math.min(options.length - 1, Math.max(0, Math.round(index)));

  return options[clampedIndex]?.midi ?? resolveSeedingCenterMidi(defaultSeedingCenterMidi, root, modeIndex);
}

/** @param {number} root @param {number} modeIndex @param {number} centerMidi @param {number} semitoneOffset */
function midiForSeedingOffsetFromCenter(root, modeIndex, centerMidi, semitoneOffset) {
  return snapMidiToScale(centerMidi + Math.round(semitoneOffset), root, modeIndex);
}

/**
 * @param {Partial<SeedingRowSettings> & { rangeIndex?: number }} settings
 */
export function normalizeSeedingRowSettings(settings = {}) {
  const merged = {
    ...defaultSeedingRowSettings,
    ...settings,
  };

  return {
    phraseLength: clampPhraseLength(merged.phraseLength),
    centerMidi: clampCenterMidi(merged.centerMidi),
    rangeSemitones: resolveRangeSemitones(settings),
    repetition: clampPercent(merged.repetition),
    complexity: clampPercent(merged.complexity),
    randomness: clampPercent(merged.randomness),
    timingMeanMultiplierIndex: clampSeedTimingMultiplierIndex(merged.timingMeanMultiplierIndex),
    timingVariance: clampPercent(merged.timingVariance),
    symmetry: Boolean(merged.symmetry),
    seed: Math.max(1, Math.round(clamp(merged.seed, 1, 2147483647))),
  };
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
    ...normalizeSeedingRowSettings(settings),
    rhythmStep: resolveRhythmStep(settings),
  };
}

/** @param {boolean[] | undefined} rowTargets */
export function normalizeSeedModeRowTargets(rowTargets) {
  return Array.from({ length: 4 }, (_, row) => Boolean(rowTargets?.[row]));
}

/**
 * @param {Partial<typeof defaultSeedModeState> & {
 *   rowSettings?: Partial<SeedingRowSettings>[],
 * }} state
 */
export function normalizeSeedModeState(state = {}) {
  const sourceRows = state.rowSettings ?? defaultSeedModeState.rowSettings;

  return {
    rhythmStep: resolveRhythmStep({
      rhythmStep: state.rhythmStep,
      rhythmMode: state.rhythmMode,
    }),
    rowSettings: Array.from({ length: 4 }, (_, row) => (
      normalizeSeedingRowSettings(sourceRows[row] ?? defaultSeedingRowSettings)
    )),
    rowTargets: normalizeSeedModeRowTargets(state.rowTargets ?? defaultSeedModeRowTargets),
  };
}

/** @param {boolean[]} rowTargets */
export function seedingTargetRowIndices(rowTargets) {
  return normalizeSeedModeRowTargets(rowTargets)
    .map((targeted, row) => (targeted ? row : -1))
    .filter((row) => row >= 0);
}

/**
 * Settings shown in the panel for the current row-target selection.
 *
 * @param {SeedingRowSettings[]} rowSettings
 * @param {boolean[]} rowTargets
 */
export function displaySeedingRowSettings(rowSettings, rowTargets) {
  const indices = seedingTargetRowIndices(rowTargets);
  const index = indices[0] ?? 0;

  return normalizeSeedingRowSettings(rowSettings[index] ?? defaultSeedingRowSettings);
}

/**
 * Apply partial row-setting updates to every currently targeted row.
 *
 * @param {SeedingRowSettings[]} rowSettings
 * @param {boolean[]} rowTargets
 * @param {Partial<SeedingRowSettings>} updates
 */
export function applySeedingRowSettingsUpdate(rowSettings, rowTargets, updates) {
  const indices = seedingTargetRowIndices(rowTargets);
  const targets = indices.length > 0 ? indices : [0, 1, 2, 3];

  return rowSettings.map((settings, row) => (
    targets.includes(row)
      ? normalizeSeedingRowSettings({ ...settings, ...updates })
      : normalizeSeedingRowSettings(settings)
  ));
}

/**
 * @param {{
 *   rhythmStep?: number,
 *   rowSettings?: SeedingRowSettings[],
 *   root?: number,
 *   modeIndex?: number,
 * }} state
 */
function resolveSeedModeGenerationInputs(state = {}) {
  const rhythmStep = resolveRhythmStep(state);
  const rowSettings = state.rowSettings
    ? normalizeSeedModeState(state).rowSettings
    : Array.from({ length: 4 }, () => normalizeSeedingRowSettings(state));

  return {
    rhythmStep,
    rowSettings,
    root: clampScaleRoot(state.root ?? 0),
    modeIndex: clampScaleModeIndex(state.modeIndex ?? 0),
  };
}

/**
 * @param {RowContour} contour
 * @param {ReturnType<typeof normalizeSeedingRowSettings>} rowOptions
 * @param {number} rhythmStep
 * @param {() => number} random
 */
function generateSeededTimingMultiplierIndices(contour, rowOptions, rhythmStep, random) {
  const stepCount = rowOptions.phraseLength;
  const targetIndex = rowOptions.timingMeanMultiplierIndex;
  const varianceRatio = rowOptions.timingVariance / 100;
  const accentPhase = contour.accentPhase;

  if (stepCount <= 0) {
    return [];
  }

  if (varianceRatio <= 0) {
    return Array.from({ length: stepCount }, () => targetIndex);
  }

  const rhythmBlend = rhythmInterleaveRatio(rhythmStep);
  const accentTimingIndex = clampSeedTimingMultiplierIndex(
    timingMultiplierIndexForValue(rhythmAccentTimingMultiplier(rhythmStep)),
  );
  const maxSpread = Math.max(1, Math.round(varianceRatio * 4));
  const lockedSteps = new Set();
  const multipliers = Array.from({ length: stepCount }, (_, step) => {
    const randomOffset = Math.round((random() * 2 - 1) * maxSpread);
    const baseIndex = clampSeedTimingMultiplierIndex(targetIndex + randomOffset);
    const weight = stepRhythmWeight(step, accentPhase, stepCount);
    const shapedIndex = baseIndex
      + (accentTimingIndex - baseIndex) * weight * rhythmBlend * varianceRatio;

    return clampSeedTimingMultiplierIndex(shapedIndex);
  });

  if (
    stepCount >= 2
    && targetIndex > seedingTimingMultiplierMinIndex
    && targetIndex < seedingTimingMultiplierMaxIndex
  ) {
    const symmetricSpread = Math.max(1, Math.min(
      maxSpread,
      targetIndex - seedingTimingMultiplierMinIndex,
      seedingTimingMultiplierMaxIndex - targetIndex,
    ));
    const firstStep = Math.floor(random() * stepCount);
    const secondStep = (firstStep + 1 + Math.floor(random() * (stepCount - 1))) % stepCount;

    multipliers[firstStep] = targetIndex - symmetricSpread;
    multipliers[secondStep] = targetIndex + symmetricSpread;
    lockedSteps.add(firstStep);
    lockedSteps.add(secondStep);
  }

  const desiredSum = targetIndex * stepCount;
  let currentSum = multipliers.reduce((sum, index) => sum + index, 0);
  const increaseOrder = Array.from({ length: stepCount }, (_, step) => step)
    .sort((left, right) => (
      stepRhythmWeight(left, accentPhase, stepCount) - stepRhythmWeight(right, accentPhase, stepCount)
      || left - right
    ));
  const decreaseOrder = [...increaseOrder].reverse();
  let guard = stepCount * (seedingTimingMultiplierMaxIndex - seedingTimingMultiplierMinIndex + 1);

  while (currentSum !== desiredSum && guard > 0) {
    const direction = currentSum < desiredSum ? 1 : -1;
    const order = direction > 0 ? increaseOrder : decreaseOrder;
    let changed = false;

    for (const step of order) {
      if (lockedSteps.has(step)) {
        continue;
      }

      const next = multipliers[step] + direction;

      if (next < seedingTimingMultiplierMinIndex || next > seedingTimingMultiplierMaxIndex) {
        continue;
      }

      multipliers[step] = next;
      currentSum += direction;
      changed = true;
      break;
    }

    if (!changed) {
      break;
    }

    guard -= 1;
  }

  return multipliers;
}

/**
 * @param {number} row
 * @param {RowContour} contour
 * @param {ReturnType<typeof normalizeSeedingRowSettings>} rowOptions
 * @param {number} root
 * @param {number} modeIndex
 * @param {number} rhythmStep
 */
function generateSeededPhraseRow(row, contour, rowOptions, root, modeIndex, rhythmStep) {
  const random = mulberry32(seedForSeedingRow(rowOptions.seed, row));
  const span = rowOptions.rangeSemitones;
  const halfSpan = Math.max(2, Math.floor(span / 2));
  const complexityRatio = rowOptions.complexity / 100;
  const repetitionRatio = rowOptions.repetition / 100;
  const randomnessRatio = rowOptions.randomness / 100;
  const durationPenalty = rhythmDurationPenalty(rhythmStep);
  const accentPhase = contour.accentPhase;
  const motifLength = rowOptions.symmetry
    ? Math.ceil(rowOptions.phraseLength / 2)
    : rowOptions.phraseLength;
  const motif = [];
  const centerMidi = resolveSeedingCenterMidi(rowOptions.centerMidi, root, modeIndex);
  const center = Math.round(contour.centerSlot * Math.max(1, span / 6));
  let previous = center;

  for (let step = 0; step < motifLength; step += 1) {
    const reusePrevious = step > 0 && random() < repetitionRatio;
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

    const periodic = Math.sin(
      (step / Math.max(1, motifLength - 1)) * Math.PI * contour.waveFactor,
    );
    const directed = Math.round(periodic * halfSpan * (0.35 + complexityRatio * 0.45));
    const leapSpan = Math.max(0, Math.round(randomnessRatio * halfSpan * 0.9));
    const randomPush = leapSpan > 0
      ? randomInt(random, -Math.max(1, leapSpan), Math.max(1, leapSpan))
      : 0;

    previous = Math.round(clamp(center + directed + randomPush, -halfSpan, halfSpan));
    motif.push(previous);
  }

  const degrees = rowOptions.symmetry
    ? [...motif, ...motif.slice(0, rowOptions.phraseLength - motif.length).reverse()]
    : motif;
  const stepTimingMultiplier = generateSeededTimingMultiplierIndices(contour, rowOptions, rhythmStep, random);
  const velocityBase = contour.velocityBase;
  const velocitySwing = Math.round(10 + complexityRatio * 28);
  const velocityNoise = Math.round(randomnessRatio * 12);

  return {
    notes: degrees.map((semitoneOffset) => (
      midiForSeedingOffsetFromCenter(root, modeIndex, centerMidi, semitoneOffset)
    )),
    stepTimingMultiplier,
    stepDurationFraction: degrees.map((_, step) => {
      const accentBase = (step + accentPhase) % 4 === 0 ? 0.95 : 0.82;
      const accent = accentBase + (random() * 0.18 - 0.09) * randomnessRatio;
      const weight = stepRhythmWeight(step, accentPhase, rowOptions.phraseLength);
      const stepPenalty = durationPenalty * weight;

      return Number(clamp(accent - stepPenalty, 0.35, 1).toFixed(2));
    }),
    stepVelocity: degrees.map((_, step) => (
      Math.round(clamp(
        velocityBase
          + Math.sin((step + accentPhase) * 1.7) * velocitySwing
          + (velocityNoise > 0 ? randomInt(random, -velocityNoise, velocityNoise) : 0),
        28,
        127,
      ))
    )),
    stepMuted: degrees.map(() => false),
    stepSkipped: degrees.map((_, step) => (
      complexityRatio < 0.3 && random() < 0.08 && step % 4 !== accentPhase % 4
    )),
    stepProbability: degrees.map((_, step) => (
      randomnessRatio > 0 && random() < randomnessRatio * 0.5 && step % 4 !== 0
        ? Math.round(clamp(100 - randomnessRatio * randomInt(random, 18, 42), 20, 100))
        : defaultStepProbabilityValue
    )),
    stepCycle: degrees.map(() => defaultStepCycle),
    stepCycleOffset: degrees.map(() => defaultStepCycleMask),
  };
}

/**
 * Generate four independent monophonic phrase rows.
 *
 * Accepts either legacy flat settings or per-row `rowSettings` plus global `rhythmStep`.
 *
 * @param {Partial<typeof defaultSeedingSettings> & {
 *   root?: number,
 *   modeIndex?: number,
 *   rowSettings?: Partial<SeedingRowSettings>[],
 * }} settings
 */
export function generateSeededPhraseRows(settings = {}) {
  const { rhythmStep, rowSettings, root, modeIndex } = resolveSeedModeGenerationInputs(settings);
  const masterSeed = resolveMasterSeed(rowSettings);
  const { contours, archetypeIndices } = deriveRowGenerationLayout(masterSeed);
  const rowTimingOffset = rhythmRowTimingOffsetsForLayout(rhythmStep, archetypeIndices);
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
      timingOffsetValues[index] === undefined ? defaultRowTimingOffsetIndex : index
    )),
  };

  for (let row = 0; row < 4; row += 1) {
    const generated = generateSeededPhraseRow(
      row,
      contours[row],
      rowSettings[row],
      root,
      modeIndex,
      rhythmStep,
    );

    rows.notes[row] = generated.notes;
    rows.stepTimingMultiplier[row] = generated.stepTimingMultiplier;
    rows.stepDurationFraction[row] = generated.stepDurationFraction;
    rows.stepVelocity[row] = generated.stepVelocity;
    rows.stepMuted[row] = generated.stepMuted;
    rows.stepSkipped[row] = generated.stepSkipped;
    rows.stepProbability[row] = generated.stepProbability;
    rows.stepCycle[row] = generated.stepCycle;
    rows.stepCycleOffset[row] = generated.stepCycleOffset;
  }

  return rows;
}

/**
 * @param {ReturnType<typeof normalizeSeedModeState>} state
 * @param {number} root
 * @param {number} modeIndex
 */
export function generateSeededPhraseRowsFromSeedModeState(state, root, modeIndex) {
  return generateSeededPhraseRows({
    rhythmStep: state.rhythmStep,
    rowSettings: state.rowSettings,
    root,
    modeIndex,
  });
}

/** @typedef {ReturnType<typeof generateSeededPhraseRows>} GeneratedPhraseRows */

/**
 * Build phrase-row data from live grid state (same shape as generateSeededPhraseRows output).
 *
 * @param {{
 *   grid?: number[][],
 *   stepTimingMultiplier?: number[][],
 *   stepDurationFraction?: number[][],
 *   stepVelocity?: number[][],
 *   stepMuted?: boolean[][],
 *   stepSkipped?: boolean[][],
 *   stepProbability?: number[][],
 *   stepCycle?: number[][],
 *   stepCycleOffset?: number[][],
 *   rowTimingOffset?: number[],
 * }} state
 */
export function phraseRowsFromGridState(state = {}) {
  const grid = state.grid ?? [];

  /** @param {number[][] | boolean[][] | undefined} matrix */
  function rowMatrix(matrix) {
    return Array.from({ length: 4 }, (_, row) => [...(matrix?.[row] ?? [])]);
  }

  return {
    notes: rowMatrix(grid),
    stepTimingMultiplier: rowMatrix(state.stepTimingMultiplier),
    stepDurationFraction: rowMatrix(state.stepDurationFraction),
    stepVelocity: rowMatrix(state.stepVelocity),
    stepMuted: rowMatrix(state.stepMuted),
    stepSkipped: rowMatrix(state.stepSkipped),
    stepProbability: rowMatrix(state.stepProbability),
    stepCycle: rowMatrix(state.stepCycle),
    stepCycleOffset: rowMatrix(state.stepCycleOffset),
    rowTimingOffset: Array.from({ length: 4 }, (_, row) => state.rowTimingOffset?.[row] ?? defaultRowTimingOffsetIndex),
  };
}


/**
 * Merge generated phrase rows into existing state, overwriting only targeted rows.
 *
 * @param {GeneratedPhraseRows} existing
 * @param {GeneratedPhraseRows} generated
 * @param {boolean[]} rowTargets
 */
export function mergeSeededPhraseRows(existing, generated, rowTargets) {
  const targets = normalizeSeedModeRowTargets(rowTargets);
  const perRowKeys = [
    "notes",
    "stepTimingMultiplier",
    "stepDurationFraction",
    "stepVelocity",
    "stepMuted",
    "stepSkipped",
    "stepProbability",
    "stepCycle",
    "stepCycleOffset",
  ];

  /** @type {GeneratedPhraseRows} */
  const merged = {
    notes: [],
    stepTimingMultiplier: [],
    stepDurationFraction: [],
    stepVelocity: [],
    stepMuted: [],
    stepSkipped: [],
    stepProbability: [],
    stepCycle: [],
    stepCycleOffset: [],
    rowTimingOffset: [...(existing.rowTimingOffset ?? [])],
  };

  for (let row = 0; row < 4; row += 1) {
    const source = targets[row] ? generated : existing;

    for (const key of perRowKeys) {
      merged[key][row] = [...(source[key]?.[row] ?? [])];
    }

    merged.rowTimingOffset[row] = targets[row]
      ? (generated.rowTimingOffset?.[row] ?? existing.rowTimingOffset?.[row] ?? defaultRowTimingOffsetIndex)
      : (existing.rowTimingOffset?.[row] ?? defaultRowTimingOffsetIndex);
  }

  return merged;
}

/** @param {boolean[]} rowTargets */
export function hasSeedingRowTargets(rowTargets) {
  return normalizeSeedModeRowTargets(rowTargets).some(Boolean);
}
