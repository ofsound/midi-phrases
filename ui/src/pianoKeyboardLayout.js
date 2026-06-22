import { defaultStepNoteForScaleRoot, midiToNoteName } from "./midiNoteNames.js";
import { isBlackKey } from "./phraseSchedule.js";

export const scalePreviewPianoOctaves = 2;

export const scalePreviewPianoSemitones = scalePreviewPianoOctaves * 12;

/** Octaves visible in the record keyboard (5 × 12 = 60 semitones). */
export const recordPianoVisibleOctaves = 5;

export const recordPianoVisibleSemitones = recordPianoVisibleOctaves * 12;

/** @param {number} offset */
export function clampRecordPianoOctaveOffset(offset) {
  const maxOffset = Math.max(0, Math.floor((127 - recordPianoVisibleSemitones + 1) / 12));

  return Math.min(maxOffset, Math.max(0, offset));
}

/** @param {number} octaveOffset */
export function recordPianoMidiRange(octaveOffset) {
  const clamped = clampRecordPianoOctaveOffset(octaveOffset);
  const lowest = clamped * 12;
  const highest = Math.min(127, lowest + recordPianoVisibleSemitones - 1);

  return { lowest, highest };
}

/** Black keys are this fraction of a white key’s width (typical piano ~60–65%). */
export const recordPianoBlackKeyWidthRatio = 0.62;

/** Narrower blacks for short preview ranges so adjacent accidentals don’t touch. */
export const scalePreviewBlackKeyWidthRatio = 0.52;

/** White-key width ÷ height — keeps virtual keyboards near real piano proportions (~23×150 mm). */
export const pianoWhiteKeyWidthToHeight = 23 / 150;

/**
 * Keyboard surface width ÷ height for a row of white keys at {@link pianoWhiteKeyWidthToHeight}.
 * @param {number} whiteCount
 */
export function pianoKeyboardAspectRatio(whiteCount) {
  const count = Math.max(1, Math.round(whiteCount));

  return count * pianoWhiteKeyWidthToHeight;
}

/**
 * White and black keys for a MIDI inclusive range.
 * Black keys are centered on the seam between the white key below and above.
 * @param {number} lowestMidi
 * @param {number} highestMidi
 * @param {{ blackKeyWidthRatio?: number }} [options]
 */
export function buildRecordPianoKeys(lowestMidi, highestMidi, options = {}) {
  const blackKeyWidthRatio = options.blackKeyWidthRatio ?? recordPianoBlackKeyWidthRatio;
  /** @type {{ midi: number }[]} */
  const whites = [];

  for (let midi = lowestMidi; midi <= highestMidi; midi += 1) {
    if (!isBlackKey(midi)) {
      whites.push({ midi });
    }
  }

  const whiteCount = whites.length;
  const whiteWidthPercent = whiteCount > 0 ? 100 / whiteCount : 0;
  const blackWidthPercent = whiteWidthPercent * blackKeyWidthRatio;

  /** @type {Map<number, number>} */
  const whiteIndexByMidi = new Map(whites.map((white, index) => [white.midi, index]));

  /** @type {{ midi: number, centerPercent: number, widthPercent: number }[]} */
  const blacks = [];

  for (let midi = lowestMidi; midi <= highestMidi; midi += 1) {
    if (!isBlackKey(midi)) {
      continue;
    }

    let leftWhiteMidi = midi - 1;

    while (leftWhiteMidi >= lowestMidi && isBlackKey(leftWhiteMidi)) {
      leftWhiteMidi -= 1;
    }

    const whiteIndex = whiteIndexByMidi.get(leftWhiteMidi);

    if (whiteIndex === undefined) {
      continue;
    }

    const centerPercent = ((whiteIndex + 1) / whiteCount) * 100;

    blacks.push({
      midi,
      centerPercent,
      widthPercent: blackWidthPercent,
    });
  }

  return { whites, blacks, whiteCount };
}

/** @param {number} lowestMidi @param {number} highestMidi */
export function recordPianoRangeLabel(lowestMidi, highestMidi) {
  return `${midiToNoteName(lowestMidi)} – ${midiToNoteName(highestMidi)}`;
}

/** Two octaves from the pattern key center at the default step octave. */
export function scalePreviewMidiRange(scaleRoot) {
  const lowest = defaultStepNoteForScaleRoot(scaleRoot);
  const highest = Math.min(127, lowest + scalePreviewPianoSemitones - 1);

  return { lowest, highest };
}

/** Octaves visible in the step inspector keyboard. */
export const stepInspectorPianoOctaves = 3;

export const stepInspectorPianoSemitones = stepInspectorPianoOctaves * 12;

/** @param {number} offset */
export function clampStepInspectorOctaveOffset(offset) {
  const maxOffset = Math.max(0, Math.floor((127 - stepInspectorPianoSemitones + 1) / 12));

  return Math.min(maxOffset, Math.max(0, offset));
}

/** @param {number} octaveOffset */
export function stepInspectorMidiRange(octaveOffset) {
  const clamped = clampStepInspectorOctaveOffset(octaveOffset);
  const lowest = clamped * 12;
  const highest = Math.min(127, lowest + stepInspectorPianoSemitones - 1);

  return { lowest, highest };
}

/** Octave offset that centers `noteMidi` in the inspector keyboard window when possible. */
export function stepInspectorOctaveOffsetForNote(noteMidi) {
  const note = Math.min(127, Math.max(0, Math.round(noteMidi)));
  const idealOffset = (note - (stepInspectorPianoSemitones - 1) / 2) / 12;

  return clampStepInspectorOctaveOffset(Math.round(idealOffset));
}
