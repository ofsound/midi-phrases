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

/** @param {{ midi: number }[]} scheduled */
export function fittedPitchRangeForSchedule(scheduled) {
  if (scheduled.length === 0) {
    return {
      minMidi: pianoRollFallbackMinMidi,
      maxMidi: pianoRollFallbackMaxMidi,
    };
  }

  let minMidi = scheduled[0].midi;
  let maxMidi = scheduled[0].midi;

  for (const note of scheduled) {
    minMidi = Math.min(minMidi, note.midi);
    maxMidi = Math.max(maxMidi, note.midi);
  }

  return expandMidiRange(
    Math.max(0, minMidi - pianoRollPitchPaddingSemitones),
    Math.min(127, maxMidi + pianoRollPitchPaddingSemitones),
    pianoRollMinimumVisibleSemitones,
  );
}
