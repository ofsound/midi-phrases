<script>
  import SpeakerIcon from "./SpeakerIcon.svelte";
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

  $: iconClasses = value
    ? accent.textAccent
    : muted
      ? "text-zinc-600"
      : "text-zinc-500 hover:text-zinc-300";
</script>

<button
  type="button"
  data-step-pointer
  aria-label={ariaLabel}
  aria-pressed={value}
  style="cursor: pointer"
  class="flex h-6 w-6 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors {accent.ringFocusWithWidth} {iconClasses}"
  onpointerdown={(event) => event.stopPropagation()}
  onclick={toggle}
>
  <SpeakerIcon class="pointer-events-none h-3 w-3" />
</button>
