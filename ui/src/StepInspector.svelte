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

<section class="flex min-h-0 w-full flex-1 bg-app/90">
  <aside class="flex w-[13.5rem] shrink-0 flex-col gap-3 border-r border-border-subtle px-6 py-4">
    <div class="flex min-w-0 w-full flex-col gap-1">
      <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">Cycle</span>
      <CyclePatternEditor
        compact
        {accent}
        {cycle}
        {cycleMask}
        ariaLabel="Step cycle pattern"
        onPatternCommit={onCyclePatternCommit}
      />
    </div>

    <ContinuousSlider
      {accent}
      label="Probability"
      value={probability}
      min={0}
      max={100}
      ariaLabel="Step probability"
      fullWidth={true}
      formatDisplay={(value) => `${Math.round(value)}%`}
      onValueChange={onProbabilityChange}
    />
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
    <button
      type="button"
      data-cursor="pointer"
      aria-label="Close step inspector"
      class="flex h-8 w-full shrink-0 items-center justify-center rounded-md border border-border bg-surface text-lg leading-none text-text-muted transition-colors outline-none hover:border-border-strong hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring"
      onclick={onClose}
    >
      X
    </button>
  </aside>

  <div class="flex min-h-0 min-w-0 flex-1 flex-col overflow-hidden">
    <StepInspectorKeyboard
      {note}
      {stepKey}
      {scaleRoot}
      {scaleModeIndex}
      {accent}
      onNoteChange={onNoteChange}
    />
  </div>
</section>
