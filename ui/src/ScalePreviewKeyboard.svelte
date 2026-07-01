<script>
  import PianoKeyboardAspectFrame from "./PianoKeyboardAspectFrame.svelte";
  import {
    buildRecordPianoKeys,
    scalePreviewBlackKeyWidthRatio,
    scalePreviewMidiRange,
  } from "./pianoKeyboardLayout.js";
  import { pianoBlackKeyClass, pianoWhiteKeyClass } from "./pianoKeyboardTheme.js";
  import { isMidiInScale } from "./scaleUtils.js";

  /**
   * @typedef {Object} Props
   * @property {number} [root]
   * @property {number} [modeIndex]
   */

  /** @type {Props} */
  let {
    root = 0,
    modeIndex = 0,
  } = $props();

  const scaleToneMarkerClass =
    "pointer-events-none h-2 w-2 shrink-0 rounded-full bg-accent shadow-[0_0_6px_color-mix(in_srgb,var(--color-accent)_80%,transparent)]";

  let range = $derived(scalePreviewMidiRange(root));
  let layout = $derived(
    buildRecordPianoKeys(range.lowest, range.highest, {
      blackKeyWidthRatio: scalePreviewBlackKeyWidthRatio,
    }),
  );

  /** @param {number} midi */
  function inScale(midi) {
    return isMidiInScale(midi, root, modeIndex);
  }
</script>

<div
  inert
  class="pointer-events-none overflow-hidden rounded-xl border border-border-subtle bg-app/80 select-none"
  aria-hidden="true"
>
  <PianoKeyboardAspectFrame whiteCount={layout.whiteCount} class="min-h-[10rem] w-full">
    <div class="relative h-full w-full">
      <div class="relative z-0 flex h-full w-full">
        {#each layout.whites as { midi } (midi)}
          <div
            class="relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end pb-1.5 last:border-r-0 {pianoWhiteKeyClass}"
          >
            {#if inScale(midi)}
              <span class={scaleToneMarkerClass}></span>
            {/if}
          </div>
        {/each}
      </div>

      <div class="pointer-events-none absolute inset-0 z-10">
        {#each layout.blacks as { midi, centerPercent, widthPercent } (midi)}
          <div
            class="absolute top-0 z-10 flex h-[58%] -translate-x-1/2 flex-col items-center justify-end rounded-b-md pb-1.5 {pianoBlackKeyClass}"
            style:left="{centerPercent}%"
            style:width="{widthPercent}%"
          >
            {#if inScale(midi)}
              <span class={scaleToneMarkerClass}></span>
            {/if}
          </div>
        {/each}
      </div>
    </div>
  </PianoKeyboardAspectFrame>
</div>
