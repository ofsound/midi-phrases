<script>
  import { SvelteMap } from "svelte/reactivity";
  import PhraseRow from "./PhraseRow.svelte";
  import { defaultStepNoteForScaleRoot } from "./midiNoteNames.js";
  import {
    phraseGridOriginLeftOffsetPx,
    phraseGridVisualOffsetCompensationPx,
    phraseRowInterRowGapPx,
    phraseRowMinHeightPx,
    phraseRowScrollPaddingRightPx,
  } from "./phraseRowLayout.js";
  import { rowAccentFor } from "./rowAccentTheme.js";
  import {
    defaultSeedingSettings,
    generateSeededPhraseRows,
    seedingPhraseLengthMax,
    seedingPhraseLengthMin,
    seedingPresets,
    seedingRangeOptions,
    seedingRhythmOptions,
  } from "./seeding.js";
  import {
    compactPhraseGridLayout,
    phraseRowsScrollContentWidthPx,
    rowTimingOffsetShiftPx,
    timingMultiplierOptions,
  } from "./stepCellLayout.js";

  let {
    settings = { ...defaultSeedingSettings },
    activeScaleName = "C Chromatic",
    root = 0,
    modeIndex = 0,
    busy = false,
    onGestureStart = () => {},
    onSettingsPreview = () => {},
    onSettingsCommit = () => {},
    onShuffle = () => {},
    onNextSeed = () => {},
  } = $props();

  let previewViewportWidth = $state(0);
  let previewViewportHeight = $state(0);
  const previewViewportPaddingPx = 12;

  let preview = $derived(generateSeededPhraseRows({
    ...settings,
    root,
    modeIndex,
  }));
  let previewStepIds = $derived(preview.notes.map((rowNotes, row) => (
    rowNotes.map((_, stepIndex) => `seed-preview-${settings.seed}-${row}-${stepIndex}`)
  )));
  let previewTimingMultiplierById = $derived.by(() => {
    const byId = new SvelteMap();

    previewStepIds.forEach((ids, row) => {
      ids.forEach((id, stepIndex) => {
        byId.set(id, preview.stepTimingMultiplier[row]?.[stepIndex] ?? 3);
      });
    });

    return byId;
  });
  let previewActiveGates = $derived(preview.notes.map((rowNotes) => rowNotes.map(() => false)));
  let previewDefaultStepNote = $derived(defaultStepNoteForScaleRoot(root));
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
  let previewStageNaturalWidthPx = $derived(
    previewMaxTimingPaddingPx
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

  /** @param {number} value */
  function clampPercent(value) {
    return Math.min(100, Math.max(0, Math.round(value)));
  }

  /** @param {Partial<typeof defaultSeedingSettings>} updates */
  function nextSettings(updates) {
    return {
      ...settings,
      ...updates,
    };
  }

  /** @param {Partial<typeof defaultSeedingSettings>} updates */
  function previewSettings(updates) {
    onSettingsPreview(nextSettings(updates));
  }

  /** @param {Partial<typeof defaultSeedingSettings>} updates */
  function commitSettings(updates) {
    onSettingsCommit(nextSettings(updates));
  }

  /** @param {{ id: string, settings: Partial<typeof defaultSeedingSettings> }} preset */
  function applyPreset(preset) {
    onSettingsCommit({
      ...settings,
      ...preset.settings,
      presetId: preset.id,
      seed: Math.max(1, Math.floor(Math.random() * 2147483646)),
    });
  }
</script>

<div class="grid min-h-[29rem] gap-5 border-y border-border-subtle bg-surface/20 px-4 py-4 sm:px-5 lg:grid-cols-[minmax(20rem,25rem)_minmax(0,1fr)]">
  <div class="grid content-start gap-4">
    <div class="min-w-0">
      <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Scale</p>
      <h2 class="mt-1 truncate text-lg font-semibold text-text">{activeScaleName}</h2>
    </div>

    <div class="grid grid-cols-2 gap-2">
      {#each seedingPresets as preset (preset.id)}
        <button
          type="button"
          aria-pressed={settings.presetId === preset.id}
          data-cursor="pointer"
          disabled={busy}
          class="h-10 border px-3 text-center text-sm font-semibold outline-none transition-colors focus:ring-1 focus:ring-focus-ring disabled:opacity-40 {settings.presetId === preset.id
            ? 'border-accent bg-accent text-control-primary-text'
            : 'border-border bg-surface text-text hover:border-border-strong'}"
          onclick={() => applyPreset(preset)}
        >
          {preset.label}
        </button>
      {/each}
    </div>

    <div class="grid gap-3">
      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Phrase length</span><span class="font-mono text-accent">{settings.phraseLength}</span>
        </span>
        <input
          type="range"
          min={seedingPhraseLengthMin}
          max={seedingPhraseLengthMax}
          step="1"
          value={settings.phraseLength}
          disabled={busy}
          class="w-full accent-[var(--color-accent)] disabled:opacity-40"
          onpointerdown={onGestureStart}
          onkeydown={onGestureStart}
          oninput={(event) => previewSettings({ phraseLength: Number(event.currentTarget.value) })}
          onchange={(event) => commitSettings({ phraseLength: Number(event.currentTarget.value) })}
        />
        <div class="flex justify-between font-mono text-[9px] leading-none text-text-muted">
          <span>{seedingPhraseLengthMin}</span>
          <span>{seedingPhraseLengthMax}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Range</span>
        <select
          class="h-9 border border-border bg-surface px-2 text-sm font-semibold text-text outline-none focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
          value={settings.rangeIndex}
          disabled={busy}
          onchange={(event) => commitSettings({ rangeIndex: Number(event.currentTarget.value) })}
        >
          {#each seedingRangeOptions as option (option.index)}
            <option value={option.index}>{option.label}</option>
          {/each}
        </select>
      </label>

      <div class="grid grid-cols-2 gap-2">
        {#each seedingRhythmOptions as option (option.value)}
          <button
            type="button"
            aria-pressed={settings.rhythmMode === option.value}
            data-cursor="pointer"
            disabled={busy}
            class="h-10 border px-3 text-sm font-semibold outline-none transition-colors focus:ring-1 focus:ring-focus-ring disabled:opacity-40 {settings.rhythmMode === option.value
              ? 'border-accent bg-accent text-control-primary-text'
              : 'border-border bg-surface text-text hover:border-border-strong'}"
            onclick={() => commitSettings({ rhythmMode: option.value })}
          >
            {option.label}
          </button>
        {/each}
      </div>

      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Repetition</span><span class="font-mono text-accent">{settings.repetition}</span>
        </span>
        <input
          type="range"
          min="0"
          max="100"
          value={settings.repetition}
          disabled={busy}
          class="w-full accent-[var(--color-accent)] disabled:opacity-40"
          onpointerdown={onGestureStart}
          onkeydown={onGestureStart}
          oninput={(event) => previewSettings({ repetition: clampPercent(Number(event.currentTarget.value)) })}
          onchange={(event) => commitSettings({ repetition: clampPercent(Number(event.currentTarget.value)) })}
        />
      </label>

      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Complexity</span><span class="font-mono text-accent">{settings.complexity}</span>
        </span>
        <input
          type="range"
          min="0"
          max="100"
          value={settings.complexity}
          disabled={busy}
          class="w-full accent-[var(--color-accent)] disabled:opacity-40"
          onpointerdown={onGestureStart}
          onkeydown={onGestureStart}
          oninput={(event) => previewSettings({ complexity: clampPercent(Number(event.currentTarget.value)) })}
          onchange={(event) => commitSettings({ complexity: clampPercent(Number(event.currentTarget.value)) })}
        />
      </label>

      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Randomness</span><span class="font-mono text-accent">{settings.randomness}</span>
        </span>
        <input
          type="range"
          min="0"
          max="100"
          value={settings.randomness}
          disabled={busy}
          class="w-full accent-[var(--color-accent)] disabled:opacity-40"
          onpointerdown={onGestureStart}
          onkeydown={onGestureStart}
          oninput={(event) => previewSettings({ randomness: clampPercent(Number(event.currentTarget.value)) })}
          onchange={(event) => commitSettings({ randomness: clampPercent(Number(event.currentTarget.value)) })}
        />
      </label>

      <label class="flex h-9 items-center justify-between gap-3 border border-border bg-surface px-3 text-sm font-semibold text-text">
        <span>Symmetry</span>
        <input
          type="checkbox"
          checked={settings.symmetry}
          disabled={busy}
          class="h-4 w-4 accent-[var(--color-accent)] disabled:opacity-40"
          onchange={(event) => commitSettings({ symmetry: event.currentTarget.checked })}
        />
      </label>
    </div>
  </div>

  <div class="grid min-h-0 content-start gap-3">
    <div class="flex items-center justify-between gap-3">
      <div>
        <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Preview</p>
        <h2 class="mt-1 text-lg font-semibold text-text">4 phrases</h2>
      </div>
      <div class="flex shrink-0 items-center gap-2">
        <button
          type="button"
          data-cursor="pointer"
          disabled={busy}
          class="h-9 border border-border bg-surface px-3 text-xs font-semibold uppercase tracking-[0.16em] text-text outline-none transition-colors hover:border-accent/70 focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
          onclick={onNextSeed}
        >
          New seed
        </button>
        <button
          type="button"
          data-cursor="pointer"
          disabled={busy}
          class="h-9 border border-accent bg-accent px-4 text-xs font-semibold uppercase tracking-[0.16em] text-control-primary-text outline-none transition-opacity hover:opacity-90 focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
          onclick={onShuffle}
        >
          Shuffle
        </button>
      </div>
    </div>

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
            <div
              class="relative z-10 min-w-0 shrink-0 overflow-hidden"
              style={`width: ${previewStageNaturalWidthPx}px; height: ${phraseRowMinHeightPx()}px;`}
            >
              <div
                class="flex min-w-0 items-stretch"
                style={`width: ${previewStageNaturalWidthPx}px;`}
              >
                <PhraseRow
                  {row}
                  muted={false}
                  accent={rowAccent}
                  timingOffsetIndex={preview.rowTimingOffset[row]}
                  timingOffsetVisualCompensationPx={previewVisualOffsetCompensationPx}
                  pulseIndex={1}
                  stepIds={previewStepIds[row]}
                  notes={rowNotes}
                  stepDurationFraction={preview.stepDurationFraction[row]}
                  stepTimingMultiplier={preview.stepTimingMultiplier[row]}
                  stepTimingMultiplierById={previewTimingMultiplierById}
                  stepVelocity={preview.stepVelocity[row]}
                  stepMuted={preview.stepMuted[row]}
                  stepSkipped={preview.stepSkipped[row]}
                  stepProbability={preview.stepProbability[row]}
                  stepCycle={preview.stepCycle[row]}
                  stepCycleOffset={preview.stepCycleOffset[row]}
                  activeGates={previewActiveGates[row]}
                  selectedStepIds={[]}
                  stepInspectionActive={false}
                  stepInspectorOpen={false}
                  stretchToFit
                  contentFitScale={1}
                  fitGridColumns={previewCompactGridLayout.totalColumns}
                  showAddStepControls={false}
                  inspectedStepId={null}
                  stepInspectorHighlightedId={null}
                  stepNoteValue={(value) => value}
                  defaultStepNote={previewDefaultStepNote}
                  {timingMultiplierOptions}
                />
              </div>
            </div>
            {#if row < preview.notes.length - 1}
              <div class="shrink-0" style:height="{phraseRowInterRowGapPx()}px"></div>
            {/if}
          {/each}
        </div>
      </div>
    </div>
  </div>
</div>
