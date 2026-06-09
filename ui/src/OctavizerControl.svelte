<script>
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
   * @property {(value: number) => void | Promise<void>} [onDown8vaRelativeVelocityChange]
   * @property {(value: number) => void | Promise<void>} [onUp8vaRelativeVelocityChange]
   * @property {(value: number) => void | Promise<void>} [onDown8vaRelativeVelocityCommit]
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
    onDown8vaRelativeVelocityChange = () => {},
    onUp8vaRelativeVelocityChange = () => {},
    onDown8vaRelativeVelocityCommit = () => {},
    onUp8vaRelativeVelocityCommit = () => {},
  } = $props();
</script>

<div class="octavizer-control" role="group" aria-label="Octavizer">
  <div class="combination-mode-connector" aria-hidden="true"></div>

  <div class="octavizer-slot">
    <button
      type="button"
      class="combination-mode-button octavizer-toggle"
      aria-label="Toggle down one octave"
      aria-pressed={down8vaEnabled}
      title="-8va"
      data-cursor="pointer"
      onpointerdown={(event) => {
        event.preventDefault();
        onDown8vaToggle(!down8vaEnabled);
      }}
    >
      <span class="combination-mode-button-face octavizer-toggle-face">
        <span class="octavizer-toggle-label">-8va</span>
      </span>
    </button>

    <RelativeVelocityDragInput
      value={down8vaRelativeVelocity}
      active={down8vaEnabled}
      ariaLabel="-8va relative velocity"
      onValueChange={onDown8vaRelativeVelocityChange}
      onValueCommit={onDown8vaRelativeVelocityCommit}
    />
  </div>

  <div class="combination-mode-connector" aria-hidden="true"></div>

  <div class="octavizer-slot">
    <button
      type="button"
      class="combination-mode-button octavizer-toggle"
      aria-label="Toggle up one octave"
      aria-pressed={up8vaEnabled}
      title="8va"
      data-cursor="pointer"
      onpointerdown={(event) => {
        event.preventDefault();
        onUp8vaToggle(!up8vaEnabled);
      }}
    >
      <span class="combination-mode-button-face octavizer-toggle-face">
        <span class="octavizer-toggle-label">8va</span>
      </span>
    </button>

    <RelativeVelocityDragInput
      value={up8vaRelativeVelocity}
      active={up8vaEnabled}
      ariaLabel="8va relative velocity"
      onValueChange={onUp8vaRelativeVelocityChange}
      onValueCommit={onUp8vaRelativeVelocityCommit}
    />
  </div>
</div>
