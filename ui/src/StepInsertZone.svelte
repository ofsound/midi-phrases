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
   * @property {() => void | Promise<void>} [onDuplicate]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {{ index: number, label: string }[]} [timingMultiplierOptions]
   */

  /** @type {Props} */
  let {
    onInsert = undefined,
    onDuplicate = undefined,
    accent = emeraldRowAccent,
    muted = false,
    timingMultiplierOptions: multiplierOptions = timingMultiplierOptions,
  } = $props();

  let insertMultiplierOptions = $derived(insertStepTimingMultiplierOptions(multiplierOptions));
</script>

<div
  class="group relative flex h-full w-full flex-col items-center justify-center"
  role="group"
  aria-label="Step insert controls"
>
  <div class="relative z-10 flex shrink-0 flex-col items-center justify-center gap-4">
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

    {#if onDuplicate}
      <button
        type="button"
        aria-label="Duplicate step to the left"
        data-cursor="pointer"
        class="flex h-4 w-4 shrink-0 items-center justify-center border-0 bg-transparent p-0 opacity-0 outline-none transition-opacity group-hover:opacity-100 group-focus-within:opacity-100 {muted
          ? 'text-text-muted'
          : accent.textAccent} {accent.insertZoneFocus}"
        onclick={onDuplicate}
      >
        <svg
          viewBox="0 0 16 16"
          class="h-4 w-4"
          fill="none"
          stroke="currentColor"
          stroke-width="1.5"
          stroke-linejoin="round"
          aria-hidden="true"
        >
          <rect x="5.5" y="2.5" width="7" height="10" rx="0.75" />
          <path d="M5.5 4.5h-2a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h7" />
          <path d="M5.5 2.5v2h2" />
        </svg>
      </button>
    {/if}
  </div>
</div>
