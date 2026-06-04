<script>
  import { onDestroy } from "svelte";
  import { flip } from "svelte/animate";
  import { dragHandle, dragHandleZone, TRIGGERS } from "svelte-dnd-action";
  import DurationBar from "./DurationBar.svelte";
  import NoteDragInput from "./NoteDragInput.svelte";
  import VelocityDragInput from "./VelocityDragInput.svelte";
  import StepInsertZone from "./StepInsertZone.svelte";
  import StepCardFlip from "./StepCardFlip.svelte";
  import StepGearIcon from "./StepGearIcon.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import StepMuteToggle from "./StepMuteToggle.svelte";
  import StepSkipToggle from "./StepSkipToggle.svelte";
  import ProbabilityDragInput from "./ProbabilityDragInput.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import { clearActiveCursor, setActiveCursor } from "./cursor.js";
  import { isShadowItem, withoutShadowItems } from "./dndUtils.js";
  import {
    defaultStepNote,
    defaultStepDurationFraction,
    defaultStepVelocity,
  } from "./midiNoteNames.js";
  import {
    emeraldRowAccent,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";
  import { phraseRowMinHeightPx } from "./phraseRowLayout.js";
  import {
    compensatedResizeBoundsPx,
    defaultStepTimingMultiplierIndex,
    multiplierIndexFromCompensatedWidth,
    multiplierLabelForIndex,
    rowCellDisplayWidthsPx,
    rowTimingOffsetShiftPx,
    stepCellBaseWidthPx,
    stepCellWidthPx,
    stepFooterActionSlotWidthPx,
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
  /** Fullest phrase row for solo-step gap compensation; from {@link phraseFullestRowReference}. */
  /** @type {import('./stepCellLayout.js').PhraseReferenceRow | null} */
  export let phraseReferenceRow = null;

  /** @type {(row: number, orderedIds: string[]) => void} */
  export let onReorder = () => {};
  /** @type {(row: number, beforeIds: string[], afterIds: string[]) => void | Promise<void>} */
  export let onMoveCommitted = () => {};
  /** @type {(row: number, step: number) => void | Promise<void>} */
  export let onRemoveStep = () => {};
  /** @type {(row: number, step: number) => void | Promise<void>} */
  export let onInsertStep = () => {};
  /** @type {(row: number, step: number) => void | Promise<void>} */
  export let onDuplicateStep = () => {};
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
  /** @type {number[] | null} */
  let resizePreviewMultipliers = null;
  /** @type {HTMLElement | null} */
  let resizeHandleElement = null;
  let resizePointerId = -1;
  let resizePointerX = 0;
  let resizeFrameId = 0;
  let resizeEndHandled = false;
  let dragYLockFrameId = 0;
  /** @type {Set<number>} */
  let flippedSteps = new Set();
  /** @type {Map<number, HTMLElement>} */
  const cellShellElements = new Map();
  /** @type {[string, EventListener, AddEventListenerOptions | boolean][]} */
  let resizeListenerEntries = [];
  const resizeCapture = { capture: true };
  const resizePassiveCapture = { capture: true, passive: true };

  $: reorderDisabled = stepIds.length <= 1;

  $: {
    const validFlippedSteps = new Set(
      [...flippedSteps].filter((step) => step >= 0 && step < stepIds.length),
    );

    if (validFlippedSteps.size !== flippedSteps.size) {
      flippedSteps = validFlippedSteps;
    }
  }

  $: stepFlipInteractionDisabled =
    isDragging || removeBlocked || resizingStep >= 0;

  /** @param {MouseEvent} event */
  function shouldIgnoreFlipDoubleClick(event) {
    const target = event.target;

    if (!(target instanceof Element)) return true;

    return Boolean(
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [data-remove-button], [data-multiplier-resize]",
      ),
    );
  }

  /** @param {MouseEvent} event @param {number} step */
  function handleOpenFlipDoubleClick(event, step) {
    if (stepFlipInteractionDisabled || shouldIgnoreFlipDoubleClick(event)) return;

    event.preventDefault();
    setStepFlipped(step, true);
  }

  /** @param {number} step @param {boolean} flipped */
  function setStepFlipped(step, flipped) {
    if (step < 0 || step >= stepIds.length) return;

    const nextFlippedSteps = new Set(flippedSteps);

    if (flipped) {
      nextFlippedSteps.add(step);
    } else {
      nextFlippedSteps.delete(step);
    }

    flippedSteps = nextFlippedSteps;
  }

  /** @param {number} step @param {boolean} flipped */
  function handleStepFlipChange(step, flipped) {
    setStepFlipped(step, flipped);
  }

  function closeAllStepFlips() {
    if (flippedSteps.size === 0) {
      return;
    }

    flippedSteps = new Set();
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
    closeAllStepFlips();
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

  $: rowLayoutOptions = { phraseReferenceRow };
  $: rowDisplayWidths = rowCellDisplayWidthsPx(
    resizePreviewMultipliers ?? stepTimingMultiplier,
    rowLayoutOptions,
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
            rowLayoutOptions,
          )
        : stepTimingMultiplier[step];

    return multiplierLabelForIndex(index, timingMultiplierOptions);
  }

  /** @param {number} widthPx */
  function fixedFlexStyle(widthPx) {
    return `flex-grow: 0; flex-shrink: 0; flex-basis: ${widthPx}px; width: ${widthPx}px; min-width: ${widthPx}px; max-width: ${widthPx}px;`;
  }

  /** @param {number} insertStep */
  function gapInsertStyle() {
    return `left: -${stepInsertZoneWidthPx}px; width: ${stepInsertZoneWidthPx}px;`;
  }

  $: layoutFingerprint = `${stepCellBaseWidthPx}:${phraseReferenceRow?.totalDuration ?? 0}:${phraseReferenceRow?.stepCount ?? 0}:${stepIds.length}:${stepTimingMultiplier.join(",")}`;
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
    const widths = rowDisplayWidths;

    cellShellElements.forEach((shell, step) => {
      if (step < 0 || step >= widths.length) return;

      applyCellShellWidthPx(shell, widths[step]);
    });
  }

  /** @param {HTMLElement} shell @param {number} widthPx */
  function applyCellShellWidthPx(shell, widthPx) {
    shell.style.flexGrow = "0";
    shell.style.flexShrink = "0";
    shell.style.flexBasis = `${widthPx}px`;
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
    const { min, max } = compensatedResizeBoundsPx(
      stepTimingMultiplier,
      resizingStep,
      rowLayoutOptions,
    );

    return Math.round(
      Math.min(max, Math.max(min, resizeStartWidth + (clientX - resizeStartX))),
    );
  }

  function syncActiveResizeVisuals() {
    if (resizingStep < 0) return;

    const previewIndex = multiplierIndexFromCompensatedWidth(
      stepTimingMultiplier,
      resizingStep,
      resizeDisplayWidth,
      rowLayoutOptions,
    );
    const previewMultipliers = stepTimingMultiplier.slice();
    previewMultipliers[resizingStep] = previewIndex;
    resizePreviewMultipliers = previewMultipliers;
    const widths = rowCellDisplayWidthsPx(previewMultipliers, rowLayoutOptions);

    cellShellElements.forEach((shell, step) => {
      if (step < 0 || step >= widths.length) return;

      applyCellShellWidthPx(shell, widths[step]);
    });

    const shell = cellShellElements.get(resizingStep);
    const labels = shell?.querySelectorAll("[data-multiplier-label]");

    if (labels?.length) {
      const previewLabel = multiplierLabelForIndex(previewIndex, timingMultiplierOptions);

      for (const label of labels) {
        label.textContent = previewLabel;
      }
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

    if ("buttons" in event && event.buttons !== 1) {
      finishMultiplierResize(/** @type {PointerEvent} */ (event));
      return;
    }

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
    stopResizeFrameLoop();
    clearResizeListeners();
    clearActiveCursor("ew-resize");

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
    resizePreviewMultipliers = stepTimingMultiplier.slice();
    resizeStartX = event.clientX;
    resizeStartWidth = displayWidth;
    resizeDisplayWidth = displayWidth;
    resizePointerX = event.clientX;
    resizeEndHandled = false;

    syncActiveResizeVisuals();
    setActiveCursor("ew-resize");

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

    const step = resizingStep;
    const shell = cellShellElements.get(step);
    const snappedIndex = multiplierIndexFromCompensatedWidth(
      stepTimingMultiplier,
      step,
      resizeDisplayWidth,
      rowLayoutOptions,
    );
    const previewMultipliers = stepTimingMultiplier.slice();
    previewMultipliers[step] = snappedIndex;
    resizePreviewMultipliers = previewMultipliers;
    const targetWidth = rowCellDisplayWidthsPx(previewMultipliers, rowLayoutOptions)[step];

    resizingStep = -1;
    stopResizeFrameLoop();
    clearResizeListeners();
    clearActiveCursor("ew-resize");

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

    if (shell) {
      applyCellShellWidthPx(shell, targetWidth);

      const labels = shell.querySelectorAll("[data-multiplier-label]");
      const snappedLabel = multiplierLabelForIndex(snappedIndex, timingMultiplierOptions);

      for (const label of labels) {
        label.textContent = snappedLabel;
      }
    }

    const committedWidths = rowCellDisplayWidthsPx(previewMultipliers, rowLayoutOptions);
    cellShellElements.forEach((shell, shellStep) => {
      if (shellStep < 0 || shellStep >= committedWidths.length) return;

      applyCellShellWidthPx(shell, committedWidths[shellStep]);
    });

    const committedIndex = stepTimingMultiplier[step];

    if (snappedIndex !== committedIndex) {
      try {
        await onMultiplierChange(row, step, snappedIndex);
      } finally {
        resizePreviewMultipliers = null;
        resyncAllCellShellWidths();
      }
    } else {
      resizePreviewMultipliers = null;
      resyncAllCellShellWidths();
    }
  }

  /** @param {PointerEvent} event */
  function cancelMultiplierResize(event) {
    if (resizeEndHandled || resizingStep < 0) return;

    resizeEndHandled = true;
    const step = resizingStep;

    teardownActiveResize();
    resizePreviewMultipliers = null;
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

  /** Row muted or step skipped — same grayed-out step chrome. */
  const stepCellPlaybackClass = (active, dimmed) => {
    if (dimmed) return "border-zinc-800/90";

    return active ? accent.borderActive : "border-zinc-700";
  };

  const stepCellPlaybackGlowClass = (active, dimmed) =>
    dimmed || !active ? "" : accent.playbackGlow;

  const stepCellSurfaceClass = (dimmed) =>
    dimmed ? "bg-zinc-950/95" : "bg-zinc-900";

  const stepHeaderClass = (dimmed) =>
    dimmed
      ? "border-b border-zinc-800/90 bg-zinc-900/70"
      : "border-b border-zinc-800 bg-zinc-800/60";

  const stepHeaderLabelClass = (dimmed) =>
    dimmed ? "text-zinc-500" : "text-zinc-300";
</script>

{#snippet stepHeaderRemoveButton(step, dimmed)}
  <button
    type="button"
    data-remove-button
    data-cursor="pointer"
    aria-label="Remove step"
    disabled={removeBlocked}
    class="relative z-30 flex h-5 w-5 shrink-0 items-center justify-center p-0 transition-colors outline-none disabled:pointer-events-none disabled:opacity-50 {dimmed
      ? 'text-zinc-600 hover:text-zinc-500'
      : `text-zinc-400 hover:text-zinc-200 ${accent.textAccentFocus}`}"
    onpointerdown={(event) => event.stopPropagation()}
    onmousedown={(event) => event.stopPropagation()}
    onclick={(event) => handleRemoveClick(event, step)}
  >
    <RemoveXIcon class="pointer-events-none h-2 w-2" />
  </button>
{/snippet}

{#snippet multiplierResizeHandle(step)}
  <button
    type="button"
    data-multiplier-resize
    data-no-long-press
    data-cursor="ew-resize"
    aria-label="Resize step timing multiplier"
    disabled={isDragging || removeBlocked}
    class="absolute top-0 right-0 bottom-5 z-[60] w-4 touch-none select-none border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth} disabled:pointer-events-none disabled:opacity-50"
    onpointerdown={(event) => beginMultiplierResize(event, step)}
    onmousedown={(event) => beginMultiplierResize(event, step)}
  ></button>
{/snippet}

{#snippet stepSkipMuteFooter(step, isFlipped)}
  {@const stepIsMuted = stepMuted[step]}
  {@const stepIsSkipped = stepSkipped[step]}
  {@const footerDimmed = muted || stepIsSkipped}
  {@const multiplierIndex = stepTimingMultiplier[step] ?? defaultStepTimingMultiplierIndex}
  {@const isQuarterStep = multiplierIndex === 0}
  {@const footerShellClass = footerDimmed
    ? "border-t border-zinc-800/90 bg-zinc-900/70"
    : "border-t border-zinc-800 bg-zinc-800/60"}
  {@const footerButtonClass = `flex h-full shrink-0 items-center justify-center border-0 bg-zinc-800/30 p-0 outline-none ${accent.ringFocusWithWidth}`}
  {@const footerSlotStyle = `width: ${stepFooterActionSlotWidthPx}px`}
  <div
    class="flex h-5 w-full shrink-0 {isQuarterStep
      ? 'divide-x divide-zinc-800'
      : 'justify-between'} {footerShellClass}"
    data-no-long-press
  >
    {#if isQuarterStep}
      <StepSkipToggle
        {accent}
        {muted}
        value={stepIsSkipped}
        buttonClass={`${footerButtonClass} min-w-0 flex-1 basis-0`}
        iconClass="pointer-events-none h-3 w-3"
        ariaLabel="Skip step in sequence"
        onValueChange={(value) => onStepSkipChange(row, step, value)}
      />
      <StepMuteToggle
        {accent}
        {muted}
        value={stepIsMuted}
        buttonClass={`${footerButtonClass} min-w-0 flex-1 basis-0`}
        iconClass="pointer-events-none h-3 w-3"
        ariaLabel="Mute step"
        onValueChange={(value) => onStepMuteChange(row, step, value)}
      />
      <button
        type="button"
        data-cursor="pointer"
        aria-label={isFlipped ? "Close step settings" : "Open step settings"}
        aria-pressed={isFlipped}
        disabled={stepFlipInteractionDisabled}
        class="{footerButtonClass} min-w-0 flex-1 basis-0 disabled:pointer-events-none disabled:opacity-50 {isFlipped
          ? toggleIconActiveClasses
          : toggleIconRestClasses}"
        onpointerdown={(event) => event.stopPropagation()}
        onmousedown={(event) => event.stopPropagation()}
        onclick={() => handleStepFlipChange(step, !isFlipped)}
      >
        <StepGearIcon class="pointer-events-none h-3 w-3" />
      </button>
    {:else}
      <div class="flex shrink-0 divide-x divide-zinc-800">
        <StepSkipToggle
          {accent}
          {muted}
          value={stepIsSkipped}
          buttonClass={footerButtonClass}
          iconClass="pointer-events-none h-3 w-3"
          ariaLabel="Skip step in sequence"
          onValueChange={(value) => onStepSkipChange(row, step, value)}
          style={footerSlotStyle}
        />
        <StepMuteToggle
          {accent}
          {muted}
          value={stepIsMuted}
          buttonClass={footerButtonClass}
          iconClass="pointer-events-none h-3 w-3"
          ariaLabel="Mute step"
          onValueChange={(value) => onStepMuteChange(row, step, value)}
          style={footerSlotStyle}
        />
      </div>
      <button
        type="button"
        data-cursor="pointer"
        aria-label={isFlipped ? "Close step settings" : "Open step settings"}
        aria-pressed={isFlipped}
        disabled={stepFlipInteractionDisabled}
        style={footerSlotStyle}
        class="{footerButtonClass} disabled:pointer-events-none disabled:opacity-50 {isFlipped
          ? toggleIconActiveClasses
          : toggleIconRestClasses}"
        onpointerdown={(event) => event.stopPropagation()}
        onmousedown={(event) => event.stopPropagation()}
        onclick={() => handleStepFlipChange(step, !isFlipped)}
      >
        <StepGearIcon class="pointer-events-none h-3 w-3" />
      </button>
    {/if}
  </div>
{/snippet}

{#snippet stepCell(step, reorderEnabled)}
  {@const multiplierLabel = multiplierLabelForStep(step)}
  {@const stepFlipped = flippedSteps.has(step)}
  {@const stepIsMuted = stepMuted[step]}
  {@const stepIsSkipped = stepSkipped[step]}
  {@const stepDimmed = muted || stepIsSkipped}
  <div
    class="relative h-full w-full min-w-0 overflow-visible rounded-lg transition-[box-shadow] duration-200 {stepCellPlaybackGlowClass(
      activeGates[step],
      stepDimmed,
    )}"
  >
    <div class="relative z-0 h-full min-h-0 w-full min-w-0">
    <StepCardFlip
      {accent}
      muted={stepDimmed}
      flipped={stepFlipped}
      disabled={stepFlipInteractionDisabled}
      surfaceClass={stepCellSurfaceClass(stepDimmed)}
      borderClass={stepCellPlaybackClass(activeGates[step], stepDimmed)}
      headerClass={stepHeaderClass(stepDimmed)}
      onFlipChange={(flipped) => handleStepFlipChange(step, flipped)}
    >
      <div slot="front" class="h-full min-h-0 w-full min-w-0">
        <div
          class="relative flex h-full min-w-0 flex-col overflow-hidden rounded-lg border-2 outline-none transition-[border-color,background-color,opacity] duration-200 {stepCellSurfaceClass(
            stepDimmed,
          )} {stepCellPlaybackClass(activeGates[step], stepDimmed)} {stepDimmed
            ? ''
            : accent.cellFocusWithinBorder}"
        >
          {#if reorderEnabled}
            <div
              class="flex h-5 w-full shrink-0 items-center gap-0 px-1 {stepHeaderClass(stepDimmed)}"
              data-no-long-press
            >
              {#if !stepFlipped}
                {@render stepHeaderRemoveButton(step, stepDimmed)}
              {:else}
                <span class="inline-block h-5 w-5 shrink-0" aria-hidden="true"></span>
              {/if}
              <!-- svelte-ignore a11y_no_static_element_interactions -->
              <div
                use:dragHandle
                aria-label="Drag to reorder step. Double-click header to open step settings."
                data-cursor="grab"
                class="flex min-h-5 min-w-0 flex-1 items-center justify-end"
                ondblclick={(event) => handleOpenFlipDoubleClick(event, step)}
                title="Double-click to open step settings"
              >
                <span
                  data-multiplier-label
                  class="pointer-events-none font-sans text-xs leading-none font-semibold tabular-nums {stepHeaderLabelClass(
                    stepDimmed,
                  )}"
                  aria-hidden="true"
                >
                  {multiplierLabel}
                </span>
              </div>
            </div>
          {:else}
            <div
              class="flex h-5 w-full shrink-0 items-center gap-0 px-1 {stepHeaderClass(stepDimmed)}"
              data-no-long-press
            >
              {#if !stepFlipped}
                {@render stepHeaderRemoveButton(step, stepDimmed)}
              {:else}
                <span class="inline-block h-5 w-5 shrink-0" aria-hidden="true"></span>
              {/if}
              <div
                role="presentation"
                aria-label="Double-click header to open step settings."
                data-cursor="default"
                class="flex min-h-5 min-w-0 flex-1 items-center justify-end {stepDimmed
                  ? 'opacity-80'
                  : 'opacity-60'}"
                onpointerdown={stopPointerPropagation}
                ondblclick={(event) => handleOpenFlipDoubleClick(event, step)}
                title="Double-click to open step settings"
              >
                <span
                  data-multiplier-label
                  class="pointer-events-none font-sans text-xs leading-none font-semibold tabular-nums {stepHeaderLabelClass(
                    stepDimmed,
                  )}"
                  aria-hidden="true"
                >
                  {multiplierLabel}
                </span>
              </div>
            </div>
          {/if}

          <div
            class="relative flex min-h-0 min-w-0 flex-1 flex-col gap-1 px-1 py-1 {stepDimmed
              ? 'opacity-80'
              : ''}"
          >
            <DurationBar
              {accent}
              muted={stepDimmed}
              stepMuted={stepIsMuted && !stepDimmed}
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
                  muted={stepDimmed}
                  value={notes[step]}
                  resetValue={defaultStepNote}
                  ariaLabel="Step note"
                  onValueChange={(midi) => onNoteChange(row, step, midi)}
                />
                <VelocityDragInput
                  {accent}
                  muted={stepDimmed}
                  value={stepVelocity[step]}
                  resetValue={defaultStepVelocity}
                  ariaLabel="Step velocity"
                  onValueChange={(value) => onVelocityChange(row, step, value)}
                />
              </div>
              <div
                class="min-h-5 min-w-4 flex-1 touch-none"
                role="presentation"
                aria-label="Double-click to open step settings"
                ondblclick={(event) => handleOpenFlipDoubleClick(event, step)}
                title="Double-click to open step settings"
              ></div>
            </div>
          </div>

          {@render stepSkipMuteFooter(step, stepFlipped)}
        </div>
      </div>

      <div
        slot="back-header"
        class="flex min-w-0 flex-1 items-center justify-start gap-1"
        data-no-long-press
      >
        {#if stepFlipped}
          {@render stepHeaderRemoveButton(step, stepDimmed)}
        {/if}
        <div
          data-cursor="default"
          class="flex min-h-5 min-w-0 flex-1 items-center justify-end"
          role="presentation"
          aria-hidden="true"
        >
          <span
            data-multiplier-label
            class="pointer-events-none font-sans text-xs leading-none font-semibold tabular-nums {stepHeaderLabelClass(
              stepDimmed,
            )}"
          >
            {multiplierLabel}
          </span>
        </div>
      </div>

      <div slot="back" class="flex min-h-0 w-full min-w-0 flex-1 items-stretch">
        <div
          data-no-flip-close
          class="grid h-full w-max shrink-0 grid-rows-[1fr_1fr] items-center"
        >
          <ProbabilityDragInput
            {accent}
            muted={stepDimmed}
            value={stepProbability[step] ?? 100}
            resetValue={100}
            ariaLabel="Step probability"
            onValueChange={(value) => onStepProbabilityChange(row, step, value)}
          />
          <div class="flex shrink-0 items-baseline justify-start gap-0.5">
            <StepNumberDragInput
              {accent}
              muted={stepDimmed}
              value={stepCycle[step] ?? 1}
              min={1}
              max={64}
              resetValue={1}
              ariaLabel="Step cycle length"
              onValueChange={(value) => onStepCycleChange(row, step, value)}
            />
            <span
              class="pointer-events-none font-sans text-xs leading-none font-bold text-zinc-400 select-none"
              aria-hidden="true">/</span
            >
            <StepNumberDragInput
              {accent}
              muted={stepDimmed}
              value={stepCycleOffset[step] ?? 0}
              min={0}
              max={Math.max(0, (stepCycle[step] ?? 1) - 1)}
              displayAdd={1}
              resetValue={0}
              ariaLabel="Step cycle offset"
              onValueChange={(value) => onStepCycleOffsetChange(row, step, value)}
            />
          </div>
        </div>
      </div>

      <div slot="back-footer">
        {@render stepSkipMuteFooter(step, stepFlipped)}
      </div>
    </StepCardFlip>
    </div>

    {@render multiplierResizeHandle(step)}
  </div>
{/snippet}

{#snippet gapInsert(insertStep)}
  <div
    data-insert-slot
    class="pointer-events-auto absolute inset-y-0 z-50"
    style={gapInsertStyle()}
  >
    <StepInsertZone
      {accent}
      {muted}
      onInsert={() => onInsertStep(row, insertStep)}
      onDuplicate={() => onDuplicateStep(row, insertStep)}
    />
  </div>
{/snippet}

<div
  class="flex min-w-0 flex-1 items-stretch overflow-x-auto pt-2 pr-2 pb-2 pl-2"
  style:min-height="{phraseRowMinHeightPx}px"
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
          class="relative shrink-0 overflow-visible {resizingStep >= 0
            ? 'step-cell-resize-tween'
            : ''}"
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
          class="relative shrink-0 overflow-visible {resizingStep >= 0
            ? 'step-cell-resize-tween'
            : ''} {isShadowItem(item) ? 'pointer-events-none' : ''}"
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
    <StepInsertZone
      {accent}
      {muted}
      onInsert={() => onInsertStep(row, stepIds.length)}
      onDuplicate={stepIds.length > 0
        ? () => onDuplicateStep(row, stepIds.length)
        : undefined}
    />
  </div>
</div>

<style>
  .step-cell-resize-tween {
    transition:
      flex-basis 100ms ease-out,
      width 100ms ease-out,
      min-width 100ms ease-out,
      max-width 100ms ease-out;
  }
</style>
