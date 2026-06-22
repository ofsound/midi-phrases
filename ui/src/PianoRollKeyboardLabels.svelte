<script>
  import {
    isPianoRollKeyboardLabelMidi,
    pianoRollKeyboardLabelFontSizePx,
    pianoRollKeyboardLabelForMidi,
  } from "./pianoRollKeyboardLabels.js";

  /**
   * @typedef {Object} Props
   * @property {number[]} pitchRows
   * @property {number} rowHeightPx
   * @property {(midi: number) => number} pitchTopPx
   */

  /** @type {Props} */
  let { pitchRows, rowHeightPx, pitchTopPx } = $props();

  let labelFontSizePx = $derived(pianoRollKeyboardLabelFontSizePx(rowHeightPx));
</script>

<div class="pointer-events-none absolute inset-0 z-[1] overflow-visible">
  {#each pitchRows as midi (midi)}
    {#if isPianoRollKeyboardLabelMidi(midi)}
      <span
        class="absolute right-1.5 -translate-y-1/2 whitespace-nowrap font-medium leading-none text-text-muted"
        style:top="{pitchTopPx(midi) + rowHeightPx / 2}px"
        style:font-size="{labelFontSizePx}px"
      >
        {pianoRollKeyboardLabelForMidi(midi)}
      </span>
    {/if}
  {/each}
</div>
