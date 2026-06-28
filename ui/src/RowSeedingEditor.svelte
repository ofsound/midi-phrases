<script>
  import AccentRangeSlider from "./AccentRangeSlider.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import {
    defaultSeedingRowSettings,
    seedingCenterMidiForIndex,
    seedingCenterNoteIndex,
    seedingCenterNoteLabel,
    seedingCenterNoteOptions,
    seedingPhraseLengthMax,
    seedingPhraseLengthMin,
    seedingRangeSemitonesMax,
    seedingRangeSemitonesMin,
  } from "./seeding.js";
  import { rowAccentScopeStyle } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {number} row
   * @property {import('./seeding.js').SeedingRowSettings} rowSettings
   * @property {number} [root]
   * @property {number} [modeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} accent
   * @property {() => void} [onGestureStart]
   * @property {(updates: Partial<import('./seeding.js').SeedingRowSettings>) => void | Promise<void>} [onRowSettingsPreview]
   * @property {(updates: Partial<import('./seeding.js').SeedingRowSettings>) => void | Promise<void>} [onRowSettingsCommit]
   * @property {() => void} [onShuffle]
   * @property {() => void} [onNextSeed]
   * @property {() => void} [onClose]
   */

  /** @type {Props} */
  let {
    row = 0,
    rowSettings = defaultSeedingRowSettings,
    root = 0,
    modeIndex = 0,
    accent,
    onGestureStart = () => {},
    onRowSettingsPreview = () => {},
    onRowSettingsCommit = () => {},
    onShuffle = () => {},
    onNextSeed = () => {},
    onClose = () => {},
  } = $props();

  /** @param {number} value */
  function clampPercent(value) {
    return Math.min(100, Math.max(0, Math.round(value)));
  }

  /** @param {Partial<import('./seeding.js').SeedingRowSettings>} updates */
  function previewRowSettings(updates) {
    onRowSettingsPreview(updates);
  }

  /** @param {Partial<import('./seeding.js').SeedingRowSettings>} updates */
  function commitRowSettings(updates) {
    onRowSettingsCommit(updates);
  }

  let centerNoteOptions = $derived(seedingCenterNoteOptions(root, modeIndex));
  let centerNoteIndex = $derived(seedingCenterNoteIndex(rowSettings.centerMidi, root, modeIndex));
  let centerNoteLabel = $derived(seedingCenterNoteLabel(rowSettings.centerMidi, root, modeIndex));
</script>

<section class="step-inspector flex min-h-0 w-full flex-1 overflow-hidden bg-app/90" style={rowAccentScopeStyle(accent)}>
  <aside class="inspector-sidebar flex w-[13rem] shrink-0 flex-col gap-3 py-2 pr-3 pl-0">
    <div class="flex items-center">
      <button
        type="button"
        data-cursor="pointer"
        aria-label="Close seeding editor"
        title="Close seeding editor"
        class="flex h-5 w-5 items-center justify-center rounded-md border border-border bg-surface/30 text-text-faint hover:border-border-strong hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={onClose}
      >
        <RemoveXIcon class="h-3 w-3" />
      </button>
    </div>

    <div class="flex flex-col gap-2">
      <button
        type="button"
        data-cursor="pointer"
        class="h-8 border border-border bg-surface/50 px-3 text-xs font-semibold uppercase tracking-[0.16em] text-text outline-none transition-colors hover:border-accent/70 focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={onNextSeed}
      >
        New seed
      </button>
      <button
        type="button"
        data-cursor="pointer"
        class="h-8 border border-accent bg-accent px-4 text-xs font-semibold uppercase tracking-[0.16em] text-control-primary-text outline-none transition-opacity hover:opacity-90 focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={onShuffle}
      >
        Shuffle
      </button>
    </div>
  </aside>

  <div class="inspector-main flex min-h-0 min-w-0 flex-1 flex-col justify-start items-start overflow-hidden pt-3 pb-4 pl-2 pr-4">
    <div class="grid w-full gap-4">
      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Phrase length</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={rowSettings.phraseLength}
            min={seedingPhraseLengthMin}
            max={seedingPhraseLengthMax}
            ariaLabel="Phrase length"
            onGestureStart={onGestureStart}
            onValuePreview={(phraseLength) => previewRowSettings({ phraseLength })}
            onValueCommit={(phraseLength) => commitRowSettings({ phraseLength })}
          />
          <span class="seed-param-value" aria-hidden="true">{rowSettings.phraseLength}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Center</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={centerNoteIndex}
            max={Math.max(0, centerNoteOptions.length - 1)}
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

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Range</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={rowSettings.rangeSemitones}
            min={seedingRangeSemitonesMin}
            max={seedingRangeSemitonesMax}
            ariaLabel="Range in semitones"
            onGestureStart={onGestureStart}
            onValuePreview={(rangeSemitones) => previewRowSettings({ rangeSemitones })}
            onValueCommit={(rangeSemitones) => commitRowSettings({ rangeSemitones })}
          />
          <span class="seed-param-value" aria-hidden="true">{rowSettings.rangeSemitones} st</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Repetition</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={rowSettings.repetition}
            max={100}
            ariaLabel="Repetition"
            onGestureStart={onGestureStart}
            onValuePreview={(repetition) => previewRowSettings({ repetition: clampPercent(repetition) })}
            onValueCommit={(repetition) => commitRowSettings({ repetition: clampPercent(repetition) })}
          />
          <span class="seed-param-value" aria-hidden="true">{rowSettings.repetition}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Complexity</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={rowSettings.complexity}
            max={100}
            ariaLabel="Complexity"
            onGestureStart={onGestureStart}
            onValuePreview={(complexity) => previewRowSettings({ complexity: clampPercent(complexity) })}
            onValueCommit={(complexity) => commitRowSettings({ complexity: clampPercent(complexity) })}
          />
          <span class="seed-param-value" aria-hidden="true">{rowSettings.complexity}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Randomness</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={rowSettings.randomness}
            max={100}
            ariaLabel="Randomness"
            onGestureStart={onGestureStart}
            onValuePreview={(randomness) => previewRowSettings({ randomness: clampPercent(randomness) })}
            onValueCommit={(randomness) => commitRowSettings({ randomness: clampPercent(randomness) })}
          />
          <span class="seed-param-value" aria-hidden="true">{rowSettings.randomness}</span>
        </div>
      </label>

      <div class="flex items-center mt-2">
        <label class="flex shrink-0 items-center gap-2 text-xs font-semibold text-text">
          <input
            type="checkbox"
            checked={rowSettings.symmetry}
            class="h-3.5 w-3.5 accent-[var(--color-accent)] cursor-pointer"
            onchange={(event) => commitRowSettings({ symmetry: event.currentTarget.checked })}
          />
          <span>Symmetry</span>
        </label>
      </div>
    </div>
  </div>
</section>
