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

  function fractionFromClientX(clientX) {
    if (!trackEl) return value;

    const rect = trackEl.getBoundingClientRect();
    const ratio = (clientX - rect.left) / rect.width;

    return clampFraction(ratio);
  }

  function updateFromClientX(clientX) {
    const next = fractionFromClientX(clientX);

    if (Math.abs(next - value) < 0.0001) return;

    onValueChange(next);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerDown(event) {
    if (event.target !== trackEl && !trackEl?.contains(event.target)) return;

    trackEl?.setPointerCapture(event.pointerId);
    dragging = true;
    updateFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerMove(event) {
    if (!dragging) return;

    updateFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onTrackPointerUp(event) {
    dragging = false;
    trackEl?.releasePointerCapture(event.pointerId);
  }

  /** @param {number} snapValue @param {PointerEvent} event */
  function handleTickClick(snapValue, event) {
    event.stopPropagation();

    if (snapValue === value) return;

    onValueChange(snapValue);
  }

  /** @param {MouseEvent} event */
  function onDoubleClick(event) {
    if (resetValue === undefined || dragging) return;

    event.preventDefault();

    if (Math.abs(value - resetValue) > 0.0001) onValueChange(resetValue);
  }
</script>

<div class="flex min-w-0 w-full flex-col gap-1">
  <div
    bind:this={trackEl}
    class="relative h-4 cursor-pointer touch-none select-none outline-none {accent.ringFocusWithWidth} {muted
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
    title={resetValue !== undefined ? "Drag to change · double-click to reset" : undefined}
    onkeydown={(event) => {
      const step = event.shiftKey ? 0.01 : 0.05;

      if (event.key === "ArrowLeft" || event.key === "ArrowDown") {
        event.preventDefault();

        if (value > 0) onValueChange(clampFraction(value - step));
      } else if (event.key === "ArrowRight" || event.key === "ArrowUp") {
        event.preventDefault();

        if (value < 1) onValueChange(clampFraction(value + step));
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

  <div class="relative h-2.5">
    {#each snapValues as snapValue}
      <button
        type="button"
        aria-label="Set duration to {snapValue}"
        class="absolute top-0 h-2 w-3 -translate-x-1/2 cursor-pointer border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth}"
        style:left="{snapValue * 100}%"
        onpointerdown={(event) => event.stopPropagation()}
        onclick={(event) => handleTickClick(snapValue, event)}
      >
        <span class="mx-auto block h-2 w-px bg-zinc-500" aria-hidden="true"></span>
      </button>
    {/each}
  </div>
</div>
