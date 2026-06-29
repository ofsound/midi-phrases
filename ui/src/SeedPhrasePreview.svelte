<script>
  import DurationBar from "./DurationBar.svelte";
  import RowDisableIcon from "./RowDisableIcon.svelte";
  import StepGearIcon from "./StepGearIcon.svelte";
  import StepMuteIcon from "./StepMuteIcon.svelte";
  import StepMutedOverlay from "./StepMutedOverlay.svelte";
  import StepSkipIcon from "./StepSkipIcon.svelte";
  import StepSkippedOverlay from "./StepSkippedOverlay.svelte";
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
    stepFooterHeightPx,
  } from "./phraseRowLayout.js";
  import {
    defaultStepTimingMultiplierIndex,
    multiplierLabelForIndex,
    phraseRowsScrollContentWidthPx,
    rowTimingOffsetShiftPx,
    stepCellGridSpanPx,
    stepCellPaddingPx,
    stepDisplayWidthPx,
    stepFooterActionSlotWidthPx,
    stepInsertZoneWidthPx,
    timingMultiplierOptions,
  } from "./stepCellLayout.js";

  /**
   * @typedef {ReturnType<import("./seeding.js").generateSeededPhraseRows>} GeneratedPhraseRows
   * @property {GeneratedPhraseRows} preview
   * @property {boolean[][]} [activeGates]
   * @property {boolean[]} [rowMuted]
   * @property {boolean} [compact]
   * @property {(row: number, soloRequested?: boolean) => void | Promise<void>} [onRowMuteToggle]
   */
  let {
    preview,
    activeGates = [],
    rowMuted = [false, false, false, false],
    compact = false,
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

  /** @param {number} multiplierIndex */
  function fullPreviewStepWidthPx(multiplierIndex) {
    return stepDisplayWidthPx(multiplierIndex);
  }

  const fullStepCellSurfaceClass = (dimmed) =>
    dimmed ? "bg-app/95" : "bg-surface";

  const fullStepHeaderClass = (dimmed) =>
    dimmed
      ? "border-b border-border-subtle/90 bg-surface/70"
      : "border-b border-border-subtle bg-surface-muted/60";

  const fullStepHeaderLabelClass = (dimmed) =>
    dimmed ? "text-text-muted" : "text-text-secondary";

  const fullStepFooterShellClass = (dimmed) =>
    dimmed
      ? "border-t border-border-subtle/90 bg-surface/70"
      : "border-t border-border-subtle bg-surface-muted/60";

  const fullStepFooterIconClass = (active, muted) =>
    muted
      ? "text-text-faint"
      : active
        ? "text-accent"
        : "text-text-secondary";

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

{#snippet fullStepFooter(rowIsMuted, stepIsMuted, stepIsSkipped, multiplierIndex)}
  {@const footerDimmed = rowIsMuted || stepIsSkipped}
  {@const isQuarterStep = multiplierIndex === 0}
  {@const footerSlotStyle = `width: ${stepFooterActionSlotWidthPx()}px`}
  {@const iconClass = "pointer-events-none h-3.5 w-3.5"}
  <div
    class="relative z-[80] flex w-full shrink-0 {isQuarterStep
      ? 'divide-x divide-border-subtle'
      : 'justify-between'} {fullStepFooterShellClass(footerDimmed)}"
    style:height="{stepFooterHeightPx()}px"
    aria-hidden="true"
  >
    {#if isQuarterStep}
      <div class="flex h-full min-w-0 flex-1 basis-0 items-center justify-center {fullStepFooterIconClass(stepIsSkipped, rowIsMuted)}">
        <StepSkipIcon class={iconClass} />
      </div>
      <div class="flex h-full min-w-0 flex-1 basis-0 items-center justify-center {fullStepFooterIconClass(stepIsMuted, rowIsMuted)}">
        <StepMuteIcon class={iconClass} />
      </div>
      <div class="flex h-full min-w-0 flex-1 basis-0 items-center justify-center text-text-secondary">
        <StepGearIcon class="pointer-events-none h-3 w-3" />
      </div>
    {:else}
      <div class="flex shrink-0 divide-x divide-border-subtle">
        <div
          class="flex h-full shrink-0 items-center justify-center {fullStepFooterIconClass(
            stepIsSkipped,
            rowIsMuted,
          )}"
          style={footerSlotStyle}
        >
          <StepSkipIcon class={iconClass} />
        </div>
        <div
          class="flex h-full shrink-0 items-center justify-center {fullStepFooterIconClass(
            stepIsMuted,
            rowIsMuted,
          )}"
          style={footerSlotStyle}
        >
          <StepMuteIcon class={iconClass} />
        </div>
      </div>
      <div class="flex h-full shrink-0 items-center justify-center text-text-secondary" style={footerSlotStyle}>
        <StepGearIcon class="pointer-events-none h-3 w-3" />
      </div>
    {/if}
  </div>
{/snippet}

{#snippet fullStepCell(rowNotes, rowAccent, rowIsMuted, row, step)}
  {@const multiplierIndex = preview.stepTimingMultiplier[row]?.[step] ?? defaultStepTimingMultiplierIndex}
  {@const stepIsMuted = preview.stepMuted[row]?.[step] ?? false}
  {@const stepIsSkipped = preview.stepSkipped[row]?.[step] ?? false}
  {@const stepDimmed = rowIsMuted || stepIsSkipped}
  {@const stepActive = activeGates[row]?.[step] ?? false}
  {@const velocity = preview.stepVelocity[row]?.[step] ?? 127}
  {@const durationFraction = preview.stepDurationFraction[row]?.[step] ?? 1}
  {@const multiplierLabel = multiplierLabelForIndex(multiplierIndex, timingMultiplierOptions)}
  <div
    class="relative h-full w-full min-w-0 overflow-visible rounded-lg transition-[opacity,box-shadow] duration-150 {stepActive && !stepDimmed
      ? rowAccent.playbackGlow
      : ''}"
  >
    <div
      class="relative z-0 flex h-full min-h-0 w-full min-w-0 flex-col overflow-hidden rounded-lg border-2 outline-none transition-[border-color,background-color,box-shadow,opacity] duration-150 {fullStepCellSurfaceClass(
        stepDimmed,
      )} {stepDimmed
        ? 'border-border-subtle/90'
        : stepActive
          ? rowAccent.borderActive
          : 'border-border'}"
    >
      <div
        class="flex w-full shrink-0 items-center justify-end gap-0 pl-5 pr-1 {fullStepHeaderClass(stepDimmed)}"
        style:height="{stepFooterHeightPx()}px"
      >
        <span
          class="pointer-events-none font-sans text-xs leading-none font-semibold tabular-nums {fullStepHeaderLabelClass(
            stepDimmed,
          )}"
          aria-hidden="true"
        >
          {multiplierLabel}
        </span>
      </div>

      <div class="relative flex min-h-0 min-w-0 flex-1 flex-col gap-1 px-1 pt-0.5 pb-1 {stepDimmed ? 'opacity-80' : ''}">
        <div class="-mx-1 -mt-0.5">
          <DurationBar
            accent={rowAccent}
            muted={stepDimmed}
            stepMuted={stepIsMuted && !stepDimmed}
            value={durationFraction}
            velocity={velocity}
            ariaLabel="Preview step duration fraction"
          />
        </div>
        <div class="flex min-w-0 items-center overflow-hidden">
          <div class="flex shrink-0 items-baseline gap-1">
            <span
              class="font-sans text-base leading-none font-bold tabular-nums {stepDimmed
                ? 'text-text-muted'
                : 'text-text'}"
            >
              {midiToNoteName(rowNotes[step])}
            </span>
            <span
              class="font-sans text-[0.625rem] leading-none font-bold tabular-nums {stepDimmed
                ? 'text-text-faint'
                : 'text-text-secondary'}"
            >
              {velocity}
            </span>
          </div>
          <div class="min-h-5 min-w-0 flex-1" role="presentation"></div>
        </div>
      </div>

      {@render fullStepFooter(rowIsMuted, stepIsMuted, stepIsSkipped, multiplierIndex)}
    </div>

    {#if stepIsSkipped}
      <StepSkippedOverlay active={true} />
    {/if}
    {#if stepIsMuted}
      <StepMutedOverlay active={true} />
    {/if}
  </div>
{/snippet}

<div
  {@attach previewViewportAttachment}
  class="flex h-full min-h-0 w-full items-start justify-start overflow-hidden border border-border-subtle bg-surface/50 p-3"
>
  <div
    class="relative flex w-full min-w-0 select-none flex-col"
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
              class="h-full min-w-0 flex-1 {compact ? 'overflow-hidden' : 'overflow-x-auto overflow-y-hidden'}"
            >
              <div
                class="pointer-events-none flex h-full w-max items-stretch justify-start {compact ? 'max-w-full' : ''}"
                style:gap={compact ? `${previewGutterPx}px` : undefined}
                style:padding-left={compact ? undefined : `${stepCellPaddingPx()}px`}
                style:padding-right="{previewStepsTrailingReservePx}px"
              >
              {#each rowNotes as _note, step (step)}
                {@const multiplierIndex = preview.stepTimingMultiplier[row]?.[step] ?? defaultStepTimingMultiplierIndex}
                <div
                  class="relative box-border min-h-full shrink-0"
                  style:width="{compact
                    ? previewStepWidthPx(multiplierIndex)
                    : fullPreviewStepWidthPx(multiplierIndex)}px"
                  style:margin-left={compact || step === 0 ? undefined : `${stepInsertZoneWidthPx()}px`}
                >
                  {#if compact}
                    {@const skipped = preview.stepSkipped[row]?.[step] ?? false}
                    {@const stepDimmed = rowIsMuted || skipped}
                    {@const velocityOpacity = compactStepVelocityOpacity(
                      preview.stepVelocity[row]?.[step] ?? 127,
                      skipped,
                    )}
                    {@const stepActive = activeGates[row]?.[step] ?? false}
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
                      {#if skipped}
                        <StepSkippedOverlay active={true} />
                      {/if}
                      {#if preview.stepMuted[row]?.[step] ?? false}
                        <StepMutedOverlay active={true} />
                      {/if}
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
                  {:else}
                    {@render fullStepCell(rowNotes, rowAccent, rowIsMuted, row, step)}
                  {/if}
                </div>
              {/each}
              </div>
            </div>
          </div>
        </div>
      </div>
    {/each}
  </div>
</div>
