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
    seedingRhythmOptions,
  } from "./seeding.js";

  let {
    settings = { ...defaultSeedingSettings },
    root = 0,
    modeIndex = 0,
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

<div class="grid min-h-[29rem] gap-5 border-y border-border-subtle bg-surface/20 px-4 py-4 sm:px-5 lg:grid-cols-[minmax(20rem,25rem)_minmax(0,1fr)]">
  <div class="grid content-start gap-4">
    <div class="grid gap-3">
      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Phrase length</span><span class="font-mono text-accent">{settings.phraseLength}</span>
        </span>
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
        <div class="flex justify-between font-mono text-[9px] leading-none text-text-muted">
          <span>{seedingPhraseLengthMin}</span>
          <span>{seedingPhraseLengthMax}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Range</span><span class="font-mono text-accent">{settings.rangeSemitones} st</span>
        </span>
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
        <div class="flex justify-between font-mono text-[9px] leading-none text-text-muted">
          <span>{seedingRangeSemitonesMin} st</span>
          <span>4 oct</span>
        </div>
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
        <AccentRangeSlider
          value={settings.repetition}
          max={100}
          disabled={busy}
          ariaLabel="Repetition"
          onGestureStart={onGestureStart}
          onValuePreview={(repetition) => previewSettings({ repetition: clampPercent(repetition) })}
          onValueCommit={(repetition) => commitSettings({ repetition: clampPercent(repetition) })}
        />
      </label>

      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Complexity</span><span class="font-mono text-accent">{settings.complexity}</span>
        </span>
        <AccentRangeSlider
          value={settings.complexity}
          max={100}
          disabled={busy}
          ariaLabel="Complexity"
          onGestureStart={onGestureStart}
          onValuePreview={(complexity) => previewSettings({ complexity: clampPercent(complexity) })}
          onValueCommit={(complexity) => commitSettings({ complexity: clampPercent(complexity) })}
        />
      </label>

      <label class="grid gap-1">
        <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
          <span>Randomness</span><span class="font-mono text-accent">{settings.randomness}</span>
        </span>
        <AccentRangeSlider
          value={settings.randomness}
          max={100}
          disabled={busy}
          ariaLabel="Randomness"
          onGestureStart={onGestureStart}
          onValuePreview={(randomness) => previewSettings({ randomness: clampPercent(randomness) })}
          onValueCommit={(randomness) => commitSettings({ randomness: clampPercent(randomness) })}
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
    <div class="flex shrink-0 items-center justify-end gap-2">
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

    <SeedPhrasePreview {preview} {rowMuted} {onRowMuteToggle} />
  </div>
</div>
