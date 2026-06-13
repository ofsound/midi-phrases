<script>
  import CombinationModeButton from "./CombinationModeButton.svelte";
  import RelativeVelocityDragInput from "./RelativeVelocityDragInput.svelte";

  /**
   * Octavizer section for the post-row processing rail.
   *
   * @typedef {Object} Props
   * @property {boolean} down8vaEnabled
   * @property {boolean} up8vaEnabled
   * @property {number} down8vaRelativeVelocity
   * @property {number} up8vaRelativeVelocity
   * @property {(enabled: boolean) => void | Promise<void>} [onDown8vaToggle]
   * @property {(enabled: boolean) => void | Promise<void>} [onUp8vaToggle]
   * @property {() => void} [onRelativeVelocityGestureStart]
   * @property {(value: number) => void} [onDown8vaRelativeVelocityPreview]
   * @property {(value: number) => void | Promise<void>} [onDown8vaRelativeVelocityCommit]
   * @property {(value: number) => void} [onUp8vaRelativeVelocityPreview]
   * @property {(value: number) => void | Promise<void>} [onUp8vaRelativeVelocityCommit]
   */

  /** @type {Props} */
  let {
    down8vaEnabled = false,
    up8vaEnabled = false,
    down8vaRelativeVelocity = 0,
    up8vaRelativeVelocity = 0,
    onDown8vaToggle = () => {},
    onUp8vaToggle = () => {},
    onRelativeVelocityGestureStart = () => {},
    onDown8vaRelativeVelocityPreview = () => {},
    onDown8vaRelativeVelocityCommit = () => {},
    onUp8vaRelativeVelocityPreview = () => {},
    onUp8vaRelativeVelocityCommit = () => {},
  } = $props();
</script>

<div class="octavizer-control" role="group" aria-label="Octavizer">
  <div class="octavizer-slot">
    <CombinationModeButton
      class="octavizer-toggle"
      faceClass="octavizer-toggle-face"
      pressed={down8vaEnabled}
      ariaLabel="Toggle down one octave"
      title="-8va"
      onToggle={() => onDown8vaToggle(!down8vaEnabled)}
    >
      <span class="octavizer-toggle-label">-8va</span>
    </CombinationModeButton>

    <div class="processing-param-labeled">
      <RelativeVelocityDragInput
        value={down8vaRelativeVelocity}
        active={down8vaEnabled}
        ariaLabel="-8va relative velocity"
        onGestureStart={onRelativeVelocityGestureStart}
        onValuePreview={onDown8vaRelativeVelocityPreview}
        onValueCommit={onDown8vaRelativeVelocityCommit}
      />
      <span class="processing-param-label" aria-hidden="true">Vel</span>
    </div>
  </div>

  <div class="octavizer-slot">
    <CombinationModeButton
      class="octavizer-toggle"
      faceClass="octavizer-toggle-face"
      pressed={up8vaEnabled}
      ariaLabel="Toggle up one octave"
      title="8va"
      onToggle={() => onUp8vaToggle(!up8vaEnabled)}
    >
      <span class="octavizer-toggle-label">8va</span>
    </CombinationModeButton>

    <div class="processing-param-labeled">
      <RelativeVelocityDragInput
        value={up8vaRelativeVelocity}
        active={up8vaEnabled}
        ariaLabel="8va relative velocity"
        onGestureStart={onRelativeVelocityGestureStart}
        onValuePreview={onUp8vaRelativeVelocityPreview}
        onValueCommit={onUp8vaRelativeVelocityCommit}
      />
      <span class="processing-param-label" aria-hidden="true">Vel</span>
    </div>
  </div>
</div>
