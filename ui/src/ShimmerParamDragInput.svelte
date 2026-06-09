<script>
  /**
   * Vertical drag field for shimmer rail parameters.
   *
   * @typedef {Object} Props
   * @property {number} value
   * @property {number} min
   * @property {number} max
   * @property {number} defaultValue
   * @property {(value: number) => string} formatValue
   * @property {(value: number) => number} clampValue
   * @property {boolean} [active]
   * @property {string} [ariaLabel]
   * @property {string} [title]
   * @property {(value: number) => void | Promise<void>} [onValueChange]
   * @property {(value: number) => void | Promise<void>} [onValueCommit]
   */

  /** @type {Props} */
  let {
    value,
    min,
    max,
    defaultValue,
    formatValue,
    clampValue,
    active = false,
    ariaLabel = "Shimmer parameter",
    title = "Drag vertically to adjust · double-click to reset",
    onValueChange = () => {},
    onValueCommit = () => {},
  } = $props();

  const pixelsPerStep = 4;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;

  let displayValue = $derived(formatValue(value));

  /** @param {number} clientY */
  function valueFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampValue(dragStartValue + steps);
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

    if (value !== defaultValue) {
      onValueChange(defaultValue);
      onValueCommit(defaultValue);
    }
  }
</script>

<div
  data-cursor="vertical-drag"
  class={`shimmer-param ${active ? "shimmer-param-active" : ""} ${dragging ? "shimmer-param-dragging" : ""}`}
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={min}
  aria-valuemax={max}
  aria-valuenow={value}
  aria-valuetext={displayValue}
  tabindex="0"
  {title}
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  onkeydown={(event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (value < max) onValueChange(clampValue(value + 1));
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (value > min) onValueChange(clampValue(value - 1));
    }
  }}
>
  <span class="shimmer-param-value" aria-hidden="true">{displayValue}</span>
</div>
