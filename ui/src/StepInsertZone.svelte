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
  <span
    class="pointer-events-none min-h-0 w-0.5 flex-1 opacity-0 transition-opacity group-hover:opacity-100 group-focus-within:opacity-100 {muted
      ? 'bg-zinc-600'
      : accent.bgAccent}"
    aria-hidden="true"
  ></span>

  <div class="relative z-10 flex shrink-0 flex-col items-center justify-center gap-2">
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
        class="flex h-3.5 w-3.5 shrink-0 items-center justify-center border-0 bg-transparent p-0 opacity-0 outline-none transition-opacity group-hover:opacity-100 group-focus-within:opacity-100 {muted
          ? 'text-zinc-500'
          : accent.textAccent} {accent.insertZoneFocus}"
        onclick={onDuplicate}
      >
        <svg
          viewBox="0 0 16 16"
          class="h-3 w-3"
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

  <span
    class="pointer-events-none min-h-0 w-0.5 flex-1 opacity-0 transition-opacity group-hover:opacity-100 group-focus-within:opacity-100 {muted
      ? 'bg-zinc-600'
      : accent.bgAccent}"
    aria-hidden="true"
  ></span>
</div>
