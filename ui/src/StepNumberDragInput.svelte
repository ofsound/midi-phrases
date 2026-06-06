<script>
  import { onDestroy } from "svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  
  
  
  
  
  
  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {any} value
   * @property {number} [min]
   * @property {number} [max]
   * @property {any} [formatValue]
   * @property {number} [displayAdd] - Added to the stored value for display and aria only (e.g. 1 for 1-based cycle offset).
   * @property {any} [resetValue] - Value restored on double-click; omit to disable reset.
   * @property {string} [ariaLabel]
   * @property {boolean} [disabled]
   * @property {boolean} [boxed] - When true, use header control box styling (matches DiscreteDragSelect).
   * @property {boolean} [compact] - Tight boxed width for compact header controls with short values.
   * @property {boolean} [deferCommit] - Preview while dragging; commit on release.
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
    min = 0,
    max = 127,
    formatValue = undefined,
    displayAdd = 0,
    resetValue = undefined,
    ariaLabel = "Value",
    disabled = false,
    boxed = false,
    compact = false,
    deferCommit = false,
    onGestureStart = undefined,
    onValuePreview = undefined,
    onValueCommit = undefined,
    onValueChange = () => {}
  } = $props();

  const boxedControlBaseClasses =
    "flex h-8 items-center justify-center rounded-md border bg-gradient-to-b from-zinc-700/50 to-zinc-950 px-2 text-sm font-semibold tabular-nums transition-[border-color,box-shadow] duration-75";

  const pixelsPerStep = 4;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;
  let previewFrameId = 0;
  /** @type {number | null} */
  let pendingPreviewValue = null;

  let displayValue =
    $derived(formatValue !== undefined
      ? formatValue(value)
      : String(Math.round(value + displayAdd)));
  let ariaValueMin = $derived(min + displayAdd);
  let ariaValueMax = $derived(max + displayAdd);
  let ariaValueNow = $derived(value + displayAdd);

  function clampValue(next) {
    return Math.min(max, Math.max(min, Math.round(next)));
  }

  function valueFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampValue(dragStartValue + steps);
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
    if (disabled) return;

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
    if (!dragging || disabled) return;

    schedulePreview(valueFromDrag(event.clientY));
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);

    if (!deferCommit || !onValueCommit) {
      cancelPreviewFrame();
      pendingPreviewValue = null;
      return;
    }

    const finalValue =
      pendingPreviewValue ?? clampValue(valueFromDrag(event.clientY));

    cancelPreviewFrame();
    pendingPreviewValue = null;
    onValueCommit(finalValue);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (disabled || resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) applyValue(resetValue);
  }

  onDestroy(() => {
    cancelPreviewFrame();
  });
</script>

<div
  data-cursor={disabled ? "default" : "vertical-drag"}
  class="touch-none select-none outline-none {boxed
    ? `${boxedControlBaseClasses} ${compact ? 'w-14' : 'w-[4.5rem]'}`
    : 'inline-flex items-center rounded-sm'} {disabled
    ? 'opacity-50'
    : ''} {accent.ringFocusWithWidth} {boxed
    ? dragging && !muted
      ? `${accent.dragBorder} ${accent.dragShadow}`
      : muted
        ? 'border-zinc-800'
        : 'border-zinc-700'
    : ''} {muted
    ? 'text-zinc-500'
    : dragging
      ? accent.textAccentLight
      : 'text-zinc-100'}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={ariaValueMin}
  aria-valuemax={ariaValueMax}
  aria-valuenow={ariaValueNow}
  aria-valuetext={displayValue}
  aria-disabled={disabled}
  tabindex={disabled ? -1 : 0}
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  title={!disabled && resetValue !== undefined
    ? "Drag to change · double-click to reset"
    : undefined}
  onkeydown={(event) => {
    if (disabled) return;

    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (value < max) applyValue(value + 1);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > min) applyValue(value - 1);
    }
  }}
>
  <span
    class="font-sans leading-none tabular-nums {boxed
      ? ''
      : 'text-xs font-bold'} {dragging && !muted ? accent.textAccentLight : ''}"
    >{displayValue}</span
  >
</div>
