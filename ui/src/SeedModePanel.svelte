<script>
  import AccentRangeSlider from "./AccentRangeSlider.svelte";
  import RowRandomizeOrderIcon from "./RowRandomizeOrderIcon.svelte";
  import SeedRowTargetRail from "./SeedRowTargetRail.svelte";
  import { inspectorTextButtonClasses } from "./inspectorSidebar.js";
  import {
    applySeedingRowSettingsUpdate,
    defaultSeedModeState,
    displaySeedingRowSettings,
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
    seedingTimingMultiplierMaxIndex,
    seedingTimingMultiplierMinIndex,
  } from "./seeding.js";
  import { rowAccentFor, rowAccentScopeStyle } from "./rowAccentTheme.js";
  import { multiplierLabelForIndex, timingMultiplierOptions } from "./stepCellLayout.js";

  /** @typedef {import("./seeding.js").SeedingRowSettings} SeedingRowSettings */

  let {
    rhythmStep = defaultSeedModeState.rhythmStep,
    rowSettings = defaultSeedModeState.rowSettings,
    rowTargets = defaultSeedModeState.rowTargets,
    root = 0,
    modeIndex = 0,
    rowColorsEnabled = true,
    busy = false,
    onGestureStart = () => {},
    onRhythmPreview = () => {},
    onRhythmCommit = () => {},
    onRowSettingsPreview = () => {},
    onRowSettingsCommit = () => {},
    onShuffle = () => {},
    onNextSeed = () => {},
    onReshuffleAspect = () => {},
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
  let timingMeanLabel = $derived(
    multiplierLabelForIndex(displaySettings.timingMeanMultiplierIndex, timingMultiplierOptions),
  );

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
  /** @param {import("./seeding.js").SeedingReshuffleableAspect} aspect */
  function reshuffleAspect(aspect) {
    onReshuffleAspect(aspect);
  }

</script>

<aside class="seed-mode-panel grid w-[25rem] max-w-[42vw] shrink-0 content-start gap-2 self-stretch border-r border-border-subtle bg-transparent pt-2 pb-4 pr-4 pl-0 sm:pr-5">
  <div class="grid gap-2">
    <label class="seed-param-control">
      <span class="seed-param-label">Global Overlap</span>
      <div class="seed-param-row">
        <AccentRangeSlider
          value={rhythmStep}
          min={seedingRhythmStepMin}
          max={seedingRhythmStepMax}
          disabled={busy}
          ariaLabel="Global rhythm overlap to interleave"
          onGestureStart={onGestureStart}
          onValuePreview={onRhythmPreview}
          onValueCommit={onRhythmCommit}
        />
        <span class="seed-param-value" aria-hidden="true">{rhythmStep}</span>
      </div>
    </label>

    <div
      class="-mr-4 border-t border-border-subtle sm:-mr-5"
      role="presentation"
      aria-hidden="true"
    ></div>

    <SeedRowTargetRail
      class="min-w-0 pb-1"
      {rowTargets}
      {rowColorsEnabled}
      {busy}
      {onRowTargetToggle}
      {onToggleAllRowTargets}
    />

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Steps</span>
      <div class="seed-param-row">
        <AccentRangeSlider
          value={displaySettings.phraseLength}
          min={seedingPhraseLengthMin}
          max={seedingPhraseLengthMax}
          disabled={busy}
          ariaLabel="Steps"
          onGestureStart={onGestureStart}
          onValuePreview={(phraseLength) => previewRowSettings({ phraseLength })}
          onValueCommit={(phraseLength) => commitRowSettings({ phraseLength })}
        />
        <span class="seed-param-value" aria-hidden="true">{displaySettings.phraseLength}</span>
      </div>
    </label>

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Length Avg</span>
      <div class="seed-param-row">
        <AccentRangeSlider
          value={displaySettings.timingMeanMultiplierIndex}
          min={seedingTimingMultiplierMinIndex}
          max={seedingTimingMultiplierMaxIndex}
          disabled={busy}
          ariaLabel="Seeded step length average"
          onGestureStart={onGestureStart}
          onValuePreview={(timingMeanMultiplierIndex) => previewRowSettings({ timingMeanMultiplierIndex })}
          onValueCommit={(timingMeanMultiplierIndex) => commitRowSettings({ timingMeanMultiplierIndex })}
        />
        <span class="seed-param-value" aria-hidden="true">{timingMeanLabel}</span>
      </div>
    </label>

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Length Var</span>
      <div class="seed-param-row">
        <AccentRangeSlider
          value={displaySettings.timingVariance}
          max={100}
          disabled={busy}
          ariaLabel="Seeded step length variance"
          onGestureStart={onGestureStart}
          onValuePreview={(timingVariance) => previewRowSettings({ timingVariance: clampPercent(timingVariance) })}
          onValueCommit={(timingVariance) => commitRowSettings({ timingVariance: clampPercent(timingVariance) })}
        />
        <button
          type="button"
          class="seed-param-value seed-param-value--reshuffle"
          disabled={busy}
          aria-label="Re-shuffle length variance"
          title="Re-shuffle length variance"
          onclick={() => reshuffleAspect("timingVariance")}
        >
          {displaySettings.timingVariance}
        </button>
      </div>
    </label>

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Center</span>
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

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Range</span>
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

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Repetition</span>
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
        <button
          type="button"
          class="seed-param-value seed-param-value--reshuffle"
          disabled={busy}
          aria-label="Re-shuffle repetition"
          title="Re-shuffle repetition"
          onclick={() => reshuffleAspect("repetition")}
        >
          {displaySettings.repetition}
        </button>
      </div>
    </label>

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Complexity</span>
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
        <button
          type="button"
          class="seed-param-value seed-param-value--reshuffle"
          disabled={busy}
          aria-label="Re-shuffle complexity"
          title="Re-shuffle complexity"
          onclick={() => reshuffleAspect("complexity")}
        >
          {displaySettings.complexity}
        </button>
      </div>
    </label>

    <label class="seed-param-control" style={seedParamAccentStyle}>
      <span class="seed-param-label">Randomness</span>
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
        <button
          type="button"
          class="seed-param-value seed-param-value--reshuffle"
          disabled={busy}
          aria-label="Re-shuffle randomness"
          title="Re-shuffle randomness"
          onclick={() => reshuffleAspect("randomness")}
        >
          {displaySettings.randomness}
        </button>
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
          class="{inspectorTextButtonClasses({ disabled: busy })} gap-1.5"
          onclick={onNextSeed}
        >
          <RowRandomizeOrderIcon class="pointer-events-none h-3.5 w-3.5 shrink-0" />
          Steps
        </button>
        <button
          type="button"
          data-cursor="pointer"
          disabled={busy}
          aria-label="Randomize all seed settings"
          title="Randomize all seed settings"
          class="{inspectorTextButtonClasses({ disabled: busy })} gap-1.5"
          onclick={onShuffle}
        >
          <RowRandomizeOrderIcon class="pointer-events-none h-3.5 w-3.5 shrink-0" />
          All
        </button>
      </div>
    </div>
  </div>
</aside>
