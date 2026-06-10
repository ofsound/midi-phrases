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
   * Keeps a local drag value so parent state (and piano-roll preview) update only on commit.
   *
   * @typedef {Object} Props
   * @property {number} value
   * @property {boolean} [active]
   * @property {string} [ariaLabel]
   * @property {(value: number) => void | Promise<void>} [onValueCommit]
   */

  /** @type {Props} */
  let {
    value = defaultOctavizerRelativeVelocity,
    active = false,
    ariaLabel = "Relative velocity",
    onValueCommit = () => {},
  } = $props();

  const pixelsPerStep = 4;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;
  let dragValue = $state(defaultOctavizerRelativeVelocity);

  let displayedValue = $derived(dragging ? dragValue : value);
  let displayValue = $derived(formatOctavizerRelativeVelocity(displayedValue));

  /** @param {number} clientY */
  function valueFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampOctavizerRelativeVelocity(dragStartValue + steps);
  }

  /** @param {number} next */
  function commitValue(next) {
    const clamped = clampOctavizerRelativeVelocity(next);

    if (clamped !== value) onValueCommit(clamped);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;
    dragValue = value;
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    const next = valueFromDrag(event.clientY);

    if (next !== dragValue) dragValue = next;
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    if (!dragging) return;

    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    commitValue(dragValue);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (dragging) return;

    event.preventDefault();

    if (value !== defaultOctavizerRelativeVelocity) {
      commitValue(defaultOctavizerRelativeVelocity);
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
  aria-valuenow={displayedValue}
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

      if (displayedValue < maxOctavizerRelativeVelocity) {
        commitValue(displayedValue + 1);
      }
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (displayedValue > minOctavizerRelativeVelocity) {
        commitValue(displayedValue - 1);
      }
    }
  }}
>
  <span class="octavizer-relative-velocity-value" aria-hidden="true">{displayValue}</span>
</div>
