<script>
  import ShimmerParamDragInput from "./ShimmerParamDragInput.svelte";
  import {
    clampGlobalTransposeSemitones,
    defaultGlobalTransposeSemitones,
    formatGlobalTransposeSemitones,
    maxGlobalTransposeSemitones,
    minGlobalTransposeSemitones,
  } from "./globalTranspose.js";

  /**
   * @typedef {Object} Props
   * @property {number} semitones
   * @property {() => void} [onGestureStart]
   * @property {(value: number) => void} [onPreview]
   * @property {(value: number) => void | Promise<void>} [onCommit]
   */

  /** @type {Props} */
  let {
    semitones = defaultGlobalTransposeSemitones,
    onGestureStart = () => {},
    onPreview = () => {},
    onCommit = () => {},
  } = $props();

  /** @param {number} delta */
  function transposeBy(delta) {
    onGestureStart();
    onCommit(clampGlobalTransposeSemitones(semitones + delta));
  }
</script>

<div class="global-transpose-control" role="group" aria-label="Global transpose">
  <button
    type="button"
    class="global-transpose-octave-button"
    data-cursor="pointer"
    aria-label="Transpose down one octave"
    title="Transpose down one octave"
    onpointerdown={(event) => {
      event.preventDefault();
      transposeBy(-12);
    }}
  >-8va</button>

  <div class="processing-param-labeled">
    <ShimmerParamDragInput
      value={semitones}
      min={minGlobalTransposeSemitones}
      max={maxGlobalTransposeSemitones}
      defaultValue={defaultGlobalTransposeSemitones}
      formatValue={formatGlobalTransposeSemitones}
      clampValue={clampGlobalTransposeSemitones}
      active={semitones !== 0}
      boxChars={3}
      ariaLabel="Global transpose in semitones"
      title="Global transpose · drag vertically by semitone · double-click to reset"
      onGestureStart={onGestureStart}
      onValuePreview={onPreview}
      onValueCommit={onCommit}
    />
    <span class="processing-param-label" aria-hidden="true">Semi</span>
  </div>

  <button
    type="button"
    class="global-transpose-octave-button"
    data-cursor="pointer"
    aria-label="Transpose up one octave"
    title="Transpose up one octave"
    onpointerdown={(event) => {
      event.preventDefault();
      transposeBy(12);
    }}
  >8va</button>
</div>
