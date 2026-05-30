<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let muted = false;
  export let value;
  export let min = 0;
  export let max = 127;
  export let formatValue = undefined;
  /** Value restored on double-click; omit to disable reset. */
  export let resetValue = undefined;
  export let ariaLabel = "Value";
  export let disabled = false;
  /** @type {(value: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  const pixelsPerStep = 4;

  let dragging = false;
  let dragStartY = 0;
  let dragStartValue = 0;

  $: displayValue =
    formatValue !== undefined ? formatValue(value) : String(Math.round(value));

  function clampValue(next) {
    return Math.min(max, Math.max(min, Math.round(next)));
  }

  function valueFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampValue(dragStartValue + steps);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    if (disabled) return;

    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging || disabled) return;

    const next = valueFromDrag(event.clientY);

    if (next !== value) onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (disabled || resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) onValueChange(resetValue);
  }
</script>

<div
  class="inline-flex touch-none select-none items-center rounded-sm outline-none {disabled
    ? 'cursor-default opacity-50'
    : 'cursor-ns-resize'} {accent.ringFocusWithWidth} {muted
    ? 'text-zinc-500'
    : dragging
      ? accent.textAccentLight
      : 'text-zinc-100'}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={min}
  aria-valuemax={max}
  aria-valuenow={value}
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

      if (value < max) onValueChange(value + 1);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > min) onValueChange(value - 1);
    }
  }}
>
  <span class="font-sans text-xs leading-none font-bold tabular-nums">{displayValue}</span>
</div>
