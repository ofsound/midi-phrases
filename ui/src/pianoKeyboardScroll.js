import { pianoWhiteKeyWidthToHeight } from "./pianoKeyboardLayout.js";

/**
 * @typedef {Object} PianoKeyboardScrollViewportHandle
 * @property {() => number} getScrollLeft
 * @property {() => number} getViewportWidth
 * @property {() => number} getKeyWidthPx
 * @property {(scrollLeftPx: number) => void} scrollToLeft
 * @property {(octaveDelta: number) => void} scrollByOctave
 */

/** @returns {{ lowest: number, highest: number }} */
export function pianoKeyboardFullMidiRange() {
  return { lowest: 0, highest: 127 };
}

/** @param {number} keyboardHeightPx */
export function whiteKeyWidthPx(keyboardHeightPx) {
  return Math.max(0, keyboardHeightPx) * pianoWhiteKeyWidthToHeight;
}

/** @param {number} whiteCount @param {number} keyWidthPx */
export function pianoKeyboardContentWidthPx(whiteCount, keyWidthPx) {
  return Math.max(0, Math.round(whiteCount)) * Math.max(0, keyWidthPx);
}

/**
 * @param {number} scrollLeftPx
 * @param {number} contentWidthPx
 * @param {number} viewportWidthPx
 */
export function clampScrollLeft(scrollLeftPx, contentWidthPx, viewportWidthPx) {
  const maxScrollLeft = Math.max(0, contentWidthPx - viewportWidthPx);

  return Math.min(maxScrollLeft, Math.max(0, scrollLeftPx));
}

/** @param {number} keyWidthPx */
export function octaveScrollDeltaPx(keyWidthPx) {
  return 7 * keyWidthPx;
}

/**
 * @param {number} scrollLeftPx
 * @param {number} keyWidthPx
 * @param {number} whiteCount
 */
export function whiteIndexAtScrollLeft(scrollLeftPx, keyWidthPx, whiteCount) {
  if (whiteCount <= 0 || keyWidthPx <= 0) return 0;

  return Math.min(whiteCount - 1, Math.max(0, Math.floor(scrollLeftPx / keyWidthPx)));
}

/**
 * @param {number} lowestMidi
 * @param {{ midi: number }[]} whites
 * @param {number} keyWidthPx
 */
export function scrollLeftForLowestMidi(lowestMidi, whites, keyWidthPx) {
  if (whites.length === 0 || keyWidthPx <= 0) return 0;

  let whiteIndex = whites.findIndex((white) => white.midi >= lowestMidi);

  if (whiteIndex < 0) {
    whiteIndex = whites.length - 1;
  }

  return whiteIndex * keyWidthPx;
}

/**
 * @param {number} scrollLeftPx
 * @param {{ midi: number }[]} whites
 * @param {number} keyWidthPx
 */
export function lowestVisibleMidiFromScrollLeft(scrollLeftPx, whites, keyWidthPx) {
  if (whites.length === 0) return 0;

  const index = whiteIndexAtScrollLeft(scrollLeftPx, keyWidthPx, whites.length);

  return whites[index].midi;
}

/**
 * @param {number} scrollLeftPx
 * @param {number} viewportWidthPx
 * @param {{ midi: number }[]} whites
 * @param {number} keyWidthPx
 */
export function highestVisibleMidiFromScrollLeft(scrollLeftPx, viewportWidthPx, whites, keyWidthPx) {
  if (whites.length === 0) return 127;
  if (keyWidthPx <= 0) return whites[whites.length - 1].midi;

  const rightPx = scrollLeftPx + Math.max(0, viewportWidthPx);
  const index = Math.min(
    whites.length - 1,
    Math.max(0, Math.ceil(rightPx / keyWidthPx) - 1),
  );

  return whites[index].midi;
}

/**
 * Horizontal wheel delta for trackpad panning.
 * @param {WheelEvent} event
 */
export function horizontalWheelDeltaPx(event) {
  let delta = event.deltaX;

  if (event.shiftKey && Math.abs(event.deltaY) > Math.abs(delta)) {
    delta = event.deltaY;
  }

  if (Math.abs(delta) < Math.abs(event.deltaY) && !event.shiftKey) {
    return 0;
  }

  return delta;
}
