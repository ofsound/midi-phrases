<script>
  import { durationBarFillOpacity } from "./compactStepVisuals.js";
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import StepMutedOverlay from "./StepMutedOverlay.svelte";

  
  
  
  
  
  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted] - Row off or step skipped — gray duration chrome, no hatch.
   * @property {boolean} [stepMuted] - Per-step mute — front only: empty duration bar + hatch on track background.
   * @property {number} [value]
   * @property {number} [velocity]
   * @property {any} [resetValue] - Duration fraction restored on double-click; omit to disable reset.
   * @property {string} [ariaLabel]
   * @property {boolean} [deferCommit] - Preview while dragging; commit on release.
   * @property {() => void} [onGestureStart] - Called at drag start when {@link deferCommit} is true.
   * @property {(value: number) => void} [onValuePreview] - Lightweight preview while dragging.
   * @property {(value: number) => void | Promise<void>} [onValueCommit] - Final commit on release.
   * @property {(value: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    accent = emeraldRowAccent,
    muted = false,
    stepMuted = false,
    value = 1,
    velocity = 127,
    resetValue = undefined,
    ariaLabel = "Step duration",
    deferCommit = false,
    onGestureStart = undefined,
    onValuePreview = undefined,
    onValueCommit = undefined,
    onValueChange = () => {}
  } = $props();

  const snapValues = [0, 0.25, 0.5, 0.75, 1];

  /** @type {HTMLDivElement | null} */
  let trackEl = $state(null);
  let dragging = $state(false);

  let fillPercent = $derived(Math.min(100, Math.max(0, value * 100)));
  let fillOpacity = $derived(durationBarFillOpacity(velocity));
  let showMutedHatch = $derived(stepMuted && !muted);
  let displayFillPercent = $derived(showMutedHatch ? 0 : fillPercent);

  /** @param {HTMLDivElement} node */
  function trackAttachment(node) {
    trackEl = node;

    return () => {
      if (trackEl === node) {
        trackEl = null;
      }
    };
  }

  function clampFraction(fraction) {
    return Math.min(1, Math.max(0, fraction));
  }

  function snapFraction(fraction) {
    return snapValues.reduce((closest, snapValue) =>
      Math.abs(snapValue - fraction) < Math.abs(closest - fraction) ? snapValue : closest,
    );
  }

  function nextSnapValue(direction) {
    const epsilon = 0.0001;

    if (direction < 0) {
      for (let index = snapValues.length - 1; index >= 0; index -= 1) {
        if (snapValues[index] < value - epsilon) return snapValues[index];
      }

      return snapValues[0];
    }

    for (const snapValue of snapValues) {
      if (snapValue > value + epsilon) return snapValue;
    }

    return snapValues[snapValues.length - 1];
  }

  function fractionFromClientX(clientX) {
    if (!trackEl) return value;

    const rect = trackEl.getBoundingClientRect();
    const ratio = (clientX - rect.left) / rect.width;

    return clampFraction(ratio);
  }

  function updateFromClientX(clientX, constrained) {
    const fraction = fractionFromClientX(clientX);
    const next = constrained ? snapFraction(fraction) : fraction;

    if (Math.abs(next - value) < 0.0001) return;

    if (deferCommit && onValuePreview) {
      onValuePreview(next);
      return;
    }

    onValueChange(next);
  }

  /** @param {number} next */
  function applyValue(next) {
    if (deferCommit && onValueCommit) {
      onGestureStart?.();
      onValuePreview?.(next);
      onValueCommit(next);
      return;
    }

    onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerDown(event) {
    if (event.target !== trackEl && !trackEl?.contains(event.target)) return;

    absorbPointerDragFocus(event);
    trackEl?.setPointerCapture(event.pointerId);
    dragging = true;

    if (deferCommit) {
      onGestureStart?.();
    }

    updateFromClientX(event.clientX, event.shiftKey);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerMove(event) {
    if (!dragging) return;

    updateFromClientX(event.clientX, event.shiftKey);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerUp(event) {
    if (deferCommit && onValueCommit && dragging) {
      const next = fractionFromClientX(event.clientX);

      onValueCommit(next);
    }

    dragging = false;
    trackEl?.releasePointerCapture(event.pointerId);
    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (Math.abs(value - resetValue) > 0.0001) applyValue(resetValue);
  }
</script>

<div class="flex min-w-0 w-full flex-col">
  <div
    {@attach trackAttachment}
    data-step-duration
    data-cursor="pointer"
    class="duration-track mp-duration-track-gradient relative h-3.5 touch-none select-none overflow-hidden outline-none {accent.ringFocusWithWidth} {muted
      ? 'bg-surface-muted'
      : 'bg-surface-subtle'}"
    role="slider"
    aria-label={ariaLabel}
    aria-valuemin={0}
    aria-valuemax={1}
    aria-valuenow={value}
    tabindex="-1"
    onpointerdown={onTrackPointerDown}
    onpointermove={onTrackPointerMove}
    onpointerup={onTrackPointerUp}
    onpointercancel={onTrackPointerUp}
    ondblclick={onDoubleClick}
    title={resetValue !== undefined
      ? "Drag to change · hold Shift to snap · double-click to reset"
      : "Drag to change · hold Shift to snap"}
    onkeydown={(event) => {
      if (event.key === "ArrowLeft" || event.key === "ArrowDown") {
        event.preventDefault();

        if (value > 0) applyValue(event.shiftKey ? nextSnapValue(-1) : clampFraction(value - 0.05));
      } else if (event.key === "ArrowRight" || event.key === "ArrowUp") {
        event.preventDefault();

        if (value < 1) applyValue(event.shiftKey ? nextSnapValue(1) : clampFraction(value + 0.05));
      }
    }}
  >
    {#if showMutedHatch}
      <StepMutedOverlay active={true} />
    {/if}
    {#if displayFillPercent > 0}
      <div
        class="duration-fill mp-duration-fill-gradient absolute inset-y-0 left-0 {muted ? 'bg-surface-subtle' : accent.bgAccent} {dragging
          ? ''
          : 'transition-[width,opacity] duration-75'}"
        style:width="{displayFillPercent}%"
        style:opacity={fillOpacity}
      ></div>
    {/if}
  </div>
</div>
