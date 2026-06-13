<script>
  import CombinationModeButton from "./CombinationModeButton.svelte";
  import ShimmerIcon from "./ShimmerIcon.svelte";
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
    <CombinationModeButton
      class="shimmer-toggle"
      faceClass="shimmer-toggle-face"
      pressed={enabled}
      ariaLabel="Toggle shimmer"
      title="Shimmer"
      onToggle={() => onToggle(!enabled)}
    >
      <ShimmerIcon />
    </CombinationModeButton>

    <div class="shimmer-params" aria-hidden={!enabled}>
      <div class="processing-param-labeled">
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
        <span class="processing-param-label" aria-hidden="true">Del</span>
      </div>
      <div class="processing-param-labeled">
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
        <span class="processing-param-label" aria-hidden="true">Fbk</span>
      </div>
      <div class="processing-param-labeled">
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
        <span class="processing-param-label" aria-hidden="true">Mix</span>
      </div>
    </div>
  </div>
</div>
