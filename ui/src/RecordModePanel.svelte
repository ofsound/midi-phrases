<script>
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import { emeraldRowAccent, rowAccentScopeStyle } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {number} [cycleLengthPulses]
   * @property {number} [progress]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(value: number) => void | Promise<void>} [onCycleLengthChange]
   */

  /** @type {Props} */
  let {
    row = 0,
    cycleLengthPulses = 16,
    progress = 0,
    accent = emeraldRowAccent,
    onCycleLengthChange = () => {},
  } = $props();

  let clampedProgress = $derived(Math.min(1, Math.max(0, Number(progress) || 0)));
  let progressDegrees = $derived(Math.round(clampedProgress * 360));
  let progressPercent = $derived(Math.round(clampedProgress * 100));
  let progressBackground = $derived(
    `conic-gradient(var(--color-accent) ${progressDegrees}deg, color-mix(in srgb, var(--color-border) 70%, transparent) ${progressDegrees}deg 360deg)`,
  );
</script>

<aside
  class="flex w-[13rem] shrink-0 flex-col gap-4 py-2 pr-3 pl-0"
  style={rowAccentScopeStyle(accent)}
>
  <label class="grid gap-3">
    <span class="text-sm font-semibold leading-none tracking-normal text-text">
      Recording Cycle
    </span>
    <div class="seed-param-row">
      <div class="record-cycle-input">
        <StepNumberDragInput
          {accent}
          value={cycleLengthPulses}
          min={1}
          max={64}
          resetValue={16}
          ariaLabel="Record cycle length in pulses"
          boxed
          compact
          boxChars={2}
          onValueChange={onCycleLengthChange}
        />
      </div>
      <span class="seed-param-value" aria-hidden="true">pulses</span>
    </div>
  </label>

  <div class="flex flex-1 flex-col items-center justify-center gap-3">
    <div
      class="relative h-24 w-24 rounded-full border border-border-subtle shadow-inner"
      style:background={progressBackground}
      aria-label="Record loop progress"
      title="Record loop progress"
      role="img"
    >
      <div class="absolute inset-[0.72rem] rounded-full border border-border-subtle bg-workspace"></div>
      <div
        class="absolute top-1/2 left-1/2 h-[2.35rem] w-0.5 origin-bottom rounded-full bg-text-secondary"
        style:transform="translate(-50%, -100%) rotate({progressDegrees}deg)"
      ></div>
    </div>
    <span class="text-xs font-semibold tabular-nums text-text-secondary">
      {progressPercent}%
    </span>
  </div>
</aside>

<style>
  .record-cycle-input :global(.mp-param-box) {
    width: calc(2ch + var(--param-box-padding-x, 0.3125rem) * 2);
  }
</style>
