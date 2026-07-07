<script>
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import { emeraldRowAccent, rowAccentScopeStyle } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {"loop" | "starter"} [mode]
   * @property {number} [cycleLengthPulses]
   * @property {number} [progress]
   * @property {number} [elapsedSeconds]
   * @property {number} [suggestedTempoBpm]
   * @property {number} [appliedTempoBpm]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(value: number) => void | Promise<void>} [onCycleLengthChange]
   */

  /** @type {Props} */
  let {
    row = 0,
    mode = "loop",
    cycleLengthPulses = 16,
    progress = 0,
    elapsedSeconds = 0,
    suggestedTempoBpm = 0,
    appliedTempoBpm = 0,
    accent = emeraldRowAccent,
    onCycleLengthChange = () => {},
  } = $props();

  let starterMode = $derived(mode === "starter");
  let clampedProgress = $derived(Math.min(1, Math.max(0, Number(progress) || 0)));
  let progressDegrees = $derived(Math.round(clampedProgress * 360));
  let progressPercent = $derived(Math.round(clampedProgress * 100));
  let elapsedLabel = $derived(`${Math.max(0, Number(elapsedSeconds) || 0).toFixed(1)}s`);
  let tempoLabel = $derived(
    (Number(suggestedTempoBpm) || 0) > 0
      ? `${Math.round(Number(suggestedTempoBpm))} BPM`
      : "-- BPM",
  );
  let appliedTempoLabel = $derived(
    (Number(appliedTempoBpm) || 0) > 0
      ? `${Math.round(Number(appliedTempoBpm))} BPM`
      : "",
  );
  let progressBackground = $derived(
    `conic-gradient(var(--color-accent) ${progressDegrees}deg, color-mix(in srgb, var(--color-border) 70%, transparent) ${progressDegrees}deg 360deg)`,
  );
</script>

<aside
  class="flex w-[13rem] shrink-0 flex-col gap-4 py-2 pr-3 pl-0"
  style={rowAccentScopeStyle(accent)}
>
  {#if starterMode}
    <div class="grid gap-3">
      <span class="text-sm font-semibold leading-none tracking-normal text-text">
        Starter
      </span>
      <div class="grid gap-2 rounded-md border border-border-subtle bg-control-surface px-3 py-2">
        <div class="flex items-center justify-between gap-2">
          <span class="text-xs font-semibold uppercase text-text-muted">Time</span>
          <span class="text-sm font-semibold tabular-nums text-text">{elapsedLabel}</span>
        </div>
        <div class="flex items-center justify-between gap-2">
          <span class="text-xs font-semibold uppercase text-text-muted">Fit</span>
          <span class="text-sm font-semibold tabular-nums text-text">{tempoLabel}</span>
        </div>
        {#if appliedTempoLabel}
          <div class="flex items-center justify-between gap-2">
            <span class="text-xs font-semibold uppercase text-text-muted">Set</span>
            <span class="text-sm font-semibold tabular-nums text-text">{appliedTempoLabel}</span>
          </div>
        {/if}
      </div>
    </div>
  {:else}
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
  {/if}

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
