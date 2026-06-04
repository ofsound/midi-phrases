<script>
  import StepMuteIcon from "./StepMuteIcon.svelte";
  import {
    emeraldRowAccent,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let muted = false;
  export let value = false;
  export let ariaLabel = "Mute step";
  export let buttonClass = "";
  export let iconClass = "pointer-events-none h-3 w-3";
  /** @type {string | undefined} */
  export let style = undefined;
  /** @type {(value: boolean) => void | Promise<void>} */
  export let onValueChange = () => {};

  function toggle() {
    onValueChange(!value);
  }

  $: iconClasses = muted
    ? "text-zinc-600"
    : value
      ? toggleIconActiveClasses
      : toggleIconRestClasses;

  $: resolvedButtonClass =
    buttonClass ||
    `flex h-6 w-6 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors ${accent.ringFocusWithWidth}`;
</script>

<button
  type="button"
  data-cursor="pointer"
  aria-label={ariaLabel}
  aria-pressed={value}
  style={style}
  class="{resolvedButtonClass} {iconClasses}"
  onpointerdown={(event) => event.stopPropagation()}
  onclick={toggle}
>
  <StepMuteIcon class={iconClass} />
</button>
