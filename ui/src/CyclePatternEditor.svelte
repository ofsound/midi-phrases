<script>
  import { absorbPointerDragFocus, releasePointerDragFocus } from "./pointerDragFocus.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    cycleLengthFromCellElements,
    isCycleCellActive,
    maxCyclePatternCells,
    normalizeEditorCyclePattern,
    resizeCyclePattern,
    toggleCycleCell,
  } from "./cyclePattern.js";

  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {number} [cycle]
   * @property {number} [cycleMask]
   * @property {string} [ariaLabel]
   * @property {boolean} [compact]
   * @property {string} [playbackKey]
   * @property {number} [playbackTriggerCount]
   * @property {() => void} [onGestureStart]
   * @property {(cycle: number, cycleMask: number) => void | Promise<void>} [onPatternPreview]
   * @property {(cycle: number, cycleMask: number) => void | Promise<void>} [onPatternCommit]
   */

  /** @type {Props} */
  let {
    accent = emeraldRowAccent,
    cycle = 1,
    cycleMask = 1,
    ariaLabel = "Step cycle pattern",
    compact = false,
    playbackKey = "",
    playbackTriggerCount = -1,
    onGestureStart = () => {},
    onPatternPreview = () => {},
    onPatternCommit = () => {},
  } = $props();

  /** @type {Array<HTMLButtonElement | undefined>} */
  const cellRefs = Array.from({length: maxCyclePatternCells}, () => undefined);

  /**
   * @param {number} index
   * @returns {import('svelte/attachments').Attachment<HTMLButtonElement>}
   */
  function registerCell(index) {
    return (element) => {
      cellRefs[index] = element;

      return () => {
        if (cellRefs[index] === element) {
          cellRefs[index] = undefined;
        }
      };
    };
  }

  let draftCycle = $state(1);
  let draftMask = $state(1);
  let draggingLength = $state(false);
  /** @type {number} */
  let dragPointerId = -1;

  $effect(() => {
    if (draggingLength) return;

    const next = normalizeEditorCyclePattern(cycle, cycleMask);
    draftCycle = next.cycle;
    draftMask = next.mask;
  });

  /** @param {number} nextCycle @param {number} nextMask */
  function applyDraft(nextCycle, nextMask) {
    const next = normalizeEditorCyclePattern(nextCycle, nextMask);
    draftCycle = next.cycle;
    draftMask = next.mask;
    onPatternPreview(next.cycle, next.mask);
  }

  /** @param {number} clientX */
  function applyLengthFromClientX(clientX) {
    const next = resizeCyclePattern(
      draftCycle,
      draftMask,
      cycleLengthFromCellElements(clientX, cellRefs),
    );

    draftCycle = next.cycle;
    draftMask = next.mask;
    onPatternPreview(next.cycle, next.mask);
  }

  /** @param {number} index */
  function onCellPointerDown(index) {
    if (draggingLength) return;

    if (index >= draftCycle) {
      onGestureStart();
      applyDraft(index + 1, draftMask);
      onPatternCommit(draftCycle, draftMask);
      return;
    }

    const next = toggleCycleCell(draftCycle, draftMask, index);
    onGestureStart();
    applyDraft(next.cycle, next.mask);
    onPatternCommit(draftCycle, draftMask);
  }

  /** @param {PointerEvent} event */
  function onHandlePointerDown(event) {
    absorbPointerDragFocus(event);
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    onGestureStart();
    draggingLength = true;
    dragPointerId = event.pointerId;
    applyLengthFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onHandlePointerMove(event) {
    if (!draggingLength || event.pointerId !== dragPointerId) return;

    applyLengthFromClientX(event.clientX);
  }

  /** @param {PointerEvent} event */
  function onHandlePointerUp(event) {
    if (!draggingLength || event.pointerId !== dragPointerId) return;

    draggingLength = false;
    dragPointerId = -1;
    event.currentTarget.releasePointerCapture(event.pointerId);
    releasePointerDragFocus(event);
    onPatternCommit(draftCycle, draftMask);
  }

  let shellClass = $derived(
    compact
      ? "relative min-w-0 rounded-md border border-border bg-surface/60 p-0.5"
      : "relative min-w-0 rounded-md border border-border bg-surface/60 p-1",
  );
  let gridClass = $derived(compact ? "grid grid-cols-8 gap-0.5" : "grid grid-cols-8 gap-1");
  let cellClass = $derived(compact ? "cycle-cell relative z-[1] h-5 rounded-sm border" : "cycle-cell relative z-[1] h-8 rounded-sm border");
  let handleClass = $derived(
    compact
      ? "cycle-handle pointer-events-auto z-10 flex h-5 w-2.5 translate-x-1/2 touch-none select-none items-center justify-center rounded-sm border border-border bg-surface text-text-muted shadow-sm outline-none hover:border-border-strong hover:text-text"
      : "cycle-handle pointer-events-auto z-10 flex h-8 w-3.5 translate-x-1/2 touch-none select-none items-center justify-center rounded-sm border border-border bg-surface text-text-muted shadow-sm outline-none hover:border-border-strong hover:text-text",
  );
  let overlayInsetClass = $derived(compact ? "pointer-events-none absolute inset-0.5 grid grid-cols-8 gap-0.5" : "pointer-events-none absolute inset-1 grid grid-cols-8 gap-1");
  let playbackPhase = $derived(
    playbackTriggerCount < 0
      ? -1
      : ((playbackTriggerCount % draftCycle) + draftCycle) % draftCycle,
  );
</script>

<div class="flex min-h-0 w-full min-w-0 flex-col" role="group" aria-label={ariaLabel}>
  <div class={shellClass}>
    <div class={gridClass}>
      {#each Array.from({length: maxCyclePatternCells}, (_, index) => index) as index (index)}
        {@const inPattern = index < draftCycle}
        {@const active = inPattern && isCycleCellActive(draftCycle, draftMask, index)}
        {@const playbackFlashing = active && index === playbackPhase}
        <button
          type="button"
          {@attach registerCell(index)}
          data-cursor="pointer"
          class="{cellClass} transition-colors outline-none {inPattern
            ? active
              ? `${accent.dragBorder} ${accent.bgAccent}`
              : 'border-border-strong bg-surface-subtle shadow-[inset_0_1px_0_color-mix(in_srgb,var(--color-text)_8%,transparent)] hover:border-text-muted'
            : 'border-border/20 bg-app/30 opacity-30 hover:border-border hover:bg-surface/60 hover:opacity-80'}"
          aria-label={inPattern
            ? `Cycle step ${index + 1}, ${active ? "on" : "off"}`
            : `Cycle step ${index + 1}, extend pattern`}
          aria-pressed={inPattern ? active : undefined}
          data-cycle-playback-flash={playbackFlashing ? true : undefined}
          onpointerdown={(event) => {
            event.preventDefault();
            onCellPointerDown(index);
          }}
        >
          {#if playbackFlashing}
            {#key `${playbackKey}:${playbackTriggerCount}`}
              <span
                class="cycle-playback-pulse pointer-events-none absolute -inset-px rounded-[inherit] {accent.textAccentLight}"
                aria-hidden="true"
              ></span>
            {/key}
          {/if}
        </button>
      {/each}
    </div>

    <div
      class={overlayInsetClass}
      aria-hidden="true"
    >
      <div
        class="pointer-events-none flex items-stretch justify-end"
        style:grid-column={draftCycle}
      >
        <div
          data-cursor="horizontal-drag"
          class="{handleClass} {accent.ringFocusWithWidth} {draggingLength
            ? `${accent.dragBorder} ${accent.dragShadow}`
            : ''}"
          role="slider"
          tabindex="0"
          aria-label="Cycle length"
          aria-valuemin={1}
          aria-valuemax={maxCyclePatternCells}
          aria-valuenow={draftCycle}
          aria-valuetext={`${draftCycle} steps`}
          onpointerdown={onHandlePointerDown}
          onpointermove={onHandlePointerMove}
          onpointerup={onHandlePointerUp}
          onpointercancel={onHandlePointerUp}
          title="Drag horizontally to set cycle length"
        >
          <span class="flex gap-0.5" aria-hidden="true">
            <span class="block {compact ? 'h-2 w-px' : 'h-2.5 w-px'} bg-current"></span>
            <span class="block {compact ? 'h-2 w-px' : 'h-2.5 w-px'} bg-current"></span>
          </span>
        </div>
      </div>
    </div>
  </div>
</div>

<style>
  .cycle-playback-pulse {
    background: currentColor;
    box-shadow:
      0 0 4px currentColor,
      0 0 10px currentColor;
    animation: cycle-playback-blink 180ms ease-out both;
  }

  @keyframes cycle-playback-blink {
    0% {
      opacity: 0;
      transform: scale(0.82);
    }

    25% {
      opacity: 0.95;
      transform: scale(1.06);
    }

    100% {
      opacity: 0;
      transform: scale(1.12);
    }
  }
</style>
