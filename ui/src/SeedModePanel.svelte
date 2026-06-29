<script>
  import AccentRangeSlider from "./AccentRangeSlider.svelte";
  import SeedPhrasePreview from "./SeedPhrasePreview.svelte";
  import SeedRowTargetRail from "./SeedRowTargetRail.svelte";
  import {
    applySeedingRowSettingsUpdate,
    defaultSeedModeState,
    displaySeedingRowSettings,
    generateSeededPhraseRowsFromSeedModeState,
    mergeSeededPhraseRows,
    phraseRowsFromGridState,
    seedingCenterMidiForIndex,
    seedingCenterNoteIndex,
    seedingCenterNoteLabel,
    seedingCenterNoteOptions,
    seedingPhraseLengthMax,
    seedingPhraseLengthMin,
    seedingRangeSemitonesMax,
    seedingRangeSemitonesMin,
    seedingRhythmStepMax,
    seedingRhythmStepMin,
  } from "./seeding.js";
  import { rowAccentFor, rowAccentScopeStyle } from "./rowAccentTheme.js";

  /** @typedef {import("./seeding.js").SeedingRowSettings} SeedingRowSettings */

  let {
    rhythmStep = defaultSeedModeState.rhythmStep,
    rowSettings = defaultSeedModeState.rowSettings,
    rowTargets = defaultSeedModeState.rowTargets,
    stretchStepsToFit = false,
    root = 0,
    modeIndex = 0,
    activeGates = [],
    rowMuted = [false, false, false, false],
    rowColorsEnabled = true,
    currentPhraseRows = phraseRowsFromGridState(),
    busy = false,
    onGestureStart = () => {},
    onRhythmPreview = () => {},
    onRhythmCommit = () => {},
    onRowSettingsPreview = () => {},
    onRowSettingsCommit = () => {},
    onShuffle = () => {},
    onNextSeed = () => {},
    onRowMuteToggle = () => {},
    onRowTargetToggle = () => {},
    onToggleAllRowTargets = () => {},
  } = $props();

  let displaySettings = $derived(displaySeedingRowSettings(rowSettings, rowTargets));
  let singleTargetedRow = $derived.by(() => {
    const targetedRows = rowTargets
      .map((targeted, row) => (targeted ? row : -1))
      .filter((row) => row >= 0);

    return targetedRows.length === 1 ? targetedRows[0] : null;
  });
  let seedParamAccentStyle = $derived(
    singleTargetedRow === null
      ? ""
      : rowAccentScopeStyle(rowAccentFor(singleTargetedRow, rowColorsEnabled)),
  );
  let centerNoteOptions = $derived(seedingCenterNoteOptions(root, modeIndex));
  let centerNoteIndex = $derived(seedingCenterNoteIndex(displaySettings.centerMidi, root, modeIndex));
  let centerNoteLabel = $derived(seedingCenterNoteLabel(displaySettings.centerMidi, root, modeIndex));

  let generatedPreview = $derived(generateSeededPhraseRowsFromSeedModeState(
    { rhythmStep, rowSettings },
    root,
    modeIndex,
  ));

  let preview = $derived(mergeSeededPhraseRows(
    currentPhraseRows,
    generatedPreview,
    rowTargets,
  ));

  /** @param {number} value */
  function clampPercent(value) {
    return Math.min(100, Math.max(0, Math.round(value)));
  }

  /** @param {Partial<SeedingRowSettings>} updates */
  function previewRowSettings(updates) {
    onRowSettingsPreview(applySeedingRowSettingsUpdate(rowSettings, rowTargets, updates));
  }

  /** @param {Partial<SeedingRowSettings>} updates */
  function commitRowSettings(updates) {
    onRowSettingsCommit(applySeedingRowSettingsUpdate(rowSettings, rowTargets, updates));
  }

</script>

<div class="grid min-h-[29rem] items-stretch gap-5 border-y border-border-subtle bg-surface/20 py-4 pl-4 pr-0 sm:pl-5 lg:grid-cols-[minmax(20rem,25rem)_minmax(0,1fr)]">
  <div class="grid content-start gap-2">
    <div class="grid gap-2">
      <div class="grid grid-cols-[minmax(0,1fr)_minmax(0,50%)] items-end gap-3">
        <SeedRowTargetRail
          class="min-w-0"
          {rowTargets}
          {rowColorsEnabled}
          {busy}
          {onRowTargetToggle}
          {onToggleAllRowTargets}
        />

        <label class="grid min-w-0 gap-0.5">
          <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Overlap</span>
          <div class="seed-param-row">
            <AccentRangeSlider
              value={rhythmStep}
              min={seedingRhythmStepMin}
              max={seedingRhythmStepMax}
              disabled={busy}
              ariaLabel="Rhythm overlap to interleave"
              onGestureStart={onGestureStart}
              onValuePreview={onRhythmPreview}
              onValueCommit={onRhythmCommit}
            />
            <span class="seed-param-value" aria-hidden="true">{rhythmStep}</span>
          </div>
        </label>
      </div>

      <label class="grid gap-0.5" style={seedParamAccentStyle}>
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Phrase length</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={displaySettings.phraseLength}
            min={seedingPhraseLengthMin}
            max={seedingPhraseLengthMax}
            disabled={busy}
            ariaLabel="Phrase length"
            onGestureStart={onGestureStart}
            onValuePreview={(phraseLength) => previewRowSettings({ phraseLength })}
            onValueCommit={(phraseLength) => commitRowSettings({ phraseLength })}
          />
          <span class="seed-param-value" aria-hidden="true">{displaySettings.phraseLength}</span>
        </div>
      </label>

      <label class="grid gap-0.5" style={seedParamAccentStyle}>
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Center</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={centerNoteIndex}
            max={Math.max(0, centerNoteOptions.length - 1)}
            disabled={busy}
            ariaLabel="Center note"
            onGestureStart={onGestureStart}
            onValuePreview={(index) => previewRowSettings({
              centerMidi: seedingCenterMidiForIndex(index, root, modeIndex),
            })}
            onValueCommit={(index) => commitRowSettings({
              centerMidi: seedingCenterMidiForIndex(index, root, modeIndex),
            })}
          />
          <span class="seed-param-value" aria-hidden="true">{centerNoteLabel}</span>
        </div>
      </label>

      <label class="grid gap-0.5" style={seedParamAccentStyle}>
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Range</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={displaySettings.rangeSemitones}
            min={seedingRangeSemitonesMin}
            max={seedingRangeSemitonesMax}
            disabled={busy}
            ariaLabel="Range in semitones"
            onGestureStart={onGestureStart}
            onValuePreview={(rangeSemitones) => previewRowSettings({ rangeSemitones })}
            onValueCommit={(rangeSemitones) => commitRowSettings({ rangeSemitones })}
          />
          <span class="seed-param-value" aria-hidden="true">{displaySettings.rangeSemitones} st</span>
        </div>
      </label>

      <label class="grid gap-0.5" style={seedParamAccentStyle}>
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Repetition</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={displaySettings.repetition}
            max={100}
            disabled={busy}
            ariaLabel="Repetition"
            onGestureStart={onGestureStart}
            onValuePreview={(repetition) => previewRowSettings({ repetition: clampPercent(repetition) })}
            onValueCommit={(repetition) => commitRowSettings({ repetition: clampPercent(repetition) })}
          />
          <span class="seed-param-value" aria-hidden="true">{displaySettings.repetition}</span>
        </div>
      </label>

      <label class="grid gap-0.5" style={seedParamAccentStyle}>
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Complexity</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={displaySettings.complexity}
            max={100}
            disabled={busy}
            ariaLabel="Complexity"
            onGestureStart={onGestureStart}
            onValuePreview={(complexity) => previewRowSettings({ complexity: clampPercent(complexity) })}
            onValueCommit={(complexity) => commitRowSettings({ complexity: clampPercent(complexity) })}
          />
          <span class="seed-param-value" aria-hidden="true">{displaySettings.complexity}</span>
        </div>
      </label>

      <label class="grid gap-0.5" style={seedParamAccentStyle}>
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Randomness</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={displaySettings.randomness}
            max={100}
            disabled={busy}
            ariaLabel="Randomness"
            onGestureStart={onGestureStart}
            onValuePreview={(randomness) => previewRowSettings({ randomness: clampPercent(randomness) })}
            onValueCommit={(randomness) => commitRowSettings({ randomness: clampPercent(randomness) })}
          />
          <span class="seed-param-value" aria-hidden="true">{displaySettings.randomness}</span>
        </div>
      </label>

      <div class="flex items-center justify-between gap-3">
        <label class="flex shrink-0 items-center gap-2 text-xs font-semibold text-text">
          <input
            type="checkbox"
            checked={displaySettings.symmetry}
            disabled={busy}
            class="h-3.5 w-3.5 accent-[var(--color-accent)] disabled:opacity-40"
            onchange={(event) => commitRowSettings({ symmetry: event.currentTarget.checked })}
          />
          <span>Symmetry</span>
        </label>
        <div class="flex shrink-0 items-center justify-end gap-2">
          <button
            type="button"
            data-cursor="pointer"
            disabled={busy}
            class="h-8 border border-border bg-surface px-3 text-xs font-semibold uppercase tracking-[0.16em] text-text outline-none transition-colors hover:border-accent/70 focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
            onclick={onNextSeed}
          >
            New notes
          </button>
          <button
            type="button"
            data-cursor="pointer"
            disabled={busy}
            class="h-8 border border-border bg-surface px-3 text-xs font-semibold uppercase tracking-[0.16em] text-text outline-none transition-colors hover:border-accent/70 focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
            onclick={onShuffle}
          >
            Randomize
          </button>
        </div>
      </div>
    </div>
  </div>

  <div class="flex min-h-0 min-w-0 flex-col self-stretch">
    <SeedPhrasePreview {preview} {activeGates} {rowMuted} {onRowMuteToggle} compact={stretchStepsToFit} />
  </div>
</div>
