<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let value;
  export let ariaLabel = "Note";
  /** @type {(value: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  const pixelsPerStep = 10;

  let dragging = false;
  let dragStartY = 0;
  let dragStartValue = 0;

  $: displayName = midiToNoteName(value);

  function clampMidi(note) {
    return Math.min(127, Math.max(0, Math.round(note)));
  }

  function noteFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampMidi(dragStartValue + steps);
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

    const next = noteFromDrag(event.clientY);

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
  aria-valuemin={0}
  aria-valuemax={127}
  aria-valuenow={value}
  aria-valuetext={displayName}
  tabindex="0"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  onkeydown={(event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (value < 127) onValueChange(value + 1);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > 0) onValueChange(value - 1);
    }
  }}
>
  <span class="font-sans text-base leading-none font-bold tabular-nums">{displayName}</span>
</div>
