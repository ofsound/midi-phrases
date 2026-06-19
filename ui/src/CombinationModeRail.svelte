<script>
  import CombinationModeButton from "./CombinationModeButton.svelte";
  import CombinationModeIcon from "./CombinationModeIcon.svelte";
  import NoteBandpassSlider from "./NoteBandpassSlider.svelte";
  import OctavizerControl from "./OctavizerControl.svelte";
  import ShimmerControl from "./ShimmerControl.svelte";
  import VelocityTiltControl from "./VelocityTiltControl.svelte";
  import { combinationModes } from "./phraseSchedule.js";

  /**
   * Full-width hardware-style rail for Cross-Mod / Bloom / Counter / Echo / Weave modes.
   *
   * @typedef {Object} Props
   * @property {number} mask
   * @property {(modeIndex: number) => void | Promise<void>} onToggle
   * @property {number} noteBandpassLowMidi
   * @property {number} noteBandpassHighMidi
   * @property {(lowMidi: number, highMidi: number) => void} [onNoteBandpassChange]
   * @property {(lowMidi: number, highMidi: number) => void | Promise<void>} [onNoteBandpassCommit]
   * @property {number} velocityTiltPivotMidi
   * @property {number} velocityTiltAmount
   * @property {(value: number) => void} [onVelocityTiltPivotPreview]
   * @property {(value: number) => void | Promise<void>} [onVelocityTiltPivotCommit]
   * @property {(value: number) => void} [onVelocityTiltAmountPreview]
   * @property {(value: number) => void | Promise<void>} [onVelocityTiltAmountCommit]
   * @property {(pivotMidi: number, amount: number) => void | Promise<void>} [onVelocityTiltXYCommit]
   * @property {boolean} octavizerDown8vaEnabled
   * @property {boolean} octavizerUp8vaEnabled
   * @property {number} octavizerDown8vaRelativeVelocity
   * @property {number} octavizerUp8vaRelativeVelocity
   * @property {(enabled: boolean) => void | Promise<void>} [onOctavizerDown8vaToggle]
   * @property {(enabled: boolean) => void | Promise<void>} [onOctavizerUp8vaToggle]
   * @property {() => void} [onProcessingParamGestureStart]
   * @property {(value: number) => void} [onOctavizerDown8vaRelativeVelocityPreview]
   * @property {(value: number) => void | Promise<void>} [onOctavizerDown8vaRelativeVelocityCommit]
   * @property {(value: number) => void} [onOctavizerUp8vaRelativeVelocityPreview]
   * @property {(value: number) => void | Promise<void>} [onOctavizerUp8vaRelativeVelocityCommit]
   * @property {boolean} shimmerEnabled
   * @property {number} shimmerDelayMultiplierIndex
   * @property {number} shimmerFeedbackPercent
   * @property {number} shimmerMixPercent
   * @property {(enabled: boolean) => void | Promise<void>} [onShimmerToggle]
   * @property {(value: number) => void} [onShimmerDelayPreview]
   * @property {(value: number) => void | Promise<void>} [onShimmerDelayCommit]
   * @property {(value: number) => void} [onShimmerFeedbackPreview]
   * @property {(value: number) => void | Promise<void>} [onShimmerFeedbackCommit]
   * @property {(value: number) => void} [onShimmerMixPreview]
   * @property {(value: number) => void | Promise<void>} [onShimmerMixCommit]
   */

  /** @type {Props} */
  let {
    mask = 0,
    onToggle = () => {},
    noteBandpassLowMidi = 36,
    noteBandpassHighMidi = 108,
    onNoteBandpassChange = () => {},
    onNoteBandpassCommit = () => {},
    velocityTiltPivotMidi = 60,
    velocityTiltAmount = 0,
    onVelocityTiltPivotPreview = () => {},
    onVelocityTiltPivotCommit = () => {},
    onVelocityTiltAmountPreview = () => {},
    onVelocityTiltAmountCommit = () => {},
    onVelocityTiltXYCommit = undefined,
    octavizerDown8vaEnabled = false,
    octavizerUp8vaEnabled = false,
    octavizerDown8vaRelativeVelocity = 0,
    octavizerUp8vaRelativeVelocity = 0,
    onOctavizerDown8vaToggle = () => {},
    onOctavizerUp8vaToggle = () => {},
    onProcessingParamGestureStart = () => {},
    onOctavizerDown8vaRelativeVelocityPreview = () => {},
    onOctavizerDown8vaRelativeVelocityCommit = () => {},
    onOctavizerUp8vaRelativeVelocityPreview = () => {},
    onOctavizerUp8vaRelativeVelocityCommit = () => {},
    shimmerEnabled = false,
    shimmerDelayMultiplierIndex = 3,
    shimmerFeedbackPercent = 70,
    shimmerMixPercent = 100,
    onShimmerToggle = () => {},
    onShimmerDelayPreview = () => {},
    onShimmerDelayCommit = () => {},
    onShimmerFeedbackPreview = () => {},
    onShimmerFeedbackCommit = () => {},
    onShimmerMixPreview = () => {},
    onShimmerMixCommit = () => {},
  } = $props();
</script>

<div class="mp-honeycomb-rail relative z-20 mb-2">
  <div class="combination-mode-rail-layout">
    <div class="combination-mode-group" role="group" aria-label="Combination modes">
      {#each combinationModes as mode, index (mode.index)}
        {#if index > 0}
          <div class="combination-mode-connector" aria-hidden="true"></div>
        {/if}
        <CombinationModeButton
          pressed={(mask & mode.bit) !== 0}
          ariaLabel={`Toggle ${mode.name} mode`}
          title={mode.name}
          onToggle={() => onToggle(mode.index)}
        >
          <CombinationModeIcon kind={mode.icon} />
        </CombinationModeButton>
      {/each}
    </div>

    <div class="processing-rail-center">
      <div class="processing-rail-controls">
        <OctavizerControl
          down8vaEnabled={octavizerDown8vaEnabled}
          up8vaEnabled={octavizerUp8vaEnabled}
          down8vaRelativeVelocity={octavizerDown8vaRelativeVelocity}
          up8vaRelativeVelocity={octavizerUp8vaRelativeVelocity}
          onDown8vaToggle={onOctavizerDown8vaToggle}
          onUp8vaToggle={onOctavizerUp8vaToggle}
          onRelativeVelocityGestureStart={onProcessingParamGestureStart}
          onDown8vaRelativeVelocityPreview={onOctavizerDown8vaRelativeVelocityPreview}
          onDown8vaRelativeVelocityCommit={onOctavizerDown8vaRelativeVelocityCommit}
          onUp8vaRelativeVelocityPreview={onOctavizerUp8vaRelativeVelocityPreview}
          onUp8vaRelativeVelocityCommit={onOctavizerUp8vaRelativeVelocityCommit}
        />

        <ShimmerControl
          enabled={shimmerEnabled}
          delayMultiplierIndex={shimmerDelayMultiplierIndex}
          feedbackPercent={shimmerFeedbackPercent}
          mixPercent={shimmerMixPercent}
          onToggle={onShimmerToggle}
          onParamGestureStart={onProcessingParamGestureStart}
          onDelayPreview={onShimmerDelayPreview}
          onDelayCommit={onShimmerDelayCommit}
          onFeedbackPreview={onShimmerFeedbackPreview}
          onFeedbackCommit={onShimmerFeedbackCommit}
          onMixPreview={onShimmerMixPreview}
          onMixCommit={onShimmerMixCommit}
        />
      </div>
    </div>

    <NoteBandpassSlider
      class="note-bandpass-filter"
      lowMidi={noteBandpassLowMidi}
      highMidi={noteBandpassHighMidi}
      onChange={onNoteBandpassChange}
      onCommit={onNoteBandpassCommit}
    />

    <VelocityTiltControl
      pivotMidi={velocityTiltPivotMidi}
      amount={velocityTiltAmount}
      bandpassLowMidi={noteBandpassLowMidi}
      bandpassHighMidi={noteBandpassHighMidi}
      onParamGestureStart={onProcessingParamGestureStart}
      onPivotPreview={onVelocityTiltPivotPreview}
      onPivotCommit={onVelocityTiltPivotCommit}
      onAmountPreview={onVelocityTiltAmountPreview}
      onAmountCommit={onVelocityTiltAmountCommit}
      onXYCommit={onVelocityTiltXYCommit}
    />
  </div>
</div>
