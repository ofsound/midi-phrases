<script>
  import CombinationModeIcon from "./CombinationModeIcon.svelte";
  import NoteBandpassSlider from "./NoteBandpassSlider.svelte";
  import { combinationModes } from "./phraseSchedule.js";

  /**
   * Full-width hardware-style rail for Logic / Cross-Mod / Echo / Weave modes.
   *
   * @typedef {Object} Props
   * @property {number} mask
   * @property {(modeIndex: number) => void | Promise<void>} onToggle
   * @property {number} noteBandpassLowMidi
   * @property {number} noteBandpassHighMidi
   * @property {(lowMidi: number, highMidi: number) => void} [onNoteBandpassChange]
   * @property {(lowMidi: number, highMidi: number) => void | Promise<void>} [onNoteBandpassCommit]
   */

  /** @type {Props} */
  let {
    mask = 0,
    onToggle = () => {},
    noteBandpassLowMidi = 36,
    noteBandpassHighMidi = 108,
    onNoteBandpassChange = () => {},
    onNoteBandpassCommit = () => {},
  } = $props();
</script>

<div
  class="combination-mode-rail relative z-20 -mx-6 my-4 flex min-h-[4.75rem] shrink-0 items-center border-t border-b border-zinc-900/80 px-6 py-2.5"
>
  <div class="flex flex-1 items-center justify-center" role="group" aria-label="Combination modes">
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
        onpointerdown={(event) => {
          event.preventDefault();
          onToggle(mode.index);
        }}
      >
        <span class="combination-mode-button-face">
          <CombinationModeIcon kind={mode.icon} />
        </span>
      </button>
    {/each}
  </div>

  <NoteBandpassSlider
    class="ml-6 shrink-0"
    lowMidi={noteBandpassLowMidi}
    highMidi={noteBandpassHighMidi}
    onChange={onNoteBandpassChange}
    onCommit={onNoteBandpassCommit}
  />
</div>
