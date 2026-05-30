<script>
  import { onDestroy } from "svelte";
  import { flip } from "svelte/animate";
  import gsap from "gsap";
  import { dragHandle, dragHandleZone, TRIGGERS } from "svelte-dnd-action";
  import DurationBar from "./DurationBar.svelte";
  import NoteDragInput from "./NoteDragInput.svelte";
  import VelocityDragInput from "./VelocityDragInput.svelte";
  import StepInsertZone from "./StepInsertZone.svelte";
  import StepCardFlip from "./StepCardFlip.svelte";
  import StepMuteToggle from "./StepMuteToggle.svelte";
  import StepSkipToggle from "./StepSkipToggle.svelte";
  import ProbabilityDragInput from "./ProbabilityDragInput.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import { longPress } from "./longPressAction.js";
  import { isShadowItem, withoutShadowItems } from "./dndUtils.js";
  import {
    defaultNoteForRow,
    defaultStepDurationFraction,
    defaultStepVelocity,
  } from "./midiNoteNames.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    compensatedResizeBoundsPx,
    defaultStepTimingMultiplierIndex,
    multiplierIndexFromCompensatedWidth,
    multiplierLabelForIndex,
    rowCellDisplayWidthsPx,
    rowTimingOffsetShiftPx,
    stepCellBaseWidthPx,
    stepCellWidthPx,
    stepInsertZoneWidthPx,
  } from "./stepCellLayout.js";

  export let row = 0;
  export let muted = false;
  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  /** Index into timingOffsetValues (default 3 = 0 quarters). */
  export let timingOffsetIndex = 3;
  /** UI-only margin added when any row has a negative offset (see phraseRowLayout). */
  export let timingOffsetVisualCompensationPx = 0;
  export let pulseIndex = 1;
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
  export let stepMuted = [];
  /** @type {boolean[]} */
  export let stepSkipped = [];
  /** @type {number[]} */
  export let stepProbability = [];
  /** @type {number[]} */
  export let stepCycle = [];
  /** @type {number[]} */
  export let stepCycleOffset = [];
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
  /** @type {(row: number, step: number, muted: boolean) => void | Promise<void>} */
  export let onStepMuteChange = () => {};
  /** @type {(row: number, step: number, skipped: boolean) => void | Promise<void>} */
  export let onStepSkipChange = () => {};
  /** @type {(row: number, step: number, probability: number) => void | Promise<void>} */
  export let onStepProbabilityChange = () => {};
  /** @type {(row: number, step: number, cycle: number) => void | Promise<void>} */
  export let onStepCycleChange = () => {};
  /** @type {(row: number, step: number, cycleOffset: number) => void | Promise<void>} */
  export let onStepCycleOffsetChange = () => {};
  const flipDurationMs = 200;
  const removeBlockMs = 500;
  const stepFlipLongPressMs = 800;
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
  let flippedStep = -1;
  /** @type {Map<number, HTMLElement>} */
  const cellShellElements = new Map();
  /** @type {[string, EventListener, AddEventListenerOptions | boolean][]} */
  let resizeListenerEntries = [];
  const resizeCapture = { capture: true };
  const resizePassiveCapture = { capture: true, passive: true };

  $: reorderDisabled = stepIds.length <= 1;

  $: if (flippedStep >= stepIds.length) {
    flippedStep = -1;
  }

  $: stepFlipLongPressDisabled =
    isDragging || removeBlocked || resizingStep >= 0;

  /** @param {number} step */
  function longPressFlipParams(step) {
    return {
      duration: stepFlipLongPressMs,
      disabled: stepFlipLongPressDisabled,
      onLongPress: () => {
        flippedStep = step;
      },
    };
  }

  /** @param {number} step @param {boolean} flipped */
  function handleStepFlipChange(step, flipped) {
    if (flipped) {
      flippedStep = step;
      return;
    }

    if (flippedStep === step) {
      flippedStep = -1;
    }
  }

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
    flippedStep = -1;
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

  $: rowDisplayWidths = rowCellDisplayWidthsPx(
    stepTimingMultiplier,
    resizingStep >= 0 ? { resizeStep: resizingStep, resizeDisplayWidth } : {},
  );

  /** @param {number} step */
  function cellWidthForStep(step) {
    return (
      rowDisplayWidths[step]
      ?? stepCellWidthPx(stepTimingMultiplier[step] ?? defaultStepTimingMultiplierIndex)
    );
  }

  /** @param {number} step */
  function multiplierLabelForStep(step) {
    const index =
      resizingStep === step
        ? multiplierIndexFromCompensatedWidth(
            stepTimingMultiplier,
            step,
            resizeDisplayWidth,
          )
        : stepTimingMultiplier[step];

    return multiplierLabelForIndex(index, timingMultiplierOptions);
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

  /** @type {{ cellWidth: number, step: number, gapBefore: boolean }[]} */
  $: rowCellLayouts = dndItems.map((item, index) => {
    const step = isShadowItem(item) ? stepIndexFromId(draggedStepId) : stepIndexFromId(item.id);
    const cellWidth =
      step >= 0 ? rowDisplayWidths[step] : stepCellWidthPx(2);

    return {
      cellWidth,
      step: isShadowItem(item) ? -1 : step,
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
    const widths =
      resizingStep >= 0
        ? rowCellDisplayWidthsPx(stepTimingMultiplier, {
            resizeStep: resizingStep,
            resizeDisplayWidth,
          })
        : rowDisplayWidths;

    cellShellElements.forEach((shell, step) => {
      if (step < 0 || step >= widths.length) return;

      applyCellShellWidthPx(shell, widths[step]);
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

    if (!shell || step < 0 || step >= rowDisplayWidths.length) return;

    applyCellShellWidthPx(shell, rowDisplayWidths[step]);
  }

  /** @param {number} clientX */
  function resizeWidthFromClientX(clientX) {
    const { min, max } = compensatedResizeBoundsPx(stepTimingMultiplier, resizingStep);

    return Math.round(
      Math.min(max, Math.max(min, resizeStartWidth + (clientX - resizeStartX))),
    );
  }

  function syncActiveResizeVisuals() {
    if (resizingStep < 0) return;

    const widths = rowCellDisplayWidthsPx(stepTimingMultiplier, {
      resizeStep: resizingStep,
      resizeDisplayWidth,
    });

    cellShellElements.forEach((shell, step) => {
      if (step < 0 || step >= widths.length) return;

      applyCellShellWidthPx(shell, widths[step]);
    });

    const shell = cellShellElements.get(resizingStep);
    const label = shell?.querySelector("[data-multiplier-label]");

    if (label) {
      const previewIndex = multiplierIndexFromCompensatedWidth(
        stepTimingMultiplier,
        resizingStep,
        resizeDisplayWidth,
      );
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

    const displayWidth = rowDisplayWidths[step];

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
    const snappedIndex = multiplierIndexFromCompensatedWidth(
      stepTimingMultiplier,
      step,
      resizeDisplayWidth,
    );
    const previewMultipliers = stepTimingMultiplier.slice();
    previewMultipliers[step] = snappedIndex;
    const targetWidth = rowCellDisplayWidthsPx(previewMultipliers)[step];

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
            applyCellShellWidthPx(shell, widthPx);

            const label = shell.querySelector("[data-multiplier-label]");

            if (label) {
              label.textContent = multiplierLabelForIndex(
                multiplierIndexFromCompensatedWidth(stepTimingMultiplier, step, widthPx),
                timingMultiplierOptions,
              );
            }
          },
          onComplete: resolve,
        });
      });
    }

    const committedWidths = rowCellDisplayWidthsPx(previewMultipliers);
    cellShellElements.forEach((shell, shellStep) => {
      if (shellStep < 0 || shellStep >= committedWidths.length) return;

      applyCellShellWidthPx(shell, committedWidths[shellStep]);
    });

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

  const stepCellPlaybackClass = (active) => {
    if (muted) return "border-zinc-800/90";

    return active ? accent.borderActive : "border-zinc-700";
  };

  const stepCellPlaybackGlowClass = (active) => (muted || !active ? "" : accent.playbackGlow);

  const stepCellSurfaceClass = muted ? "bg-zinc-950/95" : "bg-zinc-900";

  const stepHeaderClass = muted
    ? "border-b border-zinc-800/90 bg-zinc-900/70"
    : "border-b border-zinc-800 bg-zinc-800/60";

  const stepHeaderLabelClass = muted
    ? "text-zinc-500"
    : "text-zinc-300";
</script>

{#snippet stepHeaderRemoveButton(step)}
  <button
    type="button"
    data-remove-button
    aria-label="Remove step"
    disabled={removeBlocked}
    class="z-10 flex h-4 w-4 shrink-0 items-center justify-start p-0 transition-colors outline-none disabled:pointer-events-none disabled:opacity-50 {muted
      ? 'text-zinc-600 hover:text-zinc-500'
      : `text-zinc-400 hover:text-zinc-200 ${accent.textAccentFocus}`}"
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
  {@const stepFlipped = flippedStep === step}
  {@const stepIsMuted = stepMuted[step]}
  {@const stepIsSkipped = stepSkipped[step]}
  <div
    class="relative h-full w-full min-w-0 overflow-visible rounded-lg transition-[box-shadow] duration-200 {stepCellPlaybackGlowClass(
      activeGates[step],
    )}"
  >
    <StepCardFlip
      {accent}
      {muted}
      flipped={stepFlipped}
      stepSilenced={stepIsMuted}
      stepSkipped={stepIsSkipped}
      disabled={stepFlipLongPressDisabled}
      longPressMs={stepFlipLongPressMs}
      surfaceClass={stepCellSurfaceClass}
      borderClass={stepCellPlaybackClass(activeGates[step])}
      headerClass={stepHeaderClass}
      onFlipChange={(flipped) => handleStepFlipChange(step, flipped)}
    >
      <div slot="front" class="h-full min-h-0 w-full min-w-0">
        <div
          class="relative flex h-full min-w-0 flex-col overflow-hidden rounded-lg border-2 outline-none transition-[border-color,background-color,opacity] duration-200 {stepCellSurfaceClass} {stepCellPlaybackClass(
            activeGates[step],
          )} {stepIsSkipped ? 'opacity-50' : stepIsMuted ? 'opacity-55 saturate-[0.35]' : ''} {muted
            ? ''
            : accent.cellFocusWithinBorder}"
        >
          {#if reorderEnabled}
            <div
              use:dragHandle
              use:longPress={longPressFlipParams(step)}
              aria-label="Drag to reorder step. Hold to open step settings."
              class="flex h-5 w-full shrink-0 cursor-grab items-center justify-start gap-1 px-1 active:cursor-grabbing {stepHeaderClass}"
            >
              {@render stepHeaderRemoveButton(step)}
              <span
                data-multiplier-label
                class="pointer-events-none ml-auto font-sans text-xs leading-none font-semibold tabular-nums {stepHeaderLabelClass}"
                aria-hidden="true"
              >
                {multiplierLabel}
              </span>
            </div>
          {:else}
            <div
              role="presentation"
              use:longPress={longPressFlipParams(step)}
              aria-label="Hold to open step settings"
              class="flex h-5 w-full shrink-0 cursor-default items-center justify-start gap-1 px-1 {stepHeaderClass} {muted
                ? 'opacity-80'
                : 'opacity-60'}"
              onpointerdown={stopPointerPropagation}
            >
              {@render stepHeaderRemoveButton(step)}
              <span
                data-multiplier-label
                class="pointer-events-none ml-auto font-sans text-xs leading-none font-semibold tabular-nums {stepHeaderLabelClass}"
                aria-hidden="true"
              >
                {multiplierLabel}
              </span>
            </div>
          {/if}

          <div class="flex min-w-0 flex-col gap-1 px-1 py-1 {muted ? 'opacity-80' : ''}">
            <DurationBar
              {accent}
              {muted}
              value={stepDurationFraction[step]}
              velocity={stepVelocity[step]}
              resetValue={defaultStepDurationFraction}
              ariaLabel="Step duration fraction"
              onValueChange={(fraction) => onDurationChange(row, step, fraction)}
            />
            <div class="flex min-w-0 items-center pt-1 pb-1">
              <div class="flex min-w-0 items-baseline gap-1.5">
                <NoteDragInput
                  {accent}
                  {muted}
                  value={notes[step]}
                  resetValue={defaultNoteForRow(row)}
                  ariaLabel="Step note"
                  onValueChange={(midi) => onNoteChange(row, step, midi)}
                />
                <VelocityDragInput
                  {accent}
                  {muted}
                  value={stepVelocity[step]}
                  resetValue={defaultStepVelocity}
                  ariaLabel="Step velocity"
                  onValueChange={(value) => onVelocityChange(row, step, value)}
                />
              </div>
              <div
                use:longPress={longPressFlipParams(step)}
                class="min-h-5 min-w-4 flex-1 touch-none"
                role="presentation"
                aria-label="Hold to open step settings"
              ></div>
            </div>
          </div>
        </div>
      </div>

      <div slot="back" class="flex min-h-0 flex-1 flex-col justify-end gap-1">
        <div class="flex items-center justify-between gap-1 px-0.5">
          <span
            class="shrink-0 font-sans text-[9px] font-semibold tracking-wide text-zinc-500 uppercase"
            >Prob</span
          >
          <ProbabilityDragInput
            {accent}
            {muted}
            value={stepProbability[step] ?? 100}
            resetValue={100}
            ariaLabel="Step probability"
            onValueChange={(value) => onStepProbabilityChange(row, step, value)}
          />
        </div>
        <div class="flex items-center justify-between gap-1 px-0.5">
          <span
            class="shrink-0 font-sans text-[9px] font-semibold tracking-wide text-zinc-500 uppercase"
            >Cyc</span
          >
          <StepNumberDragInput
            {accent}
            {muted}
            value={stepCycle[step] ?? 1}
            min={1}
            max={64}
            resetValue={1}
            ariaLabel="Step cycle length"
            onValueChange={(value) => onStepCycleChange(row, step, value)}
          />
          <span
            class="shrink-0 font-sans text-[9px] font-semibold tracking-wide text-zinc-500 uppercase"
            >Off</span
          >
          <StepNumberDragInput
            {accent}
            {muted}
            value={stepCycleOffset[step] ?? 0}
            min={0}
            max={Math.max(0, (stepCycle[step] ?? 1) - 1)}
            resetValue={0}
            ariaLabel="Step cycle offset"
            onValueChange={(value) => onStepCycleOffsetChange(row, step, value)}
          />
        </div>
        <StepSkipToggle
          {accent}
          {muted}
          value={stepIsSkipped}
          ariaLabel="Skip step in sequence"
          onValueChange={(value) => onStepSkipChange(row, step, value)}
        />
        <StepMuteToggle
          {accent}
          {muted}
          value={stepIsMuted}
          ariaLabel="Mute step"
          onValueChange={(value) => onStepMuteChange(row, step, value)}
        />
      </div>
    </StepCardFlip>

    {#if !stepFlipped}
      <button
        type="button"
        data-multiplier-resize
        aria-label="Resize step timing multiplier"
        disabled={isDragging || removeBlocked}
        class="absolute top-0 bottom-0 right-0 z-[60] w-4 cursor-ew-resize touch-none select-none border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth} disabled:pointer-events-none disabled:opacity-50"
        onpointerdown={(event) => beginMultiplierResize(event, step)}
        onmousedown={(event) => beginMultiplierResize(event, step)}
      ></button>
    {/if}
  </div>
{/snippet}

{#snippet gapInsert(insertStep)}
  <div
    data-insert-slot
    class="pointer-events-auto absolute inset-y-0 z-50"
    style={gapInsertStyle()}
  >
    <StepInsertZone {accent} {muted} onInsert={() => onInsertStep(row, insertStep)} />
  </div>
{/snippet}

<div
  class="flex min-w-0 flex-1 overflow-x-auto pt-2 pr-2 pb-2 pl-2"
  style:margin-left="{rowTimingOffsetShiftPx(timingOffsetIndex, pulseIndex) +
    timingOffsetVisualCompensationPx}px"
>
  <div class="relative z-50 shrink-0 self-stretch">
    <StepInsertZone {accent} {muted} onInsert={() => onInsertStep(row, 0)} />
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
    <StepInsertZone {accent} {muted} onInsert={() => onInsertStep(row, stepIds.length)} />
  </div>
</div>
