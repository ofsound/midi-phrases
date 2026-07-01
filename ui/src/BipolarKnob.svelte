<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { scaledPx } from "./uiScale.svelte.js";

  const baseBorderWidthPx = 1.5;
  const baseIndicatorSizePx = 4;

  
  
  
  
  /**
   * @typedef {Object} Props
   * @property {{ index: number, label: string }[]} options
   * @property {any} value
   * @property {any} [resetValue] - Option index restored on double-click; omit to disable reset.
   * @property {boolean} [muted]
   * @property {string} [ariaLabel]
   * @property {string} [sizeClass]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [deferCommit]
   * @property {(detail: { shiftKey: boolean, startValue: number, currentValue: number, positionDelta: number }) => void} [onGestureStart]
   * @property {(index: number, detail: { shiftKey: boolean, startValue: number, currentValue: number, positionDelta: number }) => void | Promise<void>} [onValuePreview]
   * @property {(index: number, detail: { shiftKey: boolean, startValue: number, currentValue: number, positionDelta: number }) => void | Promise<void>} [onValueCommit]
   * @property {(index: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    options,
    value,
    resetValue = undefined,
    muted = false,
    ariaLabel = "Bipolar knob",
    sizeClass = "h-8 w-8",
    accent = emeraldRowAccent,
    deferCommit = false,
    onGestureStart = undefined,
    onValuePreview = undefined,
    onValueCommit = undefined,
    onValueChange = () => {}
  } = $props();

  const pixelsPerStep = 10;
  const minAngleDeg = -135;
  const maxAngleDeg = 135;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;
  let dragStartPosition = 0;
  let dragShiftKey = false;
  let dragValue = $state(0);
  let dragPositionDelta = 0;

  let maxIndex = $derived(Math.max(0, options.length - 1));
  let displayedValue = $derived(dragging && deferCommit ? dragValue : value);
  let valuePosition = $derived(Math.max(0, options.findIndex((option) => option.index === displayedValue)));
  let currentLabel = $derived(options.find((option) => option.index === displayedValue)?.label ?? "");
  let indicatorAngleDeg =
    $derived(maxIndex > 0 ? minAngleDeg + (valuePosition / maxIndex) * (maxAngleDeg - minAngleDeg) : 0);

  /** @param {number} position */
  function indexAtPosition(position) {
    const clamped = Math.min(maxIndex, Math.max(0, position));

    return options[clamped]?.index ?? value;
  }

  /** @param {number} clientY */
  function positionFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return dragStartPosition + steps;
  }

  /** @param {number} currentValue @param {number} positionDelta */
  function dragDetail(currentValue, positionDelta) {
    return {
      shiftKey: dragShiftKey,
      startValue: dragStartValue,
      currentValue,
      positionDelta,
    };
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    absorbPointerDragFocus(event);
    event.currentTarget.setPointerCapture(event.pointerId);
    dragging = true;
    dragStartY = event.clientY;
    dragStartValue = value;
    dragStartPosition = Math.max(0, options.findIndex((option) => option.index === value));
    dragShiftKey = event.shiftKey;
    dragValue = value;
    dragPositionDelta = 0;

    if (deferCommit) {
      onGestureStart?.(dragDetail(value, 0));
    }
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    const position = positionFromDrag(event.clientY);
    const next = indexAtPosition(position);
    const positionDelta = position - dragStartPosition;

    if (deferCommit) {
      if (next === dragValue && positionDelta === dragPositionDelta) return;

      dragValue = next;
      dragPositionDelta = positionDelta;
      onValuePreview?.(next, dragDetail(next, positionDelta));
      return;
    }

    if (next !== value) onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    event.currentTarget.releasePointerCapture(event.pointerId);

    if (deferCommit) {
      onValueCommit?.(dragValue, dragDetail(dragValue, dragPositionDelta));
    }

    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value === resetValue) return;

    if (deferCommit) {
      onValueCommit?.(resetValue, {
        shiftKey: false,
        startValue: value,
        currentValue: resetValue,
        positionDelta: resetValue - value,
      });
      return;
    }

    onValueChange(resetValue);
  }
</script>

<div class="transition-opacity duration-200 {muted ? 'opacity-75' : ''}">
  <div
    data-cursor="vertical-drag"
    class="relative {sizeClass} touch-none select-none rounded-full border border-solid bg-surface outline-none transition-[border-color,box-shadow] duration-75 {dragging && !muted
      ? `${accent.dragBorder} ${accent.dragShadow}`
      : muted
        ? 'border-border-subtle'
        : 'border-border-strong'}"
    role="slider"
    aria-label={ariaLabel}
    aria-valuemin={options[0]?.index}
    aria-valuemax={options[options.length - 1]?.index}
    aria-valuenow={displayedValue}
    aria-valuetext={currentLabel}
    style:border-width="{scaledPx(baseBorderWidthPx)}px"
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
        class="absolute left-1/2 -translate-x-1/2 -translate-y-1/2 {muted ? 'bg-surface-subtle' : accent.bgAccent}"
        style:top="{scaledPx(-baseBorderWidthPx / 2)}px"
        style:width="{scaledPx(baseIndicatorSizePx)}px"
        style:height="{scaledPx(baseIndicatorSizePx)}px"
        style:border-radius="{scaledPx(baseIndicatorSizePx / 2)}px"
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
