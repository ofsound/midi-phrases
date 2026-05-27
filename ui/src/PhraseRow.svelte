<script>
  import { flip } from "svelte/animate";
  import gsap from "gsap";
  import { dragHandle, dragHandleZone, TRIGGERS } from "svelte-dnd-action";
  import DurationBar from "./DurationBar.svelte";
  import NoteDragInput from "./NoteDragInput.svelte";
  import VelocityDragInput from "./VelocityDragInput.svelte";
  import StepInsertZone from "./StepInsertZone.svelte";
  import { isShadowItem, withoutShadowItems } from "./dndUtils.js";
  import {
    maxMultiplierCellWidthPx,
    minMultiplierCellWidthPx,
    multiplierIndexFromWidth,
    multiplierLabelForIndex,
    stepCellWidthPx,
    stepInsertZoneWidthPx,
  } from "./stepCellLayout.js";

  export let row = 0;
  /** @type {string[]} */
  export let stepIds = [];
  /** @type {number[]} */
  export let notes = [];
  /** @type {number[]} */
  export let stepDurationFraction = [];
  /** @type {number[]} */
  export let stepTimingMultiplier = [];
  /** @type {number[]} */
  export let stepVelocity = [];
  /** @type {boolean[]} */
  export let activeGates = [];
  /** @type {{ index: number, label: string }[]} */
  export let timingMultiplierOptions = [];

  /** @type {(row: number, orderedIds: string[]) => void} */
  export let onReorder = () => {};
  /** @type {(row: number, beforeIds: string[], afterIds: string[]) => void | Promise<void>} */
  export let onMoveCommitted = () => {};
  /** @type {(row: number, step: number) => void | Promise<void>} */
  export let onRemoveStep = () => {};
  /** @type {(row: number, step: number) => void | Promise<void>} */
  export let onInsertStep = () => {};
  /** @type {(row: number, step: number, midi: number) => void | Promise<void>} */
  export let onNoteChange = () => {};
  /** @type {(row: number, step: number, multiplierIndex: number) => void | Promise<void>} */
  export let onMultiplierChange = () => {};
  /** @type {(row: number, step: number, fraction: number) => void | Promise<void>} */
  export let onDurationChange = () => {};
  /** @type {(row: number, step: number, value: number) => void | Promise<void>} */
  export let onVelocityChange = () => {};

  const flipDurationMs = 200;
  const removeBlockMs = 500;

  /** @type {{ id: string }[]} */
  let dndItems = stepIds.map((id) => ({ id }));
  let isDragging = false;
  let removeBlocked = false;
  /** @type {ReturnType<typeof setTimeout> | null} */
  let removeBlockTimeout = null;
  /** @type {string[] | null} */
  let idsBeforeDrag = null;
  /** @type {string | null} */
  let draggedStepId = null;
  /** @type {{ step: number, startX: number, startWidth: number, previewIndex: number, displayWidth: number } | null} */
  let activeResize = null;
  /** @type {gsap.core.Tween | null} */
  let widthTween = null;

  $: reorderDisabled = stepIds.length <= 1;

  $: dndZoneOptions = {
    items: dndItems,
    flipDurationMs,
    type: `phrase-row-${row}`,
    dropFromOthersDisabled: true,
    morphDisabled: true,
    transformDraggedElement,
  };

  function blockRemoveTemporarily() {
    removeBlocked = true;

    if (removeBlockTimeout) {
      clearTimeout(removeBlockTimeout);
    }

    removeBlockTimeout = setTimeout(() => {
      removeBlocked = false;
      removeBlockTimeout = null;
    }, removeBlockMs);
  }

  function beginDragSession() {
    isDragging = true;
    blockRemoveTemporarily();
  }

  function endDragSession() {
    isDragging = false;
    draggedStepId = null;
    blockRemoveTemporarily();
  }

  /** @param {string} stepId */
  function stepIndexFromId(stepId) {
    return stepIds.indexOf(stepId);
  }

  /** @param {number} step */
  function cellWidthForStep(step) {
    if (activeResize?.step === step) {
      return Math.round(activeResize.displayWidth);
    }

    return stepCellWidthPx(stepTimingMultiplier[step]);
  }

  /** @param {number} step */
  function multiplierLabelForStep(step) {
    const index =
      activeResize?.step === step ? activeResize.previewIndex : stepTimingMultiplier[step];

    return multiplierLabelForIndex(index, timingMultiplierOptions);
  }

  /** @param {string | null} stepId */
  function cellWidthForStepId(stepId) {
    if (!stepId) return stepCellWidthPx(2);

    const step = stepIndexFromId(stepId);

    if (step < 0) return stepCellWidthPx(2);

    return cellWidthForStep(step);
  }

  /** @param {number} widthPx */
  function fixedFlexStyle(widthPx) {
    return `flex: 0 0 ${widthPx}px; width: ${widthPx}px; min-width: ${widthPx}px; max-width: ${widthPx}px;`;
  }

  /** @param {number} insertStep */
  function gapInsertStyle() {
    return `left: -${stepInsertZoneWidthPx}px; width: ${stepInsertZoneWidthPx}px;`;
  }

  $: layoutFingerprint = `${stepIds.length}:${stepTimingMultiplier.join(",")}`;
  let appliedLayoutFingerprint = "";

  /** @type {{ cellWidth: number, step: number, gapBefore: boolean }[]} */
  $: rowCellLayouts = dndItems.map((item, index) => {
    const cellWidth = isShadowItem(item)
      ? cellWidthForStepId(draggedStepId)
      : cellWidthForStepId(item.id);
    const step = isShadowItem(item) ? -1 : stepIndexFromId(item.id);

    return {
      cellWidth,
      step,
      gapBefore: index > 0,
    };
  });

  // Sync when steps are inserted/removed, parent order reverts, or cell widths change.
  $: if (!isDragging) {
    const currentIds = withoutShadowItems(dndItems).map((item) => item.id);
    const idsOutOfSync =
      currentIds.length !== stepIds.length ||
      currentIds.some((id, index) => id !== stepIds[index]);
    const layoutOutOfSync = layoutFingerprint !== appliedLayoutFingerprint;

    if (idsOutOfSync || layoutOutOfSync) {
      dndItems = stepIds.map((id) => ({ id }));
      appliedLayoutFingerprint = layoutFingerprint;
    }
  }

  /** @param {CustomEvent} event */
  function handleConsider(event) {
    if (reorderDisabled) return;

    if (event.detail.info.trigger === TRIGGERS.DRAG_STARTED) {
      beginDragSession();
      idsBeforeDrag = stepIds.slice();
      draggedStepId = event.detail.info.id;
    }

    dndItems = event.detail.items;
  }

  /** @param {CustomEvent} event */
  async function handleFinalize(event) {
    if (reorderDisabled) return;

    const trigger = event.detail.info.trigger;

    endDragSession();

    const filtered = withoutShadowItems(event.detail.items);

    if (trigger === TRIGGERS.DROPPED_OUTSIDE_OF_ANY && idsBeforeDrag) {
      dndItems = idsBeforeDrag.map((id) => ({ id }));
      idsBeforeDrag = null;
      return;
    }

    if (filtered.length !== stepIds.length) {
      if (idsBeforeDrag) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
      }

      idsBeforeDrag = null;
      return;
    }

    dndItems = filtered;
    const afterIds = filtered.map((item) => item.id);

    onReorder(row, afterIds);

    if (idsBeforeDrag) {
      await onMoveCommitted(row, idsBeforeDrag, afterIds);
      idsBeforeDrag = null;
    }
  }

  /** @type {import('svelte-dnd-action').TransformDraggedElementFunction} */
  function transformDraggedElement(element) {
    if (!element) return;

    element.style.setProperty("opacity", "1", "important");
    element.style.setProperty("visibility", "visible", "important");
    element.querySelector("[data-remove-button]")?.style.setProperty("display", "none");
    element.querySelector("[data-insert-slot]")?.style.setProperty("display", "none");
    element.querySelector("[data-multiplier-resize]")?.style.setProperty("display", "none");
  }

  /** @param {PointerEvent} event */
  function stopPointerPropagation(event) {
    event.stopPropagation();
  }

  /** @param {MouseEvent} event @param {number} step */
  function handleRemoveClick(event, step) {
    event.stopPropagation();

    if (removeBlocked || isDragging || activeResize || step < 0 || step >= stepIds.length) return;

    onRemoveStep(row, step);
  }

  /** @param {PointerEvent} event @param {number} step */
  function beginMultiplierResize(event, step) {
    event.stopPropagation();
    event.preventDefault();

    if (isDragging || removeBlocked) return;

    widthTween?.kill();

    const handle = /** @type {HTMLElement} */ (event.currentTarget);
    handle.setPointerCapture(event.pointerId);

    const previewIndex = stepTimingMultiplier[step];
    const displayWidth = stepCellWidthPx(previewIndex);

    activeResize = {
      step,
      startX: event.clientX,
      startWidth: displayWidth,
      previewIndex,
      displayWidth,
    };
  }

  /** @param {PointerEvent} event */
  function updateMultiplierResize(event) {
    if (!activeResize) return;

    const rawWidth = Math.min(
      maxMultiplierCellWidthPx(),
      Math.max(
        minMultiplierCellWidthPx(),
        activeResize.startWidth + (event.clientX - activeResize.startX),
      ),
    );
    const nextIndex = multiplierIndexFromWidth(rawWidth);

    if (nextIndex === activeResize.previewIndex) return;

    const targetWidth = stepCellWidthPx(nextIndex);
    activeResize.previewIndex = nextIndex;

    widthTween?.kill();

    const state = activeResize;
    widthTween = gsap.to(state, {
      displayWidth: targetWidth,
      duration: 0.12,
      ease: "power2.out",
      onUpdate: () => {
        state.displayWidth = Math.round(state.displayWidth);
        activeResize = activeResize;
      },
    });
  }

  /** @param {PointerEvent} event */
  async function finishMultiplierResize(event) {
    if (!activeResize) return;

    widthTween?.kill();

    const { step, previewIndex } = activeResize;
    const targetWidth = stepCellWidthPx(previewIndex);

    activeResize.displayWidth = targetWidth;
    activeResize = activeResize;

    /** @type {HTMLElement} */ (event.currentTarget).releasePointerCapture(event.pointerId);

    const committedIndex = stepTimingMultiplier[step];

    if (previewIndex !== committedIndex) {
      await onMultiplierChange(row, step, previewIndex);
    }

    activeResize = null;
  }

  /** @param {PointerEvent} event */
  function cancelMultiplierResize(event) {
    if (!activeResize) return;

    widthTween?.kill();
    /** @type {HTMLElement} */ (event.currentTarget).releasePointerCapture(event.pointerId);
    activeResize = null;
  }

  /** @param {{ id: string }} item @param {number} index */
  function layoutForItem(item, index) {
    return rowCellLayouts[index] ?? {
      cellWidth: stepCellWidthPx(2),
      step: -1,
      gapBefore: index > 0,
    };
  }

  const stepCellPlaybackClass = (active) =>
    active
      ? "border-emerald-300 ring-2 ring-emerald-400/90"
      : "border-zinc-700";
</script>

{#snippet stepCell(step, reorderEnabled)}
  {@const multiplierLabel = multiplierLabelForStep(step)}
  <div class="relative h-full w-full min-w-0">
    <button
      type="button"
      data-remove-button
      aria-label="Remove step"
      disabled={removeBlocked}
      class="absolute top-0 left-0 z-40 flex h-4 w-4 -translate-x-1/2 -translate-y-1/2 items-center justify-center rounded-full border border-white/90 bg-zinc-700 text-white shadow-md transition-colors outline-none hover:bg-zinc-600 focus:border-emerald-400 focus:ring-1 focus:ring-emerald-400 disabled:pointer-events-none disabled:opacity-50"
      onmousedown={(event) => event.stopPropagation()}
      onclick={(event) => handleRemoveClick(event, step)}
    >
      <svg viewBox="0 0 10 10" class="pointer-events-none h-2 w-2" aria-hidden="true">
        <path
          d="M2 2 L8 8 M8 2 L2 8"
          fill="none"
          stroke="currentColor"
          stroke-width="1.75"
          stroke-linecap="round"
        />
      </svg>
    </button>

    <div
      class="relative flex min-w-0 flex-col overflow-hidden rounded-lg border bg-zinc-900 outline-none transition-[border-color,box-shadow] duration-200 {stepCellPlaybackClass(
        activeGates[step],
      )} focus-within:border-emerald-500 focus-within:ring-1 focus-within:ring-emerald-500"
    >
      {#if reorderEnabled}
        <div
          use:dragHandle
          aria-label="Drag to reorder step"
          class="flex h-5 w-full shrink-0 cursor-grab items-center border-b border-zinc-800 bg-zinc-800/60 pl-3 active:cursor-grabbing"
        >
          <div class="ml-auto flex h-full items-center px-2">
            <svg viewBox="0 0 12 6" class="h-2 w-3 text-zinc-400" aria-hidden="true">
              <path
                d="M1 1 H11 M1 5 H11"
                fill="none"
                stroke="currentColor"
                stroke-width="1.75"
                stroke-linecap="round"
              />
            </svg>
          </div>
        </div>
      {:else}
        <div
          role="presentation"
          aria-label="Reorder unavailable for single step"
          class="flex h-5 w-full shrink-0 cursor-default items-center border-b border-zinc-800 bg-zinc-800/60 pl-3 opacity-60"
          onpointerdown={stopPointerPropagation}
        >
          <div class="ml-auto flex h-full items-center px-2">
            <svg viewBox="0 0 12 6" class="h-2 w-3 text-zinc-400" aria-hidden="true">
              <path
                d="M1 1 H11 M1 5 H11"
                fill="none"
                stroke="currentColor"
                stroke-width="1.75"
                stroke-linecap="round"
              />
            </svg>
          </div>
        </div>
      {/if}

      <div class="flex min-h-0 min-w-0 flex-1 flex-col px-2 py-1.5">
        <DurationBar
          value={stepDurationFraction[step]}
          velocity={stepVelocity[step]}
          ariaLabel="Step duration fraction"
          onValueChange={(fraction) => onDurationChange(row, step, fraction)}
        />
        <div class="relative mt-2 flex min-h-0 flex-1 flex-col justify-between">
          <NoteDragInput
            value={notes[step]}
            ariaLabel="Step note"
            onValueChange={(midi) => onNoteChange(row, step, midi)}
          />
          <VelocityDragInput
            value={stepVelocity[step]}
            ariaLabel="Step velocity"
            onValueChange={(value) => onVelocityChange(row, step, value)}
          />
          <span
            class="pointer-events-none absolute right-4 bottom-0 font-sans text-2xl leading-none font-bold tabular-nums text-zinc-100"
            aria-hidden="true"
          >
            {multiplierLabel}
          </span>
        </div>
      </div>

      <button
        type="button"
        data-multiplier-resize
        aria-label="Resize step timing multiplier"
        disabled={isDragging || removeBlocked}
        class="absolute right-0 bottom-0 z-30 flex h-6 w-5 cursor-ew-resize items-center justify-center rounded-tr-lg border-0 bg-transparent text-zinc-400 outline-none hover:text-zinc-200 focus-visible:text-emerald-300 disabled:pointer-events-none disabled:opacity-50"
        onpointerdown={(event) => beginMultiplierResize(event, step)}
        onpointermove={updateMultiplierResize}
        onpointerup={finishMultiplierResize}
        onpointercancel={cancelMultiplierResize}
      >
        <svg viewBox="0 0 6 12" class="pointer-events-none h-3 w-1.5" aria-hidden="true">
          <path
            d="M1 1 V11 M5 1 V11"
            fill="none"
            stroke="currentColor"
            stroke-width="1.75"
            stroke-linecap="round"
          />
        </svg>
      </button>
    </div>
  </div>
{/snippet}

{#snippet gapInsert(insertStep)}
  <div
    data-insert-slot
    class="pointer-events-auto absolute inset-y-0 z-50"
    style={gapInsertStyle()}
  >
    <StepInsertZone onInsert={() => onInsertStep(row, insertStep)} />
  </div>
{/snippet}

<div class="flex min-w-0 flex-1 overflow-x-auto overflow-y-visible pt-2 pl-2">
  <div class="relative z-50 shrink-0 self-stretch">
    <StepInsertZone onInsert={() => onInsertStep(row, 0)} />
  </div>

  {#if reorderDisabled}
    <div class="flex w-max shrink-0 items-stretch overflow-visible">
      {#each stepIds as stepId, step (stepId)}
        {@const cellWidth = cellWidthForStep(step)}
        <div
          class="relative shrink-0 overflow-visible"
          style={fixedFlexStyle(cellWidth)}
          style:margin-left={step > 0 ? `${stepInsertZoneWidthPx}px` : undefined}
        >
          {#if step > 0}
            {@render gapInsert(step)}
          {/if}
          <div class="pointer-events-auto h-full">
            {@render stepCell(step, false)}
          </div>
        </div>
      {/each}
    </div>
  {:else}
    <div
      use:dragHandleZone={dndZoneOptions}
      onconsider={handleConsider}
      onfinalize={handleFinalize}
      class="flex w-max shrink-0 items-stretch overflow-visible"
    >
      {#each dndItems as item, index (`${item.id}:${layoutFingerprint}`)}
        {@const layout = layoutForItem(item, index)}
        <div
          animate:flip={{ duration: flipDurationMs }}
          class="relative shrink-0 overflow-visible {isShadowItem(item) ? 'pointer-events-none' : ''}"
          style={fixedFlexStyle(layout.cellWidth)}
          style:margin-left={layout.gapBefore ? `${stepInsertZoneWidthPx}px` : undefined}
          aria-hidden={isShadowItem(item) ? true : undefined}
        >
          {#if layout.gapBefore && !isShadowItem(item)}
            {@render gapInsert(layout.step)}
          {/if}
          {#if isShadowItem(item)}
            <div class="shrink-0" style={fixedFlexStyle(layout.cellWidth)}></div>
          {:else}
            <div class="pointer-events-auto h-full">
              {@render stepCell(layout.step, true)}
            </div>
          {/if}
        </div>
      {/each}
    </div>
  {/if}

  <div class="relative z-50 shrink-0 self-stretch">
    <StepInsertZone onInsert={() => onInsertStep(row, stepIds.length)} />
  </div>
</div>
