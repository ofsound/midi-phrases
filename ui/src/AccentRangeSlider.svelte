<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";

  /**
   * @typedef {Object} Props
   * @property {number} value
   * @property {number} [min]
   * @property {number} [max]
   * @property {number} [step]
   * @property {boolean} [disabled]
   * @property {string} [ariaLabel]
   * @property {() => void} [onGestureStart]
   * @property {(value: number) => void} [onValuePreview]
   * @property {(value: number) => void} [onValueCommit]
   */

  /** @type {Props} */
  let {
    value,
    min = 0,
    max = 100,
    step = 1,
    disabled = false,
    ariaLabel = "Slider",
    onGestureStart = () => {},
    onValuePreview = () => {},
    onValueCommit = () => {},
  } = $props();

  /** @type {HTMLDivElement | null} */
  let trackEl = $state(null);
  let dragging = $state(false);

  let range = $derived(Math.max(0, max - min));
  let thumbPercent = $derived(range > 0 ? ((value - min) / range) * 100 : 0);

  /** @param {number} raw */
  function snapValue(raw) {
    const clamped = Math.min(max, Math.max(min, raw));

    if (step <= 1) {
      return Math.round(clamped);
    }

    const stepped = Math.round((clamped - min) / step) * step + min;
    return Math.min(max, Math.max(min, stepped));
  }

  /** @param {number} clientX */
  function valueFromClientX(clientX) {
    if (!trackEl) return value;

    const rect = trackEl.getBoundingClientRect();
    const ratio = (clientX - rect.left) / rect.width;

    return snapValue(Math.round(ratio * range + min));
  }

  /** @param {number} clientX */
  function previewFromClientX(clientX) {
    const next = valueFromClientX(clientX);

    if (next === value) return;

    onValuePreview(next);
  }

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    if (disabled) return;

    absorbPointerDragFocus(event);
    trackEl?.setPointerCapture(event.pointerId);
    dragging = true;
    onGestureStart();
    previewFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onPointerMove(event) {
    if (!dragging || disabled) return;

    previewFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onPointerUp(event) {
    if (!dragging || disabled) return;

    const next = valueFromClientX(event.clientX);
    dragging = false;
    trackEl?.releasePointerCapture(event.pointerId);
    releasePointerDragFocus(event);
    onValueCommit(next);
  }

  /** @param {number} delta */
  function nudge(delta) {
    if (disabled) return;

    const next = snapValue(value + delta);

    if (next === value) return;

    onGestureStart();
    onValuePreview(next);
    onValueCommit(next);
  }
</script>

<div
  {@attach (node) => {
    trackEl = node;
    return () => {
      if (trackEl === node) trackEl = null;
    };
  }}
  data-cursor={disabled ? undefined : "pointer"}
  class="relative h-5 touch-none select-none {disabled ? 'pointer-events-none opacity-40' : ''}"
  role="slider"
  aria-label={ariaLabel}
  aria-valuemin={min}
  aria-valuemax={max}
  aria-valuenow={value}
  aria-disabled={disabled}
  tabindex={disabled ? -1 : 0}
  onpointerdown={onPointerDown}
  onpointermove={onPointerMove}
  onpointerup={onPointerUp}
  onpointercancel={onPointerUp}
  onkeydown={(event) => {
    if (disabled) return;

    if (event.key === "ArrowLeft" || event.key === "ArrowDown") {
      event.preventDefault();
      nudge(-step);
    } else if (event.key === "ArrowRight" || event.key === "ArrowUp") {
      event.preventDefault();
      nudge(step);
    }
  }}
>
  <div class="accent-range-track absolute inset-x-0 top-1/2 -translate-y-1/2 overflow-hidden">
    <div
      class="accent-range-fill h-full bg-accent {dragging ? '' : 'transition-[width] duration-75'}"
      style:width="{thumbPercent}%"
    ></div>
  </div>

  <div
    class="accent-range-thumb absolute top-1/2 -translate-x-1/2 -translate-y-1/2 {dragging
      ? 'scale-110'
      : 'transition-[left,transform] duration-75'}"
    style:left="{thumbPercent}%"
  ></div>
</div>

<style>
  .accent-range-track {
    height: 0.25rem;
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

  .accent-range-fill {
    border-radius: inherit;
    background-image: linear-gradient(
      180deg,
      color-mix(in srgb, var(--color-text) 26%, transparent),
      transparent 48%
    );
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, var(--color-text) 18%, transparent),
      inset 0 -1px 1px color-mix(in srgb, var(--color-app) 28%, transparent);
  }

  .accent-range-thumb {
    width: 0.875rem;
    height: 0.875rem;
    border-radius: 9999px;
    background: var(--color-accent);
    box-shadow: 0 0 6px color-mix(in srgb, var(--color-accent) 80%, transparent);
  }
</style>
