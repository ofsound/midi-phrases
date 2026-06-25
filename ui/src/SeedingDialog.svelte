<script>
  import { SvelteMap } from "svelte/reactivity";
  import ScalePreviewKeyboard from "./ScalePreviewKeyboard.svelte";
  import PhraseRow from "./PhraseRow.svelte";
  import { defaultStepNoteForScaleRoot } from "./midiNoteNames.js";
  import {
    phraseGridOriginLeftOffsetPx,
    phraseGridVisualOffsetCompensationPx,
    phraseRowInterRowGapPx,
    phraseRowMinHeightPx,
    phraseRowScrollPaddingRightPx,
  } from "./phraseRowLayout.js";
  import {
    rowAccentFor,
  } from "./rowAccentTheme.js";
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
  import { keyCenters, scaleModes, scaleName } from "./scaleUtils.js";

  let {
    root = 0,
    modeIndex = 0,
    patternSlot = 0,
    busy = false,
    scaleLocked = false,
    confirmLabel = "Start Seeded",
    onClose = () => {},
    onStartBlank = () => {},
    onSeed = () => {},
  } = $props();

  let titleId = "seeding-title";
  let step = $state("scale");
  let draftRoot = $state(0);
  let draftModeIndex = $state(0);
  let settings = $state({ ...defaultSeedingSettings });
  let previewViewportWidth = $state(0);
  let previewViewportHeight = $state(0);
  const previewViewportPaddingPx = 12;

  function initialRoot() {
    return root;
  }

  function initialModeIndex() {
    return modeIndex;
  }

  function initialStep() {
    return scaleLocked ? "seed" : "scale";
  }

  draftRoot = initialRoot();
  draftModeIndex = initialModeIndex();
  step = initialStep();

  let activeScaleName = $derived(scaleName(draftRoot, draftModeIndex));
  let preview = $derived(generateSeededPhraseRows({
    ...settings,
    root: draftRoot,
    modeIndex: draftModeIndex,
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
  let previewDefaultStepNote = $derived(defaultStepNoteForScaleRoot(draftRoot));
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

  function closeOnEscape(event) {
    if (event.key !== "Escape") return;

    event.preventDefault();
    onClose();
  }

  /** @param {number} value */
  function clampPercent(value) {
    return Math.min(100, Math.max(0, Math.round(value)));
  }

  function nextSeed() {
    settings.seed = Math.max(1, Math.floor(Math.random() * 2147483646));
  }

  function shuffleSettings() {
    settings.repetition = clampPercent(18 + Math.random() * 70);
    settings.complexity = clampPercent(20 + Math.random() * 72);
    settings.randomness = clampPercent(24 + Math.random() * 68);
    settings.symmetry = Math.random() > 0.62;
    settings.rhythmMode = Math.random() > 0.5 ? "overlap" : "interleave";
    nextSeed();
  }

  /** @param {{ id: string, settings: Partial<typeof defaultSeedingSettings> }} preset */
  function applyPreset(preset) {
    settings = {
      ...settings,
      ...preset.settings,
      presetId: preset.id,
    };
    nextSeed();
  }

  /** @param {number} rootValue @param {number} modeValue */
  function setDraftScale(rootValue, modeValue) {
    draftRoot = rootValue;
    draftModeIndex = modeValue;
  }

  function startBlank() {
    onStartBlank({
      root: draftRoot,
      modeIndex: draftModeIndex,
    });
  }

  function startSeeded() {
    onSeed({
      root: draftRoot,
      modeIndex: draftModeIndex,
      settings: { ...settings },
      generated: preview,
    });
  }
</script>

<svelte:window onkeydown={closeOnEscape} />

<div class="fixed inset-0 z-[80] flex items-center justify-center bg-app/45 p-4 backdrop-blur-[14px] sm:p-6">
  <button
    type="button"
    aria-label="Dismiss seeding"
    class="absolute inset-0 border-0 bg-transparent"
    disabled={busy}
    onclick={onClose}
  ></button>

  <div
    role="dialog"
    aria-modal="true"
    aria-labelledby={titleId}
    tabindex="-1"
    class="relative z-[1] flex max-h-[min(94vh,58rem)] w-full max-w-[min(64rem,calc(100vw-2rem))] flex-col overflow-hidden border border-border/90 bg-app/88 shadow-[0_24px_80px_rgba(0,0,0,0.55)] backdrop-blur-[22px]"
  >
    <div class="flex items-start justify-between gap-4 border-b border-border-subtle px-5 py-4 sm:px-6">
      <div class="min-w-0">
        <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Pattern {patternSlot + 1}</p>
        <h2 id={titleId} class="mt-1 text-2xl font-semibold text-text">Seeding</h2>
      </div>
      <button
        type="button"
        aria-label="Close seeding"
        data-cursor="pointer"
        disabled={busy}
        class="flex h-8 w-8 shrink-0 items-center justify-center border border-border bg-surface text-text-secondary outline-none transition-colors hover:border-border-strong hover:text-text focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
        onclick={onClose}
      >
        <svg
          class="h-4 w-4"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
          aria-hidden="true"
        >
          <path d="M18 6 6 18" />
          <path d="m6 6 12 12" />
        </svg>
      </button>
    </div>

    <div class="flex min-h-0 flex-col overflow-y-auto p-5 sm:p-6">
      {#if step === "scale"}
        <div class="grid gap-5">
          <div class="grid gap-3">
            <div>
              <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Key center</p>
              <h3 class="mt-1 text-lg font-semibold text-text">{keyCenters[draftRoot]?.label ?? "C"}</h3>
            </div>
            <div class="grid grid-cols-6 gap-2 sm:grid-cols-12">
              {#each keyCenters as key (key.value)}
                <button
                  type="button"
                  aria-label={`Set key center to ${key.label}`}
                  aria-pressed={draftRoot === key.value}
                  data-cursor="pointer"
                  class="flex h-11 items-center justify-center border text-sm font-semibold outline-none transition-colors focus:ring-1 focus:ring-focus-ring {draftRoot === key.value
                    ? 'border-accent bg-accent text-control-primary-text'
                    : 'border-border bg-surface text-text hover:border-border-strong'}"
                  onclick={() => setDraftScale(key.value, draftModeIndex)}
                >
                  {key.label}
                </button>
              {/each}
            </div>
          </div>

          <div class="grid gap-3">
            <div>
              <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Mode</p>
              <h3 class="mt-1 text-lg font-semibold text-text">{scaleModes[draftModeIndex]?.label ?? "Chromatic"}</h3>
            </div>
            <div class="grid gap-2 sm:grid-cols-2 lg:grid-cols-4">
              {#each scaleModes as mode, index (mode.label)}
                <button
                  type="button"
                  aria-label={`Set mode to ${mode.label}`}
                  aria-pressed={draftModeIndex === index}
                  data-cursor="pointer"
                  class="flex min-h-14 items-center justify-between gap-3 border px-3 py-2 text-left outline-none transition-colors focus:ring-1 focus:ring-focus-ring {draftModeIndex === index
                    ? 'border-accent bg-accent text-control-primary-text'
                    : 'border-border bg-surface text-text hover:border-border-strong'}"
                  onclick={() => setDraftScale(draftRoot, index)}
                >
                  <span class="min-w-0 text-sm font-semibold leading-tight">{mode.label}</span>
                  <span class="shrink-0 text-xs font-bold uppercase opacity-70">{mode.shortLabel}</span>
                </button>
              {/each}
            </div>
          </div>

          <ScalePreviewKeyboard root={draftRoot} modeIndex={draftModeIndex} />
        </div>
      {:else}
        <div class="grid min-h-0 gap-5 lg:grid-cols-[minmax(20rem,24rem)_minmax(0,1fr)]">
          <div class="grid content-start gap-4">
            <div>
              <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Scale</p>
              <h3 class="mt-1 text-lg font-semibold text-text">{activeScaleName}</h3>
            </div>

            <div class="grid grid-cols-2 gap-2">
              {#each seedingPresets as preset (preset.id)}
                <button
                  type="button"
                  aria-pressed={settings.presetId === preset.id}
                  data-cursor="pointer"
                  class="h-10 border px-3 text-sm font-semibold outline-none transition-colors focus:ring-1 focus:ring-focus-ring {settings.presetId === preset.id
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
                  class="w-full accent-[var(--color-accent)]"
                  oninput={(event) => settings.phraseLength = Number(event.currentTarget.value)}
                />
                <div class="flex justify-between font-mono text-[9px] leading-none text-text-muted">
                  <span>{seedingPhraseLengthMin}</span>
                  <span>{seedingPhraseLengthMax}</span>
                </div>
              </label>

              <label class="grid gap-1">
                <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Range</span>
                <select
                  class="h-9 border border-border bg-surface px-2 text-sm font-semibold text-text outline-none focus:ring-1 focus:ring-focus-ring"
                  value={settings.rangeIndex}
                  onchange={(event) => settings.rangeIndex = Number(event.currentTarget.value)}
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
                    class="h-9 border px-3 text-sm font-semibold outline-none transition-colors focus:ring-1 focus:ring-focus-ring {settings.rhythmMode === option.value
                      ? 'border-accent bg-accent text-control-primary-text'
                      : 'border-border bg-surface text-text hover:border-border-strong'}"
                    onclick={() => settings.rhythmMode = option.value}
                  >
                    {option.label}
                  </button>
                {/each}
              </div>
            </div>

            <div class="grid gap-3">
              <label class="grid gap-1">
                <span class="flex items-center justify-between gap-2 text-[9px] font-medium uppercase tracking-wide text-text-muted">
                  <span>Repetition</span><span class="font-mono text-accent">{settings.repetition}</span>
                </span>
                <input
                  type="range"
                  min="0"
                  max="100"
                  value={settings.repetition}
                  class="w-full accent-[var(--color-accent)]"
                  oninput={(event) => settings.repetition = clampPercent(Number(event.currentTarget.value))}
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
                  class="w-full accent-[var(--color-accent)]"
                  oninput={(event) => settings.complexity = clampPercent(Number(event.currentTarget.value))}
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
                  class="w-full accent-[var(--color-accent)]"
                  oninput={(event) => settings.randomness = clampPercent(Number(event.currentTarget.value))}
                />
              </label>
            </div>

            <label class="flex h-10 items-center justify-between gap-3 border border-border bg-surface px-3 text-sm font-semibold text-text">
              <span>Symmetry</span>
              <input
                type="checkbox"
                checked={settings.symmetry}
                class="h-4 w-4 accent-[var(--color-accent)]"
                onchange={(event) => settings.symmetry = event.currentTarget.checked}
              />
            </label>
          </div>

          <div class="grid min-h-0 content-start gap-3">
            <div class="flex items-center justify-between gap-3">
              <div>
                <p class="text-xs font-semibold uppercase tracking-[0.28em] text-text-muted">Preview</p>
                <h3 class="mt-1 text-lg font-semibold text-text">4 phrases</h3>
              </div>
              <button
                type="button"
                data-cursor="pointer"
                class="h-9 shrink-0 border border-border bg-surface px-4 text-xs font-semibold uppercase tracking-[0.16em] text-text outline-none transition-colors hover:border-accent/70 focus:ring-1 focus:ring-focus-ring"
                onclick={shuffleSettings}
              >
                Shuffle
              </button>
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
      {/if}
    </div>

    <div class="flex flex-wrap items-center justify-between gap-3 border-t border-border-subtle px-5 py-4 sm:px-6">
      {#if !scaleLocked}
        <button
          type="button"
          data-cursor="pointer"
          disabled={busy}
          class="h-10 border border-border bg-surface px-4 text-xs font-semibold uppercase tracking-[0.16em] text-text outline-none transition-colors hover:border-border-strong focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
          onclick={startBlank}
        >
          Blank Project
        </button>
      {:else}
        <div class="min-w-0 text-xs font-semibold uppercase tracking-[0.16em] text-text-muted">
          Overwrites pattern {patternSlot + 1}
        </div>
      {/if}

      <div class="flex items-center gap-2">
        {#if step === "seed" && !scaleLocked}
          <button
            type="button"
            data-cursor="pointer"
            disabled={busy}
            class="h-10 border border-border bg-surface px-4 text-xs font-semibold uppercase tracking-[0.16em] text-text outline-none transition-colors hover:border-border-strong focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
            onclick={() => step = "scale"}
          >
            Scale
          </button>
        {/if}

        {#if step === "scale"}
          <button
            type="button"
            data-cursor="pointer"
            class="h-10 border border-accent bg-accent px-5 text-xs font-semibold uppercase tracking-[0.16em] text-control-primary-text outline-none transition-opacity hover:opacity-90 focus:ring-1 focus:ring-focus-ring"
            onclick={() => step = "seed"}
          >
            Seeding
          </button>
        {:else}
          <button
            type="button"
            data-cursor="pointer"
            disabled={busy}
            class="h-10 border border-accent bg-accent px-5 text-xs font-semibold uppercase tracking-[0.16em] text-control-primary-text outline-none transition-opacity hover:opacity-90 focus:ring-1 focus:ring-focus-ring disabled:opacity-40"
            onclick={startSeeded}
          >
            {confirmLabel}
          </button>
        {/if}
      </div>
    </div>
  </div>
</div>
