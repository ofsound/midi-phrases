<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let muted = false;
  export let value = false;
  export let ariaLabel = "Skip step in sequence";
  /** @type {(value: boolean) => void | Promise<void>} */
  export let onValueChange = () => {};

  function toggle() {
    onValueChange(!value);
  }
</script>

<button
  type="button"
  aria-label={ariaLabel}
  aria-pressed={value}
  class="flex h-6 w-full items-center justify-between gap-2 rounded-sm border px-1.5 text-left outline-none transition-colors {accent.ringFocusWithWidth} {value
    ? 'border-slate-400/70 bg-slate-900/90 text-slate-100'
    : muted
      ? 'border-zinc-800 bg-zinc-900/80 text-zinc-500'
      : 'border-zinc-700 bg-zinc-800/50 text-zinc-400 hover:border-zinc-600 hover:text-zinc-200'}"
  onpointerdown={(event) => event.stopPropagation()}
  onclick={toggle}
>
  <span class="font-sans text-[9px] font-semibold tracking-wide uppercase">Skip</span>
  <span
    class="font-sans text-[9px] font-bold tabular-nums {value ? 'text-slate-200' : 'text-zinc-500'}"
  >
    {value ? "On" : "Off"}
  </span>
</button>
