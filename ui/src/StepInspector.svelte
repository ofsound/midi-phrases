<script>
  import AccentRangeSlider from "./AccentRangeSlider.svelte";
  import CyclePatternEditor from "./CyclePatternEditor.svelte";
  import StepInspectorKeyboard from "./StepInspectorKeyboard.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import StepMuteIcon from "./StepMuteIcon.svelte";
  import StepSkipIcon from "./StepSkipIcon.svelte";
  import { emeraldRowAccent, rowAccentScopeStyle } from "./rowAccentTheme.js";
  import {
    formatTimingMultiplierLabel,
    defaultStepTimingMultiplierIndex,
    stepTimingMultiplierCount,
    timingMultiplierAtIndex,
  } from "./stepCellLayout.js";
  import { inspectorToggleClasses } from "./inspectorSidebar.js";
  import { maxPercentValue } from "./percentLimits.js";

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {number} [step]
   * @property {number} [bulkEditStepCount]
   * @property {number} [note]
   * @property {number} [velocity]
   * @property {number} [durationFraction]
   * @property {number} [timingMultiplierIndex]
   * @property {number} [probability]
   * @property {number} [cycle]
   * @property {number} [cycleMask]
   * @property {number} [cycleTriggerCount]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {boolean} [skipped]
   * @property {(midi: number) => void | Promise<void>} [onNoteChange]
   * @property {(value: number) => void | Promise<void>} [onVelocityChange]
   * @property {(value: number) => void | Promise<void>} [onDurationChange]
   * @property {(value: number) => void | Promise<void>} [onTimingMultiplierChange]
   * @property {() => void} [onProbabilityGestureStart]
   * @property {(value: number) => void | Promise<void>} [onProbabilityPreview]
   * @property {(value: number) => void | Promise<void>} [onProbabilityCommit]
   * @property {() => void} [onCycleGestureStart]
   * @property {(cycle: number, cycleMask: number) => void | Promise<void>} [onCyclePatternPreview]
   * @property {(cycle: number, cycleMask: number) => void | Promise<void>} [onCyclePatternCommit]
   * @property {(value: boolean) => void | Promise<void>} [onMutedChange]
   * @property {(value: boolean) => void | Promise<void>} [onSkippedChange]
   * @property {() => void | Promise<void>} [onRemove]
   */

  /** @type {Props} */
  let {
    row = 0,
    step = 0,
    bulkEditStepCount = 1,
    note = 60,
    velocity = 127,
    durationFraction = 1,
    timingMultiplierIndex = defaultStepTimingMultiplierIndex,
    probability = maxPercentValue,
    cycle = 1,
    cycleMask = 1,
    cycleTriggerCount = -1,
    scaleRoot = 0,
    scaleModeIndex = 0,
    accent = emeraldRowAccent,
    muted = false,
    skipped = false,
    onNoteChange = () => {},
    onVelocityChange = () => {},
    onDurationChange = () => {},
    onTimingMultiplierChange = () => {},
    onProbabilityGestureStart = () => {},
    onProbabilityPreview = () => {},
    onProbabilityCommit = () => {},
    onCycleGestureStart = () => {},
    onCyclePatternPreview = () => {},
    onCyclePatternCommit = () => {},
    onMutedChange = () => {},
    onSkippedChange = () => {},
    onRemove = () => {},
  } = $props();

  let stepKey = $derived(`${row}:${step}`);
  let durationPercent = $derived(Math.round(Math.min(1, Math.max(0, durationFraction)) * 100));
  let multiplierDisplay = $derived(
    formatTimingMultiplierLabel(timingMultiplierAtIndex(timingMultiplierIndex)),
  );
</script>

<section class="step-inspector flex min-h-0 w-full flex-1 overflow-hidden bg-app/90">
  <aside class="inspector-sidebar flex w-[13rem] shrink-0 flex-col gap-2 py-2 pr-3 pl-0">
    <div class="inspector-toggles flex w-full gap-1.5">
        <button
          type="button"
          data-cursor="pointer"
          aria-label={skipped ? "Unskip step in sequence" : "Skip step in sequence"}
          aria-pressed={skipped}
          title={skipped ? "Unskip step in sequence" : "Skip step in sequence"}
          class={inspectorToggleClasses(accent, skipped)}
          onclick={() => onSkippedChange(!skipped)}
        >
          <StepSkipIcon class="pointer-events-none h-4 w-4 shrink-0" />
          <span class="truncate text-[9px] font-semibold uppercase tracking-wide">Skip</span>
        </button>
        <button
          type="button"
          data-cursor="pointer"
          aria-label={muted ? "Unmute step" : "Mute step"}
          aria-pressed={muted}
          title={muted ? "Unmute step" : "Mute step"}
          class={inspectorToggleClasses(accent, muted)}
          onclick={() => onMutedChange(!muted)}
        >
          <StepMuteIcon class="pointer-events-none h-4 w-4 shrink-0" />
          <span class="truncate text-[9px] font-semibold uppercase tracking-wide">Mute</span>
        </button>
    </div>

    <div
      class="inspector-sliders flex min-h-0 w-full flex-1 flex-col justify-center gap-3"
      style={rowAccentScopeStyle(accent)}
    >
      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Multiplier</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={timingMultiplierIndex}
            min={0}
            max={stepTimingMultiplierCount - 1}
            ariaLabel="Step timing multiplier"
            onValuePreview={onTimingMultiplierChange}
            onValueCommit={onTimingMultiplierChange}
          />
          <span class="seed-param-value" aria-hidden="true">{multiplierDisplay}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Duration</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={durationPercent}
            min={0}
            max={maxPercentValue}
            ariaLabel="Step duration"
            onValuePreview={onDurationChange}
            onValueCommit={onDurationChange}
          />
          <span class="seed-param-value" aria-hidden="true">{durationPercent}</span>
        </div>
      </label>

      <label class="grid gap-1">
        <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Velocity</span>
        <div class="seed-param-row">
          <AccentRangeSlider
            value={velocity}
            min={1}
            max={127}
            ariaLabel="Step velocity"
            onValuePreview={onVelocityChange}
            onValueCommit={onVelocityChange}
          />
          <span class="seed-param-value" aria-hidden="true">{velocity}</span>
        </div>
      </label>
    </div>

    <div class="inspector-actions flex h-8 w-full shrink-0">
      <button
        type="button"
        data-cursor="pointer"
        aria-label="Remove step"
        title="Remove step"
        class="flex w-full items-center justify-center gap-1.5 rounded-md border border-border mp-control-gradient px-2 text-[9px] font-semibold uppercase tracking-wide text-text-secondary transition-[background-color,border-color,color] outline-none hover:border-danger hover:text-danger focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={onRemove}
      >
        <RemoveXIcon class="pointer-events-none h-3 w-3 shrink-0" />
        Delete
      </button>
    </div>
  </aside>

  <div class="inspector-main grid min-h-0 min-w-0 flex-1 grid-rows-[minmax(4rem,28%)_minmax(0,1fr)] content-start gap-y-2 overflow-hidden pb-2">
    <div class="flex min-h-0 min-w-0 flex-col items-center justify-center gap-1 bg-surface/15 px-3 py-2">
      {#if bulkEditStepCount > 1}
        <p
          class="text-[10px] font-semibold uppercase tracking-[0.12em] text-text-muted"
          aria-live="polite"
        >
          {bulkEditStepCount} steps inspected
        </p>
      {/if}
      <div class="grid w-full max-w-[46rem] min-w-0 grid-cols-2 items-end gap-x-20">
        <div class="flex min-h-0 min-w-0 flex-col justify-center gap-1">
          <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Cycle</span>
          <CyclePatternEditor
            {accent}
            compact
            borderless
            {cycle}
            {cycleMask}
            playbackKey={stepKey}
            playbackTriggerCount={cycleTriggerCount}
            ariaLabel="Step cycle pattern"
            onGestureStart={onCycleGestureStart}
            onPatternPreview={onCyclePatternPreview}
            onPatternCommit={onCyclePatternCommit}
          />
        </div>

        <div class="flex min-h-0 min-w-0 w-full flex-col justify-center" style={rowAccentScopeStyle(accent)}>
          <label class="grid gap-1">
            <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Probability</span>
            <div class="seed-param-row">
              <AccentRangeSlider
                value={probability}
                min={0}
                max={maxPercentValue}
                ariaLabel="Step probability"
                onGestureStart={onProbabilityGestureStart}
                onValuePreview={onProbabilityPreview}
                onValueCommit={onProbabilityCommit}
              />
              <span class="seed-param-value" aria-hidden="true">{Math.round(probability)}%</span>
            </div>
          </label>
        </div>
      </div>
    </div>

    <div class="flex min-h-0 min-w-0 overflow-hidden">
      <StepInspectorKeyboard
        {note}
        {stepKey}
        {scaleRoot}
        {scaleModeIndex}
        {accent}
        onNoteChange={onNoteChange}
      />
    </div>
  </div>
</section>
