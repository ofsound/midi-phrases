<script>
  import RowRandomizeLengthIcon from "./RowRandomizeLengthIcon.svelte";
  import RowRandomizeOctaveIcon from "./RowRandomizeOctaveIcon.svelte";
  import RowRandomizeOrderIcon from "./RowRandomizeOrderIcon.svelte";
  import RowReverseOrderIcon from "./RowReverseOrderIcon.svelte";
  import RowShiftOrderIcon from "./RowShiftOrderIcon.svelte";
  import StepMuteIcon from "./StepMuteIcon.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import StepSkipIcon from "./StepSkipIcon.svelte";
  import { inspectorInactiveControlClasses } from "./inspectorSidebar.js";
  import { minSignedRelativePercentValue, maxSignedRelativePercentValue } from "./percentLimits.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    formatSignedTimingMultiplierDelta,
    stepTimingMultiplierMax,
    stepTimingMultiplierMin,
    stepTimingMultiplierQuarterStep,
  } from "./stepCellLayout.js";

  /**
   * @typedef {Object} Props
   * @property {string} [className]
   * @property {"stacked" | "inline" | "sidebar" | "operations-grid"} [layout]
   * @property {boolean} [compact]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {number} [selectedStepCount]
   * @property {number} [totalStepCount]
   * @property {boolean} [requireSelection]
   * @property {boolean} [omitSkipMuteToggles]
   * @property {boolean} [inspectorEmbedded]
   * @property {boolean} [shiftAvailable]
   * @property {boolean} [reverseAvailable]
   * @property {boolean} [skipActive]
   * @property {boolean} [muteActive]
   * @property {number} [durationPercent]
   * @property {number} [velocityPercent]
   * @property {number} [lengthDelta]
   * @property {number} [transposeSemitones]
   * @property {string} [pitchAriaLabel]
   * @property {() => void | Promise<void>} [onShift]
   * @property {() => void | Promise<void>} [onReverse]
   * @property {() => void | Promise<void>} [onShuffle]
   * @property {() => void | Promise<void>} [onRandomizeOctaves]
   * @property {() => void | Promise<void>} [onRandomizeLengths]
   * @property {() => void | Promise<void>} [onToggleSkip]
   * @property {() => void | Promise<void>} [onToggleMute]
   * @property {() => void} [onGestureStart]
   * @property {(value: number) => void} [onLengthPreview]
   * @property {(value: number) => void | Promise<void>} [onLengthCommit]
   * @property {(value: number) => void} [onDurationPreview]
   * @property {(value: number) => void | Promise<void>} [onDurationCommit]
   * @property {(value: number) => void} [onVelocityPreview]
   * @property {(value: number) => void | Promise<void>} [onVelocityCommit]
   * @property {(value: number) => void} [onTransposePreview]
   * @property {(value: number) => void | Promise<void>} [onTransposeCommit]
   */

  /** @type {Props} */
  let {
    className = "",
    layout = "stacked",
    compact = false,
    accent = emeraldRowAccent,
    selectedStepCount = 0,
    totalStepCount = 0,
    requireSelection = true,
    omitSkipMuteToggles = false,
    inspectorEmbedded = false,
    shiftAvailable = false,
    reverseAvailable = false,
    skipActive = false,
    muteActive = false,
    durationPercent = 0,
    velocityPercent = 0,
    lengthDelta = 0,
    transposeSemitones = 0,
    pitchAriaLabel = "Bulk step pitch semitones",
    onShift = () => {},
    onReverse = () => {},
    onShuffle = () => {},
    onRandomizeOctaves = () => {},
    onRandomizeLengths = () => {},
    onToggleSkip = () => {},
    onToggleMute = () => {},
    onGestureStart = () => {},
    onLengthPreview = () => {},
    onLengthCommit = () => {},
    onDurationPreview = () => {},
    onDurationCommit = () => {},
    onVelocityPreview = () => {},
    onVelocityCommit = () => {},
    onTransposePreview = () => {},
    onTransposeCommit = () => {},
  } = $props();

  let effectiveStepCount = $derived(
    requireSelection || selectedStepCount > 0 ? selectedStepCount : totalStepCount,
  );

  function actionButtonClasses(enabled = true, fillGridCell = false, active = false) {
    const sizeClass = fillGridCell ? "h-8 w-full" : "h-8 w-8 shrink-0";

    return `flex ${sizeClass} items-center justify-center rounded-md border p-0 transition-[background-color,border-color,color,box-shadow] outline-none focus:ring-1 focus:ring-focus-ring ${
      enabled
        ? active
          ? `${accent.bgAccentStrong} ${accent.borderFocus} border text-white ${accent.playbackGlow}`
          : inspectorInactiveControlClasses()
        : inspectorInactiveControlClasses({ disabled: true })
    }`;
  }

  function formatSignedValue(value) {
    const rounded = Math.round(value);

    return rounded > 0 ? `+${rounded}` : String(rounded);
  }
  let inlineLayout = $derived(layout === "inline");
  let sidebarLayout = $derived(layout === "sidebar");
  let sidebarHeaderStyleOps = $derived(sidebarLayout && omitSkipMuteToggles);
  let showOperationLabel = $derived(!sidebarLayout || inspectorEmbedded);
  let operationsGridLayout = $derived(layout === "operations-grid");
  let inlineLabels = $derived(inlineLayout);
  let stackedCompactLayout = $derived(compact && !inlineLayout && !sidebarLayout);
  let stackedColumnClass = $derived(stackedCompactLayout ? " h-full" : "");
  let controlAnchorClass = $derived(stackedCompactLayout ? "mt-auto" : "");
  let groupClass = $derived(
    sidebarLayout
      ? "flex min-w-0 flex-col items-start gap-1"
      : inlineLabels
        ? "flex items-center gap-1.5"
        : `flex flex-col items-start gap-1${stackedColumnClass}`,
  );
  let operationGroupClass = $derived(
    sidebarLayout
      ? inspectorEmbedded
        ? "flex w-full min-w-0 shrink-0 flex-col items-start gap-1"
        : "w-full"
      : stackedCompactLayout
        ? `shrink-0 ${groupClass}`
        : `${groupClass}`,
  );
  let operationButtonsClass = $derived(
    sidebarHeaderStyleOps
      ? "flex w-full items-center gap-1"
      : sidebarLayout
        ? omitSkipMuteToggles
          ? "flex w-full items-center gap-1"
          : "grid w-full grid-cols-7 gap-1"
        : operationsGridLayout
          ? "grid grid-cols-4 gap-1"
          : "flex items-center gap-1",
  );
  let labelClass = $derived(
    `text-xs font-semibold leading-none text-text${inlineLabels ? " shrink-0" : ""}`,
  );
  /** @param {string} text */
  function labelText(text) {
    return inlineLabels ? `${text}:` : text;
  }
  let rootClass = $derived(
    sidebarLayout
      ? `flex min-h-0 w-full flex-1 flex-col items-stretch ${
          inspectorEmbedded
            ? "inspector-bulk-root min-h-0 justify-start gap-4"
            : "justify-between gap-4"
        } ${className}`
      : `flex ${
          stackedCompactLayout ? "gap-3" : compact ? "gap-1" : "gap-2"
        } ${
          inlineLayout ? "items-center" : stackedCompactLayout ? "items-stretch" : "items-end"
        } ${className}`,
  );
  let parameterControlsClass = $derived(
    sidebarLayout
      ? "grid w-full shrink-0 grid-cols-4 gap-2"
      : inlineLayout
        ? `flex items-center ${compact ? "gap-1 px-1" : "gap-1.5 px-5"}`
        : stackedCompactLayout
          ? "flex shrink-0 items-stretch gap-1"
          : `flex items-end ${compact ? "gap-1 px-1" : "gap-1.5 px-5"}`,
  );
  let operationButtonsWrapperClass = $derived(
    stackedCompactLayout ? `${operationButtonsClass} mt-auto` : operationButtonsClass,
  );
</script>

<div class={rootClass} data-no-marquee>
  <div class={operationGroupClass}>
    {#if showOperationLabel}
      <span class={labelClass}>{labelText("Operation")}</span>
    {/if}
    <div class={operationButtonsWrapperClass}>
      {#if sidebarLayout && !omitSkipMuteToggles}
          <button
            type="button"
            aria-label={skipActive ? "Unskip selected steps" : "Skip selected steps"}
            title={skipActive ? "Unskip selected steps" : "Skip selected steps in sequence"}
            aria-pressed={skipActive}
            disabled={effectiveStepCount === 0}
            data-cursor="pointer"
            class={actionButtonClasses(effectiveStepCount > 0, true, skipActive)}
            onclick={onToggleSkip}
          >
            <StepSkipIcon class="pointer-events-none h-4 w-4" />
          </button>
          <button
            type="button"
            aria-label={muteActive ? "Unmute selected steps" : "Mute selected steps"}
            title={muteActive ? "Unmute selected steps" : "Mute selected steps"}
            aria-pressed={muteActive}
            disabled={effectiveStepCount === 0}
            data-cursor="pointer"
            class={actionButtonClasses(effectiveStepCount > 0, true, muteActive)}
            onclick={onToggleMute}
          >
            <StepMuteIcon class="pointer-events-none h-4 w-4" />
          </button>
          <button
            type="button"
            aria-label="Shift selected steps left by row"
            title="Shift selected steps left by row"
            disabled={!shiftAvailable}
            data-cursor="pointer"
            class={actionButtonClasses(shiftAvailable, true)}
            onclick={onShift}
          >
            <RowShiftOrderIcon class="pointer-events-none h-4 w-4" />
          </button>
          <button
            type="button"
            aria-label="Reverse selected steps by row"
            title="Reverse selected steps by row"
            disabled={!reverseAvailable}
            data-cursor="pointer"
            class={actionButtonClasses(reverseAvailable, true)}
            onclick={onReverse}
          >
            <RowReverseOrderIcon class="pointer-events-none h-4 w-4" />
          </button>
          <button
            type="button"
            aria-label="Shuffle selected steps"
            title="Shuffle selected steps across rows"
            disabled={effectiveStepCount <= 1}
            data-cursor="pointer"
            class={actionButtonClasses(effectiveStepCount > 1, true)}
            onclick={onShuffle}
          >
            <RowRandomizeOrderIcon class="pointer-events-none h-4 w-4" />
          </button>
          <button
            type="button"
            aria-label="Randomize selected step octaves"
            title="Randomize selected step octaves"
            disabled={effectiveStepCount === 0}
            data-cursor="pointer"
            class={actionButtonClasses(effectiveStepCount > 0, true)}
            onclick={onRandomizeOctaves}
          >
            <RowRandomizeOctaveIcon class="pointer-events-none h-4 w-4" />
          </button>
          <button
            type="button"
            aria-label="Randomize selected step lengths"
            title="Randomize selected step lengths"
            disabled={effectiveStepCount === 0}
            data-cursor="pointer"
            class={actionButtonClasses(effectiveStepCount > 0, true)}
            onclick={onRandomizeLengths}
          >
            <RowRandomizeLengthIcon class="pointer-events-none h-4 w-4" />
          </button>
      {:else}
      {#if !omitSkipMuteToggles}
      <button
        type="button"
        aria-label={skipActive ? "Unskip selected steps" : "Skip selected steps"}
        title={skipActive ? "Unskip selected steps" : "Skip selected steps in sequence"}
        aria-pressed={skipActive}
        disabled={effectiveStepCount === 0}
        data-cursor="pointer"
        class={actionButtonClasses(effectiveStepCount > 0, false, skipActive)}
        onclick={onToggleSkip}
      >
        <StepSkipIcon class="pointer-events-none h-5 w-5" />
      </button>
      <button
        type="button"
        aria-label={muteActive ? "Unmute selected steps" : "Mute selected steps"}
        title={muteActive ? "Unmute selected steps" : "Mute selected steps"}
        aria-pressed={muteActive}
        disabled={effectiveStepCount === 0}
        data-cursor="pointer"
        class={actionButtonClasses(effectiveStepCount > 0, false, muteActive)}
        onclick={onToggleMute}
      >
        <StepMuteIcon class="pointer-events-none h-5 w-5" />
      </button>
      {/if}
      <button
        type="button"
        aria-label="Shift selected steps left by row"
        title="Shift selected steps left by row"
        disabled={!shiftAvailable}
        data-cursor="pointer"
        class={actionButtonClasses(shiftAvailable)}
        onclick={onShift}
      >
        <RowShiftOrderIcon class="pointer-events-none h-5 w-5" />
      </button>
      <button
        type="button"
        aria-label="Reverse selected steps by row"
        title="Reverse selected steps by row"
        disabled={!reverseAvailable}
        data-cursor="pointer"
        class={actionButtonClasses(reverseAvailable)}
        onclick={onReverse}
      >
        <RowReverseOrderIcon class="pointer-events-none h-5 w-5" />
      </button>
      <button
        type="button"
        aria-label="Shuffle selected steps"
        title="Shuffle selected steps across rows"
        disabled={effectiveStepCount <= 1}
        data-cursor="pointer"
        class={actionButtonClasses(effectiveStepCount > 1)}
        onclick={onShuffle}
      >
        <RowRandomizeOrderIcon class="pointer-events-none h-5 w-5" />
      </button>
      <button
        type="button"
        aria-label="Randomize selected step octaves"
        title="Randomize selected step octaves"
        disabled={effectiveStepCount === 0}
        data-cursor="pointer"
        class={actionButtonClasses(effectiveStepCount > 0)}
        onclick={onRandomizeOctaves}
      >
        <RowRandomizeOctaveIcon class="pointer-events-none h-5 w-5" />
      </button>
      <button
        type="button"
        aria-label="Randomize selected step lengths"
        title="Randomize selected step lengths"
        disabled={effectiveStepCount === 0}
        data-cursor="pointer"
        class={actionButtonClasses(effectiveStepCount > 0)}
        onclick={onRandomizeLengths}
      >
        <RowRandomizeLengthIcon class="pointer-events-none h-5 w-5" />
      </button>
      {/if}
    </div>
  </div>
  <div class={parameterControlsClass}>
    <div class={groupClass}>
      <span class={labelClass}>{labelText("Len")}</span>
      <div class={controlAnchorClass}>
      <StepNumberDragInput
        boxed
        compact
        deferCommit
        {accent}
        value={lengthDelta}
        min={stepTimingMultiplierMin - stepTimingMultiplierMax}
        max={stepTimingMultiplierMax - stepTimingMultiplierMin}
        valueStep={stepTimingMultiplierQuarterStep}
        resetValue={0}
        formatValue={formatSignedTimingMultiplierDelta}
        ariaLabel="Bulk step relative length multiplier"
        disabled={effectiveStepCount === 0}
        {onGestureStart}
        onValuePreview={onLengthPreview}
        onValueCommit={onLengthCommit}
      />
      </div>
    </div>
    <div class={groupClass}>
      <span class={labelClass}>{labelText("Dur %")}</span>
      <div class={controlAnchorClass}>
      <StepNumberDragInput
        boxed
        compact
        deferCommit
        {accent}
        value={durationPercent}
        min={minSignedRelativePercentValue}
        max={maxSignedRelativePercentValue}
        resetValue={0}
        formatValue={formatSignedValue}
        ariaLabel="Bulk step relative duration percent"
        disabled={effectiveStepCount === 0}
        {onGestureStart}
        onValuePreview={onDurationPreview}
        onValueCommit={onDurationCommit}
      />
      </div>
    </div>
    <div class={groupClass}>
      <span class={labelClass}>{labelText("Vel %")}</span>
      <div class={controlAnchorClass}>
      <StepNumberDragInput
        boxed
        compact
        deferCommit
        {accent}
        value={velocityPercent}
        min={minSignedRelativePercentValue}
        max={maxSignedRelativePercentValue}
        resetValue={0}
        formatValue={formatSignedValue}
        ariaLabel="Bulk step relative velocity percent"
        disabled={effectiveStepCount === 0}
        {onGestureStart}
        onValuePreview={onVelocityPreview}
        onValueCommit={onVelocityCommit}
      />
      </div>
    </div>
    <div class={groupClass}>
      <span class={labelClass}>{labelText("Pitch")}</span>
      <div class={controlAnchorClass}>
      <StepNumberDragInput
        boxed
        compact
        deferCommit
        {accent}
        value={transposeSemitones}
        min={-48}
        max={48}
        resetValue={0}
        formatValue={formatSignedValue}
        ariaLabel={pitchAriaLabel}
        disabled={effectiveStepCount === 0}
        {onGestureStart}
        onValuePreview={onTransposePreview}
        onValueCommit={onTransposeCommit}
      />
      </div>
    </div>
  </div>
</div>
