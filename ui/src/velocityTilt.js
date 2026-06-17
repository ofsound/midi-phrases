import { maxMidiNote, minMidiNote } from "./noteBandpass.js";
import { midiToNoteName } from "./midiNoteNames.js";

export const defaultVelocityTiltPivotMidi = 60;
export const defaultVelocityTiltAmount = 0;
export const minVelocityTiltAmount = -48;
export const maxVelocityTiltAmount = 48;

/** @param {number} value */
export function clampVelocityTiltPivotMidi(value) {
  const parsed = Number.parseInt(String(value), 10);
  return Number.isNaN(parsed)
    ? defaultVelocityTiltPivotMidi
    : Math.min(maxMidiNote, Math.max(minMidiNote, parsed));
}

/** @param {number} value */
export function clampVelocityTiltAmount(value) {
  const parsed = Number.parseInt(String(value), 10);
  return Number.isNaN(parsed)
    ? defaultVelocityTiltAmount
    : Math.min(maxVelocityTiltAmount, Math.max(minVelocityTiltAmount, parsed));
}

/** @param {number} value */
export function formatVelocityTiltPivot(value) {
  return midiToNoteName(clampVelocityTiltPivotMidi(value));
}

/** @param {number} value */
export function formatVelocityTiltAmount(value) {
  const clamped = clampVelocityTiltAmount(value);
  if (clamped > 0) return `+${clamped}`;
  return String(clamped);
}

/** @param {number} velocity @param {number} midi @param {number} pivotMidi @param {number} amount */
export function velocityTiltOutputVelocity(velocity, midi, pivotMidi, amount) {
  const baseVelocity = Math.min(127, Math.max(1, Math.round(velocity)));
  const pivot = clampVelocityTiltPivotMidi(pivotMidi);
  const tilt = clampVelocityTiltAmount(amount);

  if (tilt === 0 || midi === pivot) return baseVelocity;

  const note = Math.min(maxMidiNote, Math.max(minMidiNote, Math.round(midi)));
  const octaveDistance = (note - pivot) / 12;
  const delta = Math.round(octaveDistance * tilt);

  return Math.min(127, Math.max(1, baseVelocity + delta));
}

/**
 * Apply a tilt slope in velocity-per-octave around a pivot note.
 *
 * @template {{ midi: number, velocity: number }} T
 * @param {T[]} events
 * @param {number} pivotMidi
 * @param {number} amount
 * @returns {T[]}
 */
export function applyVelocityTilt(events, pivotMidi, amount) {
  const tilt = clampVelocityTiltAmount(amount);

  if (tilt === 0) return events;

  const pivot = clampVelocityTiltPivotMidi(pivotMidi);

  return events.map((event) => ({
    ...event,
    velocity: velocityTiltOutputVelocity(event.velocity, event.midi, pivot, tilt),
  }));
}
