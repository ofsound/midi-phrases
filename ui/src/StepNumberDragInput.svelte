<script>
  import { onDestroy } from "svelte";
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
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
   * @property {boolean} [deferCommit] - Keep a local drag value during the gesture; throttled preview while dragging; commit on release.
   * @property {() => void} [onGestureStart] - Called at drag start when {@link deferCommit} is true.
   * @property {(value: number) => void} [onValuePreview] - Throttled preview while dragging (100ms).
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
    onValueChange = () => {},
  } = $props();

  const boxedControlBaseClasses =
    "mp-control-gradient flex h-8 items-center justify-center rounded-md border px-2 text-sm font-semibold tabular-nums transition-[border-color,box-shadow,filter] duration-75";

  const pixelsPerStep = 4;
  const previewThrottleMs = 100;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;
  let dragValue = $state(0);
  let previewTimerId = 0;
  let lastPreviewAt = 0;
  /** @type {number | null} */
  let pendingPreviewValue = null;

  let displayedValue = $derived(dragging ? dragValue : value);
  let displayValue = $derived(
    formatValue !== undefined
      ? formatValue(displayedValue)
      : String(Math.round(displayedValue + displayAdd)),
  );
  let ariaValueMin = $derived(min + displayAdd);
  let ariaValueMax = $derived(max + displayAdd);
  let ariaValueNow = $derived(displayedValue + displayAdd);

  /** @param {number} next */
  function clampValue(next) {
    return Math.min(max, Math.max(min, Math.round(next)));
  }

  /** @param {number} clientY */
  function valueFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampValue(dragStartValue + steps);
  }

  function cancelPreviewThrottle() {
    if (!previewTimerId) return;

    clearTimeout(previewTimerId);
    previewTimerId = 0;
  }

  /** @param {number} next */
  function flushPreview(next) {
    cancelPreviewThrottle();
    pendingPreviewValue = null;
    lastPreviewAt = Date.now();
    onValuePreview?.(next);
  }

  /** @param {number} next */
  function scheduleThrottledPreview(next) {
    if (!onValuePreview) return;

    pendingPreviewValue = next;

    const elapsed = Date.now() - lastPreviewAt;

    if (elapsed >= previewThrottleMs) {
      flushPreview(next);
      return;
    }

    if (previewTimerId) return;

    previewTimerId = window.setTimeout(() => {
      previewTimerId = 0;
      const pending = pendingPreviewValue;

      if (pending !== null) flushPreview(pending);
    }, previewThrottleMs - elapsed);
  }

  /** @param {number} next */
  function commitDeferredValue(next) {
    onGestureStart?.();
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

    absorbPointerDragFocus(event);
    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;
    dragValue = value;
    pendingPreviewValue = null;
    lastPreviewAt = 0;

    if (deferCommit) {
      onGestureStart?.();
    }
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging || disabled) return;

    const next = valueFromDrag(event.clientY);

    if (next === dragValue) return;

    dragValue = next;

    if (deferCommit && onValueCommit) {
      scheduleThrottledPreview(next);
      return;
    }

    if (next !== value) onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    if (!dragging) return;

    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    cancelPreviewThrottle();
    pendingPreviewValue = null;

    if (deferCommit && onValueCommit) {
      onValueCommit(dragValue);
    }

    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (disabled || resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) applyValue(resetValue);
  }

  onDestroy(() => {
    cancelPreviewThrottle();
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
  tabindex="-1"
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

      if (displayedValue < max) applyValue(displayedValue + 1);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (displayedValue > min) applyValue(displayedValue - 1);
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
