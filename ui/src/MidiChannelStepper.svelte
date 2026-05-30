<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let value = 1;
  export let min = 1;
  export let max = 16;
  export let muted = false;
  /** Channel restored on double-click; omit to disable reset. */
  export let resetValue = undefined;
  export let ariaLabel = "MIDI channel";
  /** @type {(channel: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  const pixelsPerStep = 10;

  let dragging = false;
  let dragStartY = 0;
  let dragStartValue = 0;

  $: displayValue = String(value);

  function clampChannel(channel) {
    return Math.min(max, Math.max(min, Math.round(channel)));
  }

  function channelFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampChannel(dragStartValue + steps);
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

    const next = channelFromDrag(event.clientY);

    if (next !== value) onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) onValueChange(resetValue);
  }
</script>

<div
  class="inline-flex cursor-ns-resize touch-none select-none items-center rounded-sm outline-none {accent.ringFocusWithWidth} {muted
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
  tabindex="0"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  title={resetValue !== undefined ? "Drag to change · double-click to reset" : undefined}
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
  <span class="font-sans text-base leading-none font-bold tabular-nums">{displayValue}</span>
</div>
