<script>
  import RowDisableIcon from "./RowDisableIcon.svelte";
  import { midiToNoteName } from "./midiNoteNames.js";
  import {
    rowAccentFor,
    rowMutedOverlayClasses,
    rowMuteControlClasses,
    rowPowerToggleOffClasses,
  } from "./rowAccentTheme.js";
  import { compactStepVelocityOpacity } from "./compactStepVisuals.js";
  import {
    phraseGridOriginLeftOffsetPx,
    phraseGridVisualOffsetCompensationPx,
    phraseRowInterRowGapPx,
    phraseRowMinHeightPx,
    phraseRowMuteOnlyLeadingWidthPx,
    phraseRowScrollPaddingRightPx,
  } from "./phraseRowLayout.js";
  import {
    compactPhraseGridLayout,
    compactStepShellPaddingPercent,
    compactStepShellTrailingPaddingPercent,
    phraseRowsScrollContentWidthPx,
    quarterGridColumnsForMultiplierIndex,
    rowTimingOffsetShiftPx,
  } from "./stepCellLayout.js";

  /**
   * @typedef {ReturnType<import("./seeding.js").generateSeededPhraseRows>} GeneratedPhraseRows
   * @property {GeneratedPhraseRows} preview
   * @property {boolean[]} [rowMuted]
   * @property {(row: number, soloRequested?: boolean) => void | Promise<void>} [onRowMuteToggle]
   */
  let {
    preview,
    rowMuted = [false, false, false, false],
    onRowMuteToggle = () => {},
  } = $props();

  let previewViewportWidth = $state(0);
  let previewViewportHeight = $state(0);
  const previewViewportPaddingPx = 12;

  let previewVisualOffsetCompensationPx = $derived(
    phraseGridVisualOffsetCompensationPx(preview.rowTimingOffset),
  );
  let previewCompactGridLayout = $derived(
    compactPhraseGridLayout(preview.stepTimingMultiplier, preview.rowTimingOffset),
  );
  let previewMaxTimingPaddingPx = $derived.by(() => {
    let maxPadding = 0;

    for (const offsetIndex of preview.rowTimingOffset) {
      const padding = rowTimingOffsetShiftPx(offsetIndex) + previewVisualOffsetCompensationPx;

      if (padding > maxPadding) {
        maxPadding = padding;
      }
    }

    return maxPadding;
  });
  let previewStepsScrollContentWidthPx = $derived(
    phraseRowsScrollContentWidthPx(preview.stepTimingMultiplier, preview.rowTimingOffset),
  );
  let previewMuteLeadingWidthPx = $derived(phraseRowMuteOnlyLeadingWidthPx());
  let previewStageNaturalWidthPx = $derived(
    previewMuteLeadingWidthPx
      + previewMaxTimingPaddingPx
      + phraseGridOriginLeftOffsetPx()
      + previewStepsScrollContentWidthPx
      + phraseRowScrollPaddingRightPx(),
  );
  let previewStageNaturalHeightPx = $derived(
    (preview.notes.length * phraseRowMinHeightPx())
      + (Math.max(0, preview.notes.length - 1) * phraseRowInterRowGapPx()),
  );
  let previewStageAvailableWidthPx = $derived(Math.max(1, previewViewportWidth - previewViewportPaddingPx * 2));
  let previewStageAvailableHeightPx = $derived(Math.max(1, previewViewportHeight - previewViewportPaddingPx * 2));
  let previewStageScale = $derived.by(() => {
    if (previewStageNaturalWidthPx <= 0 || previewStageNaturalHeightPx <= 0) return 1;

    return Math.min(
      previewStageAvailableWidthPx / previewStageNaturalWidthPx,
      previewStageAvailableHeightPx / previewStageNaturalHeightPx,
    );
  });

  /** @param {HTMLElement} node */
  function previewViewportAttachment(node) {
    const updateSize = () => {
      const rect = node.getBoundingClientRect();
      previewViewportWidth = rect.width;
      previewViewportHeight = rect.height;
    };
    const observer = new ResizeObserver(updateSize);

    updateSize();
    observer.observe(node);

    return () => {
      observer.disconnect();
      previewViewportWidth = 0;
      previewViewportHeight = 0;
    };
  }
</script>

<div
  {@attach previewViewportAttachment}
  class="flex h-[24rem] items-center justify-center overflow-hidden border border-border-subtle bg-surface/50 p-3"
>
  <div
    class="pointer-events-none min-w-0 select-none"
    style={`width: ${previewStageNaturalWidthPx * previewStageScale}px; height: ${previewStageNaturalHeightPx * previewStageScale}px;`}
  >
    <div
      class="relative flex flex-col"
      style={`width: ${previewStageNaturalWidthPx}px; height: ${previewStageNaturalHeightPx}px; transform: scale(${previewStageScale}); transform-origin: top left;`}
    >
      {#each preview.notes as rowNotes, row (row)}
        {@const rowAccent = rowAccentFor(row, true)}
        {@const rowIsMuted = rowMuted[row] ?? false}
        {@const rowLayout = compactPhraseGridLayout(
          [preview.stepTimingMultiplier[row]],
          [preview.rowTimingOffset[row]],
        )}
        {@const rowStartColumn = rowLayout.rowStartColumns[0] ?? 0}
        {@const rowStepsWidthPx = previewStageNaturalWidthPx - previewMuteLeadingWidthPx}
        <div
          class="relative z-10 flex min-w-0 shrink-0 items-stretch overflow-hidden"
          style={`width: ${previewStageNaturalWidthPx}px; height: ${phraseRowMinHeightPx()}px;`}
        >
          <div
            class="relative flex shrink-0 items-center justify-center"
            style:width="{previewMuteLeadingWidthPx}px"
          >
            {#if rowIsMuted}
              <div class={rowMutedOverlayClasses} aria-hidden="true"></div>
            {/if}
            <button
              type="button"
              aria-label={rowIsMuted ? "Turn row on" : "Turn row off"}
              aria-pressed={!rowIsMuted}
              data-cursor="pointer"
              class="pointer-events-auto {rowMuteControlClasses} {rowIsMuted
                ? rowPowerToggleOffClasses
                : rowAccent.textAccent}"
              onclick={(event) => onRowMuteToggle(row, event.shiftKey)}
              title="Shift-click to solo row"
            >
              <RowDisableIcon class="h-[1.9375rem] w-[1.9375rem]" />
            </button>
          </div>
          <div
            class="grid h-full min-w-0"
            style={`width: ${rowStepsWidthPx}px; grid-template-columns: repeat(${previewCompactGridLayout.totalColumns}, minmax(0, 1fr));`}
          >
            {#if rowStartColumn > 0}
              <div style:grid-column={`span ${rowStartColumn}`} aria-hidden="true"></div>
            {/if}
            {#each rowNotes as note, step (step)}
              {@const multiplierIndex = preview.stepTimingMultiplier[row]?.[step] ?? 3}
              {@const isLastStep = step === rowNotes.length - 1}
              {@const skipped = preview.stepSkipped[row]?.[step] ?? false}
              {@const stepDimmed = rowIsMuted || skipped}
              {@const velocityOpacity = compactStepVelocityOpacity(
                preview.stepVelocity[row]?.[step] ?? 127,
                skipped,
              )}
              <div
                style:grid-column={`span ${quarterGridColumnsForMultiplierIndex(multiplierIndex)}`}
                class="relative box-border min-h-full"
                style:padding-left="{compactStepShellPaddingPercent(multiplierIndex)}%"
                style:padding-right="{isLastStep
                  ? compactStepShellTrailingPaddingPercent(multiplierIndex)
                  : compactStepShellPaddingPercent(multiplierIndex)}%"
              >
                <div
                  class="relative h-full min-h-full overflow-hidden rounded-md mp-duration-track-gradient {rowIsMuted
                    ? 'bg-app/95 ring-1 ring-inset ring-border-subtle/90'
                    : 'bg-surface'}"
                >
                  <div
                    class="pointer-events-none absolute inset-0 {rowIsMuted
                      ? 'mp-duration-track-gradient bg-surface/70'
                      : `mp-duration-fill-gradient ${rowAccent.bgAccent}`}"
                    style:opacity={rowIsMuted ? 1 : velocityOpacity}
                    aria-hidden="true"
                  ></div>
                  <div class="relative z-10 flex h-full min-w-0 items-center justify-center px-1">
                    <span
                      class="truncate font-sans text-lg leading-none font-black tabular-nums {stepDimmed
                        ? 'text-text-muted'
                        : 'text-white'}"
                    >
                      {midiToNoteName(note)}
                    </span>
                  </div>
                </div>
              </div>
            {/each}
          </div>
        </div>
        {#if row < preview.notes.length - 1}
          <div class="shrink-0" style:height="{phraseRowInterRowGapPx()}px"></div>
        {/if}
      {/each}
    </div>
  </div>
</div>
