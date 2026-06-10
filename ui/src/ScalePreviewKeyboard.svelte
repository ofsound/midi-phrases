<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import {
    buildRecordPianoKeys,
    scalePreviewBlackKeyWidthRatio,
    scalePreviewMidiRange,
  } from "./pianoKeyboardLayout.js";
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
    "pointer-events-none absolute bottom-1.5 left-1/2 h-2 w-2 -translate-x-1/2 rounded-full bg-accent-400 shadow-[0_0_6px_rgba(52,211,153,0.8)]";

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
  class="flex w-full min-h-[10rem] flex-col overflow-hidden rounded-xl border border-zinc-800 bg-zinc-950/80"
  aria-hidden="true"
>
  <div class="relative flex min-h-[10rem] w-full flex-1 touch-none select-none">
    <div class="relative z-0 flex h-full min-h-[10rem] w-full">
      {#each layout.whites as { midi } (midi)}
        <div
          class="relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end border-r border-b border-zinc-300/70 bg-gradient-to-b from-white to-zinc-200 last:border-r-0"
        >
          {#if midi % 12 === root}
            <span
              class="pointer-events-none mb-1 text-[11px] font-bold leading-none text-black tabular-nums"
            >
              {midiToNoteName(midi)}
            </span>
          {/if}
          {#if inScale(midi)}
            <span class={scaleToneMarkerClass}></span>
          {/if}
        </div>
      {/each}
    </div>

    <div class="pointer-events-none absolute inset-0 z-10">
      {#each layout.blacks as { midi, centerPercent, widthPercent } (midi)}
        <div
          class="absolute top-0 z-10 h-[58%] max-w-[2.75rem] min-w-[0.75rem] -translate-x-1/2 rounded-b-md border border-zinc-900/80 bg-gradient-to-b from-zinc-700 to-zinc-950 shadow-md relative"
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
</div>
