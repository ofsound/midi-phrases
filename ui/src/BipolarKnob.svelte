<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  
  
  
  
  /**
   * @typedef {Object} Props
   * @property {{ index: number, label: string }[]} options
   * @property {any} value
   * @property {any} [resetValue] - Option index restored on double-click; omit to disable reset.
   * @property {boolean} [muted]
   * @property {string} [ariaLabel]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(index: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    options,
    value,
    resetValue = undefined,
    muted = false,
    ariaLabel = "Bipolar knob",
    accent = emeraldRowAccent,
    onValueChange = () => {}
  } = $props();

  const pixelsPerStep = 10;
  const minAngleDeg = -135;
  const maxAngleDeg = 135;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;

  let maxIndex = $derived(Math.max(0, options.length - 1));
  let valuePosition = $derived(Math.max(0, options.findIndex((option) => option.index === value)));
  let currentLabel = $derived(options.find((option) => option.index === value)?.label ?? "");
  let indicatorAngleDeg =
    $derived(maxIndex > 0 ? minAngleDeg + (valuePosition / maxIndex) * (maxAngleDeg - minAngleDeg) : 0);

  /** @param {number} position */
  function indexAtPosition(position) {
    const clamped = Math.min(maxIndex, Math.max(0, position));

    return options[clamped]?.index ?? value;
  }

  /** @param {number} clientY */
  function indexFromDrag(clientY) {
    const startPosition = options.findIndex((option) => option.index === dragStartValue);
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return indexAtPosition(startPosition + steps);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    absorbPointerDragFocus(event);
    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    const next = indexFromDrag(event.clientY);

    if (next !== value) onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) onValueChange(resetValue);
  }
</script>

<div class="transition-opacity duration-200 {muted ? 'opacity-75' : ''}">
  <div
    data-cursor="vertical-drag"
    class="relative h-[1.75rem] w-[1.75rem] touch-none select-none rounded-full border-[1.5px] bg-surface outline-none transition-[border-color,box-shadow] duration-75 {dragging && !muted
      ? `${accent.dragBorder} ${accent.dragShadow}`
      : muted
        ? 'border-border-subtle'
        : 'border-border-strong'}"
    role="slider"
    aria-label={ariaLabel}
    aria-valuemin={options[0]?.index}
    aria-valuemax={options[options.length - 1]?.index}
    aria-valuenow={value}
    aria-valuetext={currentLabel}
    tabindex="-1"
    onpointerdown={onPointerDown}
    onpointermove={onPointerMove}
    onpointerup={onPointerUp}
    onpointercancel={onPointerUp}
    ondblclick={onDoubleClick}
    title={resetValue !== undefined ? "Drag to change · double-click to reset" : undefined}
    onkeydown={(event) => {
      if (event.key === "ArrowUp" || event.key === "ArrowRight") {
        event.preventDefault();

        if (valuePosition < maxIndex) onValueChange(options[valuePosition + 1].index);
      } else if (event.key === "ArrowDown" || event.key === "ArrowLeft") {
        event.preventDefault();

        if (valuePosition > 0) onValueChange(options[valuePosition - 1].index);
      }
    }}
  >
    <div
      class="pointer-events-none absolute inset-0 rounded-full transition-transform duration-75"
      style:transform="rotate({indicatorAngleDeg}deg)"
      aria-hidden="true"
    >
      <span
        class="absolute left-1/2 top-[1.5px] h-0.5 w-0.5 -translate-x-1/2 rounded-[1px] {muted
          ? 'bg-surface-subtle'
          : accent.bgAccent}"
      ></span>
    </div>

    <span
      class="pointer-events-none absolute inset-0 flex items-center justify-center font-mono text-[9px] leading-none font-semibold tabular-nums {muted
        ? 'text-text-muted'
        : dragging
          ? accent.textAccentLight
          : 'text-text'}"
      aria-hidden="true"
    >
      {currentLabel}
    </span>
  </div>
</div>
