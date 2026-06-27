<script>
  import AccentRangeSlider from "./AccentRangeSlider.svelte";
  import SeedPhrasePreview from "./SeedPhrasePreview.svelte";
  import {
    defaultSeedingSettings,
    generateSeededPhraseRows,
    seedingPhraseLengthMax,
    seedingPhraseLengthMin,
    seedingRangeSemitonesMax,
    seedingRangeSemitonesMin,
    seedingRhythmStepMax,
    seedingRhythmStepMin,
  } from "./seeding.js";

  let {
    settings = { ...defaultSeedingSettings },
    root = 0,
    modeIndex = 0,
    activeGates = [],
    rowMuted = [false, false, false, false],
    busy = false,
    onGestureStart = () => {},
    onSettingsPreview = () => {},
    onSettingsCommit = () => {},
    onShuffle = () => {},
    onNextSeed = () => {},
    onRowMuteToggle = () => {},
  } = $props();

  let preview = $derived(generateSeededPhraseRows({
    ...settings,
    root,
    modeIndex,
  }));

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

</script>

<div class="grid min-h-[29rem] items-stretch gap-5 border-y border-border-subtle bg-surface/20 py-4 pl-4 pr-0 sm:pl-5 lg:grid-cols-[minmax(20rem,25rem)_minmax(0,1fr)]">
  <div class="grid content-start gap-4">
    <div class="grid gap-3">
      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Phrase length</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={settings.phraseLength}
            min={seedingPhraseLengthMin}
            max={seedingPhraseLengthMax}
            disabled={busy}
            ariaLabel="Phrase length"
            onGestureStart={onGestureStart}
            onValuePreview={(phraseLength) => previewSettings({ phraseLength })}
            onValueCommit={(phraseLength) => commitSettings({ phraseLength })}
          />
          <span class="seed-param-value" aria-hidden="true">{settings.phraseLength}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Range</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={settings.rangeSemitones}
            min={seedingRangeSemitonesMin}
            max={seedingRangeSemitonesMax}
            disabled={busy}
            ariaLabel="Range in semitones"
            onGestureStart={onGestureStart}
            onValuePreview={(rangeSemitones) => previewSettings({ rangeSemitones })}
            onValueCommit={(rangeSemitones) => commitSettings({ rangeSemitones })}
          />
          <span class="seed-param-value" aria-hidden="true">{settings.rangeSemitones} st</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Overlap</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={settings.rhythmStep}
            min={seedingRhythmStepMin}
            max={seedingRhythmStepMax}
            disabled={busy}
            ariaLabel="Rhythm overlap to interleave"
            onGestureStart={onGestureStart}
            onValuePreview={(rhythmStep) => previewSettings({ rhythmStep })}
            onValueCommit={(rhythmStep) => commitSettings({ rhythmStep })}
          />
          <span class="seed-param-value" aria-hidden="true">{settings.rhythmStep}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Repetition</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={settings.repetition}
            max={100}
            disabled={busy}
            ariaLabel="Repetition"
            onGestureStart={onGestureStart}
            onValuePreview={(repetition) => previewSettings({ repetition: clampPercent(repetition) })}
            onValueCommit={(repetition) => commitSettings({ repetition: clampPercent(repetition) })}
          />
          <span class="seed-param-value" aria-hidden="true">{settings.repetition}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Complexity</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={settings.complexity}
            max={100}
            disabled={busy}
            ariaLabel="Complexity"
            onGestureStart={onGestureStart}
            onValuePreview={(complexity) => previewSettings({ complexity: clampPercent(complexity) })}
            onValueCommit={(complexity) => commitSettings({ complexity: clampPercent(complexity) })}
          />
          <span class="seed-param-value" aria-hidden="true">{settings.complexity}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Randomness</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={settings.randomness}
            max={100}
            disabled={busy}
            ariaLabel="Randomness"
            onGestureStart={onGestureStart}
            onValuePreview={(randomness) => previewSettings({ randomness: clampPercent(randomness) })}
            onValueCommit={(randomness) => commitSettings({ randomness: clampPercent(randomness) })}
          />
          <span class="seed-param-value" aria-hidden="true">{settings.randomness}</span>
        </div>
      </label>

      <div class="flex items-center justify-between gap-3">
        <label class="flex shrink-0 items-center gap-2 text-xs font-semibold text-text">
          <input
            type="checkbox"
            checked={settings.symmetry}
            disabled={busy}
            class="h-3.5 w-3.5 accent-[var(--color-accent)] disabled:opacity-40"
            onchange={(event) => commitSettings({ symmetry: event.currentTarget.checked })}
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
            New seed
          </button>
          <button
            type="button"
            data-cursor="pointer"
            disabled={busy}
            class="h-8 border border-accent bg-accent px-4 text-xs font-semibold uppercase tracking-[0.16em] text-control-primary-text outline-none transition-opacity hover:opacity-90 focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
            onclick={onShuffle}
          >
            Shuffle
          </button>
        </div>
      </div>
    </div>
  </div>

  <div class="flex min-h-0 min-w-0 flex-col self-stretch">
    <SeedPhrasePreview {preview} {activeGates} {rowMuted} {onRowMuteToggle} />
  </div>
</div>
