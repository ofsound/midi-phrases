<script>
  import ProbabilityDragInput from "./ProbabilityDragInput.svelte";
  import CyclePatternEditor from "./CyclePatternEditor.svelte";
  import ContinuousSlider from "./ContinuousSlider.svelte";
  import StepInspectorKeyboard from "./StepInspectorKeyboard.svelte";
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
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(midi: number) => void | Promise<void>} [onNoteChange]
   * @property {(value: number) => void | Promise<void>} [onVelocityChange]
   * @property {(value: number) => void | Promise<void>} [onDurationChange]
   * @property {(value: number) => void | Promise<void>} [onTimingMultiplierChange]
   * @property {(value: number) => void | Promise<void>} [onProbabilityChange]
   * @property {(cycle: number, cycleMask: number) => void | Promise<void>} [onCyclePatternCommit]
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
    scaleRoot = 0,
    scaleModeIndex = 0,
    accent = emeraldRowAccent,
    onNoteChange = () => {},
    onVelocityChange = () => {},
    onDurationChange = () => {},
    onTimingMultiplierChange = () => {},
    onProbabilityChange = () => {},
    onCyclePatternCommit = () => {},
    onClose = () => {},
  } = $props();

  let stepKey = $derived(`${row}:${step}`);
  let durationPercent = $derived(Math.round(Math.min(1, Math.max(0, durationFraction)) * 100));
</script>

<section class="flex min-h-0 w-full flex-1 flex-col gap-2 bg-app/90 px-6 py-4">
  <div class="grid min-h-0 flex-1 grid-cols-3 gap-4">
    <div class="flex min-h-0 flex-col justify-between rounded-md border border-border-subtle bg-surface/80 p-4">
      <span class="text-xs font-semibold uppercase tracking-widest text-text-muted">Probability</span>
      <div class="step-inspector-value mt-4 {accent.textAccent}">
        <ProbabilityDragInput
          {accent}
          value={probability}
          resetValue={100}
          ariaLabel="Step probability"
          onValueChange={onProbabilityChange}
        />
      </div>
    </div>
    <div class="col-span-2 flex min-h-0 flex-col rounded-md border border-border-subtle bg-surface/80 p-4">
      <span class="mb-4 text-xs font-semibold uppercase tracking-widest text-text-muted">Cycle</span>
      <div class="flex min-h-0 flex-1 items-center">
        <CyclePatternEditor
          {accent}
          {cycle}
          {cycleMask}
          ariaLabel="Step cycle pattern"
          onPatternCommit={onCyclePatternCommit}
        />
      </div>
    </div>
  </div>

  <div class="flex min-h-0 flex-1 items-center gap-4">
    <StepInspectorKeyboard
      {note}
      {stepKey}
      {scaleRoot}
      {scaleModeIndex}
      {accent}
      onNoteChange={onNoteChange}
    />
    <div class="flex h-full w-[12rem] min-w-[9rem] shrink-0 flex-col justify-center gap-3">
      <ContinuousSlider
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
        label="Duration"
        value={durationPercent}
        min={0}
        max={100}
        ariaLabel="Step duration"
        fullWidth={true}
        onValueChange={onDurationChange}
      />
      <ContinuousSlider
        label="Velocity"
        value={velocity}
        min={1}
        max={127}
        ariaLabel="Step velocity"
        fullWidth={true}
        onValueChange={onVelocityChange}
      />
    </div>
    <button
      type="button"
      data-cursor="pointer"
      aria-label="Close step inspector"
      class="flex h-8 w-8 shrink-0 items-center justify-center rounded-md border border-border bg-surface text-lg leading-none text-text-muted transition-colors outline-none hover:border-border-strong hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring"
      onclick={onClose}
    >
      X
    </button>
  </div>
</section>

<style>
  .step-inspector-value :global([role="slider"] span) {
    font-size: 2.25rem;
    font-weight: 600;
  }
</style>
