import { clampNoteBandpass } from "./noteBandpass.js";

/** Live bandpass while dragging — kept outside App so phrase rows do not re-render. */
export const noteBandpassPreview = $state({
  low: null,
  high: null,
});

let rafId = 0;
/** @type {{ low: number, high: number } | null} */
let pending = null;

/** @param {number} lowMidi @param {number} highMidi */
export function scheduleNoteBandpassPreview(lowMidi, highMidi) {
  pending = clampNoteBandpass(lowMidi, highMidi);

  if (rafId) return;

  rafId = requestAnimationFrame(() => {
    rafId = 0;

    if (!pending) return;

    noteBandpassPreview.low = pending.low;
    noteBandpassPreview.high = pending.high;
    pending = null;
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
