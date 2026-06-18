export const pianoRollPitchPaddingSemitones = 2;
export const pianoRollMinimumVisibleSemitones = 12;
export const pianoRollFallbackMinMidi = 48;
export const pianoRollFallbackMaxMidi = 72;

/**
 * @param {number} minMidi
 * @param {number} maxMidi
 * @param {number} minimumSemitones
 */
function expandMidiRange(minMidi, maxMidi, minimumSemitones) {
  let low = Math.min(minMidi, maxMidi);
  let high = Math.max(minMidi, maxMidi);

  while (high - low + 1 < minimumSemitones) {
    if (low > 0) low -= 1;
    if (high - low + 1 >= minimumSemitones) break;
    if (high < 127) high += 1;
    if (low <= 0 && high >= 127) break;
  }

  return { minMidi: low, maxMidi: high };
}

/** @param {number[]} notes */
export function fittedPitchRangeForNotes(notes) {
  if (notes.length === 0) {
    return {
      minMidi: pianoRollFallbackMinMidi,
      maxMidi: pianoRollFallbackMaxMidi,
    };
  }

  let minMidi = notes[0];
  let maxMidi = notes[0];

  for (const note of notes) {
    minMidi = Math.min(minMidi, note);
    maxMidi = Math.max(maxMidi, note);
  }

  return expandMidiRange(
    Math.max(0, minMidi - pianoRollPitchPaddingSemitones),
    Math.min(127, maxMidi + pianoRollPitchPaddingSemitones),
    pianoRollMinimumVisibleSemitones,
  );
}

/** @param {{ midi: number }[]} scheduled */
export function fittedPitchRangeForSchedule(scheduled) {
  return fittedPitchRangeForNotes(scheduled.map((note) => note.midi));
}
