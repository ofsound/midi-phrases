<script>
  /** @type {{ index: number, label: string }[]} */
  export let options;
  export let value;
  export let label = "";
  export let ariaLabel = "Bipolar knob";
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
</script>

<div class="flex flex-col items-center gap-1">
  {#if label}
    <span class="text-[9px] font-medium uppercase tracking-wide text-zinc-500">{label}</span>
  {/if}

  <div
    class="relative h-10 w-10 cursor-ns-resize touch-none select-none rounded-full border-2 bg-zinc-900 outline-none transition-[border-color,box-shadow] duration-75 focus-visible:border-emerald-500 focus-visible:ring-1 focus-visible:ring-emerald-400 {dragging
      ? 'border-emerald-400 shadow-[0_0_10px_rgba(52,211,153,0.35)]'
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
      <span class="absolute left-1/2 top-1 h-2 w-0.5 -translate-x-1/2 rounded-full bg-emerald-400"></span>
    </div>

    <span
      class="pointer-events-none absolute inset-0 flex items-center justify-center font-mono text-[10px] leading-none font-semibold tabular-nums {dragging
        ? 'text-emerald-300'
        : 'text-zinc-100'}"
      aria-hidden="true"
    >
      {currentLabel}
    </span>
  </div>
</div>
