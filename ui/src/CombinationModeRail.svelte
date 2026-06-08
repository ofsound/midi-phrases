<script>
  import { combinationModes } from "./phraseSchedule.js";

  /**
   * Full-width hardware-style rail for W / L / X / E combination modes.
   *
   * @typedef {Object} Props
   * @property {number} mask
   * @property {(modeIndex: number) => void | Promise<void>} onToggle
   */

  /** @type {Props} */
  let { mask = 0, onToggle = () => {} } = $props();
</script>

<div
  class="combination-mode-rail -mx-6 flex min-h-[4.25rem] shrink-0 items-center justify-center border-t border-b border-zinc-900/80 px-6 py-2.5"
>
  <div class="flex items-center" role="group" aria-label="Combination modes">
    {#each combinationModes as mode, index (mode.index)}
      {#if index > 0}
        <div class="combination-mode-connector" aria-hidden="true"></div>
      {/if}
      <button
        type="button"
        class="combination-mode-button"
        aria-label={`Toggle ${mode.name} mode`}
        aria-pressed={(mask & mode.bit) !== 0}
        title={mode.name}
        data-cursor="pointer"
        onclick={() => onToggle(mode.index)}
      >
        <span class="combination-mode-button-face">{mode.label}</span>
      </button>
    {/each}
    <div class="combination-mode-connector combination-mode-connector-tail" aria-hidden="true"></div>
    <span class="pl-1 text-[0.65rem] font-semibold uppercase tracking-[0.28em] text-zinc-400/90">
      Combine
    </span>
  </div>
</div>
