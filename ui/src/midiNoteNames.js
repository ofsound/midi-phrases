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

export const defaultPhraseGrid = () => [
  [60, 60, 60, 60],
  [64, 64, 64, 64],
  [67, 67, 67, 67],
  [72, 72, 72, 72],
];

export const defaultStepDurationGrid = () => [
  [3, 3, 3, 3],
  [3, 3, 3, 3],
  [3, 3, 3, 3],
  [3, 3, 3, 3],
];

export const defaultStepTimingMultiplierGrid = () => [
  [2, 2, 2, 2],
  [2, 2, 2, 2],
  [2, 2, 2, 2],
  [2, 2, 2, 2],
];

export const defaultStepVelocityGrid = () => [
  [100, 100, 100, 100],
  [100, 100, 100, 100],
  [100, 100, 100, 100],
  [100, 100, 100, 100],
];
