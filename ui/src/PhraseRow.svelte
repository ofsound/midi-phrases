<script>
  import { flip } from "svelte/animate";
  import { dragHandle, dragHandleZone, TRIGGERS } from "svelte-dnd-action";
  import DiscreteSlider from "./DiscreteSlider.svelte";
  import ContinuousSlider from "./ContinuousSlider.svelte";
  import NoteDragInput from "./NoteDragInput.svelte";
  import StepInsertZone from "./StepInsertZone.svelte";
  import { isShadowItem, withoutShadowItems } from "./dndUtils.js";
  import { stepCellWidthPx, stepInsertZoneWidthPx } from "./stepCellLayout.js";

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
  /** @type {{ index: number, label: string }[]} */
  export let durationFractionOptions = [];

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
  /** @type {(row: number, step: number, fractionIndex: number) => void | Promise<void>} */
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

  /** @param {string | null} stepId */
  function cellWidthForStepId(stepId) {
    if (!stepId) return stepCellWidthPx(2);

    const step = stepIndexFromId(stepId);

    if (step < 0) return stepCellWidthPx(2);

    return stepCellWidthPx(stepTimingMultiplier[step]);
  }

  /** @param {number} widthPx */
  function fixedFlexStyle(widthPx) {
    return `flex: 0 0 ${widthPx}px; width: ${widthPx}px; min-width: ${widthPx}px; max-width: ${widthPx}px;`;
  }

  /** @param {number} insertStep */
  function gapInsertStyle() {
    return `left: -${stepInsertZoneWidthPx}px; width: ${stepInsertZoneWidthPx}px;`;
  }

  $: layoutFingerprint = stepTimingMultiplier.join(",");
  let appliedLayoutFingerprint = "";

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
  }

  /** @param {PointerEvent} event */
  function stopPointerPropagation(event) {
    event.stopPropagation();
  }

  /** @param {MouseEvent} event @param {number} step */
  function handleRemoveClick(event, step) {
    event.stopPropagation();

    if (removeBlocked || isDragging || step < 0 || step >= stepIds.length) return;

    onRemoveStep(row, step);
  }

  /** @param {{ id: string }} item @param {number} index */
  function layoutForItem(item, index) {
    const cellWidth = isShadowItem(item)
      ? cellWidthForStepId(draggedStepId)
      : cellWidthForStepId(item.id);

    return {
      cellWidth,
      step: isShadowItem(item) ? -1 : stepIndexFromId(item.id),
      gapBefore: index > 0,
    };
  }

  const stepCellPlaybackClass = (active) =>
    active
      ? "border-emerald-300 ring-2 ring-emerald-400/90"
      : "border-zinc-700";
</script>

{#snippet stepCell(step, reorderEnabled)}
  {@const cellWidth = cellWidthForStepId(stepIds[step])}
  <div class="relative shrink-0" style={fixedFlexStyle(cellWidth)}>
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
      class="flex min-w-0 flex-col overflow-hidden rounded-lg border bg-zinc-900 outline-none transition-[border-color,box-shadow] duration-200 {stepCellPlaybackClass(
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

      <div class="flex min-h-0 min-w-0 flex-1">
        <div class="flex shrink-0 items-stretch border-r border-zinc-800">
          <NoteDragInput
            value={notes[step]}
            ariaLabel="Step note"
            onValueChange={(midi) => onNoteChange(row, step, midi)}
          />
        </div>
        <div class="flex min-w-0 flex-1 flex-col gap-2 px-2 py-1.5">
          <DiscreteSlider
            label="Multiplier"
            fullWidth
            options={timingMultiplierOptions}
            value={stepTimingMultiplier[step]}
            ariaLabel="Step timing multiplier"
            onValueChange={(multiplierIndex) => onMultiplierChange(row, step, multiplierIndex)}
          />
          <DiscreteSlider
            label="Duration"
            fullWidth
            options={durationFractionOptions}
            value={stepDurationFraction[step]}
            ariaLabel="Step duration fraction"
            onValueChange={(fractionIndex) => onDurationChange(row, step, fractionIndex)}
          />
          <ContinuousSlider
            label="Velocity"
            fullWidth
            min={0}
            max={127}
            value={stepVelocity[step]}
            ariaLabel="Step velocity"
            onValueChange={(value) => onVelocityChange(row, step, value)}
          />
        </div>
      </div>
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
        {@const cellWidth = cellWidthForStepId(stepId)}
        <div
          class="relative shrink-0 overflow-visible"
          style={fixedFlexStyle(cellWidth)}
          style:margin-left={step > 0 ? `${stepInsertZoneWidthPx}px` : undefined}
        >
          {#if step > 0}
            {@render gapInsert(step)}
          {/if}
          <div class="pointer-events-auto">
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
      {#each dndItems as item, index (item.id)}
        <div
          animate:flip={{ duration: flipDurationMs }}
          class="relative shrink-0 overflow-visible {isShadowItem(item) ? 'pointer-events-none' : ''}"
          style={fixedFlexStyle(layoutForItem(item, index).cellWidth)}
          style:margin-left={layoutForItem(item, index).gapBefore ? `${stepInsertZoneWidthPx}px` : undefined}
          aria-hidden={isShadowItem(item) ? true : undefined}
        >
          {#if layoutForItem(item, index).gapBefore && !isShadowItem(item)}
            {@render gapInsert(layoutForItem(item, index).step)}
          {/if}
          {#if isShadowItem(item)}
            <div class="shrink-0" style={fixedFlexStyle(layoutForItem(item, index).cellWidth)}></div>
          {:else}
            <div class="pointer-events-auto">
              {@render stepCell(layoutForItem(item, index).step, true)}
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
