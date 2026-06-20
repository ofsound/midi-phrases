<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    defaultPulseIndex,
    pulseNoteKinds,
    pulseOptions,
  } from "./pulseLayout.js";
  import PulseNoteIcon from "./PulseNoteIcon.svelte";

  /**
   * @typedef {Object} Props
   * @property {any} [value]
   * @property {any} [resetValue]
   * @property {boolean} [muted]
   * @property {string} [ariaLabel]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(index: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    value = defaultPulseIndex,
    resetValue = defaultPulseIndex,
    muted = false,
    ariaLabel = "Pulse length",
    accent = emeraldRowAccent,
    onValueChange = () => {},
  } = $props();

  const pixelsPerStep = 10;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = defaultPulseIndex;

  let maxIndex = $derived(Math.max(0, pulseOptions.length - 1));
  let valuePosition = $derived(
    Math.max(0, pulseOptions.findIndex((option) => option.index === value)),
  );
  let currentLabel = $derived(
    pulseOptions.find((option) => option.index === value)?.label ?? "",
  );
  let currentKind = $derived(pulseNoteKinds[value] ?? "quarter");

  /** @param {number} position */
  function indexAtPosition(position) {
    const clamped = Math.min(maxIndex, Math.max(0, position));

    return pulseOptions[clamped]?.index ?? value;
  }

  /** @param {number} clientY */
  function indexFromDrag(clientY) {
    const startPosition = pulseOptions.findIndex((option) => option.index === dragStartValue);
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

<div
  data-cursor="vertical-drag"
  class="mp-control-gradient flex h-8 w-8 shrink-0 touch-none select-none items-center justify-center rounded-md border outline-none transition-[border-color,box-shadow,filter] duration-75 {accent.borderFocusVisible} {accent.ringFocusWithWidth} {dragging && !muted
    ? `${accent.dragBorder} ${accent.dragShadow}`
    : muted
      ? 'border-border-subtle text-text-muted'
      : 'border-border text-text'}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={pulseOptions[0]?.index}
  aria-valuemax={pulseOptions[pulseOptions.length - 1]?.index}
  aria-valuenow={value}
  aria-valuetext={currentLabel}
  tabindex="-1"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  title="Drag vertically to change pulse length · double-click to reset"
  onkeydown={(event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (valuePosition < maxIndex) onValueChange(pulseOptions[valuePosition + 1].index);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (valuePosition > 0) onValueChange(pulseOptions[valuePosition - 1].index);
    }
  }}
>
  <PulseNoteIcon
    kind={currentKind}
    class="pointer-events-none h-5 w-5 {dragging && !muted ? accent.textAccentLight : ''}"
  />
</div>
