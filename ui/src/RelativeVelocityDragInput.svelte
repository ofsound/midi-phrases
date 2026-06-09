<script>
  import {
    clampOctavizerRelativeVelocity,
    defaultOctavizerRelativeVelocity,
    formatOctavizerRelativeVelocity,
    maxOctavizerRelativeVelocity,
    minOctavizerRelativeVelocity,
  } from "./octavizer.js";

  /**
   * Bipolar relative-velocity control for the Octavizer rail.
   *
   * @typedef {Object} Props
   * @property {number} value
   * @property {boolean} [active]
   * @property {string} [ariaLabel]
   * @property {(value: number) => void | Promise<void>} [onValueChange]
   * @property {(value: number) => void | Promise<void>} [onValueCommit]
   */

  /** @type {Props} */
  let {
    value = defaultOctavizerRelativeVelocity,
    active = false,
    ariaLabel = "Relative velocity",
    onValueChange = () => {},
    onValueCommit = () => {},
  } = $props();

  const pixelsPerStep = 4;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;

  let displayValue = $derived(formatOctavizerRelativeVelocity(value));

  /** @param {number} clientY */
  function valueFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampOctavizerRelativeVelocity(dragStartValue + steps);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    const next = valueFromDrag(event.clientY);

    if (next !== value) onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    if (!dragging) return;

    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    onValueCommit(value);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (dragging) return;

    event.preventDefault();

    if (value !== defaultOctavizerRelativeVelocity) {
      onValueChange(defaultOctavizerRelativeVelocity);
      onValueCommit(defaultOctavizerRelativeVelocity);
    }
  }
</script>

<div
  data-cursor="vertical-drag"
  class={`octavizer-relative-velocity ${active ? "octavizer-relative-velocity-active" : ""} ${dragging ? "octavizer-relative-velocity-dragging" : ""}`}
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={minOctavizerRelativeVelocity}
  aria-valuemax={maxOctavizerRelativeVelocity}
  aria-valuenow={value}
  aria-valuetext={displayValue}
  tabindex="0"
  title="Drag vertically to adjust relative velocity · double-click to reset"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  onkeydown={(event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (value < maxOctavizerRelativeVelocity) onValueChange(value + 1);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > minOctavizerRelativeVelocity) onValueChange(value - 1);
    }
  }}
>
  <span class="octavizer-relative-velocity-value" aria-hidden="true">{displayValue}</span>
</div>
