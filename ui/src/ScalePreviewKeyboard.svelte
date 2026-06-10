<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import {
    buildRecordPianoKeys,
    recordPianoRangeLabel,
    scalePreviewMidiRange,
  } from "./pianoKeyboardLayout.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
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

  let range = $derived(scalePreviewMidiRange(root));
  let layout = $derived(buildRecordPianoKeys(range.lowest, range.highest));
  let rangeLabel = $derived(recordPianoRangeLabel(range.lowest, range.highest));

  /** @param {number} midi */
  function inScale(midi) {
    return isMidiInScale(midi, root, modeIndex);
  }
</script>

<div class="grid gap-2">
  <div class="flex items-center justify-between gap-3">
    <p class="text-xs font-semibold uppercase tracking-[0.28em] text-zinc-500">Scale tones</p>
    <span class="text-xs font-medium tabular-nums text-zinc-500">{rangeLabel}</span>
  </div>

  <div
    class="overflow-hidden rounded-xl border border-zinc-800 bg-zinc-950/80"
    aria-hidden="true"
  >
    <div class="relative h-28 touch-none select-none sm:h-32">
      <div class="relative z-0 flex h-full w-full">
        {#each layout.whites as { midi } (midi)}
          <div
            class="relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end border-r border-b border-zinc-700/80 last:border-r-0 {inScale(
              midi,
            )
              ? emeraldRowAccent.pianoNoteActive
              : 'bg-gradient-to-b from-zinc-200/70 to-zinc-400/55'}"
          >
            {#if midi % 12 === root}
              <span
                class="pointer-events-none mb-1 text-[10px] font-bold leading-none tabular-nums {inScale(midi)
                  ? 'text-zinc-950'
                  : 'text-zinc-700'}"
              >
                {midiToNoteName(midi)}
              </span>
            {/if}
          </div>
        {/each}
      </div>

      <div class="pointer-events-none absolute inset-0 z-10">
        {#each layout.blacks as { midi, centerPercent, widthPercent } (midi)}
          <div
            class="absolute top-0 z-10 h-[58%] max-w-[2.75rem] min-w-[0.75rem] -translate-x-1/2 rounded-b-md border border-zinc-900/80 shadow-md {inScale(
              midi,
            )
              ? emeraldRowAccent.pianoNoteActive
              : 'bg-gradient-to-b from-zinc-700/80 to-zinc-950/90'}"
            style:left="{centerPercent}%"
            style:width="{widthPercent}%"
          ></div>
        {/each}
      </div>
    </div>
  </div>
</div>
