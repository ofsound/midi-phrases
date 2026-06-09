/** @typedef {{ start: number, end: number, midi: number, velocity: number, row: number, step: number }} ScheduledNote */

export const defaultOctavizerRelativeVelocity = 0;
export const minOctavizerRelativeVelocity = -127;
export const maxOctavizerRelativeVelocity = 127;
export const octavizerSemitoneShift = 12;
export const minMidiNote = 0;
export const maxMidiNote = 127;

/** @param {number} relativeVelocity */
export function clampOctavizerRelativeVelocity(relativeVelocity) {
  return Math.min(maxOctavizerRelativeVelocity, Math.max(minOctavizerRelativeVelocity, Math.round(relativeVelocity)));
}

/** @param {number} baseVelocity @param {number} relativeVelocity */
export function octavizerOutputVelocity(baseVelocity, relativeVelocity) {
  return Math.min(127, Math.max(1, baseVelocity + clampOctavizerRelativeVelocity(relativeVelocity)));
}

/**
 * Duplicate each scheduled note with octave-shifted copies when enabled.
 *
 * @param {ScheduledNote[]} events
 * @param {object} params
 * @param {boolean} [params.down8vaEnabled]
 * @param {boolean} [params.up8vaEnabled]
 * @param {number} [params.down8vaRelativeVelocity]
 * @param {number} [params.up8vaRelativeVelocity]
 * @returns {ScheduledNote[]}
 */
export function applyOctavizer(events, {down8vaEnabled = false, up8vaEnabled = false, down8vaRelativeVelocity = defaultOctavizerRelativeVelocity, up8vaRelativeVelocity = defaultOctavizerRelativeVelocity} = {}) {
  if ((!down8vaEnabled && !up8vaEnabled) || events.length === 0) return events;

  /** @type {ScheduledNote[]} */
  const expanded = [...events];

  for (const event of events) {
    if (down8vaEnabled) {
      const midi = event.midi - octavizerSemitoneShift;
      const velocity = octavizerOutputVelocity(event.velocity, down8vaRelativeVelocity);

      if (midi >= minMidiNote && velocity > 0) {
        expanded.push({...event, midi, velocity});
      }
    }

    if (up8vaEnabled) {
      const midi = event.midi + octavizerSemitoneShift;
      const velocity = octavizerOutputVelocity(event.velocity, up8vaRelativeVelocity);

      if (midi <= maxMidiNote && velocity > 0) {
        expanded.push({...event, midi, velocity});
      }
    }
  }

  return expanded.sort((a, b) => a.start - b.start || a.midi - b.midi || a.row - b.row || a.step - b.step);
}

/** @param {number} value */
export function formatOctavizerRelativeVelocity(value) {
  const clamped = clampOctavizerRelativeVelocity(value);

  if (clamped > 0) return `+${clamped}`;
  return String(clamped);
}
