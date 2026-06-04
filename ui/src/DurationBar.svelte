<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import StepMutedOverlay from "./StepMutedOverlay.svelte";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  /** Row off or step skipped — gray duration chrome, no hatch. */
  export let muted = false;
  /** Per-step mute — front only: empty duration bar + hatch on track background. */
  export let stepMuted = false;
  export let value = 1;
  export let velocity = 127;
  /** Duration fraction restored on double-click; omit to disable reset. */
  export let resetValue = undefined;
  export let ariaLabel = "Step duration";
  /** @type {(value: number) => void | Promise<void>} */
  export let onValueChange = () => {};

  const snapValues = [0, 0.25, 0.5, 0.75, 1];

  /** @type {HTMLDivElement | null} */
  let trackEl = null;
  let dragging = false;

  $: fillPercent = Math.min(100, Math.max(0, value * 100));
  $: fillOpacity = 0.2 + (Math.min(127, Math.max(0, velocity)) / 127) * 0.8;
  $: showMutedHatch = stepMuted && !muted;
  $: displayFillPercent = showMutedHatch ? 0 : fillPercent;

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

    onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerDown(event) {
    if (event.target !== trackEl && !trackEl?.contains(event.target)) return;

    trackEl?.setPointerCapture(event.pointerId);
    dragging = true;
    updateFromClientX(event.clientX, event.shiftKey);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerMove(event) {
    if (!dragging) return;

    updateFromClientX(event.clientX, event.shiftKey);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerUp(event) {
    dragging = false;
    trackEl?.releasePointerCapture(event.pointerId);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (Math.abs(value - resetValue) > 0.0001) onValueChange(resetValue);
  }
</script>

<div class="flex min-w-0 w-full flex-col">
  <div
    bind:this={trackEl}
    data-cursor="pointer"
    class="relative h-4 touch-none select-none outline-none {accent.ringFocusWithWidth} {muted
      ? 'bg-zinc-800'
      : 'bg-zinc-600'}"
    role="slider"
    aria-label={ariaLabel}
    aria-valuemin={0}
    aria-valuemax={1}
    aria-valuenow={value}
    tabindex="0"
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

        if (value > 0) onValueChange(event.shiftKey ? nextSnapValue(-1) : clampFraction(value - 0.05));
      } else if (event.key === "ArrowRight" || event.key === "ArrowUp") {
        event.preventDefault();

        if (value < 1) onValueChange(event.shiftKey ? nextSnapValue(1) : clampFraction(value + 0.05));
      }
    }}
  >
    {#if showMutedHatch}
      <StepMutedOverlay active={true} />
    {/if}
    {#if displayFillPercent > 0}
      <div
        class="absolute inset-y-0 left-0 {muted ? 'bg-zinc-600' : accent.bgAccent} {dragging
          ? ''
          : 'transition-[width,opacity] duration-75'}"
        style:width="{displayFillPercent}%"
        style:opacity={fillOpacity}
      ></div>
    {/if}
  </div>
</div>
