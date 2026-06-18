<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  
  
  
  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {any} value
   * @property {number} [min]
   * @property {number} [max]
   * @property {any} [resetValue] - Velocity restored on double-click; omit to disable reset.
   * @property {string} [ariaLabel]
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
    resetValue = undefined,
    ariaLabel = "Velocity",
    deferCommit = false,
    onGestureStart = undefined,
    onValuePreview = undefined,
    onValueCommit = undefined,
    onValueChange = () => {}
  } = $props();

  const pixelsPerStep = 4;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;

  let displayValue = $derived(String(Math.round(value)));

  function clampVelocity(velocity) {
    return Math.min(max, Math.max(min, Math.round(velocity)));
  }

  function velocityFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampVelocity(dragStartValue + steps);
  }

  /** @param {number} next */
  function schedulePreview(next) {
    if (next === value) return;

    if (!deferCommit || !onValuePreview) {
      onValueChange(next);
      return;
    }

    onValuePreview(next);
  }

  /** @param {number} next */
  function applyValue(next) {
    if (deferCommit && onValueCommit) {
      onGestureStart?.();
      onValuePreview?.(next);
      onValueCommit(next);
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

    schedulePreview(velocityFromDrag(event.clientY));
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    if (deferCommit && onValueCommit) {
      const finalValue = velocityFromDrag(event.clientY);
      onValueCommit(finalValue);
    }

    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) applyValue(resetValue);
  }
</script>

<div
  data-step-velocity
  data-cursor="vertical-drag"
  class="inline-flex touch-none select-none items-center rounded-sm outline-none {accent.ringFocusWithWidth} {muted
    ? 'text-text-muted'
    : dragging
      ? accent.textAccentLight
      : 'text-text'}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={min}
  aria-valuemax={max}
  aria-valuenow={value}
  aria-valuetext={displayValue}
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

      if (value < max) applyValue(value + 1);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > min) applyValue(value - 1);
    }
  }}
>
  <span class="font-sans text-[0.625rem] leading-none font-bold tabular-nums">{displayValue}</span>
</div>
