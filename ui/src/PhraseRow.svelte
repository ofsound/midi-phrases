<script>
  import { onDestroy, tick } from "svelte";
  import { dragHandle, dragHandleZone, TRIGGERS } from "svelte-dnd-action";
  import DurationBar from "./DurationBar.svelte";
  import NoteDragInput from "./NoteDragInput.svelte";
  import VelocityDragInput from "./VelocityDragInput.svelte";
  import StepInsertZone from "./StepInsertZone.svelte";
  import PlusDragButton from "./PlusDragButton.svelte";
  import StepGearIcon from "./StepGearIcon.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import StepMuteToggle from "./StepMuteToggle.svelte";
  import StepSkipToggle from "./StepSkipToggle.svelte";
  import StepMutedOverlay from "./StepMutedOverlay.svelte";
  import { compactStepVelocityOpacity } from "./compactStepVisuals.js";
  import { clearActiveCursor, setActiveCursor } from "./cursor.js";
  import { preventTabFocus } from "./preventTabFocus.js";
  import { isShadowItem, withoutShadowItems } from "./dndUtils.js";
  import {
    defaultStepCycle,
    defaultStepCycleMask,
  } from "./cyclePattern.js";
  import {
    defaultStepDurationFraction,
    defaultStepVelocity,
  } from "./midiNoteNames.js";
  import {
    emeraldRowAccent,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";
  import {
    phraseGridOriginLeftOffsetPx,
    phraseRowEndAddStepInsetPx,
    phraseRowEndStepTailPaddingPx,
    phraseRowMinHeightPx,
  } from "./phraseRowLayout.js";
  import {
    defaultStepTimingMultiplierIndex,
    insertStepTimingMultiplierOptions,
    maxMultiplierCellWidthPx,
    minMultiplierCellWidthPx,
    multiplierIndexFromWidth,
    multiplierLabelForIndex,
    quarterGridColumnsForMultiplierIndex,
    compactStepShellPaddingPercent,
    compactStepShellTrailingPaddingPercent,
    rowGridWidthPx,
    insertSlotLeftPxAtGridBoundaryPx,
    rowStepLayoutsPx,
    rowTimingOffsetShiftPx,
    stepCellPaddingPx,
    stepDisplayWidthPx,
    stepFooterActionSlotWidthPx,
    stepInsertZoneWidthPx,
  } from "./stepCellLayout.js";

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {boolean} [muted]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {number} [timingOffsetIndex] - Index into timingOffsetValues (default 3 = 0 quarters).
   * @property {number} [timingOffsetVisualCompensationPx] - UI-only margin added when any row has a negative offset (see phraseRowLayout).
   * @property {number} [pulseIndex]
   * @property {string[]} [stepIds]
   * @property {number[]} [notes]
   * @property {number[]} [stepDurationFraction]
   * @property {number[]} [stepTimingMultiplier]
   * @property {number[]} [stepVelocity]
   * @property {boolean[]} [stepMuted]
   * @property {boolean[]} [stepSkipped]
   * @property {number[]} [stepProbability]
   * @property {number[]} [stepCycle]
   * @property {number[]} [stepCycleOffset]
   * @property {boolean[]} [activeGates]
   * @property {{ index: number, label: string }[]} [timingMultiplierOptions]
   * @property {string[]} [selectedStepIds]
   * @property {boolean} [stepInspectionActive]
   * @property {boolean} [stretchToFit]
   * @property {number} [fitGridColumns]
   * @property {number} [fitGridStartColumn]
   * @property {string | null} [inspectedStepId]
   * @property {string | null} [stepInspectorHighlightedId]
   * @property {(value: number, delta: number) => number} [stepNoteValue]
   * @property {number} [defaultStepNote]
   * @property {(row: number, orderedIds: string[]) => void} [onReorder]
   * @property {(row: number, beforeIds: string[], afterIds: string[]) => void | Promise<void>} [onMoveCommitted]
   * @property {(row: number, step: number) => void | Promise<void>} [onRemoveStep]
   * @property {(row: number, step: number, multiplierIndex?: number) => void | Promise<void>} [onInsertStep]
   * @property {(row: number, step: number) => void | Promise<void>} [onDuplicateStep]
   * @property {(row: number, step: number, midi: number) => void} [onNotePreview]
   * @property {(row: number, step: number, midi: number) => void | Promise<void>} [onNoteCommit]
   * @property {(row: number, step: number) => void} [onStepBulkGestureStart]
   * @property {(row: number, step: number, multiplierIndex: number) => void | Promise<void>} [onMultiplierChange]
   * @property {(row: number, step: number, fraction: number) => void} [onDurationPreview]
   * @property {(row: number, step: number, fraction: number) => void | Promise<void>} [onDurationCommit]
   * @property {(row: number, step: number, value: number) => void} [onVelocityPreview]
   * @property {(row: number, step: number, value: number) => void | Promise<void>} [onVelocityCommit]
   * @property {(row: number, step: number, muted: boolean) => void | Promise<void>} [onStepMuteChange]
   * @property {(row: number, step: number, skipped: boolean) => void | Promise<void>} [onStepSkipChange]
   * @property {(row: number, step: number, stepId: string) => void | Promise<void>} [onInspectStep]
   * @property {(row: number, step: number, stepId: string) => void} [onPrepareStepSelection]
   * @property {(row: number, step: number, stepId: string) => void | Promise<void>} [onEditRowPianoRoll]
   * @property {(event: PointerEvent) => void} [onBulkSelectPointerDown]
   * @property {(event: PointerEvent) => void} [onBulkSelectBackgroundDoubleClick]
   * @property {() => void} [onDismissPhraseBackground]
   */

  /** @type {Props} */
  let {
    row = 0,
    muted = false,
    accent = emeraldRowAccent,
    timingOffsetIndex = 3,
    timingOffsetVisualCompensationPx = 0,
    pulseIndex = 1,
    stepIds = [],
    notes = [],
    stepDurationFraction = [],
    stepTimingMultiplier = [],
    stepVelocity = [],
    stepMuted = [],
    stepSkipped = [],
    stepProbability = [],
    stepCycle = [],
    stepCycleOffset = [],
    activeGates = [],
    timingMultiplierOptions = [],
    selectedStepIds = [],
    stepInspectionActive = false,
    stretchToFit = false,
    fitGridColumns = 1,
    fitGridStartColumn = 0,
    inspectedStepId = null,
    stepInspectorHighlightedId = null,
    stepNoteValue = (value, delta) => value + delta,
    defaultStepNote = 60,
    onReorder = () => {},
    onMoveCommitted = () => {},
    onRemoveStep = () => {},
    onInsertStep = () => {},
    onDuplicateStep = () => {},
    onNotePreview = () => {},
    onNoteCommit = () => {},
    onStepBulkGestureStart = () => {},
    onMultiplierChange = () => {},
    onDurationPreview = () => {},
    onDurationCommit = () => {},
    onVelocityPreview = () => {},
    onVelocityCommit = () => {},
    onStepMuteChange = () => {},
    onStepSkipChange = () => {},
    onInspectStep = () => {},
    onPrepareStepSelection = () => {},
    onEditRowPianoRoll = () => {},
    onBulkSelectPointerDown = () => {},
    onBulkSelectBackgroundDoubleClick = () => {},
    onDismissPhraseBackground = () => {},
  } = $props();
  const defaultStepProbability = 100;
  const removeBlockMs = 500;
  const backgroundDoubleClickIntervalMs = 400;
  const backgroundDoubleClickMaxDistancePx = 16;
  const draggedElementId = "dnd-action-dragged-el";

  /** @type {{ id: string }[]} */
  let dndItems = $state([]);
  let isDragging = $state(false);
  let removeBlocked = $state(false);
  /** @type {ReturnType<typeof setTimeout> | null} */
  let removeBlockTimeout = null;
  /** @type {string[] | null} */
  let idsBeforeDrag = null;
  /** @type {string | null} */
  let draggedStepId = $state(null);
  let resizingStep = $state(-1);
  let resizeStartX = 0;
  let resizeStartWidth = 0;
  /** Preview multipliers while resizing; drives layout until commit completes. */
  /** @type {number[] | null} */
  let resizePreviewMultipliers = $state(null);
  /** @type {HTMLElement | null} */
  let resizeHandleElement = null;
  let resizePointerId = -1;
  let resizeEndHandled = false;
  let dragYLockFrameId = 0;
  let lastBulkBackgroundPointerDownTime = 0;
  let lastBulkBackgroundPointerDownX = 0;
  let lastBulkBackgroundPointerDownY = 0;
  /** @type {[string, EventListener, AddEventListenerOptions | boolean][]} */
  let resizeListenerEntries = [];
  const resizeCapture = { capture: true };
  const resizePassiveCapture = { capture: true, passive: true };

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
    clearDndZoneTransforms();
    blockRemoveTemporarily();
  }

  /** @param {string} stepId */
  function stepIndexFromId(stepId) {
    return stepIds.indexOf(stepId);
  }

  /** @param {number} dataStep */
  function multiplierIndexForDataStep(dataStep) {
    if (dataStep < 0) return defaultStepTimingMultiplierIndex;

    return (
      (resizePreviewMultipliers ?? stepTimingMultiplier)[dataStep]
      ?? defaultStepTimingMultiplierIndex
    );
  }

  /** Shell width follows the step's multiplier, not its visual slot in the row grid. */
  /** @param {number} dataStep */
  function shellWidthPx(dataStep) {
    if (dataStep < 0) {
      return stepDisplayWidthPx(defaultStepTimingMultiplierIndex);
    }

    return stepDisplayWidthPx(multiplierIndexForDataStep(dataStep));
  }

  /** @param {number} dataStep */
  function shellStyleForStep(dataStep) {
    return fixedFlexStyle(shellWidthPx(dataStep));
  }

  /** @type {HTMLDivElement | null} */
  let dndZoneElement = $state(null);

  function clearDndZoneTransforms() {
    dndZoneElement?.querySelectorAll("[data-bulk-step-cell]").forEach((element) => {
      if (element instanceof HTMLElement) {
        element.style.removeProperty("transform");
      }
    });
  }

  /** @param {number} step */
  function multiplierLabelForStep(step) {
    const index = (resizePreviewMultipliers ?? stepTimingMultiplier)[step];

    return multiplierLabelForIndex(index, timingMultiplierOptions);
  }

  /** @param {number} step */
  function stepHasAdvancedParameterChanges(step) {
    const probability = stepProbability[step] ?? defaultStepProbability;
    const cycle = stepCycle[step] ?? defaultStepCycle;
    const cycleMask = stepCycleOffset[step] ?? defaultStepCycleMask;

    return (
      probability !== defaultStepProbability ||
      cycle !== defaultStepCycle ||
      cycleMask !== defaultStepCycleMask
    );
  }

  /** @param {number} widthPx */
  function fixedFlexStyle(widthPx) {
    return `flex-grow: 0; flex-shrink: 0; flex-basis: ${widthPx}px; width: ${widthPx}px; min-width: ${widthPx}px; max-width: ${widthPx}px;`;
  }

  /** @param {number} leftPx */
  function insertSlotStyle(leftPx) {
    return `left: ${leftPx}px; width: ${stepInsertZoneWidthPx()}px;`;
  }

  /** @param {number} boundaryPx */
  function insertLeftAtBoundary(boundaryPx) {
    return insertSlotLeftPxAtGridBoundaryPx(boundaryPx);
  }

  /** @param {CustomEvent} event */
  function handleConsider(event) {
    if (reorderDisabled) return;

    if (event.detail.info.trigger === TRIGGERS.DRAG_STARTED) {
      beginDragSession();
      idsBeforeDrag = stepIds.slice();
      draggedStepId = event.detail.info.id;

      if (document.activeElement instanceof HTMLElement) {
        document.activeElement.blur();
      }
    }

    dndItems = event.detail.items;
  }

  /** @param {CustomEvent} event */
  async function handleFinalize(event) {
    if (reorderDisabled) return;

    const trigger = event.detail.info.trigger;
    const filtered = withoutShadowItems(event.detail.items);

    if (trigger === TRIGGERS.DROPPED_OUTSIDE_OF_ANY && idsBeforeDrag) {
      dndItems = idsBeforeDrag.map((id) => ({ id }));
      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    if (filtered.length !== stepIds.length) {
      if (idsBeforeDrag) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
      }

      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    dndItems = filtered;
    const afterIds = filtered.map((item) => item.id);

    onReorder(row, afterIds);

    if (idsBeforeDrag) {
      await onMoveCommitted(row, idsBeforeDrag, afterIds);
      idsBeforeDrag = null;
    }

    await tick();
    endDragSession();
  }

  /** @type {import('svelte-dnd-action').TransformDraggedElementFunction} */
  function transformDraggedElement(element) {
    if (!element) return;

    element.style.setProperty("opacity", "1", "important");
    element.style.setProperty("visibility", "visible", "important");
    element.style.setProperty("outline", "none", "important");
    element.style.setProperty("box-shadow", "none", "important");
    element.querySelector("[data-remove-button]")?.style.setProperty("display", "none");
    element.querySelector("[data-insert-slot]")?.style.setProperty("display", "none");
    element.querySelector("[data-multiplier-resize]")?.style.setProperty("display", "none");

    element.querySelectorAll(".border-2").forEach((node) => {
      if (node instanceof HTMLElement) {
        node.style.setProperty("outline", "none");
        node.style.setProperty("--tw-ring-shadow", "0 0 #0000");
        node.style.setProperty("--tw-ring-width", "0px");
      }
    });
  }

  /** @param {PointerEvent} event */
  function stopPointerPropagation(event) {
    event.stopPropagation();
  }

  /** @param {PointerEvent} event @param {number} step */
  function handleStepControlPointerDown(event, step) {
    if (stepInspectorInteractionDisabled || event.button !== 0) return;

    const target = event.target;

    if (!(target instanceof Element)) return;

    if (
      target.closest(
        "[data-remove-button], [data-insert-slot], [data-multiplier-resize], [data-no-inspect], [data-step-inspector-toggle], [aria-label='Drag to reorder step']",
      )
    ) {
      return;
    }

    const stepId = stepIds[step];

    if (target.closest("[data-step-duration]")) {
      void onEditRowPianoRoll(row, step, stepId);
      return;
    }

    if (!target.closest("button, input, textarea, select, [role='slider']")) return;

    if (
      (stepInspectionActive && inspectedStepId !== stepId) ||
      !selectedStepIdSet.has(stepId)
    ) {
      onPrepareStepSelection(row, step, stepId);
    }
  }

  /** @param {MouseEvent} event @param {number} step */
  function openStepFromCellBackground(event, step) {
    if (stepInspectorInteractionDisabled || event.defaultPrevented) return;
    if (!(event.target instanceof Element)) return;
    if (
      event.target.closest(
        "button, input, select, textarea, [role='slider'], [data-no-inspect], [data-no-long-press], [data-no-marquee]",
      )
    ) {
      return;
    }

    onEditRowPianoRoll(row, step, stepIds[step]);
  }

  /** @param {MouseEvent} event @param {number} step */
  function openRowPianoRollFromStepHeader(event, step) {
    event.stopPropagation();

    if (stepInspectorInteractionDisabled) return;

    onEditRowPianoRoll(row, step, stepIds[step]);
  }

  /** @param {PointerEvent} event */
  function shouldIgnoreBulkBackgroundInteraction(event) {
    const target = event.target;

    if (!(target instanceof Element)) return true;

    return Boolean(
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [role='slider'], [data-bulk-step-cell], [data-no-marquee], [data-no-long-press], [data-insert-slot], [data-remove-button], [data-multiplier-resize]",
      ),
    );
  }

  /** @param {PointerEvent} event */
  function handleBulkSelectPointerDown(event) {
    if (
      event.button !== 0 ||
      isDragging ||
      resizingStep >= 0 ||
      shouldIgnoreBulkBackgroundInteraction(event)
    ) {
      onBulkSelectPointerDown(event);
      return;
    }

    const now = performance.now();
    const elapsed = now - lastBulkBackgroundPointerDownTime;
    const distance = Math.hypot(
      event.clientX - lastBulkBackgroundPointerDownX,
      event.clientY - lastBulkBackgroundPointerDownY,
    );

    if (
      lastBulkBackgroundPointerDownTime > 0 &&
      elapsed <= backgroundDoubleClickIntervalMs &&
      distance <= backgroundDoubleClickMaxDistancePx
    ) {
      lastBulkBackgroundPointerDownTime = 0;
      event.preventDefault();
      event.stopPropagation();
      onBulkSelectBackgroundDoubleClick(event);
      return;
    }

    lastBulkBackgroundPointerDownTime = now;
    lastBulkBackgroundPointerDownX = event.clientX;
    lastBulkBackgroundPointerDownY = event.clientY;

    onDismissPhraseBackground();
    onBulkSelectPointerDown(event);
  }

  /** @param {MouseEvent} event @param {number} step */
  function handleRemoveClick(event, step) {
    event.stopPropagation();

    if (removeBlocked || isDragging || resizingStep >= 0 || step < 0 || step >= stepIds.length) return;

    onRemoveStep(row, step);
  }

  /** @param {number} clientX */
  function resizeWidthFromClientX(clientX) {
    const min = minMultiplierCellWidthPx();
    const max = maxMultiplierCellWidthPx();

    return Math.round(
      Math.min(max, Math.max(min, resizeStartWidth + (clientX - resizeStartX))),
    );
  }

  /** @param {number} clientX */
  function syncActiveResizeVisuals(clientX) {
    if (resizingStep < 0) return;

    const previewIndex = multiplierIndexFromWidth(resizeWidthFromClientX(clientX));
    const current = resizePreviewMultipliers ?? stepTimingMultiplier;

    if (current[resizingStep] === previewIndex) return;

    const previewMultipliers = current.slice();
    previewMultipliers[resizingStep] = previewIndex;
    resizePreviewMultipliers = previewMultipliers;
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

  /** @param {Event} event */
  function trackResizeMove(event) {
    if (resizingStep < 0) return;

    if ("buttons" in event && event.buttons !== 1) {
      finishMultiplierResize(/** @type {PointerEvent} */ (event));
      return;
    }

    syncActiveResizeVisuals(/** @type {PointerEvent | MouseEvent} */ (event).clientX);
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

    const displayWidth = shellWidthPx(step);

    resizingStep = step;
    resizePreviewMultipliers = stepTimingMultiplier.slice();
    resizeStartX = event.clientX;
    resizeStartWidth = displayWidth;
    resizeEndHandled = false;

    syncActiveResizeVisuals(event.clientX);
    setActiveCursor("ew-resize");

    addResizeListener("pointermove", trackResizeMove, resizePassiveCapture);
    addResizeListener("mousemove", trackResizeMove, resizePassiveCapture);
    addResizeListener("pointerup", trackResizeEnd, resizeCapture);
    addResizeListener("mouseup", trackResizeEnd, resizeCapture);
    addResizeListener("pointercancel", trackResizeCancel, resizeCapture);
  }

  /** @param {PointerEvent} event */
  async function finishMultiplierResize(event) {
    if (resizeEndHandled || resizingStep < 0) return;

    resizeEndHandled = true;

    const step = resizingStep;
    syncActiveResizeVisuals(event.clientX);

    const previewMultipliers = (resizePreviewMultipliers ?? stepTimingMultiplier).slice();
    const snappedIndex = previewMultipliers[step];
    const committedIndex = stepTimingMultiplier[step];
    const needsCommit = snappedIndex !== committedIndex;

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

    resizePreviewMultipliers = previewMultipliers;

    if (needsCommit) {
      await onMultiplierChange(row, step, snappedIndex);
    }

    resizePreviewMultipliers = null;
    resizingStep = -1;
  }

  /** @param {PointerEvent} event */
  async function cancelMultiplierResize(event) {
    if (resizeEndHandled || resizingStep < 0) return;

    resizeEndHandled = true;

    teardownActiveResize();
    resizePreviewMultipliers = null;
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
    if (dimmed) return "border-border-subtle/90";

    return active ? accent.borderActive : "border-border";
  };

  const stepCellPlaybackGlowClass = (active, dimmed) =>
    dimmed || !active ? "" : accent.playbackGlow;

  const stepCellSurfaceClass = (dimmed) =>
    dimmed ? "bg-app/95" : "bg-surface";

  const stepHeaderClass = (dimmed) =>
    dimmed
      ? "border-b border-border-subtle/90 bg-surface/70"
      : "border-b border-border-subtle bg-surface-muted/60";

  const stepHeaderLabelClass = (dimmed) =>
    dimmed ? "text-text-muted" : "text-text-secondary";
  let isEmptyRow = $derived(stepIds.length === 0);
  let reorderDisabled = $derived(stepIds.length <= 1);
  let selectedStepIdSet = $derived(new Set(selectedStepIds));
  let insertMultiplierOptions = $derived(insertStepTimingMultiplierOptions(timingMultiplierOptions));
  let stepInspectorInteractionDisabled = $derived(isDragging || removeBlocked || resizingStep >= 0);
  let orderedStepItems = $derived(stepIds.map((id) => ({ id })));
  let renderedDndItems = $derived(isDragging ? dndItems : orderedStepItems);
  let dndZoneOptions = $derived({
    items: renderedDndItems,
    flipDurationMs: 0,
    type: `phrase-row-${row}`,
    dropFromOthersDisabled: true,
    morphDisabled: true,
    zoneTabIndex: -1,
    autoAriaDisabled: true,
    dropTargetStyle: { outline: "none" },
    transformDraggedElement,
  });
  let dndPreviewMultiplierIndices = $derived.by(() => {
    if (!isDragging) return null;

    return withoutShadowItems(dndItems).map((item) =>
      multiplierIndexForDataStep(stepIndexFromId(item.id)),
    );
  });
  let layoutTimingMultipliers = $derived(
    resizePreviewMultipliers ?? dndPreviewMultiplierIndices ?? stepTimingMultiplier,
  );
  let rowStepLayout = $derived(rowStepLayoutsPx(layoutTimingMultipliers));
  let rowGridSpanPx = $derived(rowGridWidthPx(layoutTimingMultipliers));
  let trailingInsertLeftPx = $derived(insertLeftAtBoundary(rowGridSpanPx));
  let trailingAddStepLeftPx = $derived(rowGridSpanPx + phraseRowEndAddStepInsetPx());
  /** @type {{ cellWidth: number, step: number, gapBefore: boolean }[]} */
  let rowCellLayouts = $derived(renderedDndItems.map((item, index) => {
    const dataStep = isShadowItem(item)
      ? stepIndexFromId(draggedStepId ?? "")
      : stepIndexFromId(item.id);

    return {
      cellWidth: shellWidthPx(dataStep),
      step: isShadowItem(item) ? -1 : dataStep,
      gapBefore: index > 0,
    };
  }));
  let compactGridStyle = $derived(
    `grid-template-columns: repeat(${Math.max(1, fitGridColumns)}, minmax(0, 1fr));`,
  );
</script>

{#snippet stepHeaderRemoveButton(step, dimmed)}
  <button
    type="button"
    data-remove-button
    data-cursor="pointer"
    aria-label="Remove step"
    disabled={removeBlocked}
    class="relative z-30 flex h-5 w-5 shrink-0 items-center justify-start p-0 transition-colors outline-none disabled:pointer-events-none disabled:opacity-50 {dimmed
      ? 'text-text-faint hover:text-text-muted'
      : `text-text-secondary hover:text-text ${accent.textAccentFocus}`}"
    onpointerdown={(event) => event.stopPropagation()}
    onmousedown={(event) => event.stopPropagation()}
    onclick={(event) => handleRemoveClick(event, step)}
  >
    <RemoveXIcon class="pointer-events-none h-2 w-2" />
  </button>
{/snippet}

{#snippet stepHeaderTimingArea(step, stepDimmed, multiplierLabel, reorderEnabled)}
  {#if reorderEnabled}
    <div
      use:dragHandle
      use:preventTabFocus
      aria-label="Drag to reorder step"
      data-cursor="grab"
      data-no-marquee
      data-no-long-press
      class="flex h-5 w-4 shrink-0 items-center justify-center opacity-50"
    ></div>
  {/if}
  <button
    type="button"
    data-no-inspect
    data-cursor="pointer"
    aria-label="Edit row in piano roll"
    disabled={stepInspectorInteractionDisabled}
    class="flex min-h-5 min-w-0 flex-1 items-center justify-end border-0 bg-transparent p-0 outline-none disabled:pointer-events-none disabled:opacity-50 {stepDimmed
      ? 'opacity-80'
      : 'opacity-60'} {accent.ringFocusWithWidth}"
    onclick={(event) => openRowPianoRollFromStepHeader(event, step)}
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
    class="absolute top-0 right-0 bottom-5 z-[60] w-[11px] touch-none select-none border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth} disabled:pointer-events-none disabled:opacity-50"
    onpointerdown={(event) => beginMultiplierResize(event, step)}
    onmousedown={(event) => beginMultiplierResize(event, step)}
  ></button>
{/snippet}

{#snippet stepSkipMuteFooter(step)}
  {@const stepIsMuted = stepMuted[step]}
  {@const stepIsSkipped = stepSkipped[step]}
  {@const isInspected = stepInspectorHighlightedId === stepIds[step]}
  {@const hasAdvancedParameterChanges = stepHasAdvancedParameterChanges(step)}
  {@const footerDimmed = muted || stepIsSkipped}
  {@const multiplierIndex = stepTimingMultiplier[step] ?? defaultStepTimingMultiplierIndex}
  {@const isQuarterStep = multiplierIndex === 0}
  {@const footerShellClass = footerDimmed
    ? "border-t border-border-subtle/90 bg-surface/70"
    : "border-t border-border-subtle bg-surface-muted/60"}
  {@const footerButtonClass = `flex h-full shrink-0 items-center justify-center border-0 bg-surface-muted/30 p-0 outline-none focus-visible:outline-none ${accent.ringFocusWithWidth}`}
  {@const footerSlotStyle = `width: ${stepFooterActionSlotWidthPx()}px`}
  <div
    class="flex h-5 w-full shrink-0 {isQuarterStep
      ? 'divide-x divide-border-subtle'
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
        data-step-inspector-toggle
        data-cursor="pointer"
        aria-label={isInspected ? "Close step inspector" : "Open step inspector"}
        aria-pressed={isInspected}
        disabled={stepInspectorInteractionDisabled}
        class="{footerButtonClass} min-w-0 flex-1 basis-0 disabled:pointer-events-none disabled:opacity-50 {hasAdvancedParameterChanges
          ? accent.textAccent
          : isInspected
            ? toggleIconActiveClasses
            : toggleIconRestClasses}"
        onpointerdown={(event) => event.stopPropagation()}
        onmousedown={(event) => event.stopPropagation()}
        onclick={() => onInspectStep(row, step, stepIds[step])}
      >
        <StepGearIcon class="pointer-events-none h-3 w-3" />
      </button>
    {:else}
      <div class="flex shrink-0 divide-x divide-border-subtle">
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
        data-step-inspector-toggle
        data-cursor="pointer"
        aria-label={isInspected ? "Close step inspector" : "Open step inspector"}
        aria-pressed={isInspected}
        disabled={stepInspectorInteractionDisabled}
        style={footerSlotStyle}
        class="{footerButtonClass} disabled:pointer-events-none disabled:opacity-50 {hasAdvancedParameterChanges
          ? accent.textAccent
          : isInspected
            ? toggleIconActiveClasses
            : toggleIconRestClasses}"
        onpointerdown={(event) => event.stopPropagation()}
        onmousedown={(event) => event.stopPropagation()}
        onclick={() => onInspectStep(row, step, stepIds[step])}
      >
        <StepGearIcon class="pointer-events-none h-3 w-3" />
      </button>
    {/if}
  </div>
{/snippet}

{#snippet stepCell(step, reorderEnabled)}
  {@const multiplierLabel = multiplierLabelForStep(step)}
  {@const stepIsMuted = stepMuted[step]}
  {@const stepIsSkipped = stepSkipped[step]}
  {@const stepDimmed = muted || stepIsSkipped}
  {@const isStepSelected = selectedStepIdSet.has(stepIds[step])}
  {@const isInspectedStep = inspectedStepId === stepIds[step]}
  {@const stepInspectionMuted = stepInspectionActive && !isInspectedStep}
  {@const stepInspectionFocused = stepInspectionActive && isInspectedStep}
  {@const stepInspectorHighlighted = stepInspectorHighlightedId === stepIds[step]}
  <!-- svelte-ignore a11y_click_events_have_key_events -->
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div
    class="relative h-full w-full min-w-0 overflow-visible rounded-lg transition-[opacity,box-shadow] duration-150 {stepInspectionMuted
      ? 'opacity-[0.78]'
      : ''} {stepInspectionFocused ? 'z-[2]' : ''} {stepCellPlaybackGlowClass(
      activeGates[step],
      stepDimmed,
    )} {(isStepSelected || stepInspectorHighlighted) && !isDragging ? accent.selectionShell : ''}"
    onclick={(event) => openStepFromCellBackground(event, step)}
  >
    <div
      class="relative z-0 h-full min-h-0 w-full min-w-0"
      onpointerdowncapture={(event) => handleStepControlPointerDown(event, step)}
    >
      <div
        class="relative flex h-full min-w-0 flex-col overflow-hidden rounded-lg border-2 outline-none transition-[border-color,background-color,box-shadow,opacity] duration-150 {stepCellSurfaceClass(
          stepDimmed,
        )} {isStepSelected || stepInspectorHighlighted
          ? `${accent.selectionBorder} ${isDragging ? '' : accent.selectionRing}`
          : stepCellPlaybackClass(activeGates[step], stepDimmed)} {stepDimmed || isDragging
          ? ''
          : accent.cellFocusWithinBorder}"
      >
        {#if reorderEnabled}
          <div
            class="flex h-5 w-full shrink-0 items-center gap-0 px-1 {stepHeaderClass(stepDimmed)}"
          >
            {@render stepHeaderRemoveButton(step, stepDimmed)}
            {@render stepHeaderTimingArea(step, stepDimmed, multiplierLabel, true)}
          </div>
        {:else}
          <div
            class="flex h-5 w-full shrink-0 items-center gap-0 px-1 {stepHeaderClass(stepDimmed)}"
          >
            {@render stepHeaderRemoveButton(step, stepDimmed)}
            {@render stepHeaderTimingArea(step, stepDimmed, multiplierLabel, false)}
          </div>
        {/if}

        <div
          class="relative flex min-h-0 min-w-0 flex-1 flex-col gap-1 px-1 pt-0.5 pb-1 {stepDimmed
            ? 'opacity-80'
            : ''}"
        >
          <div class="-mx-1 -mt-0.5">
            <DurationBar
              {accent}
              muted={stepDimmed}
              stepMuted={stepIsMuted && !stepDimmed}
              value={stepDurationFraction[step]}
              velocity={stepVelocity[step]}
              resetValue={defaultStepDurationFraction}
              ariaLabel="Step duration fraction"
              deferCommit={true}
              onGestureStart={() => onStepBulkGestureStart(row, step)}
              onValuePreview={(fraction) => onDurationPreview(row, step, fraction)}
              onValueCommit={(fraction) => onDurationCommit(row, step, fraction)}
            />
          </div>
          <div class="flex min-w-0 items-center">
            <div class="flex min-w-0 items-baseline gap-1.5">
              <NoteDragInput
                {accent}
                muted={stepDimmed}
                value={notes[step]}
                resetValue={defaultStepNote}
                ariaLabel="Step note"
                stepValue={stepNoteValue}
                deferCommit={true}
                onGestureStart={() => onStepBulkGestureStart(row, step)}
                onValuePreview={(midi) => onNotePreview(row, step, midi)}
                onValueCommit={(midi) => onNoteCommit(row, step, midi)}
              />
              <VelocityDragInput
                {accent}
                muted={stepDimmed}
                value={stepVelocity[step]}
                resetValue={defaultStepVelocity}
                ariaLabel="Step velocity"
                deferCommit={true}
                onGestureStart={() => onStepBulkGestureStart(row, step)}
                onValuePreview={(value) => onVelocityPreview(row, step, value)}
                onValueCommit={(value) => onVelocityCommit(row, step, value)}
              />
            </div>
            <div class="min-h-5 min-w-4 flex-1 touch-none" role="presentation"></div>
          </div>
        </div>

        {@render stepSkipMuteFooter(step)}
      </div>
    </div>

    {#if stepInspectionMuted}
      <div
        class="pointer-events-none absolute inset-0 z-[65] rounded-lg bg-app/15"
        aria-hidden="true"
      ></div>
    {/if}
    {#if isStepSelected}
      <div
        class="pointer-events-none absolute inset-0 z-[70] rounded-lg border border-text/40 bg-text/8"
        aria-hidden="true"
      ></div>
    {/if}
    {#if stepInspectorHighlighted}
      <div
        class="pointer-events-none absolute inset-0 z-[75] rounded-lg ring-1 ring-inset ring-accent/25"
        aria-hidden="true"
      ></div>
    {/if}
    {@render multiplierResizeHandle(step)}
  </div>
{/snippet}

{#snippet largeAddStepButton(label, insertStep)}
  <PlusDragButton
    {accent}
    {muted}
    ariaLabel={label}
    variant="large"
    options={insertMultiplierOptions}
    defaultIndex={defaultStepTimingMultiplierIndex}
    onConfirm={(multiplierIndex) => onInsertStep(row, insertStep, multiplierIndex)}
  />
{/snippet}

{#snippet gridInsertSlot(leftPx, insertStep, mode)}
  <div
    data-insert-slot
    class="pointer-events-auto absolute top-0 bottom-0 z-[60]"
    style={insertSlotStyle(leftPx)}
  >
    <StepInsertZone
      {accent}
      {muted}
      {timingMultiplierOptions}
      onInsert={mode === "leading" || mode === "between"
        ? (multiplierIndex) => onInsertStep(row, insertStep, multiplierIndex)
        : undefined}
      onDuplicate={mode === "between" || mode === "trailing"
        ? () => onDuplicateStep(row, insertStep)
        : undefined}
    />
  </div>
{/snippet}

{#snippet rowInsertSlots()}
  {@render gridInsertSlot(insertLeftAtBoundary(0), 0, "leading")}
  {#each rowStepLayout.layouts as layout, step (step)}
    {#if step > 0}
      {@render gridInsertSlot(
        insertLeftAtBoundary(layout.boundaryBeforePx),
        step,
        "between",
      )}
    {/if}
  {/each}
  {@render gridInsertSlot(trailingInsertLeftPx, stepIds.length, "trailing")}
{/snippet}

{#snippet trailingAddStep()}
  <div
    class="pointer-events-auto absolute top-0 bottom-0 z-40 flex items-center"
    style:left="{trailingAddStepLeftPx}px"
  >
    {@render largeAddStepButton("Add step to end of row", stepIds.length)}
  </div>
{/snippet}

{#snippet compactStepCell(step, stepId)}
  {@const isStepSelected = selectedStepIdSet.has(stepId)}
  {@const stepIsMuted = stepMuted[step]}
  {@const stepIsSkipped = stepSkipped[step]}
  {@const velocityOpacity = compactStepVelocityOpacity(stepVelocity[step], stepIsSkipped)}
  {@const gridColumns = quarterGridColumnsForMultiplierIndex(stepTimingMultiplier[step])}
  {@const shellPaddingPercent = compactStepShellPaddingPercent(stepTimingMultiplier[step])}
  {@const trailingPaddingPercent = compactStepShellTrailingPaddingPercent(stepTimingMultiplier[step])}
  <!-- svelte-ignore a11y_click_events_have_key_events -->
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div
    data-bulk-step-cell
    data-step-row={row}
    data-step-id={stepId}
    data-step-index={step}
    data-step-selected={isStepSelected ? true : undefined}
    class="relative min-w-0"
    style:grid-column={step === 0
      ? `${fitGridStartColumn + 1} / span ${gridColumns}`
      : `span ${gridColumns}`}
    onclick={(event) => openStepFromCellBackground(event, step)}
    onpointerdowncapture={(event) => handleStepControlPointerDown(event, step)}
  >
    <div
      class="relative overflow-hidden rounded-md bg-surface transition-[box-shadow,filter] duration-75 {activeGates[step]
        ? accent.playbackGlow
        : ''} {isStepSelected
        ? 'brightness-75 saturate-75'
        : activeGates[step]
          ? 'brightness-125'
          : ''}"
      style:margin-left="{shellPaddingPercent}%"
      style:margin-right="{step === stepIds.length - 1 ? trailingPaddingPercent : shellPaddingPercent}%"
    >
      <div
        class="pointer-events-none absolute inset-0 transition-[background-color,opacity] duration-75 {activeGates[step]
          ? accent.bgAccentStrong
          : accent.bgAccent}"
        style:opacity={activeGates[step] ? 1 : velocityOpacity}
        aria-hidden="true"
      ></div>
      {#if stepIsSkipped}
        <div
          class="compact-step-skipped-overlay pointer-events-none absolute inset-0 z-20"
          aria-hidden="true"
        ></div>
      {/if}
      {#if stepIsMuted}
        <StepMutedOverlay active={true} />
      {/if}
      {#if isStepSelected}
        <div
          class="pointer-events-none absolute inset-0 z-40 rounded-md border-[3px] border-text"
          aria-hidden="true"
        ></div>
      {/if}
      <div
        class="relative z-10 flex h-9 min-w-0 items-center justify-center overflow-hidden px-1 transition-opacity duration-75 {stepIsSkipped
          ? 'opacity-45'
          : ''}"
      >
        <NoteDragInput
          {accent}
          minimal
          value={notes[step]}
          resetValue={defaultStepNote}
          ariaLabel={`Step note${stepIsSkipped ? ", skipped" : stepIsMuted ? ", muted" : ""}`}
          stepValue={stepNoteValue}
          deferCommit={true}
          onGestureStart={() => onStepBulkGestureStart(row, step)}
          onValuePreview={(midi) => onNotePreview(row, step, midi)}
          onValueCommit={(midi) => onNoteCommit(row, step, midi)}
        />
      </div>
    </div>
  </div>
{/snippet}

<style>
  .compact-step-skipped-overlay {
    background: linear-gradient(
      to bottom right,
      transparent calc(50% - 1px),
      color-mix(in srgb, var(--color-text) 72%, transparent) calc(50% - 1px),
      color-mix(in srgb, var(--color-text) 72%, transparent) calc(50% + 1px),
      transparent calc(50% + 1px)
    );
  }
</style>

<div
  class="flex min-w-0 flex-1 overflow-hidden"
  role="presentation"
  style:padding-left={stretchToFit
    ? "0px"
    : `${rowTimingOffsetShiftPx(timingOffsetIndex) + timingOffsetVisualCompensationPx}px`}
  onpointerdown={handleBulkSelectPointerDown}
>
  <div
    class="pointer-events-none shrink-0 self-stretch"
    style:width="{phraseGridOriginLeftOffsetPx()}px"
    aria-hidden="true"
  ></div>
  <div
    class="flex min-w-0 flex-1 items-stretch {stretchToFit
      ? 'overflow-hidden py-2 pr-2'
      : 'overflow-x-auto pt-2 pr-2 pb-2'}"
    role="presentation"
    style:min-height={stretchToFit ? undefined : `${phraseRowMinHeightPx()}px`}
  >
  {#if isEmptyRow}
    <div class="relative flex shrink-0 items-center" style:padding-left="{phraseRowEndAddStepInsetPx()}px">
      {@render largeAddStepButton("Add first step", 0)}
    </div>
  {:else if stretchToFit}
    <div class="grid min-w-0 flex-1" style={compactGridStyle}>
      {#each stepIds as stepId, step (stepId)}
        {@render compactStepCell(step, stepId)}
      {/each}
    </div>
  {:else}
    <div
      class="relative w-max min-w-0 shrink-0 self-stretch overflow-visible"
      style:min-width="{rowGridSpanPx}px"
      style:padding-right="{phraseRowEndStepTailPaddingPx()}px"
    >
      {@render rowInsertSlots()}

      {#if reorderDisabled}
        <div class="relative flex w-max shrink-0 items-stretch overflow-visible">
          {#each stepIds as stepId, step (stepId)}
            <div
              data-bulk-step-cell
              data-step-row={row}
              data-step-id={stepId}
              data-step-index={step}
              data-step-selected={selectedStepIdSet.has(stepId) ? true : undefined}
              class="relative shrink-0 overflow-visible"
              style={shellStyleForStep(step)}
              style:margin-left={step === 0
                ? `${stepCellPaddingPx()}px`
                : `${stepInsertZoneWidthPx()}px`}
            >
              <div class="pointer-events-auto h-full overflow-visible">
                {@render stepCell(step, false)}
              </div>
            </div>
          {/each}
        </div>
      {:else}
        <div
          bind:this={dndZoneElement}
          use:dragHandleZone={dndZoneOptions}
          onconsider={handleConsider}
          onfinalize={handleFinalize}
          data-phrase-row-dragging={isDragging ? true : undefined}
          class="relative flex w-max shrink-0 items-stretch overflow-visible outline-none"
        >
          {#each renderedDndItems as item, index (item.id)}
            {@const layout = layoutForItem(item, index)}
            <div
              data-bulk-step-cell={layout.step >= 0 ? true : undefined}
              data-step-row={layout.step >= 0 ? row : undefined}
              data-step-id={layout.step >= 0 ? stepIds[layout.step] : undefined}
              data-step-index={layout.step >= 0 ? layout.step : undefined}
              data-step-selected={layout.step >= 0 && selectedStepIdSet.has(stepIds[layout.step]) ? true : undefined}
              class="relative shrink-0 overflow-visible {isShadowItem(item) ? 'pointer-events-none' : ''}"
              style={fixedFlexStyle(layout.cellWidth)}
              style:margin-left={index === 0
                ? `${stepCellPaddingPx()}px`
                : `${stepInsertZoneWidthPx()}px`}
              aria-hidden={isShadowItem(item) ? true : undefined}
            >
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

      {@render trailingAddStep()}
    </div>
  {/if}
  </div>
</div>
