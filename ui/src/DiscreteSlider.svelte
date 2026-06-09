<script>
  
  
  /**
   * @typedef {Object} Props
   * @property {{ index: number, label: string }[]} options
   * @property {any} value
   * @property {string} [label]
   * @property {boolean} [fullWidth]
   * @property {string} [ariaLabel]
   * @property {(index: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    options,
    value,
    label = "",
    fullWidth = false,
    ariaLabel = "Discrete slider",
    onValueChange = () => {}
  } = $props();

  /** @type {HTMLDivElement | null} */
  let trackEl = $state(null);
  let dragging = false;

  let maxIndex = $derived(Math.max(0, options.length - 1));
  let valuePosition = $derived(Math.max(0, options.findIndex((option) => option.index === value)));
  let thumbPercent = $derived(maxIndex > 0 ? (valuePosition / maxIndex) * 100 : 0);
  let currentLabel = $derived(options.find((option) => option.index === value)?.label ?? "");

  /** @param {HTMLDivElement} node */
  function trackAttachment(node) {
    trackEl = node;

    return () => {
      if (trackEl === node) {
        trackEl = null;
      }
    };
  }

  function indexFromClientX(clientX) {
    if (!trackEl) return value;

    const rect = trackEl.getBoundingClientRect();
    const ratio = (clientX - rect.left) / rect.width;
    const position = Math.min(maxIndex, Math.max(0, Math.round(ratio * maxIndex)));

    return options[position]?.index ?? value;
  }

  function updateFromClientX(clientX) {
    const next = indexFromClientX(clientX);

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
    <span class="font-mono text-[10px] tabular-nums text-accent-300">{currentLabel}</span>
  </div>

  <div
    {@attach trackAttachment}
    data-cursor="pointer"
    class="relative h-5 touch-none select-none"
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
      const currentPosition = options.findIndex((option) => option.index === value);

      if (event.key === "ArrowLeft" || event.key === "ArrowDown") {
        event.preventDefault();

        if (currentPosition > 0) onValueChange(options[currentPosition - 1].index);
      } else if (event.key === "ArrowRight" || event.key === "ArrowUp") {
        event.preventDefault();

        if (currentPosition < maxIndex) onValueChange(options[currentPosition + 1].index);
      }
    }}
  >
    <div class="absolute inset-x-0 top-1/2 h-0.5 -translate-y-1/2 rounded-full bg-zinc-700">
      <div class="h-full rounded-full bg-accent-500 transition-[width] duration-75" style:width="{thumbPercent}%"></div>
    </div>

    <div
      class="absolute top-1/2 h-3 w-3 -translate-x-1/2 -translate-y-1/2 rounded-full border border-zinc-500 bg-zinc-100 shadow-sm transition-[left] duration-75"
      style:left="{thumbPercent}%"
    ></div>
  </div>

  <div class="relative h-3">
    {#each options as option, optionIndex (option.index)}
      <span
        class="absolute -translate-x-1/2 font-mono text-[9px] leading-none {value === option.index
          ? 'text-accent-300'
          : 'text-zinc-500'}"
        style:left="{(optionIndex / maxIndex) * 100}%"
      >
        {option.label}
      </span>
    {/each}
  </div>
</div>
