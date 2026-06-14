<script>
  import StepSkipIcon from "./StepSkipIcon.svelte";
  import {
    emeraldRowAccent,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";

  
  
  
  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {boolean} [value]
   * @property {string} [ariaLabel]
   * @property {string} [buttonClass]
   * @property {string} [iconClass]
   * @property {string | undefined} [style]
   * @property {(value: boolean) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    accent = emeraldRowAccent,
    muted = false,
    value = false,
    ariaLabel = "Skip step in sequence",
    buttonClass = "",
    iconClass = "pointer-events-none h-3 w-3",
    style = undefined,
    onValueChange = () => {}
  } = $props();

  function toggle() {
    onValueChange(!value);
  }

  let iconClasses = $derived(muted
    ? "text-text-faint"
    : value
      ? toggleIconActiveClasses
      : toggleIconRestClasses);

  let resolvedButtonClass =
    $derived(buttonClass ||
    `flex h-6 w-6 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors ${accent.ringFocusWithWidth}`);
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
  <StepSkipIcon class={iconClass} />
</button>
