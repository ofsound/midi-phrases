<script>
  import ShimmerParamDragInput from "./ShimmerParamDragInput.svelte";
  import {formatTimingMultiplierLabel} from "./stepCellLayout.js";
  import {
    clampShimmerDelayMultiplierIndex,
    clampShimmerFeedbackPercent,
    clampShimmerMixPercent,
    defaultShimmerDelayMultiplierIndex,
    defaultShimmerFeedbackPercent,
    defaultShimmerMixPercent,
    maxShimmerFeedbackPercent,
    maxShimmerMixPercent,
    minShimmerFeedbackPercent,
    minShimmerMixPercent,
  } from "./shimmer.js";
  import {stepTimingMultiplierCount, timingMultiplierAtIndex} from "./stepCellLayout.js";

  /**
   * Shimmer reverb section for the post-row processing rail.
   *
   * @typedef {Object} Props
   * @property {boolean} enabled
   * @property {number} delayMultiplierIndex
   * @property {number} feedbackPercent
   * @property {number} mixPercent
   * @property {(enabled: boolean) => void | Promise<void>} [onToggle]
   * @property {(value: number) => void | Promise<void>} [onDelayCommit]
   * @property {(value: number) => void | Promise<void>} [onFeedbackCommit]
   * @property {(value: number) => void | Promise<void>} [onMixCommit]
   */

  /** @type {Props} */
  let {
    enabled = false,
    delayMultiplierIndex = defaultShimmerDelayMultiplierIndex,
    feedbackPercent = defaultShimmerFeedbackPercent,
    mixPercent = defaultShimmerMixPercent,
    onToggle = () => {},
    onDelayCommit = () => {},
    onFeedbackCommit = () => {},
    onMixCommit = () => {},
  } = $props();

  /** @param {number} index */
  function formatDelay(index) {
    return formatTimingMultiplierLabel(timingMultiplierAtIndex(index));
  }

  /** @param {number} percent */
  function formatPercent(percent) {
    return `${percent}%`;
  }
</script>

<div class="shimmer-control" role="group" aria-label="Shimmer">
  <div class="shimmer-slot">
    <button
      type="button"
      class="combination-mode-button shimmer-toggle"
      aria-label="Toggle shimmer"
      aria-pressed={enabled}
      title="Shimmer"
      data-cursor="pointer"
      onpointerdown={(event) => {
        event.preventDefault();
        onToggle(!enabled);
      }}
    >
      <span class="combination-mode-button-face shimmer-toggle-face">
        <span class="shimmer-toggle-label">Shimmer</span>
      </span>
    </button>

    <div class="shimmer-params" aria-hidden={!enabled}>
      <ShimmerParamDragInput
        value={delayMultiplierIndex}
        min={0}
        max={stepTimingMultiplierCount - 1}
        defaultValue={defaultShimmerDelayMultiplierIndex}
        formatValue={formatDelay}
        clampValue={clampShimmerDelayMultiplierIndex}
        active={enabled}
        ariaLabel="Shimmer delay"
        title="Delay in pulse multiples · drag vertically · double-click to reset"
        onValueCommit={onDelayCommit}
      />
      <ShimmerParamDragInput
        value={feedbackPercent}
        min={minShimmerFeedbackPercent}
        max={maxShimmerFeedbackPercent}
        defaultValue={defaultShimmerFeedbackPercent}
        formatValue={formatPercent}
        clampValue={clampShimmerFeedbackPercent}
        active={enabled}
        ariaLabel="Shimmer feedback"
        title="Feedback percent · drag vertically · double-click to reset"
        onValueCommit={onFeedbackCommit}
      />
      <ShimmerParamDragInput
        value={mixPercent}
        min={minShimmerMixPercent}
        max={maxShimmerMixPercent}
        defaultValue={defaultShimmerMixPercent}
        formatValue={formatPercent}
        clampValue={clampShimmerMixPercent}
        active={enabled}
        ariaLabel="Shimmer mix"
        title="Tap mix percent · drag vertically · double-click to reset"
        onValueCommit={onMixCommit}
      />
    </div>
  </div>
</div>
