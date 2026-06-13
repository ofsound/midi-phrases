<script>
  import { releasePointerDragFocus } from "./pointerDragFocus.js";

  import { midiToNoteName } from "./midiNoteNames.js";
  import {
    clampNoteBandpass,
    maxMidiNote,
    minMidiNote,
  } from "./noteBandpass.js";

  /**
   * Dual-thumb MIDI note bandpass for the combination-mode rail.
   *
   * @typedef {Object} Props
   * @property {number} lowMidi
   * @property {number} highMidi
   * @property {(lowMidi: number, highMidi: number) => void} [onChange]
   * @property {(lowMidi: number, highMidi: number) => void | Promise<void>} [onCommit]
   * @property {string} [class]
   */

  /** @type {Props} */
  let {
    lowMidi = 36,
    highMidi = 108,
    onChange = () => {},
    onCommit = () => {},
    class: className = "",
  } = $props();

  /** @type {HTMLElement | null} */
  let trackElement = $state(null);
  /** @type {"low" | "high" | null} */
  let activeThumb = $state(null);
  let dragPointerId = -1;
  let dragStartX = 0;
  let dragStartLow = 0;
  let dragStartHigh = 0;
  let dragFine = false;
  let dragMoved = false;
  /** @type {{ low: number, high: number } | null} */
  let dragPreviewBounds = $state(null);

  let bounds = $derived(
    dragPreviewBounds ?? clampNoteBandpass(lowMidi, highMidi),
  );
  let lowPercent = $derived(((bounds.low - minMidiNote) / (maxMidiNote - minMidiNote)) * 100);
  let highPercent = $derived(((bounds.high - minMidiNote) / (maxMidiNote - minMidiNote)) * 100);

  /** @param {number} clientX */
  function midiFromClientX(clientX) {
    if (!trackElement) return minMidiNote;

    const rect = trackElement.getBoundingClientRect();
    const ratio = Math.min(1, Math.max(0, (clientX - rect.left) / Math.max(1, rect.width)));
    return Math.round(minMidiNote + ratio * (maxMidiNote - minMidiNote));
  }

  /** @param {number} deltaX */
  function semitoneDeltaFromDrag(deltaX) {
    if (!trackElement) return 0;

    const rect = trackElement.getBoundingClientRect();
    const semitonesPerPixel = (maxMidiNote - minMidiNote) / Math.max(1, rect.width);
    const sensitivity = dragFine ? 0.22 : 1;
    return Math.round(deltaX * semitonesPerPixel * sensitivity);
  }

  /** @param {"low" | "high"} thumb @param {number} midi */
  function setThumbValue(thumb, midi) {
    if (thumb === "low") {
      const next = clampNoteBandpass(midi, bounds.high);
      onChange(next.low, next.high);
      return;
    }

    const next = clampNoteBandpass(bounds.low, midi);
    onChange(next.low, next.high);
  }

  /** @param {PointerEvent} event @param {"low" | "high"} thumb */
  function handleThumbPointerDown(event, thumb) {
    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    activeThumb = thumb;
    dragPointerId = event.pointerId;
    dragStartX = event.clientX;
    dragStartLow = bounds.low;
    dragStartHigh = bounds.high;
    dragFine = event.shiftKey;
    dragMoved = false;
    dragPreviewBounds = clampNoteBandpass(lowMidi, highMidi);
  }

  /** @param {PointerEvent} event */
  function handleThumbPointerMove(event) {
    if (activeThumb === null || event.pointerId !== dragPointerId) return;

    event.preventDefault();
    dragFine = event.shiftKey;

    const delta = semitoneDeltaFromDrag(event.clientX - dragStartX);

    if (delta !== 0) dragMoved = true;

    if (activeThumb === "low") {
      const next = clampNoteBandpass(dragStartLow + delta, dragStartHigh);
      dragPreviewBounds = next;
      onChange(next.low, next.high);
      return;
    }

    const next = clampNoteBandpass(dragStartLow, dragStartHigh + delta);
    dragPreviewBounds = next;
    onChange(next.low, next.high);
  }

  /** @param {PointerEvent} event */
  function handleThumbPointerUp(event) {
    if (event.pointerId !== dragPointerId) return;

    event.preventDefault();
    event.currentTarget.releasePointerCapture(event.pointerId);

    const committed = clampNoteBandpass(bounds.low, bounds.high);

    if (dragMoved) {
      onCommit(committed.low, committed.high);
    }

    activeThumb = null;
    dragPointerId = -1;
    dragMoved = false;
    dragPreviewBounds = null;
    releasePointerDragFocus(event);
  }

  /** @param {PointerEvent} event */
  function handleTrackPointerDown(event) {
    if (event.button !== 0 || !trackElement) return;
    if (event.target instanceof Element && event.target.closest(".note-bandpass-thumb")) return;

    const midi = midiFromClientX(event.clientX);
    const thumb = Math.abs(midi - bounds.low) <= Math.abs(midi - bounds.high) ? "low" : "high";
    const committed =
      thumb === "low"
        ? clampNoteBandpass(midi, bounds.high)
        : clampNoteBandpass(bounds.low, midi);
    onChange(committed.low, committed.high);
    onCommit(committed.low, committed.high);
  }
</script>

<div class={`note-bandpass-slider ${className}`.trim()} role="group" aria-label="MIDI note bandpass">
  <span class="note-bandpass-display" aria-hidden="true">{midiToNoteName(bounds.low)}</span>

  <div
    class="note-bandpass-track-shell"
    role="presentation"
    bind:this={trackElement}
    onpointerdown={handleTrackPointerDown}
  >
    <div class="note-bandpass-track" aria-hidden="true">
      <div
        class="note-bandpass-range"
        style={`left: ${lowPercent}%; width: ${Math.max(0, highPercent - lowPercent)}%;`}
      ></div>
    </div>

    <div
      class="note-bandpass-thumb"
      role="slider"
      tabindex="-1"
      data-thumb="low"
      style={`left: ${lowPercent}%;`}
      aria-label={`Low note ${midiToNoteName(bounds.low)}`}
      aria-valuemin={minMidiNote}
      aria-valuemax={bounds.high}
      aria-valuenow={bounds.low}
      data-cursor="pointer"
      onpointerdown={(event) => handleThumbPointerDown(event, "low")}
      onpointermove={handleThumbPointerMove}
      onpointerup={handleThumbPointerUp}
      onpointercancel={handleThumbPointerUp}
    >
      <span class="note-bandpass-knob" aria-hidden="true"></span>
    </div>

    <div
      class="note-bandpass-thumb"
      role="slider"
      tabindex="-1"
      data-thumb="high"
      style={`left: ${highPercent}%;`}
      aria-label={`High note ${midiToNoteName(bounds.high)}`}
      aria-valuemin={bounds.low}
      aria-valuemax={maxMidiNote}
      aria-valuenow={bounds.high}
      data-cursor="pointer"
      onpointerdown={(event) => handleThumbPointerDown(event, "high")}
      onpointermove={handleThumbPointerMove}
      onpointerup={handleThumbPointerUp}
      onpointercancel={handleThumbPointerUp}
    >
      <span class="note-bandpass-knob" aria-hidden="true"></span>
    </div>
  </div>

  <span class="note-bandpass-display" aria-hidden="true">{midiToNoteName(bounds.high)}</span>
</div>
