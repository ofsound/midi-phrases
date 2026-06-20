<script>
  import RowRandomizeLengthIcon from "./RowRandomizeLengthIcon.svelte";
  import RowRandomizeOctaveIcon from "./RowRandomizeOctaveIcon.svelte";
  import RowRandomizeOrderIcon from "./RowRandomizeOrderIcon.svelte";
  import RowReverseOrderIcon from "./RowReverseOrderIcon.svelte";
  import StepMuteIcon from "./StepMuteIcon.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import StepSkipIcon from "./StepSkipIcon.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {string} [className]
   * @property {"stacked" | "inline" | "sidebar"} [layout]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {number} [selectedStepCount]
   * @property {number} [totalStepCount]
   * @property {boolean} [requireSelection]
   * @property {boolean} [reverseAvailable]
   * @property {boolean} [skipActive]
   * @property {boolean} [muteActive]
   * @property {number} [durationPercent]
   * @property {number} [velocityPercent]
   * @property {number} [transposeSemitones]
   * @property {string} [pitchAriaLabel]
   * @property {() => void | Promise<void>} [onReverse]
   * @property {() => void | Promise<void>} [onShuffle]
   * @property {() => void | Promise<void>} [onRandomizeOctaves]
   * @property {() => void | Promise<void>} [onRandomizeLengths]
   * @property {() => void | Promise<void>} [onToggleSkip]
   * @property {() => void | Promise<void>} [onToggleMute]
   * @property {() => void} [onGestureStart]
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
    accent = emeraldRowAccent,
    selectedStepCount = 0,
    totalStepCount = 0,
    requireSelection = true,
    reverseAvailable = false,
    skipActive = false,
    muteActive = false,
    durationPercent = 0,
    velocityPercent = 0,
    transposeSemitones = 0,
    pitchAriaLabel = "Bulk step pitch semitones",
    onReverse = () => {},
    onShuffle = () => {},
    onRandomizeOctaves = () => {},
    onRandomizeLengths = () => {},
    onToggleSkip = () => {},
    onToggleMute = () => {},
    onGestureStart = () => {},
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

  function actionButtonClasses(enabled = true) {
    const sizeClass = sidebarLayout ? "h-8 w-full" : "h-8 w-8 shrink-0";

    return `flex ${sizeClass} items-center justify-center rounded-md border p-0 transition-[background-color,border-color,color,box-shadow] outline-none focus:ring-1 focus:ring-focus-ring ${
      enabled
        ? "mp-control-gradient border-border text-text-secondary hover:border-border-strong hover:text-text"
        : "mp-control-gradient-muted border-border-subtle text-text-faint"
    }`;
  }

  function toggleActionButtonClasses(enabled = true, active = false) {
    const sizeClass = sidebarLayout ? "h-8 w-full" : "h-8 w-8 shrink-0";

    return `flex ${sizeClass} items-center justify-center rounded-md border p-0 transition-[background-color,border-color,color,box-shadow] outline-none focus:ring-1 focus:ring-focus-ring ${
      enabled
        ? active
          ? "mp-control-gradient border-border-strong text-text shadow-sm"
          : "mp-control-gradient border-border text-text-secondary hover:border-border-strong hover:text-text"
        : "mp-control-gradient-muted border-border-subtle text-text-faint"
    }`;
  }

  function formatSignedValue(value) {
    const rounded = Math.round(value);

    return rounded > 0 ? `+${rounded}` : String(rounded);
  }
  let inlineLayout = $derived(layout === "inline");
  let sidebarLayout = $derived(layout === "sidebar");
  let inlineLabels = $derived(inlineLayout);
  let groupClass = $derived(
    sidebarLayout
      ? "flex min-w-0 flex-col items-start gap-1"
      : inlineLabels
        ? "flex items-center gap-1.5"
        : "flex flex-col items-start gap-1",
  );
  let operationGroupClass = $derived(
    sidebarLayout
      ? "flex w-full flex-col items-start gap-1.5"
      : groupClass,
  );
  let operationButtonsClass = $derived(
    sidebarLayout ? "grid w-full grid-cols-3 gap-1" : "flex items-center gap-1",
  );
  let labelClass = $derived(
    `text-xs font-semibold leading-none text-text-muted${inlineLabels ? " shrink-0" : ""}`,
  );
  /** @param {string} text */
  function labelText(text) {
    return inlineLabels ? `${text}:` : text;
  }
  let rootClass = $derived(
    sidebarLayout
      ? `flex min-h-0 w-full flex-1 flex-col justify-between items-stretch gap-4 ${className}`
      : `flex gap-2 ${inlineLayout ? "items-center" : "items-end"} ${className}`,
  );
  let parameterControlsClass = $derived(
    sidebarLayout
      ? "grid w-full grid-cols-3 gap-2"
      : inlineLayout
        ? "flex items-center gap-1.5 px-5"
        : "flex items-end gap-1.5 px-5",
  );
</script>

<div class={rootClass} data-no-marquee>
  <div class={operationGroupClass}>
    <span class={labelClass}>{labelText("Operation")}</span>
    <div class={operationButtonsClass}>
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
      <button
        type="button"
        aria-label={skipActive ? "Unskip selected steps" : "Skip selected steps"}
        title={skipActive ? "Unskip selected steps" : "Skip selected steps in sequence"}
        aria-pressed={skipActive}
        disabled={effectiveStepCount === 0}
        data-cursor="pointer"
        class={toggleActionButtonClasses(effectiveStepCount > 0, skipActive)}
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
        class={toggleActionButtonClasses(effectiveStepCount > 0, muteActive)}
        onclick={onToggleMute}
      >
        <StepMuteIcon class="pointer-events-none h-5 w-5" />
      </button>
    </div>
  </div>
  <div class={parameterControlsClass}>
    <div class={groupClass}>
      <span class={labelClass}>{labelText("Dur %")}</span>
      <StepNumberDragInput
        boxed
        compact
        deferCommit
        {accent}
        boxChars={4}
        value={durationPercent}
        min={-100}
        max={100}
        resetValue={0}
        formatValue={formatSignedValue}
        ariaLabel="Bulk step relative duration percent"
        disabled={effectiveStepCount === 0}
        {onGestureStart}
        onValuePreview={onDurationPreview}
        onValueCommit={onDurationCommit}
      />
    </div>
    <div class={groupClass}>
      <span class={labelClass}>{labelText("Vel %")}</span>
      <StepNumberDragInput
        boxed
        compact
        deferCommit
        {accent}
        boxChars={4}
        value={velocityPercent}
        min={-100}
        max={100}
        resetValue={0}
        formatValue={formatSignedValue}
        ariaLabel="Bulk step relative velocity percent"
        disabled={effectiveStepCount === 0}
        {onGestureStart}
        onValuePreview={onVelocityPreview}
        onValueCommit={onVelocityCommit}
      />
    </div>
    <div class={groupClass}>
      <span class={labelClass}>{labelText("Pitch")}</span>
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
