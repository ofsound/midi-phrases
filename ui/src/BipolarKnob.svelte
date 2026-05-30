<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {{ index: number, label: string }[]} */
  export let options;
  export let value;
  /** Option index restored on double-click; omit to disable reset. */
  export let resetValue = undefined;
  export let label = "";
  export let muted = false;
  export let ariaLabel = "Bipolar knob";
  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  /** @type {(index: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  const pixelsPerStep = 10;
  const minAngleDeg = -135;
  const maxAngleDeg = 135;

  let dragging = false;
  let dragStartY = 0;
  let dragStartValue = 0;

  $: maxIndex = Math.max(0, options.length - 1);
  $: valuePosition = Math.max(0, options.findIndex((option) => option.index === value));
  $: currentLabel = options.find((option) => option.index === value)?.label ?? "";
  $: indicatorAngleDeg =
    maxIndex > 0 ? minAngleDeg + (valuePosition / maxIndex) * (maxAngleDeg - minAngleDeg) : 0;

  /** @param {number} position */
  function indexAtPosition(position) {
    const clamped = Math.min(maxIndex, Math.max(0, position));

    return options[clamped]?.index ?? value;
  }

  /** @param {number} clientY */
  function indexFromDrag(clientY) {
    const startPosition = options.findIndex((option) => option.index === dragStartValue);
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return indexAtPosition(startPosition + steps);
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

    const next = indexFromDrag(event.clientY);

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

<div class="flex flex-col items-center gap-1 transition-opacity duration-200 {muted ? 'opacity-75' : ''}">
  {#if label}
    <span class="text-[9px] font-medium uppercase tracking-wide {muted ? 'text-zinc-600' : 'text-zinc-500'}"
      >{label}</span
    >
  {/if}

  <div
    class="relative h-10 w-10 cursor-ns-resize touch-none select-none rounded-full border-2 bg-zinc-900 outline-none transition-[border-color,box-shadow] duration-75 {accent.borderFocusVisible} focus-visible:ring-1 {accent.ringFocus} {dragging && !muted
      ? `${accent.dragBorder} ${accent.dragShadow}`
      : muted
        ? 'border-zinc-800'
        : 'border-zinc-600'}"
    role="slider"
    aria-label={ariaLabel}
    aria-valuemin={options[0]?.index}
    aria-valuemax={options[options.length - 1]?.index}
    aria-valuenow={value}
    aria-valuetext={currentLabel}
    tabindex="0"
    onpointerdown={onPointerDown}
    onpointermove={onPointerMove}
    onpointerup={onPointerUp}
    onpointercancel={onPointerUp}
    ondblclick={onDoubleClick}
    title={resetValue !== undefined ? "Drag to change · double-click to reset" : undefined}
    onkeydown={(event) => {
      if (event.key === "ArrowUp" || event.key === "ArrowRight") {
        event.preventDefault();

        if (valuePosition < maxIndex) onValueChange(options[valuePosition + 1].index);
      } else if (event.key === "ArrowDown" || event.key === "ArrowLeft") {
        event.preventDefault();

        if (valuePosition > 0) onValueChange(options[valuePosition - 1].index);
      }
    }}
  >
    <div
      class="pointer-events-none absolute inset-0 rounded-full transition-transform duration-75"
      style:transform="rotate({indicatorAngleDeg}deg)"
      aria-hidden="true"
    >
      <span
        class="absolute left-1/2 top-[2px] h-[3px] w-[3px] -translate-x-1/2 rounded-[1px] {muted
          ? 'bg-zinc-600'
          : accent.bgAccent}"
      ></span>
    </div>

    <span
      class="pointer-events-none absolute inset-0 flex items-center justify-center font-mono text-[10px] leading-none font-semibold tabular-nums {muted
        ? 'text-zinc-500'
        : dragging
          ? accent.textAccentLight
          : 'text-zinc-100'}"
      aria-hidden="true"
    >
      {currentLabel}
    </span>
  </div>
</div>
