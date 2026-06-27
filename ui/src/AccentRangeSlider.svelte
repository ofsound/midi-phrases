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
   * @property {string} [class]
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
    class: className = "",
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
  class={`note-bandpass-track-shell touch-none select-none ${disabled ? "pointer-events-none opacity-40" : ""} ${className}`.trim()}
  role="slider"
  tabindex={disabled ? -1 : 0}
  aria-label={ariaLabel}
  aria-valuemin={min}
  aria-valuemax={max}
  aria-valuenow={value}
  aria-disabled={disabled}
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
  <div class="note-bandpass-track" aria-hidden="true">
    <div
      class="note-bandpass-range {dragging ? '' : 'transition-[width] duration-75'}"
      style:left="0"
      style:width="{thumbPercent}%"
    ></div>
  </div>

  <div
    class="note-bandpass-thumb pointer-events-none"
    style:left="{thumbPercent}%"
  >
    <span class="note-bandpass-knob" aria-hidden="true"></span>
  </div>
</div>
