export const defaultGlobalTransposeSemitones = 0;
export const minGlobalTransposeSemitones = -48;
export const maxGlobalTransposeSemitones = 48;

/** @param {number} value */
export function clampGlobalTransposeSemitones(value) {
  const parsed = Number.isFinite(value) ? Math.round(value) : defaultGlobalTransposeSemitones;

  return Math.min(maxGlobalTransposeSemitones, Math.max(minGlobalTransposeSemitones, parsed));
}

/** @param {number} value */
export function formatGlobalTransposeSemitones(value) {
  const clamped = clampGlobalTransposeSemitones(value);

  return clamped > 0 ? `+${clamped}` : String(clamped);
}

/** Final pitch transform. The stored offset is preserved; only MIDI output is clamped. */
/** @param {{ midi: number }[]} events @param {number} semitones */
export function applyGlobalTranspose(events, semitones) {
  const offset = clampGlobalTransposeSemitones(semitones);

  if (offset === 0) return events;

  return events.map((event) => ({
    ...event,
    midi: Math.min(127, Math.max(0, event.midi + offset)),
  }));
}
