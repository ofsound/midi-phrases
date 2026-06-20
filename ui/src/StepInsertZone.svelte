<script>
  import PlusDragButton from "./PlusDragButton.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    defaultStepTimingMultiplierIndex,
    insertStepTimingMultiplierOptions,
    timingMultiplierOptions,
  } from "./stepCellLayout.js";

  /**
   * @typedef {Object} Props
   * @property {(multiplierIndex: number) => void | Promise<void>} [onInsert]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {{ index: number, label: string }[]} [timingMultiplierOptions]
   */

  /** @type {Props} */
  let {
    onInsert = undefined,
    accent = emeraldRowAccent,
    muted = false,
    timingMultiplierOptions: multiplierOptions = timingMultiplierOptions,
  } = $props();

  let insertMultiplierOptions = $derived(insertStepTimingMultiplierOptions(multiplierOptions));
</script>

<div
  class="group relative flex h-full w-full items-start justify-center pointer-events-none"
  role="group"
  aria-label="Step insert controls"
>
  <div class="pointer-events-auto relative z-10 flex shrink-0 -translate-y-1/2 flex-col items-center">
    {#if onInsert}
      <PlusDragButton
        {accent}
        {muted}
        ariaLabel="Insert step"
        options={insertMultiplierOptions}
        defaultIndex={defaultStepTimingMultiplierIndex}
        onConfirm={onInsert}
      />
    {/if}
  </div>
</div>
