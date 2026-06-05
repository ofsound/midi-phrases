<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  
  
  
  
  
  /**
   * @typedef {Object} Props
   * @property {{ index: number, label: string }[]} options
   * @property {any} value
   * @property {any} [resetValue] - Option index restored on double-click; omit to disable reset.
   * @property {boolean} [muted]
   * @property {string} [ariaLabel]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [compact] - Tight boxed width for compact header controls with short labels.
   * @property {(index: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    options,
    value,
    resetValue = undefined,
    muted = false,
    ariaLabel = "Option",
    accent = emeraldRowAccent,
    compact = false,
    onValueChange = () => {}
  } = $props();

  const pixelsPerStep = 10;

  let dragging = $state(false);
  let dragStartY = 0;
  let dragStartValue = 0;

  let maxIndex = $derived(Math.max(0, options.length - 1));
  let valuePosition = $derived(Math.max(0, options.findIndex((option) => option.index === value)));
  let currentLabel = $derived(options.find((option) => option.index === value)?.label ?? "");

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
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (value !== resetValue) onValueChange(resetValue);
  }
</script>

<div
  data-cursor="ns-resize"
  class="flex h-8 touch-none select-none items-center justify-center rounded-md border bg-gradient-to-b from-zinc-700/50 to-zinc-950 px-2 text-sm font-semibold tabular-nums outline-none transition-[border-color,box-shadow] duration-75 {compact
    ? 'w-14'
    : 'w-[4.5rem]'} {accent.borderFocusVisible} {accent.ringFocusWithWidth} {dragging && !muted
    ? `${accent.dragBorder} ${accent.dragShadow}`
    : muted
      ? 'border-zinc-800 text-zinc-500'
      : 'border-zinc-700 text-zinc-100'}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={options[0]?.index}
  aria-valuemax={options[options.length - 1]?.index}
  aria-valuenow={value}
  aria-valuetext={currentLabel}
  tabindex="0"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  ondblclick={onDoubleClick}
  title={resetValue !== undefined ? "Drag to change · double-click to reset" : "Drag vertically to change"}
  onkeydown={(event) => {
    if (event.key === "ArrowUp") {
      event.preventDefault();

      if (valuePosition < maxIndex) onValueChange(options[valuePosition + 1].index);
    } else if (event.key === "ArrowDown") {
      event.preventDefault();

      if (valuePosition > 0) onValueChange(options[valuePosition - 1].index);
    }
  }}
>
  <span class={dragging && !muted ? accent.textAccentLight : ""}>{currentLabel}</span>
</div>
