<script>
  import RowDisableIcon from "./RowDisableIcon.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let muted = false;
  export let value = false;
  export let ariaLabel = "Skip step in sequence";
  export let buttonClass = "";
  export let iconClass = "pointer-events-none h-3 w-3";
  /** @type {(value: boolean) => void | Promise<void>} */
  export let onValueChange = () => {};

  function toggle() {
    onValueChange(!value);
  }

  $: iconClasses = value
    ? "text-zinc-500 hover:text-zinc-300"
    : muted
      ? "text-zinc-600"
      : accent.textAccent;

  $: resolvedButtonClass =
    buttonClass ||
    `flex h-6 w-6 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors ${accent.ringFocusWithWidth}`;
</script>

<button
  type="button"
  data-step-pointer
  aria-label={ariaLabel}
  aria-pressed={value}
  style="cursor: pointer"
  class="{resolvedButtonClass} {iconClasses}"
  onpointerdown={(event) => event.stopPropagation()}
  onclick={toggle}
>
  <RowDisableIcon class={iconClass} />
</button>
