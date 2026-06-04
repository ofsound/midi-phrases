const NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];

const LETTER_SEMITONE = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };

/** MIDI note number to name (60 = C3). */
export function midiToNoteName(note) {
  const n = Math.min(127, Math.max(0, Math.round(Number(note))));
  const octave = Math.floor(n / 12) - 2;
  return `${NOTE_NAMES[n % 12]}${octave}`;
}

/** Parse note name (e.g. C3, F#4, Bb2) to MIDI 0–127; null if invalid. */
export function noteNameToMidi(text) {
  const trimmed = String(text).trim().replace(/\s+/g, "");
  const match = /^([A-Ga-g])([#b♯♭]?)(-?\d+)$/.exec(trimmed);

  if (!match) return null;

  const letter = match[1].toUpperCase();
  let accidental = match[2];

  if (accidental === "♯") accidental = "#";
  if (accidental === "♭") accidental = "b";

  const octave = Number.parseInt(match[3], 10);

  if (Number.isNaN(octave)) return null;

  let semitone = LETTER_SEMITONE[letter];

  if (accidental === "#") semitone += 1;
  else if (accidental === "b") semitone -= 1;

  const midi = (octave + 2) * 12 + semitone;

  if (midi < 0 || midi > 127) return null;

  return midi;
}

/** Matches PluginProcessor::defaultRowTimingOffsetIndex (0 quarters). */
export const defaultRowTimingOffsetIndex = 3;

/** Matches PluginProcessor::defaultStepVelocity. */
export const defaultStepVelocity = 100;

/** Matches PluginProcessor::defaultStepNote (C3). */
export const defaultStepNote = 60;

/** Matches PluginProcessor::defaultStepDurationFraction. */
export const defaultStepDurationFraction = 1;

export const defaultPhraseGrid = () => [
  [],
  [],
  [],
  [],
];

/** Default MIDI note for a phrase row (matches PluginProcessor::defaultNoteForRow). */
export function defaultNoteForRow(_row) {
  return defaultStepNote;
}

export const defaultStepDurationGrid = () => [
  [],
  [],
  [],
  [],
];

export const defaultStepTimingMultiplierGrid = () => [
  [],
  [],
  [],
  [],
];

export const defaultStepVelocityGrid = () => [
  [],
  [],
  [],
  [],
];

export const defaultStepMutedGrid = () => [
  [],
  [],
  [],
  [],
];

export const defaultStepSkippedGrid = () => [
  [],
  [],
  [],
  [],
];

export const defaultStepProbabilityGrid = () => [
  [],
  [],
  [],
  [],
];

export const defaultStepCycleGrid = () => [
  [],
  [],
  [],
  [],
];

export const defaultStepCycleOffsetGrid = () => [
  [],
  [],
  [],
  [],
];
