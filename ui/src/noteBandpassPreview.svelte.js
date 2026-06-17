import { clampNoteBandpass } from "./noteBandpass.js";

/** Live bandpass while dragging — kept outside App so phrase rows do not re-render. */
export const noteBandpassPreview = $state({
  low: null,
  high: null,
});

let rafId = 0;
/** @type {{ low: number, high: number } | null} */
let pending = null;
/** @type {((low: number, high: number) => void) | null} */
let applyListener = null;

/** @param {((low: number, high: number) => void) | null} listener */
export function setNoteBandpassPreviewApplyListener(listener) {
  applyListener = listener;
}

/** @param {number} lowMidi @param {number} highMidi */
export function scheduleNoteBandpassPreview(lowMidi, highMidi) {
  pending = clampNoteBandpass(lowMidi, highMidi);

  if (rafId) return;

  rafId = requestAnimationFrame(() => {
    rafId = 0;

    if (!pending) return;

    const next = pending;
    pending = null;

    noteBandpassPreview.low = next.low;
    noteBandpassPreview.high = next.high;
    applyListener?.(next.low, next.high);
  });
}

export function clearNoteBandpassPreview() {
  if (rafId) {
    cancelAnimationFrame(rafId);
    rafId = 0;
  }

  pending = null;
  noteBandpassPreview.low = null;
  noteBandpassPreview.high = null;
}
