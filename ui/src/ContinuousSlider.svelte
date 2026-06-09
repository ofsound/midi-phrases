<script>
  
  /**
   * @typedef {Object} Props
   * @property {any} value
   * @property {string} [label]
   * @property {boolean} [fullWidth]
   * @property {number} [min]
   * @property {number} [max]
   * @property {string} [ariaLabel]
   * @property {(value: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    value,
    label = "",
    fullWidth = false,
    min = 0,
    max = 127,
    ariaLabel = "Slider",
    onValueChange = () => {}
  } = $props();

  /** @type {HTMLDivElement | null} */
  let trackEl = $state(null);
  let dragging = $state(false);

  let range = $derived(Math.max(0, max - min));
  let thumbPercent = $derived(range > 0 ? ((value - min) / range) * 100 : 0);
  let displayValue = $derived(String(Math.round(value)));

  /** @param {HTMLDivElement} node */
  function trackAttachment(node) {
    trackEl = node;

    return () => {
      if (trackEl === node) {
        trackEl = null;
      }
    };
  }

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
    <span class="font-mono text-[10px] tabular-nums text-accent-300">{displayValue}</span>
  </div>

  <div
    {@attach trackAttachment}
    data-cursor="pointer"
    class="relative h-5 touch-none select-none"
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
        class="h-full rounded-full bg-accent-500 {dragging ? '' : 'transition-[width] duration-75'}"
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
