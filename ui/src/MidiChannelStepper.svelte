<script>
  export let value = 1;
  export let min = 1;
  export let max = 16;
  export let muted = false;
  export let ariaLabel = "MIDI channel";
  /** @type {(channel: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  function decrement() {
    if (value <= min) return;

    onValueChange(value - 1);
  }

  function increment() {
    if (value >= max) return;

    onValueChange(value + 1);
  }
</script>

<div
  class="flex h-9 items-stretch overflow-hidden rounded-lg border bg-zinc-900 transition-[border-color,opacity] duration-200 {muted
    ? 'border-zinc-800/90 opacity-75'
    : 'border-zinc-700'}"
  role="group"
  aria-label={ariaLabel}
>
  <button
    type="button"
    class="flex w-7 shrink-0 items-center justify-center text-sm text-zinc-400 transition-colors outline-none hover:bg-zinc-800 hover:text-zinc-200 focus-visible:bg-zinc-800 disabled:cursor-default disabled:opacity-30 disabled:hover:bg-transparent disabled:hover:text-zinc-400"
    aria-label="Decrease MIDI channel"
    disabled={value <= min}
    onclick={decrement}
  >
    −
  </button>
  <div
    class="flex min-w-[1.75rem] items-center justify-center border-x border-zinc-700/80 px-1 text-sm font-semibold tabular-nums {muted
      ? 'text-zinc-500'
      : 'text-zinc-100'}"
    aria-live="polite"
  >
    {value}
  </div>
  <button
    type="button"
    class="flex w-7 shrink-0 items-center justify-center text-sm text-zinc-400 transition-colors outline-none hover:bg-zinc-800 hover:text-zinc-200 focus-visible:bg-zinc-800 disabled:cursor-default disabled:opacity-30 disabled:hover:bg-transparent disabled:hover:text-zinc-400"
    aria-label="Increase MIDI channel"
    disabled={value >= max}
    onclick={increment}
  >
    +
  </button>
</div>
