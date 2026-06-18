<script>
  import { onDestroy } from "svelte";
  import { midiToNoteName } from "./midiNoteNames.js";
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {any} value
   * @property {any} [resetValue] - MIDI note to restore on double-click; omit to disable reset.
   * @property {string} [ariaLabel]
   * @property {(value: number, delta: number) => number} [stepValue]
   * @property {boolean} [deferCommit] - Preview while dragging; commit on release.
   * @property {boolean} [minimal] - High-contrast note-only presentation for compact cells.
   * @property {() => void} [onGestureStart] - Called at drag start when {@link deferCommit} is true.
   * @property {(value: number) => void} [onValuePreview] - Lightweight preview while dragging.
   * @property {(value: number) => void | Promise<void>} [onValueCommit] - Final commit on release.
   * @property {(value: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    accent = emeraldRowAccent,
    muted = false,
    value,
    resetValue = undefined,
    ariaLabel = "Note",
    stepValue = (current, delta) => current + delta,
    deferCommit = false,
    minimal = false,
    onGestureStart = undefined,
    onValuePreview = undefined,
    onValueCommit = undefined,
    onValueChange = () => {}
  } = $props();

  const pixelsPerStep = 10;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;
  let previewFrameId = 0;
  /** @type {number | null} */
  let pendingPreviewValue = null;

  let displayName = $derived(midiToNoteName(value));

  function clampMidi(note) {
    return Math.min(127, Math.max(0, Math.round(note)));
  }

  function noteFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampMidi(stepValue(dragStartValue, steps));
  }

  function cancelPreviewFrame() {
    if (!previewFrameId) return;

    cancelAnimationFrame(previewFrameId);
    previewFrameId = 0;
  }

  function flushPreviewFrame() {
    previewFrameId = 0;

    const next = pendingPreviewValue;
    pendingPreviewValue = null;

    if (next !== null) {
      onValuePreview?.(next);
    }
  }

  /** @param {number} next */
  function schedulePreview(next) {
    if (next === value) return;

    if (!deferCommit || !onValuePreview) {
      onValueChange(next);
      return;
    }

    pendingPreviewValue = next;

    if (previewFrameId) return;

    previewFrameId = requestAnimationFrame(flushPreviewFrame);
  }

  /** @param {number} next */
  function commitDeferredValue(next) {
    onGestureStart?.();
    onValuePreview?.(next);
    onValueCommit?.(next);
  }

  /** @param {number} next */
  function applyValue(next) {
    if (deferCommit && onValueCommit) {
      commitDeferredValue(next);
      return;
    }

    onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    absorbPointerDragFocus(event);
    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;

    if (deferCommit) {
      onGestureStart?.();
    }
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    schedulePreview(noteFromDrag(event.clientY));
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);

    if (!deferCommit || !onValueCommit) {
      cancelPreviewFrame();
      pendingPreviewValue = null;
      releasePointerDragFocus(event);
      return;
    }

    const finalValue =
      pendingPreviewValue ?? clampMidi(stepValue(dragStartValue, Math.round((dragStartY - event.clientY) / pixelsPerStep)));

    cancelPreviewFrame();
    pendingPreviewValue = null;
    onValueCommit(finalValue);
    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) applyValue(resetValue);
  }

  onDestroy(() => {
    cancelPreviewFrame();
  });
</script>

<div
  data-cursor="vertical-drag"
  class="inline-flex min-w-0 touch-none select-none items-center rounded-sm outline-none {accent.ringFocusWithWidth} {minimal
    ? 'text-control-primary-text'
    : muted
      ? 'text-text-muted'
      : dragging
        ? accent.textAccentLight
        : 'text-text'}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={0}
  aria-valuemax={127}
  aria-valuenow={value}
  aria-valuetext={displayName}
  tabindex="-1"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  title={resetValue !== undefined ? "Drag to change · double-click to reset" : undefined}
  onkeydown={(event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (value < 127) applyValue(clampMidi(stepValue(value, 1)));
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > 0) applyValue(clampMidi(stepValue(value, -1)));
    }
  }}
>
  <span class="truncate font-sans leading-none font-bold tabular-nums {minimal ? 'text-xs' : 'text-sm'}"
    >{displayName}</span
  >
</div>
