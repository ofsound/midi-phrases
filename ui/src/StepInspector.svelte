<script>
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
   * @property {number} [cycleTriggerCount]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
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
    onClose = () => {},
  } = $props();

  let stepKey = $derived(`${row}:${step}`);
  let durationPercent = $derived(Math.round(Math.min(1, Math.max(0, durationFraction)) * 100));
</script>

<section class="flex min-h-0 w-full flex-1 bg-app/90">
  <aside class="relative flex w-[13.5rem] shrink-0 border-r border-border-subtle px-6 py-2">
    <button
      type="button"
      data-cursor="pointer"
      aria-label="Close step inspector"
      class="absolute top-1 left-1 z-10 flex h-5 w-5 items-center justify-center rounded-sm border border-border bg-surface text-sm leading-none text-text-muted transition-colors outline-none hover:border-border-strong hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring"
      onclick={onClose}
      title="Close step inspector"
    >
      X
    </button>

    <div class="flex min-h-0 w-full flex-1 flex-col justify-between">
      <ContinuousSlider
        large
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
        large
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
        large
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
  </aside>

  <div class="grid min-h-0 min-w-0 flex-1 grid-rows-[33%_55%] content-between overflow-hidden">
    <div class="grid min-h-0 min-w-0 grid-cols-2 divide-x divide-border-subtle border-b border-border-subtle bg-surface/15">
      <div class="flex min-h-0 min-w-0 flex-col justify-center gap-1 px-5">
        <span class="text-[11px] font-medium uppercase tracking-wide text-text-muted">Cycle</span>
        <CyclePatternEditor
          {accent}
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

      <div class="flex min-h-0 min-w-0 items-center px-5">
        <ContinuousSlider
          large
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

    <div class="flex min-h-0 min-w-0 overflow-hidden border-t border-border-subtle">
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
