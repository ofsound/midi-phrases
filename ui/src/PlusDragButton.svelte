<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { defaultStepTimingMultiplierIndex } from "./stepCellLayout.js";

  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {string} [ariaLabel]
   * @property {{ index: number, label: string }[]} [options]
   * @property {number} [defaultIndex]
   * @property {"large" | "compact"} [variant]
   * @property {string} [class]
   * @property {(multiplierIndex: number) => void | Promise<void>} [onConfirm]
   */

  /** @type {Props} */
  let {
    accent = emeraldRowAccent,
    muted = false,
    ariaLabel = "Add step",
    options = [],
    defaultIndex = defaultStepTimingMultiplierIndex,
    variant = "compact",
    class: className = "",
    onConfirm = () => {},
  } = $props();

  const pixelsPerStep = 10;
  const clickThresholdPx = 4;

  let active = $state(false);
  let dragStartY = 0;
  let dragStartIndex = 0;
  /** @type {number} */
  let previewIndex = $state(0);

  let maxOptionIndex = $derived(Math.max(0, options.length - 1));
  let displayLabel = $derived(
    options.find((option) => option.index === previewIndex)?.label ?? "1",
  );

  /** @param {number} position */
  function indexAtPosition(position) {
    const clamped = Math.min(maxOptionIndex, Math.max(0, position));

    return options[clamped]?.index ?? defaultIndex;
  }

  /** @param {number} clientY */
  function indexFromDrag(clientY) {
    const startPosition = options.findIndex((option) => option.index === dragStartIndex);
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return indexAtPosition(startPosition + steps);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    event.stopPropagation();
    event.preventDefault();
    event.currentTarget.setPointerCapture(event.pointerId);
    active = true;
    dragStartY = event.clientY;
    dragStartIndex = defaultIndex;
    previewIndex = defaultIndex;
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!active) return;

    const next = indexFromDrag(event.clientY);

    if (next !== previewIndex) previewIndex = next;
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    if (!active) return;

    const dragged = Math.abs(event.clientY - dragStartY) >= clickThresholdPx;
    const confirmedIndex = dragged ? previewIndex : defaultIndex;

    active = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    onConfirm(confirmedIndex);
  }
</script>

<button
  type="button"
  aria-label={ariaLabel}
  data-cursor={active ? "vertical-drag" : "pointer"}
  class="{variant === 'large'
    ? 'flex size-[54px] shrink-0 items-center justify-center border border-transparent bg-transparent text-[1.9rem] leading-none font-semibold outline-none focus:ring-1'
    : 'shrink-0 border-0 bg-transparent p-0 text-xl leading-none font-bold opacity-0 outline-none transition-opacity group-hover:opacity-100 group-focus-within:opacity-100'} touch-none select-none transition-colors {variant === 'compact' && active
    ? 'opacity-100'
    : ''} {muted
    ? variant === 'large'
      ? 'text-text-faint focus:ring-focus-ring'
      : 'text-text-muted'
    : active
      ? `${accent.textAccentLight}`
      : `${accent.textAccent}`} {variant === 'large' ? accent.ringFocusWithWidth : accent.insertZoneFocus} {className}"
  title="Click to add · drag vertically to set step length"
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
>
  {#if active}
    <span class="font-sans tabular-nums {variant === 'large' ? 'text-[1.35rem]' : ''}"
      >{displayLabel}</span
    >
  {:else}
    +
  {/if}
</button>
