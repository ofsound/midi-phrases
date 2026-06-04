<script>
  import { tick } from "svelte";
  import ColorsPaletteIcon from "./ColorsPaletteIcon.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  let {
    enabled = false,
    accent = emeraldRowAccent,
    onChange = () => {},
  } = $props();

  /** Overridden optimistically so the toggle paints before App's grid re-render. */
  let visualOn = $derived(enabled);

  async function handleClick() {
    const next = !visualOn;
    visualOn = next;
    await tick();
    onChange(next);
  }
</script>

<button
  type="button"
  aria-label={visualOn ? "Disable row colors" : "Enable row colors"}
  aria-pressed={visualOn}
  data-cursor="pointer"
  class="flex h-5 w-5 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors {accent.controlFocus} {visualOn
    ? 'text-white'
    : 'text-zinc-500'}"
  onclick={handleClick}
>
  <ColorsPaletteIcon class="pointer-events-none h-4 w-4" />
</button>
