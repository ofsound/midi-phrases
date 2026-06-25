<script>
  import {
    defaultSeedingSettings,
    seedingPhraseLengthMax,
    seedingPhraseLengthMin,
    seedingPresets,
    seedingRangeOptions,
    seedingRhythmOptions,
  } from "./seeding.js";

  let {
    settings = { ...defaultSeedingSettings },
    activeScaleName = "C Chromatic",
    busy = false,
    onGestureStart = () => {},
    onSettingsPreview = () => {},
    onSettingsCommit = () => {},
    onShuffle = () => {},
    onNextSeed = () => {},
  } = $props();

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

<div class="grid min-h-[20rem] gap-4 border-y border-border-subtle bg-surface/20 px-4 py-4 sm:px-5">
  <div class="flex flex-wrap items-center justify-between gap-3">
    <div class="min-w-0">
      <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Seed mode</p>
      <h2 class="mt-1 truncate text-lg font-semibold text-text">{activeScaleName}</h2>
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

  <div class="grid gap-4 lg:grid-cols-[minmax(12rem,16rem)_minmax(0,1fr)_minmax(10rem,13rem)]">
    <div class="grid content-start gap-2">
      {#each seedingPresets as preset (preset.id)}
        <button
          type="button"
          aria-pressed={settings.presetId === preset.id}
          data-cursor="pointer"
          disabled={busy}
          class="h-10 border px-3 text-left text-sm font-semibold outline-none transition-colors focus:ring-1 focus:ring-focus-ring disabled:opacity-40 {settings.presetId === preset.id
            ? 'border-accent bg-accent text-control-primary-text'
            : 'border-border bg-surface text-text hover:border-border-strong'}"
          onclick={() => applyPreset(preset)}
        >
          {preset.label}
        </button>
      {/each}
    </div>

    <div class="grid gap-3 sm:grid-cols-3">
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

    <div class="grid content-start gap-2">
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
      <div class="mt-1 truncate border border-border-subtle bg-surface/50 px-3 py-2 font-mono text-xs text-text-muted">
        {settings.seed}
      </div>
    </div>
  </div>
</div>
