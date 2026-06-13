<script>
  import { onDestroy } from "svelte";
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";

  /**
   * Vertical drag field for shimmer rail parameters.
   * Updates parent preview state while dragging; commits on release.
   *
   * @typedef {Object} Props
   * @property {number} value
   * @property {number} min
   * @property {number} max
   * @property {number} defaultValue
   * @property {(value: number) => string} formatValue
   * @property {(value: number) => number} clampValue
   * @property {boolean} [active]
   * @property {string} [ariaLabel]
   * @property {string} [title]
   * @property {() => void} [onGestureStart]
   * @property {(value: number) => void} [onValuePreview]
   * @property {(value: number) => void | Promise<void>} [onValueCommit]
   */

  /** @type {Props} */
  let {
    value,
    min,
    max,
    defaultValue,
    formatValue,
    clampValue,
    active = false,
    ariaLabel = "Shimmer parameter",
    title = "Drag vertically to adjust · double-click to reset",
    onGestureStart = undefined,
    onValuePreview = undefined,
    onValueCommit = () => {},
  } = $props();

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
  let displayValue = $derived(formatValue(displayedValue));
  let deferCommit = $derived(Boolean(onValuePreview));

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
  function commitValue(next) {
    const clamped = clampValue(next);

    if (deferCommit) {
      onValueCommit(clamped);
      return;
    }

    if (clamped !== value) onValueCommit(clamped);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    absorbPointerDragFocus(event);
    event.stopPropagation();
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
    if (!dragging) return;

    const next = valueFromDrag(event.clientY);

    if (next === dragValue) return;

    dragValue = next;

    if (deferCommit) {
      scheduleThrottledPreview(next);
    }
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    if (!dragging) return;

    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    cancelPreviewThrottle();
    pendingPreviewValue = null;
    commitValue(dragValue);
    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (dragging) return;

    event.preventDefault();

    if (value !== defaultValue) {
      commitValue(defaultValue);
    }
  }

  onDestroy(() => {
    cancelPreviewThrottle();
  });
</script>

<div
  data-cursor="vertical-drag"
  class={`shimmer-param ${active ? "shimmer-param-active" : ""} ${dragging ? "shimmer-param-dragging" : ""}`}
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={min}
  aria-valuemax={max}
  aria-valuenow={displayedValue}
  aria-valuetext={displayValue}
  tabindex="-1"
  {title}
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  onkeydown={(event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (displayedValue < max) commitValue(clampValue(displayedValue + 1));
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (displayedValue > min) commitValue(clampValue(displayedValue - 1));
    }
  }}
>
  <span class="shimmer-param-value" aria-hidden="true">{displayValue}</span>
</div>
