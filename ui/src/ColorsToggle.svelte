<script>
  import { tick } from "svelte";
  import {
    emeraldRowAccent,
    rowReverseControlClasses,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";

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
  class="shrink-0 {rowReverseControlClasses} !transition-none {accent.controlFocus} {visualOn
    ? `border-zinc-600 ${toggleIconActiveClasses}`
    : `border-zinc-700 ${toggleIconRestClasses}`}"
  onclick={handleClick}
>
  Colors
</button>
