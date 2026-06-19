<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {any} value
   * @property {string} [label]
   * @property {boolean} [fullWidth]
   * @property {number} [min]
   * @property {number} [max]
   * @property {string} [ariaLabel]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(value: number) => string} [formatDisplay]
   * @property {(value: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    value,
    label = "",
    fullWidth = false,
    min = 0,
    max = 127,
    ariaLabel = "Slider",
    accent = emeraldRowAccent,
    formatDisplay = undefined,
    onValueChange = () => {}
  } = $props();

  /** @type {HTMLDivElement | null} */
  let trackEl = $state(null);
  let dragging = $state(false);

  let range = $derived(Math.max(0, max - min));
  let thumbPercent = $derived(range > 0 ? ((value - min) / range) * 100 : 0);
  let displayValue = $derived(
    formatDisplay ? formatDisplay(value) : String(Math.round(value)),
  );

  /** @param {HTMLDivElement} node */
  function trackAttachment(node) {
    trackEl = node;

    return () => {
      if (trackEl === node) {
        trackEl = null;
      }
    };
  }

  function valueFromClientX(clientX) {
    if (!trackEl) return value;

    const rect = trackEl.getBoundingClientRect();
    const ratio = (clientX - rect.left) / rect.width;

    return Math.min(max, Math.max(min, Math.round(ratio * range + min)));
  }

  function updateFromClientX(clientX) {
    const next = valueFromClientX(clientX);

    if (next === value) return;

    onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    absorbPointerDragFocus(event);
    trackEl?.setPointerCapture(event.pointerId);
    dragging = true;
    updateFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging) return;

    updateFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    dragging = false;
    trackEl?.releasePointerCapture(event.pointerId);
    releasePointerDragFocus(event);
  }
</script>

<div class="flex flex-col gap-1 {fullWidth ? 'min-w-0 w-full' : 'w-[10.5rem]'}">
  <div class="flex items-center justify-between gap-2">
    {#if label}
      <span class="text-[9px] font-medium uppercase tracking-wide text-text-muted">{label}</span>
    {:else}
      <span></span>
    {/if}
    <span class="font-mono text-[10px] tabular-nums {accent.textAccent}">{displayValue}</span>
  </div>

  <div
    {@attach trackAttachment}
    data-cursor="pointer"
    class="relative h-5 touch-none select-none"
    role="slider"
    aria-label={ariaLabel}
    aria-valuemin={min}
    aria-valuemax={max}
    aria-valuenow={value}
    aria-valuetext={displayValue}
    tabindex="-1"
    onpointerdown={onPointerDown}
    onpointermove={onPointerMove}
    onpointerup={onPointerUp}
    onpointercancel={onPointerUp}
    onkeydown={(event) => {
      if (event.key === "ArrowLeft" || event.key === "ArrowDown") {
        event.preventDefault();

        if (value > min) onValueChange(value - 1);
      } else if (event.key === "ArrowRight" || event.key === "ArrowUp") {
        event.preventDefault();

        if (value < max) onValueChange(value + 1);
      }
    }}
  >
    <div class="compact-slider-track absolute inset-x-0 top-1/2 -translate-y-1/2">
      <div
        class="compact-slider-range h-full {accent.bgAccent} {dragging ? '' : 'transition-[width] duration-75'}"
        style:width="{thumbPercent}%"
      ></div>
    </div>

    <div
      class="compact-slider-thumb absolute top-1/2 -translate-x-1/2 -translate-y-1/2 {dragging
        ? `${accent.dragBorder} ${accent.dragShadow}`
        : 'border-border-subtle'} {dragging
        ? ''
        : 'transition-[left] duration-75'}"
      style:left="{thumbPercent}%"
    ></div>
  </div>
</div>

<style>
  .compact-slider-track {
    height: 0.375rem;
    overflow: hidden;
    border: 1px solid color-mix(in srgb, var(--color-input-border) 78%, transparent);
    border-radius: 9999px;
    background:
      linear-gradient(180deg, color-mix(in srgb, var(--color-text) 12%, transparent), transparent 34%),
      linear-gradient(180deg, var(--color-field) 0%, var(--color-surface-muted) 46%, var(--color-surface) 100%);
    box-shadow:
      inset 0 1px 1px color-mix(in srgb, var(--color-app) 68%, transparent),
      inset 0 -1px 0 color-mix(in srgb, var(--color-text) 9%, transparent),
      0 1px 0 color-mix(in srgb, var(--color-text) 10%, transparent);
  }

  .compact-slider-range {
    border-radius: inherit;
    background-image: linear-gradient(180deg, color-mix(in srgb, var(--color-text) 26%, transparent), transparent 48%);
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, var(--color-text) 18%, transparent),
      inset 0 -1px 1px color-mix(in srgb, var(--color-app) 28%, transparent);
  }

  .compact-slider-thumb {
    width: 0.45rem;
    height: 1.15rem;
    border-width: 1px;
    border-radius: 2px;
    background:
      linear-gradient(90deg, color-mix(in srgb, var(--color-app) 36%, transparent), transparent 28%, transparent 72%, color-mix(in srgb, var(--color-app) 41%, transparent)),
      linear-gradient(180deg, color-mix(in srgb, var(--color-text) 14%, transparent), transparent 32%),
      linear-gradient(145deg, var(--color-surface-subtle) 0%, var(--color-surface-muted) 48%, var(--color-field) 100%);
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, var(--color-text) 14%, transparent),
      inset 0 -1px 2px color-mix(in srgb, var(--color-app) 58%, transparent),
      0 1px 2px color-mix(in srgb, var(--color-app) 53%, transparent);
  }

  .compact-slider-thumb::before {
    content: "";
    position: absolute;
    inset: 0.18rem 50% 0.18rem auto;
    width: 1px;
    transform: translateX(50%);
    background: color-mix(in srgb, var(--color-text) 45%, transparent);
    box-shadow: 1px 0 0 color-mix(in srgb, var(--color-app) 19%, transparent);
  }
</style>
