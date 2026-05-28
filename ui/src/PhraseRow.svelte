<script>
  import { onDestroy } from "svelte";
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
    rowTimingOffsetShiftPx,
    stepCellBaseWidthPx,
    stepCellWidthPx,
    stepInsertZoneWidthPx,
  } from "./stepCellLayout.js";

  export let row = 0;
  /** Index into timingOffsetValues (default 3 = 0 quarters). */
  export let timingOffsetIndex = 3;
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
  const draggedElementId = "dnd-action-dragged-el";

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
  let resizingStep = -1;
  let resizeStartX = 0;
  let resizeStartWidth = 0;
  let resizeDisplayWidth = 0;
  /** @type {gsap.core.Tween | null} */
  let widthTween = null;
  /** @type {HTMLElement | null} */
  let resizeHandleElement = null;
  let resizePointerId = -1;
  let resizePointerX = 0;
  let resizeFrameId = 0;
  let resizeEndHandled = false;
  let dragYLockFrameId = 0;
  /** @type {Map<number, HTMLElement>} */
  const cellShellElements = new Map();
  /** @type {[string, EventListener, AddEventListenerOptions | boolean][]} */
  let resizeListenerEntries = [];
  const resizeCapture = { capture: true };
  const resizePassiveCapture = { capture: true, passive: true };

  $: reorderDisabled = stepIds.length <= 1;

  $: dndZoneOptions = {
    items: dndItems,
    flipDurationMs,
    type: `phrase-row-${row}`,
    dropFromOthersDisabled: true,
    morphDisabled: true,
    dropTargetStyle: { outline: "none" },
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

  function lockDraggedElementToRowY() {
    dragYLockFrameId = 0;

    if (!isDragging) return;

    const draggedEl = document.getElementById(draggedElementId);

    if (draggedEl) {
      const transform = draggedEl.style.transform;
      const match = transform.match(/^translate3d\(\s*([^,]+)\s*,\s*([^,]+)\s*,\s*([^)]+)\s*\)$/);

      if (match && match[2] !== "0px") {
        draggedEl.style.transform = `translate3d(${match[1]}, 0px, ${match[3]})`;
      }
    }

    dragYLockFrameId = requestAnimationFrame(lockDraggedElementToRowY);
  }

  function startDragYLock() {
    stopDragYLock();
    lockDraggedElementToRowY();
  }

  function stopDragYLock() {
    if (!dragYLockFrameId) return;

    cancelAnimationFrame(dragYLockFrameId);
    dragYLockFrameId = 0;
  }

  function beginDragSession() {
    isDragging = true;
    blockRemoveTemporarily();
    startDragYLock();
  }

  function endDragSession() {
    isDragging = false;
    draggedStepId = null;
    stopDragYLock();
    blockRemoveTemporarily();
  }

  /** @param {string} stepId */
  function stepIndexFromId(stepId) {
    return stepIds.indexOf(stepId);
  }

  /** @param {number} step */
  function cellWidthForStep(step) {
    if (resizingStep === step) {
      return resizeDisplayWidth;
    }

    return stepCellWidthPx(stepTimingMultiplier[step]);
  }

  /** @param {number} step */
  function multiplierLabelForStep(step) {
    const index =
      resizingStep === step
        ? multiplierIndexFromWidth(resizeDisplayWidth)
        : stepTimingMultiplier[step];

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

  $: layoutFingerprint = `${stepCellBaseWidthPx}:${stepIds.length}:${stepTimingMultiplier.join(",")}`;
  let appliedLayoutFingerprint = "";
  $: resizeLayoutTick = `${resizingStep}:${resizeDisplayWidth}`;

  /** @type {{ cellWidth: number, step: number, gapBefore: boolean }[]} */
  $: rowCellLayouts = (resizeLayoutTick,
    dndItems.map((item, index) => {
      const cellWidth = isShadowItem(item)
        ? cellWidthForStepId(draggedStepId)
        : cellWidthForStepId(item.id);
      const step = isShadowItem(item) ? -1 : stepIndexFromId(item.id);

      return {
        cellWidth,
        step,
        gapBefore: index > 0,
      };
    }));

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
      resyncAllCellShellWidths();
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

    if (removeBlocked || isDragging || resizingStep >= 0 || step < 0 || step >= stepIds.length) return;

    onRemoveStep(row, step);
  }

  function resyncAllCellShellWidths() {
    if (resizingStep >= 0) return;

    cellShellElements.forEach((shell, step) => {
      if (step < 0 || step >= stepTimingMultiplier.length) return;

      applyCellShellWidthPx(shell, stepCellWidthPx(stepTimingMultiplier[step]));
    });
  }

  /** @param {HTMLElement} shell @param {number} widthPx */
  function applyCellShellWidthPx(shell, widthPx) {
    shell.style.flex = `0 0 ${widthPx}px`;
    shell.style.width = `${widthPx}px`;
    shell.style.minWidth = `${widthPx}px`;
    shell.style.maxWidth = `${widthPx}px`;
  }

  /** @param {number} step */
  function resyncCellShellWidth(step) {
    const shell = cellShellElements.get(step);

    if (!shell || step < 0 || step >= stepTimingMultiplier.length) return;

    applyCellShellWidthPx(shell, stepCellWidthPx(stepTimingMultiplier[step]));
  }

  /** @param {number} clientX */
  function resizeWidthFromClientX(clientX) {
    return Math.round(
      Math.min(
        maxMultiplierCellWidthPx(),
        Math.max(
          minMultiplierCellWidthPx(),
          resizeStartWidth + (clientX - resizeStartX),
        ),
      ),
    );
  }

  function syncActiveResizeVisuals() {
    if (resizingStep < 0) return;

    const shell = cellShellElements.get(resizingStep);

    if (shell) {
      applyCellShellWidthPx(shell, resizeDisplayWidth);
    }

    const label = shell?.querySelector("[data-multiplier-label]");

    if (label) {
      const previewIndex = multiplierIndexFromWidth(resizeDisplayWidth);
      label.textContent = multiplierLabelForIndex(previewIndex, timingMultiplierOptions);
    }
  }

  function resizeFrameLoop() {
    if (resizingStep < 0) {
      resizeFrameId = 0;
      return;
    }

    resizeDisplayWidth = resizeWidthFromClientX(resizePointerX);
    syncActiveResizeVisuals();
    resizeFrameId = requestAnimationFrame(resizeFrameLoop);
  }

  function startResizeFrameLoop() {
    if (resizeFrameId) return;

    resizeFrameId = requestAnimationFrame(resizeFrameLoop);
  }

  function stopResizeFrameLoop() {
    if (!resizeFrameId) return;

    cancelAnimationFrame(resizeFrameId);
    resizeFrameId = 0;
  }

  /** @param {string} eventName @param {EventListener} listener @param {AddEventListenerOptions | boolean} options */
  function addResizeListener(eventName, listener, options) {
    document.addEventListener(eventName, listener, options);
    resizeListenerEntries.push([eventName, listener, options]);
  }

  function clearResizeListeners() {
    for (const [eventName, listener, options] of resizeListenerEntries) {
      document.removeEventListener(eventName, listener, options);
    }

    resizeListenerEntries = [];
  }

  /** @type {import('svelte/action').Action<HTMLElement, number>} */
  function cellShellAction(node, step) {
    if (step < 0) {
      return {};
    }

    cellShellElements.set(step, node);
    resyncCellShellWidth(step);

    return {
      update(nextStep) {
        if (nextStep === step) return;

        cellShellElements.delete(step);

        if (nextStep < 0) {
          step = nextStep;
          return;
        }

        step = nextStep;
        cellShellElements.set(step, node);
        resyncCellShellWidth(step);
      },
      destroy() {
        if (step >= 0) {
          cellShellElements.delete(step);
        }
      },
    };
  }

  /** @param {Event} event */
  function trackResizeMove(event) {
    if (resizingStep < 0) return;

    if ("buttons" in event && event.buttons !== 1) return;

    resizePointerX = /** @type {PointerEvent | MouseEvent} */ (event).clientX;
  }

  /** @param {Event} event */
  function trackResizeEnd(event) {
    if (resizingStep < 0) return;

    finishMultiplierResize(/** @type {PointerEvent} */ (event));
  }

  /** @param {Event} event */
  function trackResizeCancel(event) {
    if (resizingStep < 0) return;

    cancelMultiplierResize(/** @type {PointerEvent} */ (event));
  }

  function teardownActiveResize() {
    widthTween?.kill();
    stopResizeFrameLoop();
    clearResizeListeners();
    document.body.style.removeProperty("cursor");

    if (resizeHandleElement && resizePointerId >= 0) {
      try {
        if (resizeHandleElement.hasPointerCapture(resizePointerId)) {
          resizeHandleElement.releasePointerCapture(resizePointerId);
        }
      } catch {
        // Pointer may already be released in the WebView.
      }
    }

    resizeHandleElement = null;
    resizePointerId = -1;
  }

  onDestroy(() => {
    stopDragYLock();
    teardownActiveResize();
    cellShellElements.clear();
  });

  /** @param {PointerEvent} event @param {number} step */
  function beginMultiplierResize(event, step) {
    event.stopPropagation();
    event.preventDefault();

    if (isDragging || removeBlocked || resizingStep >= 0) return;

    teardownActiveResize();

    const handle = /** @type {HTMLElement} */ (event.currentTarget);
    resizeHandleElement = handle;
    resizePointerId = event.pointerId;

    if (handle.setPointerCapture) {
      handle.setPointerCapture(event.pointerId);
    }

    const displayWidth = stepCellWidthPx(stepTimingMultiplier[step]);

    resizingStep = step;
    resizeStartX = event.clientX;
    resizeStartWidth = displayWidth;
    resizeDisplayWidth = displayWidth;
    resizePointerX = event.clientX;
    resizeEndHandled = false;

    syncActiveResizeVisuals();
    document.body.style.cursor = "ew-resize";

    addResizeListener("pointermove", trackResizeMove, resizePassiveCapture);
    addResizeListener("mousemove", trackResizeMove, resizePassiveCapture);
    addResizeListener("pointerup", trackResizeEnd, resizeCapture);
    addResizeListener("mouseup", trackResizeEnd, resizeCapture);
    addResizeListener("pointercancel", trackResizeCancel, resizeCapture);

    startResizeFrameLoop();
  }

  /** @param {PointerEvent} event */
  async function finishMultiplierResize(event) {
    if (resizeEndHandled || resizingStep < 0) return;

    resizeEndHandled = true;
    widthTween?.kill();

    const step = resizingStep;
    const shell = cellShellElements.get(step);
    const snappedIndex = multiplierIndexFromWidth(resizeDisplayWidth);
    const targetWidth = stepCellWidthPx(snappedIndex);

    resizingStep = -1;
    stopResizeFrameLoop();
    clearResizeListeners();
    document.body.style.removeProperty("cursor");

    if (resizeHandleElement && resizePointerId >= 0) {
      try {
        if (resizeHandleElement.hasPointerCapture(resizePointerId)) {
          resizeHandleElement.releasePointerCapture(resizePointerId);
        }
      } catch {
        // Pointer may already be released in the WebView.
      }
    }

    resizeHandleElement = null;
    resizePointerId = -1;

    if (shell && resizeDisplayWidth !== targetWidth) {
      const tweenState = { value: resizeDisplayWidth };
      await new Promise((resolve) => {
        widthTween = gsap.to(tweenState, {
          value: targetWidth,
          duration: 0.12,
          ease: "power2.out",
          onUpdate: () => {
            const widthPx = Math.round(tweenState.value);
            resizeDisplayWidth = widthPx;
            applyCellShellWidthPx(shell, widthPx);

            const label = shell.querySelector("[data-multiplier-label]");

            if (label) {
              label.textContent = multiplierLabelForIndex(
                multiplierIndexFromWidth(widthPx),
                timingMultiplierOptions,
              );
            }
          },
          onComplete: resolve,
        });
      });
    }

    if (shell) {
      applyCellShellWidthPx(shell, targetWidth);
    }

    const committedIndex = stepTimingMultiplier[step];

    if (snappedIndex !== committedIndex) {
      await onMultiplierChange(row, step, snappedIndex);
    }
  }

  /** @param {PointerEvent} event */
  function cancelMultiplierResize(event) {
    if (resizeEndHandled || resizingStep < 0) return;

    resizeEndHandled = true;
    const step = resizingStep;

    teardownActiveResize();
    resyncCellShellWidth(step);
    resizingStep = -1;
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
    active ? "border-emerald-400" : "border-zinc-700";

  const stepCellPlaybackGlowClass = (active) =>
    active ? "shadow-[0_0_14px_rgba(52,211,153,0.55)]" : "";
</script>

{#snippet stepHeaderRemoveButton(step)}
  <button
    type="button"
    data-remove-button
    aria-label="Remove step"
    disabled={removeBlocked}
    class="z-10 flex h-4 w-4 shrink-0 items-center justify-start p-0 text-zinc-400 transition-colors outline-none hover:text-zinc-200 focus-visible:text-emerald-300 disabled:pointer-events-none disabled:opacity-50"
    onpointerdown={(event) => event.stopPropagation()}
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
{/snippet}

{#snippet stepCell(step, reorderEnabled)}
  {@const multiplierLabel = multiplierLabelForStep(step)}
  <div
    class="relative h-full w-full min-w-0 overflow-visible rounded-lg transition-[box-shadow] duration-200 {stepCellPlaybackGlowClass(
      activeGates[step],
    )}"
  >
    <div
      class="relative flex h-full min-w-0 flex-col overflow-hidden rounded-lg border-2 bg-zinc-900 outline-none transition-[border-color] duration-200 {stepCellPlaybackClass(
        activeGates[step],
      )} focus-within:has-[:focus-visible]:border-emerald-500"
    >
      {#if reorderEnabled}
        <div
          use:dragHandle
          aria-label="Drag to reorder step"
          class="flex h-5 w-full shrink-0 cursor-grab items-center justify-start gap-1 border-b border-zinc-800 bg-zinc-800/60 px-1 active:cursor-grabbing"
        >
          {@render stepHeaderRemoveButton(step)}
          <span
            data-multiplier-label
            class="pointer-events-none ml-auto font-sans text-xs leading-none font-semibold tabular-nums text-zinc-300"
            aria-hidden="true"
          >
            {multiplierLabel}
          </span>
        </div>
      {:else}
        <div
          role="presentation"
          aria-label="Reorder unavailable for single step"
          class="flex h-5 w-full shrink-0 cursor-default items-center justify-start gap-1 border-b border-zinc-800 bg-zinc-800/60 px-1 opacity-60"
          onpointerdown={stopPointerPropagation}
        >
          {@render stepHeaderRemoveButton(step)}
          <span
            data-multiplier-label
            class="pointer-events-none ml-auto font-sans text-xs leading-none font-semibold tabular-nums text-zinc-300"
            aria-hidden="true"
          >
            {multiplierLabel}
          </span>
        </div>
      {/if}

      <div class="flex min-w-0 flex-col gap-1 px-1 py-1">
        <DurationBar
          value={stepDurationFraction[step]}
          velocity={stepVelocity[step]}
          ariaLabel="Step duration fraction"
          onValueChange={(fraction) => onDurationChange(row, step, fraction)}
        />
        <div class="flex min-w-0 items-center pt-3 pb-1">
          <div class="flex min-w-0 items-baseline gap-1.5">
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
          </div>
        </div>
      </div>
    </div>

    <button
      type="button"
      data-multiplier-resize
      aria-label="Resize step timing multiplier"
      disabled={isDragging || removeBlocked}
      class="absolute top-0 bottom-0 right-0 z-[60] w-4 cursor-ew-resize touch-none select-none border-0 bg-transparent p-0 outline-none focus-visible:ring-1 focus-visible:ring-emerald-400 disabled:pointer-events-none disabled:opacity-50"
      onpointerdown={(event) => beginMultiplierResize(event, step)}
      onmousedown={(event) => beginMultiplierResize(event, step)}
    ></button>
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

<div
  class="flex min-w-0 flex-1 overflow-x-auto pt-2 pr-2 pb-2 pl-2"
  style:margin-left="{rowTimingOffsetShiftPx(timingOffsetIndex)}px"
>
  <div class="relative z-50 shrink-0 self-stretch">
    <StepInsertZone onInsert={() => onInsertStep(row, 0)} />
  </div>

  {#if reorderDisabled}
    <div class="flex w-max shrink-0 items-stretch overflow-visible">
      {#each stepIds as stepId, step (stepId)}
        {@const cellWidth = cellWidthForStep(step)}
        <div
          use:cellShellAction={step}
          class="relative shrink-0 overflow-visible"
          style={fixedFlexStyle(cellWidth)}
          style:margin-left={step > 0 ? `${stepInsertZoneWidthPx}px` : undefined}
        >
          {#if step > 0}
            {@render gapInsert(step)}
          {/if}
          <div class="pointer-events-auto h-full overflow-visible">
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
          use:cellShellAction={layout.step}
          animate:flip={resizingStep >= 0 ? undefined : { duration: flipDurationMs }}
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
            <div class="pointer-events-auto h-full overflow-visible">
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
