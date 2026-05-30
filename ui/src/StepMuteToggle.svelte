<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let muted = false;
  export let value = false;
  export let ariaLabel = "Mute step";
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
    ? 'border-zinc-500 bg-zinc-950/90 text-zinc-200'
    : muted
      ? 'border-zinc-800 bg-zinc-900/80 text-zinc-500'
      : `border-zinc-700 bg-zinc-800/50 text-zinc-400 hover:border-zinc-600 hover:text-zinc-200`}"
  onpointerdown={(event) => event.stopPropagation()}
  onclick={toggle}
>
  <span class="font-sans text-[9px] font-semibold tracking-wide uppercase">Mute</span>
  <span
    class="font-sans text-[9px] font-bold tabular-nums {value ? accent.textAccentLight : 'text-zinc-500'}"
  >
    {value ? "On" : "Off"}
  </span>
</button>
