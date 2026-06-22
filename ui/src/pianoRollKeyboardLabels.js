/** C naturals only — octave markers on piano-roll keyboard gutters. */
export function isPianoRollKeyboardLabelMidi(midi) {
  return Math.round(midi) % 12 === 0;
}

/** @param {number} midi */
export function pianoRollKeyboardOctave(midi) {
  return Math.floor(Math.round(midi) / 12) - 2;
}

/** @param {number} midi */
export function pianoRollKeyboardLabelForMidi(midi) {
  return `C${pianoRollKeyboardOctave(midi)}`;
}

/**
 * Scales label type size down as rows compress; never hides labels entirely.
 * @param {number} rowHeightPx
 */
export function pianoRollKeyboardLabelFontSizePx(rowHeightPx) {
  if (rowHeightPx >= 11) return 9;
  if (rowHeightPx >= 8) return 8;
  if (rowHeightPx >= 6) return 7;

  return 6;
}
