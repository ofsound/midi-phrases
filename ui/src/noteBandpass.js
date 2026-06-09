/** @typedef {{ low: number, high: number }} NoteBandpassBounds */

export const minMidiNote = 0;
export const maxMidiNote = 127;
export const defaultNoteBandpassLowMidi = 36; // C1
export const defaultNoteBandpassHighMidi = 108; // C7

/** @param {number} low @param {number} high @returns {NoteBandpassBounds} */
export function clampNoteBandpass(low, high) {
  const clampedLow = Math.min(maxMidiNote, Math.max(minMidiNote, Math.round(low)));
  const clampedHigh = Math.min(maxMidiNote, Math.max(minMidiNote, Math.round(high)));

  return {
    low: Math.min(clampedLow, clampedHigh),
    high: Math.max(clampedLow, clampedHigh),
  };
}

/** @param {number} midi @param {number} low @param {number} high */
export function notePassesBandpass(midi, low, high) {
  const bounds = clampNoteBandpass(low, high);
  const note = Math.round(midi);
  return note >= bounds.low && note <= bounds.high;
}

/**
 * Drop scheduled notes outside the inclusive MIDI bandpass range.
 *
 * @template {{ midi: number }} T
 * @param {T[]} events
 * @param {number} lowMidi
 * @param {number} highMidi
 * @returns {T[]}
 */
export function applyNoteBandpass(events, lowMidi, highMidi) {
  const bounds = clampNoteBandpass(lowMidi, highMidi);
  return events.filter((event) => event.midi >= bounds.low && event.midi <= bounds.high);
}
