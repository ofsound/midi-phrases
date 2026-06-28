<script>
  import { onDestroy } from "svelte";
  import ShimmerParamDragInput from "./ShimmerParamDragInput.svelte";
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import {
    clampVelocityTiltAmount,
    clampVelocityTiltPivotMidi,
    defaultVelocityTiltAmount,
    defaultVelocityTiltPivotMidi,
    formatVelocityTiltAmount,
    formatVelocityTiltPivot,
    maxVelocityTiltAmount,
    minVelocityTiltAmount,
    velocityTiltOutputVelocity,
  } from "./velocityTilt.js";
  import { maxMidiNote, minMidiNote } from "./noteBandpass.js";

  /**
   * MIDI velocity tilt stage for the post-filter processing rail.
   *
   * @typedef {Object} Props
   * @property {number} pivotMidi
   * @property {number} amount
   * @property {number} bandpassLowMidi
   * @property {number} bandpassHighMidi
   * @property {() => void} [onParamGestureStart]
   * @property {(value: number) => void} [onPivotPreview]
   * @property {(value: number) => void | Promise<void>} [onPivotCommit]
   * @property {(value: number) => void} [onAmountPreview]
   * @property {(value: number) => void | Promise<void>} [onAmountCommit]
   * @property {(pivotMidi: number, amount: number) => void | Promise<void>} [onXYCommit]
   */

  /** @type {Props} */
  let {
    pivotMidi = defaultVelocityTiltPivotMidi,
    amount = defaultVelocityTiltAmount,
    bandpassLowMidi = minMidiNote,
    bandpassHighMidi = maxMidiNote,
    onParamGestureStart = () => {},
    onPivotPreview = () => {},
    onPivotCommit = () => {},
    onAmountPreview = () => {},
    onAmountCommit = () => {},
    onXYCommit = undefined,
  } = $props();

  const pixelsPerStep = 4;
  const pivotPixelsPerStep = 8;
  const previewThrottleMs = 100;
  const representativeInputVelocity = 64;

  let draggingAmount = $state(false);
  let dragStartY = 0;
  let dragStartX = 0;
  let dragStartAmount = 0;
  let dragStartPivot = 0;
  let dragAmount = $state(defaultVelocityTiltAmount);
  let dragPivot = $state(defaultVelocityTiltPivotMidi);
  let previewTimerId = 0;
  let lastPreviewAt = 0;
  /** @type {number | null} */
  let pendingPreviewAmount = null;

  let displayedAmount = $derived(draggingAmount ? dragAmount : amount);
  let pivotBounds = $derived.by(() => {
    const low = clampVelocityTiltPivotMidi(bandpassLowMidi);
    const high = clampVelocityTiltPivotMidi(bandpassHighMidi);

    return { low: Math.min(low, high), high: Math.max(low, high) };
  });
  let displayedPivot = $derived(draggingAmount ? dragPivot : pivotMidi);
  let clampedPivotMidi = $derived(
    Math.min(pivotBounds.high, Math.max(pivotBounds.low, clampVelocityTiltPivotMidi(displayedPivot))),
  );
  let clampedAmount = $derived(clampVelocityTiltAmount(displayedAmount));
  let pivotPercent = $derived(
    pivotBounds.high === pivotBounds.low
      ? 50
      : ((clampedPivotMidi - pivotBounds.low) / (pivotBounds.high - pivotBounds.low)) * 100,
  );
  let tiltPlotPoints = $derived.by(() => {
    const noteSpan = pivotBounds.high - pivotBounds.low;

    if (noteSpan === 0) return "0,50 100,50";

    const points = [];

    for (let midi = pivotBounds.low; midi <= pivotBounds.high; midi += 1) {
      const x = ((midi - pivotBounds.low) / noteSpan) * 100;
      const outputVelocity = velocityTiltOutputVelocity(
        representativeInputVelocity,
        midi,
        clampedPivotMidi,
        clampedAmount,
      );
      const y = ((127 - outputVelocity) / 126) * 100;
      points.push(`${x},${y}`);
    }

    return points.join(" ");
  });
  let tiltStyle = $derived(
    `--velocity-tilt-pivot-x: ${pivotPercent}%;`,
  );
  let amountText = $derived(formatVelocityTiltAmount(clampedAmount));
  let pivotText = $derived(formatVelocityTiltPivot(clampedPivotMidi));

  function cancelPreviewThrottle() {
    if (!previewTimerId) return;

    clearTimeout(previewTimerId);
    previewTimerId = 0;
  }

  /** @param {number} next */
  function flushAmountPreview(next) {
    cancelPreviewThrottle();
    pendingPreviewAmount = null;
    lastPreviewAt = Date.now();
    onAmountPreview(next);
  }

  /** @param {number} next */
  function scheduleAmountPreview(next) {
    pendingPreviewAmount = next;

    const elapsed = Date.now() - lastPreviewAt;

    if (elapsed >= previewThrottleMs) {
      flushAmountPreview(next);
      return;
    }

    if (previewTimerId) return;

    previewTimerId = window.setTimeout(() => {
      previewTimerId = 0;
      const pending = pendingPreviewAmount;

      if (pending !== null) flushAmountPreview(pending);
    }, previewThrottleMs - elapsed);
  }

  /** @param {number} clientY */
  function amountFromDrag(clientY) {
    const steps = Math.round((dragStartY - clientY) / pixelsPerStep);

    return clampVelocityTiltAmount(dragStartAmount + steps);
  }

  /** @param {number} clientX */
  function pivotFromDrag(clientX) {
    const steps = Math.round((clientX - dragStartX) / pivotPixelsPerStep);
    const next = dragStartPivot + steps;

    return Math.min(pivotBounds.high, Math.max(pivotBounds.low, next));
  }

  /** @param {number} next */
  function commitAmount(next) {
    onAmountCommit(clampVelocityTiltAmount(next));
  }

  /** @param {PointerEvent} event */
  function onTiltPointerDown(event) {
    absorbPointerDragFocus(event);
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);
    draggingAmount = true;
    dragStartX = event.clientX;
    dragStartY = event.clientY;
    dragStartAmount = clampVelocityTiltAmount(amount);
    dragStartPivot = clampedPivotMidi;
    dragAmount = dragStartAmount;
    dragPivot = clampedPivotMidi;
    pendingPreviewAmount = null;
    lastPreviewAt = 0;
    onParamGestureStart();
  }

  /** @param {PointerEvent} event */
  function onTiltPointerMove(event) {
    if (!draggingAmount) return;

    const nextPivot = pivotFromDrag(event.clientX);
    const next = amountFromDrag(event.clientY);

    if (nextPivot !== dragPivot) {
      dragPivot = nextPivot;
      onPivotPreview(nextPivot);
    }

    if (next !== dragAmount) {
      dragAmount = next;
      scheduleAmountPreview(next);
    }
  }

  /** @param {PointerEvent} event */
  function onTiltPointerUp(event) {
    if (!draggingAmount) return;

    draggingAmount = false;
    event.currentTarget.releasePointerCapture(event.pointerId);
    cancelPreviewThrottle();
    pendingPreviewAmount = null;

    if (dragPivot !== dragStartPivot || dragAmount !== dragStartAmount) {
      if (onXYCommit) {
        onXYCommit(dragPivot, dragAmount);
      } else {
        onPivotCommit(dragPivot);
        commitAmount(dragAmount);
      }
    }

    releasePointerDragFocus(event);
  }

  /** @param {MouseEvent} event */
  function onTiltDoubleClick(event) {
    if (draggingAmount) return;

    event.preventDefault();
    onParamGestureStart();
    commitAmount(defaultVelocityTiltAmount);
  }

  onDestroy(() => {
    cancelPreviewThrottle();
  });
</script>

<div class="velocity-tilt-control" role="group" aria-label="Velocity tilt">
  <div
    class={`velocity-tilt-icon ${clampedAmount !== 0 ? "velocity-tilt-icon-active" : ""} ${draggingAmount ? "velocity-tilt-icon-dragging" : ""}`}
    data-cursor="vertical-drag"
    role="slider"
    aria-label="Velocity tilt amount and pivot"
    aria-valuemin={minVelocityTiltAmount}
    aria-valuemax={maxVelocityTiltAmount}
    aria-valuenow={clampedAmount}
    aria-valuetext={`${amountText}, pivot ${pivotText}`}
    tabindex="-1"
    title="Velocity tilt · drag horizontally for pivot, vertically for amount · double-click to reset amount"
    style={tiltStyle}
    onpointerdown={onTiltPointerDown}
    onpointermove={onTiltPointerMove}
    onpointerup={onTiltPointerUp}
    onpointercancel={onTiltPointerUp}
    ondblclick={onTiltDoubleClick}
    onkeydown={(event) => {
      if (event.key === "ArrowUp") {
        event.preventDefault();

        if (clampedAmount < maxVelocityTiltAmount) commitAmount(clampedAmount + 1);
      } else if (event.key === "ArrowDown") {
        event.preventDefault();

        if (clampedAmount > minVelocityTiltAmount) commitAmount(clampedAmount - 1);
      }
    }}
  >
    <div class="velocity-tilt-face" aria-hidden="true">
      <svg class="velocity-tilt-plot" viewBox="0 0 100 100" preserveAspectRatio="none">
        <polyline class="velocity-tilt-line" points={tiltPlotPoints}></polyline>
      </svg>
      <span class="velocity-tilt-pivot"></span>
    </div>
  </div>

  <div class="processing-param-labeled">
    <ShimmerParamDragInput
      value={pivotMidi}
      min={minMidiNote}
      max={maxMidiNote}
      defaultValue={defaultVelocityTiltPivotMidi}
      formatValue={formatVelocityTiltPivot}
      clampValue={clampVelocityTiltPivotMidi}
      active={true}
      ariaLabel="Velocity tilt pivot note"
      title="Pivot note · drag vertically · double-click to reset"
      onGestureStart={onParamGestureStart}
      onValuePreview={onPivotPreview}
      onValueCommit={onPivotCommit}
    />
    <span class="processing-param-label" aria-hidden="true">Pivot</span>
  </div>
</div>
