import { isBlackKey } from "./phraseSchedule.js";
import { midiToNoteName } from "./midiNoteNames.js";

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

/**
 * White and black keys for a MIDI inclusive range.
 * Black keys are centered on the seam between the white key below and above.
 * @param {number} lowestMidi
 * @param {number} highestMidi
 */
export function buildRecordPianoKeys(lowestMidi, highestMidi) {
  /** @type {{ midi: number }[]} */
  const whites = [];

  for (let midi = lowestMidi; midi <= highestMidi; midi += 1) {
    if (!isBlackKey(midi)) {
      whites.push({ midi });
    }
  }

  const whiteCount = whites.length;
  const whiteWidthPercent = whiteCount > 0 ? 100 / whiteCount : 0;
  const blackWidthPercent = whiteWidthPercent * recordPianoBlackKeyWidthRatio;

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
