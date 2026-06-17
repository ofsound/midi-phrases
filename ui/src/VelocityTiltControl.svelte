<script>
  import ShimmerParamDragInput from "./ShimmerParamDragInput.svelte";
  import {
    clampVelocityTiltAmount,
    clampVelocityTiltPivotMidi,
    defaultVelocityTiltAmount,
    defaultVelocityTiltPivotMidi,
    formatVelocityTiltAmount,
    formatVelocityTiltPivot,
    maxVelocityTiltAmount,
    minVelocityTiltAmount,
  } from "./velocityTilt.js";
  import { maxMidiNote, minMidiNote } from "./noteBandpass.js";

  /**
   * MIDI velocity tilt stage for the post-filter processing rail.
   *
   * @typedef {Object} Props
   * @property {number} pivotMidi
   * @property {number} amount
   * @property {() => void} [onParamGestureStart]
   * @property {(value: number) => void} [onPivotPreview]
   * @property {(value: number) => void | Promise<void>} [onPivotCommit]
   * @property {(value: number) => void} [onAmountPreview]
   * @property {(value: number) => void | Promise<void>} [onAmountCommit]
   */

  /** @type {Props} */
  let {
    pivotMidi = defaultVelocityTiltPivotMidi,
    amount = defaultVelocityTiltAmount,
    onParamGestureStart = () => {},
    onPivotPreview = () => {},
    onPivotCommit = () => {},
    onAmountPreview = () => {},
    onAmountCommit = () => {},
  } = $props();
</script>

<div class="velocity-tilt-control" role="group" aria-label="Velocity tilt">
  <div class="velocity-tilt-icon" aria-hidden="true">
    <span class="velocity-tilt-line"></span>
    <span class="velocity-tilt-pivot"></span>
  </div>

  <div class="processing-param-labeled">
    <ShimmerParamDragInput
      value={pivotMidi}
      min={minMidiNote}
      max={maxMidiNote}
      defaultValue={defaultVelocityTiltPivotMidi}
      formatValue={formatVelocityTiltPivot}
      clampValue={clampVelocityTiltPivotMidi}
      active={true}
      ariaLabel="Velocity tilt pivot note"
      title="Pivot note · drag vertically · double-click to reset"
      onGestureStart={onParamGestureStart}
      onValuePreview={onPivotPreview}
      onValueCommit={onPivotCommit}
    />
    <span class="processing-param-label" aria-hidden="true">Piv</span>
  </div>

  <div class="processing-param-labeled">
    <ShimmerParamDragInput
      value={amount}
      min={minVelocityTiltAmount}
      max={maxVelocityTiltAmount}
      defaultValue={defaultVelocityTiltAmount}
      formatValue={formatVelocityTiltAmount}
      clampValue={clampVelocityTiltAmount}
      active={amount !== 0}
      ariaLabel="Velocity tilt amount"
      title="Velocity change per octave · drag vertically · double-click to reset"
      onGestureStart={onParamGestureStart}
      onValuePreview={onAmountPreview}
      onValueCommit={onAmountCommit}
    />
    <span class="processing-param-label" aria-hidden="true">Tilt</span>
  </div>
</div>
