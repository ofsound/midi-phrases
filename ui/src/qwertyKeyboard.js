/** Ableton / JUCE computer MIDI keyboard base note (C3). */
export const qwertyBaseMidi = 60;

/** @type {ReadonlyMap<string, number>} */
export const qwertyKeySemitoneOffsets = new Map([
  ["KeyA", 0],
  ["KeyW", 1],
  ["KeyS", 2],
  ["KeyE", 3],
  ["KeyD", 4],
  ["KeyF", 5],
  ["KeyT", 6],
  ["KeyG", 7],
  ["KeyY", 8],
  ["KeyH", 9],
  ["KeyU", 10],
  ["KeyJ", 11],
  ["KeyK", 12],
  ["KeyO", 13],
  ["KeyL", 14],
  ["KeyP", 15],
  ["Semicolon", 16],
]);

export const qwertyOctaveDownCode = "KeyZ";
export const qwertyOctaveUpCode = "KeyX";

/** @param {number} offset */
export function clampQwertyOctaveOffset(offset) {
  const highestSemitoneOffset = 16;
  const minOffset = Math.ceil((0 - qwertyBaseMidi - highestSemitoneOffset) / 12);
  const maxOffset = Math.floor((127 - qwertyBaseMidi - highestSemitoneOffset) / 12);
  return Math.min(maxOffset, Math.max(minOffset, Math.round(offset)));
}

/** @param {string} code @param {number} octaveOffset */
export function midiFromQwertyCode(code, octaveOffset) {
  const semitone = qwertyKeySemitoneOffsets.get(code);

  if (semitone === undefined) {
    return null;
  }

  const midi = qwertyBaseMidi + octaveOffset * 12 + semitone;

  if (midi < 0 || midi > 127) {
    return null;
  }

  return midi;
}

/** @param {string} code */
export function isQwertyNoteCode(code) {
  return qwertyKeySemitoneOffsets.has(code);
}

/** @param {string} code */
export function isQwertyOctaveCode(code) {
  return code === qwertyOctaveDownCode || code === qwertyOctaveUpCode;
}
