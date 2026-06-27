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
    phraseSeedPreviewMuteColumnWidthPx,
    phraseSeedPreviewMuteSidePaddingPx,
    phraseRowEndAddStepReservePx,
    phraseRowScrollPaddingRightPx,
  } from "./phraseRowLayout.js";
  import {
    phraseRowsScrollContentWidthPx,
    rowTimingOffsetShiftPx,
    stepCellGridSpanPx,
  } from "./stepCellLayout.js";

  /**
   * @typedef {ReturnType<import("./seeding.js").generateSeededPhraseRows>} GeneratedPhraseRows
   * @property {GeneratedPhraseRows} preview
   * @property {boolean[][]} [activeGates]
   * @property {boolean[]} [rowMuted]
   * @property {(row: number, soloRequested?: boolean) => void | Promise<void>} [onRowMuteToggle]
   */
  let {
    preview,
    activeGates = [],
    rowMuted = [false, false, false, false],
    onRowMuteToggle = () => {},
  } = $props();

  let previewViewportWidth = $state(0);
  const previewViewportPaddingPx = 12;

  let previewVisualOffsetCompensationPx = $derived(
    phraseGridVisualOffsetCompensationPx(preview.rowTimingOffset),
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
  let previewGutterPx = $derived(phraseRowInterRowGapPx());
  let previewRowHeightPx = $derived(phraseRowMinHeightPx());
  let previewMuteColumnWidthPx = $derived(phraseSeedPreviewMuteColumnWidthPx());
  let previewMuteSidePaddingPx = $derived(phraseSeedPreviewMuteSidePaddingPx());
  let previewStepsTrailingReservePx = $derived(phraseRowEndAddStepReservePx());
  let previewStepsScrollContentWidthPx = $derived(
    phraseRowsScrollContentWidthPx(preview.stepTimingMultiplier, preview.rowTimingOffset),
  );
  let previewStageAvailableWidthPx = $derived(Math.max(1, previewViewportWidth - previewViewportPaddingPx * 2));
  /** Shrink step shells only; timing padding, gutters, and row height stay fixed. */
  let previewStageScaleX = $derived.by(() => {
    if (previewStepsScrollContentWidthPx <= 0) return 1;

    const stepsBudgetPx = Math.max(
      1,
      previewStageAvailableWidthPx
        - previewMuteColumnWidthPx
        - previewMaxTimingPaddingPx
        - phraseGridOriginLeftOffsetPx()
        - phraseRowScrollPaddingRightPx()
        - previewStepsTrailingReservePx,
    );

    return Math.min(1, stepsBudgetPx / previewStepsScrollContentWidthPx);
  });
  let previewStageHeightPx = $derived(
    (preview.notes.length * previewRowHeightPx)
      + (Math.max(0, preview.notes.length - 1) * previewGutterPx),
  );

  /** Matches PhraseRow: per-row offset shift plus shared negative-offset compensation. */
  function previewRowTimingPaddingPx(row) {
    return rowTimingOffsetShiftPx(preview.rowTimingOffset[row] ?? 0) + previewVisualOffsetCompensationPx;
  }

  /** @param {number} multiplierIndex */
  function previewStepWidthPx(multiplierIndex) {
    return (stepCellGridSpanPx(multiplierIndex) - previewGutterPx) * previewStageScaleX;
  }

  /** @param {HTMLElement} node */
  function previewViewportAttachment(node) {
    const updateSize = () => {
      previewViewportWidth = node.getBoundingClientRect().width;
    };
    const observer = new ResizeObserver(updateSize);

    updateSize();
    observer.observe(node);

    return () => {
      observer.disconnect();
      previewViewportWidth = 0;
    };
  }
</script>

<div
  {@attach previewViewportAttachment}
  class="flex h-full min-h-0 w-full items-start justify-start overflow-hidden border border-border-subtle bg-surface/50 p-3"
>
  <div
    class="pointer-events-none relative flex w-full min-w-0 select-none flex-col"
    style={`height: ${previewStageHeightPx}px; gap: ${previewGutterPx}px;`}
  >
    {#each preview.notes as rowNotes, row (row)}
      {@const rowAccent = rowAccentFor(row, true)}
      {@const rowIsMuted = rowMuted[row] ?? false}
      <div
        class="relative z-10 flex min-w-0 shrink-0 items-stretch overflow-hidden"
        style={`height: ${previewRowHeightPx}px;`}
      >
        <div
          class="relative box-border flex shrink-0 items-center"
          style:width="{previewMuteColumnWidthPx}px"
          style:padding-left="{previewMuteSidePaddingPx}px"
          style:padding-right="{previewMuteSidePaddingPx}px"
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
          class="flex min-w-0 flex-1 items-stretch overflow-hidden"
          style:padding-left="{previewRowTimingPaddingPx(row)}px"
          style:padding-right="{phraseRowScrollPaddingRightPx()}px"
        >
          <div
            class="pointer-events-none shrink-0 self-stretch"
            style:width="{phraseGridOriginLeftOffsetPx()}px"
            aria-hidden="true"
          ></div>
          <div class="flex min-w-0 flex-1 items-stretch overflow-hidden">
            <div
              class="flex h-full w-max max-w-full items-stretch justify-start"
              style:gap="{previewGutterPx}px"
              style:padding-right="{previewStepsTrailingReservePx}px"
            >
              {#each rowNotes as _note, step (step)}
                {@const multiplierIndex = preview.stepTimingMultiplier[row]?.[step] ?? 3}
                {@const skipped = preview.stepSkipped[row]?.[step] ?? false}
                {@const stepDimmed = rowIsMuted || skipped}
                {@const velocityOpacity = compactStepVelocityOpacity(
                  preview.stepVelocity[row]?.[step] ?? 127,
                  skipped,
                )}
                {@const stepActive = activeGates[row]?.[step] ?? false}
                <div
                  class="relative box-border min-h-full shrink-0"
                  style:width="{previewStepWidthPx(multiplierIndex)}px"
                >
                  <div
                    class="relative h-full min-h-full overflow-hidden rounded-md transition-[box-shadow,filter] duration-75 mp-duration-track-gradient {rowIsMuted
                      ? 'bg-app/95 ring-1 ring-inset ring-border-subtle/90'
                      : 'bg-surface'} {stepActive && !stepDimmed
                      ? rowAccent.playbackGlow
                      : ''} {stepActive ? 'brightness-125' : ''}"
                  >
                    <div
                      class="pointer-events-none absolute inset-0 transition-[background-color,opacity] duration-75 {rowIsMuted
                        ? 'mp-duration-track-gradient bg-surface/70'
                        : `mp-duration-fill-gradient ${stepActive ? rowAccent.bgAccentStrong : rowAccent.bgAccent}`}"
                      style:opacity={rowIsMuted ? 1 : stepActive ? 1 : velocityOpacity}
                      aria-hidden="true"
                    ></div>
                    <div class="relative z-10 flex h-full min-w-0 items-center justify-center px-1">
                      <span
                        class="truncate font-sans text-sm leading-none font-black tabular-nums {stepDimmed
                          ? 'text-text-muted'
                          : 'text-white'}"
                      >
                        {midiToNoteName(rowNotes[step])}
                      </span>
                    </div>
                  </div>
                </div>
              {/each}
            </div>
          </div>
        </div>
      </div>
    {/each}
  </div>
</div>
