<script>
  import CombinationModeIcon from "./CombinationModeIcon.svelte";
  import NoteBandpassSlider from "./NoteBandpassSlider.svelte";
  import OctavizerControl from "./OctavizerControl.svelte";
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
   * @property {boolean} octavizerDown8vaEnabled
   * @property {boolean} octavizerUp8vaEnabled
   * @property {number} octavizerDown8vaRelativeVelocity
   * @property {number} octavizerUp8vaRelativeVelocity
   * @property {(enabled: boolean) => void | Promise<void>} [onOctavizerDown8vaToggle]
   * @property {(enabled: boolean) => void | Promise<void>} [onOctavizerUp8vaToggle]
   * @property {(value: number) => void | Promise<void>} [onOctavizerDown8vaRelativeVelocityChange]
   * @property {(value: number) => void | Promise<void>} [onOctavizerUp8vaRelativeVelocityChange]
   * @property {(value: number) => void | Promise<void>} [onOctavizerDown8vaRelativeVelocityCommit]
   * @property {(value: number) => void | Promise<void>} [onOctavizerUp8vaRelativeVelocityCommit]
   */

  /** @type {Props} */
  let {
    mask = 0,
    onToggle = () => {},
    noteBandpassLowMidi = 36,
    noteBandpassHighMidi = 108,
    onNoteBandpassChange = () => {},
    onNoteBandpassCommit = () => {},
    octavizerDown8vaEnabled = false,
    octavizerUp8vaEnabled = false,
    octavizerDown8vaRelativeVelocity = 0,
    octavizerUp8vaRelativeVelocity = 0,
    onOctavizerDown8vaToggle = () => {},
    onOctavizerUp8vaToggle = () => {},
    onOctavizerDown8vaRelativeVelocityChange = () => {},
    onOctavizerUp8vaRelativeVelocityChange = () => {},
    onOctavizerDown8vaRelativeVelocityCommit = () => {},
    onOctavizerUp8vaRelativeVelocityCommit = () => {},
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

    <OctavizerControl
      down8vaEnabled={octavizerDown8vaEnabled}
      up8vaEnabled={octavizerUp8vaEnabled}
      down8vaRelativeVelocity={octavizerDown8vaRelativeVelocity}
      up8vaRelativeVelocity={octavizerUp8vaRelativeVelocity}
      onDown8vaToggle={onOctavizerDown8vaToggle}
      onUp8vaToggle={onOctavizerUp8vaToggle}
      onDown8vaRelativeVelocityChange={onOctavizerDown8vaRelativeVelocityChange}
      onUp8vaRelativeVelocityChange={onOctavizerUp8vaRelativeVelocityChange}
      onDown8vaRelativeVelocityCommit={onOctavizerDown8vaRelativeVelocityCommit}
      onUp8vaRelativeVelocityCommit={onOctavizerUp8vaRelativeVelocityCommit}
    />
  </div>

  <NoteBandpassSlider
    class="ml-6 shrink-0"
    lowMidi={noteBandpassLowMidi}
    highMidi={noteBandpassHighMidi}
    onChange={onNoteBandpassChange}
    onCommit={onNoteBandpassCommit}
  />
</div>
