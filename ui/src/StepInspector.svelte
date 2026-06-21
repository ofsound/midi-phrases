<script>
  import CyclePatternEditor from "./CyclePatternEditor.svelte";
  import ContinuousSlider from "./ContinuousSlider.svelte";
  import StepInspectorKeyboard from "./StepInspectorKeyboard.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import StepMuteIcon from "./StepMuteIcon.svelte";
  import StepSkipIcon from "./StepSkipIcon.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    formatTimingMultiplierLabel,
    defaultStepTimingMultiplierIndex,
    stepTimingMultiplierCount,
    timingMultiplierAtIndex,
  } from "./stepCellLayout.js";

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {number} [step]
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
   * @property {() => void} [onClose]
   */

  /** @type {Props} */
  let {
    row = 0,
    step = 0,
    note = 60,
    velocity = 127,
    durationFraction = 1,
    timingMultiplierIndex = defaultStepTimingMultiplierIndex,
    probability = 100,
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
    onClose = () => {},
  } = $props();

  let stepKey = $derived(`${row}:${step}`);
  let durationPercent = $derived(Math.round(Math.min(1, Math.max(0, durationFraction)) * 100));

  /** @param {boolean} active */
  function inspectorToggleClasses(active) {
    const base = `flex h-9 min-w-0 flex-1 items-center justify-center gap-1.5 rounded-md border px-2 transition-[background-color,border-color,color,box-shadow] outline-none ${accent.ringFocusWithWidth}`;

    if (active) {
      return `${base} ${accent.bgAccentStrong} ${accent.borderFocus} border text-white ${accent.playbackGlow}`;
    }

    return `${base} mp-control-gradient border-border text-text-secondary hover:border-border-strong hover:text-text`;
  }
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
          class={inspectorToggleClasses(skipped)}
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
          class={inspectorToggleClasses(muted)}
          onclick={() => onMutedChange(!muted)}
        >
          <StepMuteIcon class="pointer-events-none h-4 w-4 shrink-0" />
          <span class="truncate text-[9px] font-semibold uppercase tracking-wide">Mute</span>
        </button>
    </div>

    <div class="inspector-sliders flex min-h-0 w-full flex-1 flex-col justify-center gap-1.5">
      <ContinuousSlider
        {accent}
        label="Multiplier"
        value={timingMultiplierIndex}
        min={0}
        max={stepTimingMultiplierCount - 1}
        ariaLabel="Step timing multiplier"
        fullWidth={true}
        formatDisplay={(index) =>
          formatTimingMultiplierLabel(timingMultiplierAtIndex(index))}
        onValueChange={onTimingMultiplierChange}
      />
      <ContinuousSlider
        {accent}
        label="Duration"
        value={durationPercent}
        min={0}
        max={100}
        ariaLabel="Step duration"
        fullWidth={true}
        onValueChange={onDurationChange}
      />
      <ContinuousSlider
        {accent}
        label="Velocity"
        value={velocity}
        min={1}
        max={127}
        ariaLabel="Step velocity"
        fullWidth={true}
        onValueChange={onVelocityChange}
      />
    </div>

    <div class="inspector-actions flex h-8 w-full shrink-0 gap-1.5">
      <button
        type="button"
        data-cursor="pointer"
        aria-label="Remove step"
        title="Remove step"
        class="flex min-w-0 flex-1 items-center justify-center gap-1.5 rounded-md border border-border mp-control-gradient px-2 text-[9px] font-semibold uppercase tracking-wide text-text-secondary transition-[background-color,border-color,color] outline-none hover:border-danger hover:text-danger focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={onRemove}
      >
        <RemoveXIcon class="pointer-events-none h-3 w-3 shrink-0" />
        Delete
      </button>
      <button
        type="button"
        data-cursor="pointer"
        aria-label="Close step inspector"
        class="flex w-8 shrink-0 items-center justify-center rounded-md border border-border bg-surface/50 p-0 text-text-muted transition-colors outline-none hover:border-border-strong hover:bg-surface-raised/80 hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={onClose}
        title="Close step inspector"
      >
        <RemoveXIcon class="pointer-events-none h-3.5 w-3.5" />
      </button>
    </div>
  </aside>

  <div class="inspector-main grid min-h-0 min-w-0 flex-1 grid-rows-[minmax(4rem,28%)_minmax(0,1fr)] content-start gap-y-2 overflow-hidden pb-2">
    <div class="flex min-h-0 min-w-0 items-center justify-center bg-surface/15 px-3 py-2">
      <div class="grid w-full max-w-[46rem] min-w-0 grid-cols-[minmax(0,1.5fr)_minmax(8rem,1fr)] gap-x-4">
        <div class="flex min-h-0 min-w-0 flex-col justify-center gap-1">
          <span class="text-[11px] font-medium uppercase tracking-wide text-text-muted">Cycle</span>
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

        <div class="flex min-h-0 min-w-0 items-center">
          <ContinuousSlider
            {accent}
            label="Probability"
            value={probability}
            min={0}
            max={100}
            ariaLabel="Step probability"
            fullWidth={true}
            formatDisplay={(value) => `${Math.round(value)}%`}
            onGestureStart={onProbabilityGestureStart}
            onValueChange={onProbabilityPreview}
            onValueCommit={onProbabilityCommit}
          />
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

<style>
  .step-inspector {
    container-type: size;
  }

  @container (max-height: 13rem) {
    .inspector-sidebar {
      width: min(36rem, 56%);
      display: grid;
      grid-template-columns: minmax(12rem, 1.1fr) minmax(16rem, 1.8fr) minmax(7rem, 0.7fr);
      align-items: center;
      gap: 0.5rem;
      padding-top: 0.25rem;
      padding-bottom: 0.25rem;
      padding-right: 0.5rem;
    }

    .inspector-sliders {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      align-items: center;
      gap: 0.5rem;
    }

    .inspector-actions {
      height: 2rem;
    }

    .inspector-main {
      grid-template-rows: minmax(3.25rem, 52%) minmax(0, 1fr);
      row-gap: 0.25rem;
      padding-bottom: 0.25rem;
    }
  }
</style>
