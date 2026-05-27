<script>
  export let value;
  export let label = "";
  export let fullWidth = false;
  export let min = 0;
  export let max = 127;
  export let ariaLabel = "Slider";
  /** @type {(value: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  /** @type {HTMLDivElement | null} */
  let trackEl = null;
  let dragging = false;

  $: range = Math.max(0, max - min);
  $: thumbPercent = range > 0 ? ((value - min) / range) * 100 : 0;
  $: displayValue = String(Math.round(value));

  function valueFromClientX(clientX) {
    if (!trackEl) return value;

    const rect = trackEl.getBoundingClientRect();
    const ratio = (clientX - rect.left) / rect.width;

    return Math.min(max, Math.max(min, Math.round(ratio * range + min)));
  }

  function updateFromClientX(clientX) {
    const next = valueFromClientX(clientX);

    if (next === value) return;

    onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    trackEl?.setPointerCapture(event.pointerId);
    dragging = true;
    updateFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    updateFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    trackEl?.releasePointerCapture(event.pointerId);
  }
</script>

<div class="flex flex-col gap-1 {fullWidth ? 'min-w-0 w-full' : 'w-[10.5rem]'}">
  <div class="flex items-center justify-between gap-2">
    {#if label}
      <span class="text-[9px] font-medium uppercase tracking-wide text-zinc-500">{label}</span>
    {:else}
      <span></span>
    {/if}
    <span class="font-mono text-[10px] tabular-nums text-emerald-300">{displayValue}</span>
  </div>

  <div
    bind:this={trackEl}
    class="relative h-5 cursor-pointer touch-none select-none"
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
      if (event.key === "ArrowLeft" || event.key === "ArrowDown") {
        event.preventDefault();

        if (value > min) onValueChange(value - 1);
      } else if (event.key === "ArrowRight" || event.key === "ArrowUp") {
        event.preventDefault();

        if (value < max) onValueChange(value + 1);
      }
    }}
  >
    <div class="absolute inset-x-0 top-1/2 h-0.5 -translate-y-1/2 rounded-full bg-zinc-700">
      <div
        class="h-full rounded-full bg-emerald-500 {dragging ? '' : 'transition-[width] duration-75'}"
        style:width="{thumbPercent}%"
      ></div>
    </div>

    <div
      class="absolute top-1/2 h-3 w-3 -translate-x-1/2 -translate-y-1/2 rounded-full border border-zinc-500 bg-zinc-100 shadow-sm {dragging
        ? ''
        : 'transition-[left] duration-75'}"
      style:left="{thumbPercent}%"
    ></div>
  </div>
</div>
