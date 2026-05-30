<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let value;
  export let min = 0;
  export let max = 127;
  export let ariaLabel = "Velocity";
  /** @type {(value: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  const pixelsPerStep = 4;

  let dragging = false;
  let dragStartY = 0;
  let dragStartValue = 0;

  $: displayValue = String(Math.round(value));

  function clampVelocity(velocity) {
    return Math.min(max, Math.max(min, Math.round(velocity)));
  }

  function velocityFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampVelocity(dragStartValue + steps);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    const next = velocityFromDrag(event.clientY);

    if (next !== value) onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
  }
</script>

<div
  class="inline-flex cursor-ns-resize touch-none select-none items-center rounded-sm outline-none {accent.ringFocusWithWidth} {dragging
    ? accent.textAccentLight
    : 'text-zinc-100'}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={min}
  aria-valuemax={max}
  aria-valuenow={value}
  aria-valuetext={displayValue}
  tabindex="0"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  onkeydown={(event) => {
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
