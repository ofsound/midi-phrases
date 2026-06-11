<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  
  
  
  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {number} [value]
   * @property {number} [min]
   * @property {number} [max]
   * @property {boolean} [muted]
   * @property {any} [resetValue] - Channel restored on double-click; omit to disable reset.
   * @property {string} [ariaLabel]
   * @property {(channel: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    accent = emeraldRowAccent,
    value = 1,
    min = 1,
    max = 16,
    muted = false,
    resetValue = undefined,
    ariaLabel = "MIDI channel",
    onValueChange = () => {}
  } = $props();

  const pixelsPerStep = 10;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;

  let displayValue = $derived(String(value));

  function clampChannel(channel) {
    return Math.min(max, Math.max(min, Math.round(channel)));
  }

  function channelFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampChannel(dragStartValue + steps);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    absorbPointerDragFocus(event);
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
    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) onValueChange(resetValue);
  }
</script>

<div
  data-cursor="vertical-drag"
  class="inline-flex touch-none select-none items-center rounded-sm outline-none {accent.ringFocusWithWidth} {muted
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

      if (value < max) onValueChange(value + 1);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > min) onValueChange(value - 1);
    }
  }}
>
  <span class="font-sans text-base leading-none font-bold tabular-nums">{displayValue}</span>
</div>
