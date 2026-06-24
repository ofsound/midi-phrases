<script>
  import { onDestroy, tick } from "svelte";
  import { dragHandle, dragHandleZone, DRAGGED_ELEMENT_ID, TRIGGERS } from "svelte-dnd-action";
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
  import StepSkippedOverlay from "./StepSkippedOverlay.svelte";
  import { compactStepVelocityOpacity } from "./compactStepVisuals.js";
  import { boundaryDoubleClickCommand } from "./boundaryDoubleClick.js";
  import { maxPercentValue } from "./percentLimits.js";
  import {
    compactStepMoveThresholdPx,
  } from "./compactStepInteraction.js";
  import { clearActiveCursor, setActiveCursor } from "./cursor.js";
  import { preventTabFocus } from "./preventTabFocus.js";
  import {
    insertionIndexFromCellMidpoints,
    isShadowItem,
    placementIndicatorLeftPx,
    withoutShadowItems,
  } from "./dndUtils.js";
  import {
    blockDuplicateInsertionIndex,
    blockMoveOrder,
    blockMoveRestInsertionIndex,
    selectedIdsInRowOrder,
  } from "./bulkStepDrag.js";
  import {
    applyBulkStepDragGhost,
    bulkGhostLayoutEntries,
    captureBulkStepGhostSnapshots,
    dragGhostOpacity,
  } from "./bulkStepDragGhost.js";
  import {
    defaultStepCycle,
    defaultStepCycleMask,
  } from "./cyclePattern.js";
  import {
    defaultStepDurationFraction,
    defaultStepVelocity,
    midiToNoteName,
  } from "./midiNoteNames.js";
  import {
    emeraldRowAccent,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";
  import {
    phraseGridOriginLeftOffsetPx,
    phraseRowEndAddStepInsetPx,
    phraseRowEndAddStepReservePx,
    phraseRowMinHeightPx,
    phraseStepCellMinHeightPx,
    phraseStepDropIndicatorHeightPx,
  } from "./phraseRowLayout.js";
  import {
    defaultStepTimingMultiplierIndex,
    insertStepTimingMultiplierOptions,
    maxPhraseStepsPerRow,
    maxMultiplierCellWidthPx,
    minMultiplierCellWidthPx,
    multiplierIndexFromWidth,
    multiplierLabelForIndex,
    quarterGridColumnsForMultiplierIndex,
    rowGridWidthPx,
    insertSlotLeftPxAtGridBoundaryPx,
    boundaryResizeZoneLeftPxAtGridBoundaryPx,
    rowStepLayoutsPx,
    rowTimingOffsetShiftPx,
    stepBoundaryEndResizePx,
    stepBoundaryResizeZoneWidthPx,
    stepBoundaryStartResizePx,
    stepCellPaddingPx,
    stepDisplayWidthPx,
    stepFooterActionSlotWidthPx,
    stepInsertZoneWidthPx,
    stepTimingMultiplierQuarterStep,
    stepCellQuarterGridWidthPx,
  } from "./stepCellLayout.js";
  import { pulseQuartersForIndex } from "./pulseLayout.js";
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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
   * @property {boolean} [stepInspectorOpen]
   * @property {boolean} [stretchToFit]
   * @property {number} [contentFitScale]
   * @property {number} [fitGridColumns]
   * @property {string | null} [inspectedStepId]
   * @property {string | null} [stepInspectorHighlightedId]
   * @property {(value: number, delta: number) => number} [stepNoteValue]
   * @property {number} [defaultStepNote]
   * @property {(row: number, orderedIds: string[]) => void} [onReorder]
   * @property {(row: number, beforeIds: string[], afterIds: string[]) => void | Promise<void>} [onMoveCommitted]
   * @property {(row: number, beforeIds: string[], afterIds: string[]) => void | Promise<void>} [onBulkMoveCommitted]
   * @property {(row: number, blockIds: string[], insertionIndex: number) => void | Promise<void>} [onBulkStepDuplicateDrop]
   * @property {(targetRow: number, stepId: string, orderedTargetIds: string[], shadowIndex?: number) => void | Promise<void>} [onCrossRowMove]
   * @property {(targetRow: number, movedStepId: string, blockIds: string[], orderedTargetPreview: string[], shadowIndex?: number) => void | Promise<void>} [onBulkCrossRowMove]
   * @property {(targetRow: number, movedStepId: string, blockIds: string[], previewIds: string[], shadowIndex: number) => void | Promise<void>} [onBulkCrossRowDuplicateDrop]
   * @property {(targetRow: number, stepId: string, orderedTargetIds: string[], insertionIndex?: number) => void | Promise<void>} [onStepDuplicateDrop]
   * @property {(stepId: string | null) => void} [onDuplicateDragChange]
   * @property {string | null} [duplicateDragStepId]
   * @property {string[] | null} [bulkDragStepIds]
   * @property {(blockIds: string[] | null) => void} [onBulkDragSessionChange]
   * @property {(row: number, step: number) => void | Promise<void>} [onRemoveStep]
   * @property {(row: number, step: number, multiplierIndex?: number) => void | Promise<void>} [onInsertStep]
   * @property {(row: number, step: number) => void | Promise<void>} [onDuplicateStep]
   * @property {(row: number, step: number, midi: number) => void} [onNotePreview]
   * @property {(row: number, step: number, midi: number) => void | Promise<void>} [onNoteCommit]
   * @property {(row: number, step: number) => void} [onStepBulkGestureStart]
   * @property {(row: number, step: number, multiplierIndex: number) => void | Promise<void>} [onMultiplierChange]
   * @property {(multiplierIndices: number[]) => void} [onCompactTimingPreview]
   * @property {() => void} [onCompactTimingPreviewEnd]
   * @property {(row: number, fromStep: number, toStep: number) => void | Promise<void>} [onStepMove]
   * @property {(row: number, step: number, fraction: number) => void} [onDurationPreview]
   * @property {(row: number, step: number, fraction: number) => void | Promise<void>} [onDurationCommit]
   * @property {(row: number, step: number, value: number) => void} [onVelocityPreview]
   * @property {(row: number, step: number, value: number) => void | Promise<void>} [onVelocityCommit]
   * @property {(row: number, step: number, muted: boolean) => void | Promise<void>} [onStepMuteChange]
   * @property {(row: number, step: number, skipped: boolean) => void | Promise<void>} [onStepSkipChange]
   * @property {(row: number, step: number, stepId: string) => void | Promise<void>} [onInspectStep]
   * @property {(row: number, step: number, stepId: string) => void} [onPrepareStepSelection]
   * @property {(event: PointerEvent, origin?: { clientX?: number, clientY?: number, addToSelection?: boolean, toggleStep?: boolean }) => void} [onBulkSelectPointerDown]
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
    stepInspectorOpen = false,
    stretchToFit = false,
    contentFitScale = 1,
    fitGridColumns = 1,
    inspectedStepId = null,
    stepInspectorHighlightedId = null,
    stepNoteValue = (value, delta) => value + delta,
    defaultStepNote = 60,
    onReorder = () => {},
    onMoveCommitted = () => {},
    onBulkMoveCommitted = () => {},
    onBulkStepDuplicateDrop = () => {},
    onCrossRowMove = () => {},
    onBulkCrossRowMove = () => {},
    onBulkCrossRowDuplicateDrop = () => {},
    onStepDuplicateDrop = () => {},
    onDuplicateDragChange = () => {},
    duplicateDragStepId = null,
    bulkDragStepIds = null,
    onBulkDragSessionChange = () => {},
    onRemoveStep = () => {},
    onInsertStep = () => {},
    onDuplicateStep = () => {},
    onNotePreview = () => {},
    onNoteCommit = () => {},
    onStepBulkGestureStart = () => {},
    onMultiplierChange = () => {},
    onCompactTimingPreview = () => {},
    onCompactTimingPreviewEnd = () => {},
    onStepMove = () => {},
    onDurationPreview = () => {},
    onDurationCommit = () => {},
    onVelocityPreview = () => {},
    onVelocityCommit = () => {},
    onStepMuteChange = () => {},
    onStepSkipChange = () => {},
    onInspectStep = () => {},
    onPrepareStepSelection = () => {},
    onBulkSelectPointerDown = () => {},
    onBulkSelectBackgroundDoubleClick = () => {},
    onDismissPhraseBackground = () => {},
  } = $props();
  const defaultStepProbability = maxPercentValue;
  const removeBlockMs = 500;
  const backgroundDoubleClickIntervalMs = 400;
  const backgroundDoubleClickMaxDistancePx = 16;

  /** @type {{ id: string, multiplierIndex?: number }[]} */
  let dndItems = $state([]);
  let isDragging = $state(false);
  let removeBlocked = $state(false);
  /** @type {ReturnType<typeof setTimeout> | null} */
  let removeBlockTimeout = null;
  /** @type {string[] | null} */
  let idsBeforeDrag = null;
  /** @type {string | null} */
  let draggedStepId = $state(null);
  let draggedAsDuplicate = $state(false);
  let duplicateDropIndex = -1;
  let dropIndicatorIndex = $state(-1);
  let dropIndicatorLeftPx = $state(0);
  let lastDragClientX = null;
  let lastDragClientY = null;
  let dropIndicatorVisible = $state(false);
  let dropIndicatorRefreshFrame = 0;
  /** @type {{ cells: { step: number, left: number, width: number }[], boundaries: number[] } | null} */
  let dropGeometry = null;
  let dragPointerTrackingActive = false;
  let duplicateSourceRestoreFrame = 0;
  /** @type {string[] | null} */
  let bulkDragBlockIds = $state(null);
  /** @type {Map<string, import("./bulkStepDragGhost.js").BulkStepGhostSnapshot> | null} */
  let bulkDragGhostSnapshots = null;
  /** @type {{ stepId: string, widthPx: number, gapBeforePx: number }[] | null} */
  let bulkDragGhostLayout = $state(null);
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
  let lastBulkBackgroundPointerDownTime = 0;
  let lastBulkBackgroundPointerDownX = 0;
  let lastBulkBackgroundPointerDownY = 0;
  /** @type {{ pointerId: number, startX: number, startY: number, moved: boolean, step: number, element: HTMLElement } | null} */
  let compactStepPointerGesture = null;
  /** @type {{
   *   mode: "move" | "boundaryResize",
   *   pointerId: number,
   *   step: number,
   *   edge?: "start" | "end",
   *   startX: number,
   *   startY?: number,
   *   originalSlots?: import("./rowPianoRollShape.js").RowRollStepSlot[],
   *   dragPxPerQuarter?: number,
   *   previewPreviousStep?: number,
   *   originalPreviousMultiplierIndex?: number,
   *   previewPreviousMultiplierIndex?: number,
   *   targetStep?: number,
   *   initialMultiplierIndex?: number,
   *   previewMultiplierIndex?: number,
   *   resizePxPerQuarter?: number,
   *   didDrag?: boolean,
   * } | null} */
  let compactStepDrag = $state(null);
  let suppressCompactStepClick = false;
  /** @type {[string, EventListener, AddEventListenerOptions | boolean][]} */
  let resizeListenerEntries = [];
  const resizeCapture = { capture: true };
  const resizePassiveCapture = { capture: true, passive: true };

  /** @param {number} index */
  function clampDropIndicatorIndex(index) {
    if (!Number.isFinite(index)) return -1;

    return Math.min(stepIds.length, Math.max(0, Math.trunc(index)));
  }

  /** @param {number} index */
  function setDropIndicatorIndex(index) {
    const clamped = clampDropIndicatorIndex(index);

    dropIndicatorIndex = clamped;

    if (clamped >= 0) {
      duplicateDropIndex = clamped;
    }
  }

  function pointerIsInsideDropZone() {
    if (!dndZoneElement || lastDragClientY === null) return false;

    const rect = dndZoneElement.getBoundingClientRect();

    return lastDragClientY >= rect.top && lastDragClientY <= rect.bottom;
  }

  function stableStepCellsForDropTarget() {
    if (dropGeometry) return dropGeometry.cells;
    if (!dndZoneElement) return [];

    const rect = dndZoneElement.getBoundingClientRect();
    const leadingInset = layoutPx(stepCellPaddingPx());
    const gap = layoutPx(stepInsertZoneWidthPx());
    let left = rect.left;

    return stableDropCellWidths.map((width, step) => {
      left += step === 0 ? leadingInset : gap;

      const cell = { step, left, width };
      left += width;

      return cell;
    });
  }

  function captureDropGeometry() {
    if (!dndZoneElement || !dropIndicatorHostElement) return;

    const hostRect = dropIndicatorHostElement.getBoundingClientRect();
    const leadingInset = layoutPx(stepCellPaddingPx());
    const gap = layoutPx(stepInsertZoneWidthPx());
    const fallbackBoundaries = Array.from({ length: stepIds.length + 1 }, (_, index) =>
      placementIndicatorLeftPx(stableDropCellWidths, index, leadingInset, gap),
    );
    /** @type {{ step: number, left: number, width: number, localLeft: number, localRight: number }[]} */
    const cells = [];

    dndZoneElement.querySelectorAll("[data-bulk-step-cell]").forEach((element) => {
      if (!(element instanceof HTMLElement)) return;

      const step = Number(element.dataset.stepIndex);
      const rect = element.getBoundingClientRect();

      if (!Number.isFinite(step) || rect.width <= 0) return;

      cells.push({
        step,
        left: rect.left,
        width: rect.width,
        localLeft: rect.left - hostRect.left,
        localRight: rect.right - hostRect.left,
      });
    });

    cells.sort((left, right) => left.step - right.step);

    if (cells.length === 0) {
      dropGeometry = { cells: [], boundaries: fallbackBoundaries };
      return;
    }

    const cellsByStep = new Map(cells.map((cell) => [cell.step, cell]));
    const boundaries = fallbackBoundaries.slice();

    boundaries[0] = cells[0].localLeft - leadingInset / 2;

    for (let step = 1; step < stepIds.length; step += 1) {
      const previous = cellsByStep.get(step - 1);
      const next = cellsByStep.get(step);

      if (previous && next) {
        boundaries[step] = (previous.localRight + next.localLeft) / 2;
      }
    }

    boundaries[stepIds.length] = cells[cells.length - 1].localRight + gap / 2;

    dropGeometry = {
      cells: cells.map(({ step, left, width }) => ({ step, left, width })),
      boundaries,
    };
  }

  /** @param {number} index */
  function syncDropIndicatorLeft(index) {
    const clamped = clampDropIndicatorIndex(index);

    if (idsBeforeDrag && draggedStepId && !draggedAsDuplicate && dropGeometry) {
      const blockIds = bulkDragBlockIds?.includes(draggedStepId)
        ? bulkDragBlockIds
        : [draggedStepId];
      const blockSet = new Set(blockIds);
      const cellByStep = new Map(dropGeometry.cells.map((cell) => [cell.step, cell]));
      const restWidths = idsBeforeDrag
        .map((id, step) => (blockSet.has(id) ? null : (cellByStep.get(step)?.width ?? stableDropCellWidths[step])))
        .filter((width) => width !== null);
      const restInsertIndex = blockMoveRestInsertionIndex(idsBeforeDrag, blockIds, clamped);

      dropIndicatorLeftPx = placementIndicatorLeftPx(
        restWidths,
        restInsertIndex,
        layoutPx(stepCellPaddingPx()),
        layoutPx(stepInsertZoneWidthPx()),
      );
      return;
    }

    dropIndicatorLeftPx = dropGeometry?.boundaries[clamped] ?? placementIndicatorLeftPx(
      stableDropCellWidths,
      clamped,
      layoutPx(stepCellPaddingPx()),
      layoutPx(stepInsertZoneWidthPx()),
    );
  }

  /** @param {number} fallbackIndex */
  function syncDropIndicatorFromPointer(fallbackIndex = dropIndicatorIndex) {
    if (lastDragClientX === null) {
      setDropIndicatorIndex(fallbackIndex);
      syncDropIndicatorLeft(fallbackIndex);
      dropIndicatorVisible = fallbackIndex >= 0;
      return;
    }

    dropIndicatorVisible = pointerIsInsideDropZone();

    const cells = stableStepCellsForDropTarget();

    if (cells.length === 0 && stepIds.length > 0) {
      setDropIndicatorIndex(fallbackIndex);
      syncDropIndicatorLeft(fallbackIndex);
      return;
    }

    const pointerIndex = insertionIndexFromCellMidpoints(cells, lastDragClientX, stepIds.length);

    setDropIndicatorIndex(pointerIndex);
    syncDropIndicatorLeft(pointerIndex);
  }

  /** @param {number} fallbackIndex */
  function scheduleDropIndicatorRefresh(fallbackIndex = dropIndicatorIndex) {
    if (dropIndicatorRefreshFrame) {
      cancelAnimationFrame(dropIndicatorRefreshFrame);
    }

    dropIndicatorRefreshFrame = requestAnimationFrame(() => {
      dropIndicatorRefreshFrame = 0;
      syncDropIndicatorFromPointer(fallbackIndex);
    });
  }

  /** @param {PointerEvent} event */
  function trackDragPointer(event) {
    if (!isDragging) return;

    lastDragClientX = event.clientX;
    lastDragClientY = event.clientY;
    syncDropIndicatorFromPointer();
  }

  function beginDragPointerTracking() {
    if (dragPointerTrackingActive) return;

    document.addEventListener("pointermove", trackDragPointer, resizePassiveCapture);
    dragPointerTrackingActive = true;
  }

  function clearDragPointerTracking() {
    if (!dragPointerTrackingActive) return;

    document.removeEventListener("pointermove", trackDragPointer, resizePassiveCapture);
    dragPointerTrackingActive = false;
    lastDragClientX = null;
    lastDragClientY = null;
    dropIndicatorVisible = false;
    dropGeometry = null;
  }

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
    captureDropGeometry();
    isDragging = true;
    beginDragPointerTracking();
    blockRemoveTemporarily();

    const rowHeightPx = phraseStepCellMinHeightPx();

    if (dndZoneElement) {
      const rowWidthPx = dndZoneElement.scrollWidth;

      dndZoneElement.style.minHeight = `${rowHeightPx}px`;
      dndZoneElement.style.height = `${rowHeightPx}px`;

      if (rowWidthPx > 0) {
        dndZoneElement.style.minWidth = `${rowWidthPx}px`;
      }
    }
  }

  function endDragSession() {
    if (duplicateSourceRestoreFrame) {
      cancelAnimationFrame(duplicateSourceRestoreFrame);
      duplicateSourceRestoreFrame = 0;
    }

    isDragging = false;
    draggedStepId = null;
    resetDndItemsToRow();
    clearDndZoneTransforms();
    blockRemoveTemporarily();
    draggedAsDuplicate = false;
    duplicateDropIndex = -1;
    setDropIndicatorIndex(-1);
    dropIndicatorLeftPx = 0;
    dropGeometry = null;
    onDuplicateDragChange(null);
    onBulkDragSessionChange(null);
    bulkDragBlockIds = null;
    bulkDragGhostSnapshots = null;
    bulkDragGhostLayout = null;

    if (dndZoneElement) {
      dndZoneElement.style.removeProperty("min-height");
      dndZoneElement.style.removeProperty("height");
      dndZoneElement.style.removeProperty("min-width");
    }

    clearDragPointerTracking();
  }

  /** @param {{ id: string }} item @param {string | null} stepIdForLayout */
  function isCollapsedDndSlot(item, stepIdForLayout) {
    if (isShadowItem(item)) return true;

    return Boolean(
      isBulkDragActive
      && stepIdForLayout
      && bulkDragBlockIds?.includes(stepIdForLayout)
      && !draggedAsDuplicate,
    );
  }

  /** @param {number} index @param {boolean} collapsed */
  function dndSlotMarginLeftPx(index, collapsed) {
    if (collapsed) return 0;

    return index === 0
      ? layoutPx(stepCellPaddingPx())
      : layoutPx(stepInsertZoneWidthPx());
  }

  function refreshBulkDragGhost() {
    if (!bulkDragBlockIds || bulkDragBlockIds.length < 2 || !bulkDragGhostSnapshots) return;

    const draggedEl = document.getElementById(DRAGGED_ELEMENT_ID);

    if (!(draggedEl instanceof HTMLElement)) return;

    applyBulkStepDragGhost(
      draggedEl,
      bulkDragBlockIds,
      bulkDragGhostSnapshots,
      draggedAsDuplicate,
    );
  }

  function scheduleBulkDragGhostRefresh() {
    if (!bulkDragBlockIds || bulkDragBlockIds.length < 2) return;

    requestAnimationFrame(() => {
      refreshBulkDragGhost();
    });
  }

  /** @param {string[]} orderIds */
  function dndItemsFromIds(orderIds) {
    return orderIds.map((id) => ({
      id,
      multiplierIndex: multiplierIndexForDataStep(
        idsBeforeDrag?.indexOf(id) ?? stepIndexFromId(id),
      ),
    }));
  }

  function resetDndItemsToRow() {
    dndItems = stepIds.map((id, step) => ({
      id,
      multiplierIndex: multiplierIndexForDataStep(step),
    }));
  }

  function bulkDropShadowIndex() {
    if (dropIndicatorIndex >= 0) return dropIndicatorIndex;

    if (duplicateDropIndex >= 0) return duplicateDropIndex;

    return -1;
  }

  /** @param {string} draggedId */
  function prepareBulkDragFromStep(draggedId) {
    bulkDragBlockIds = null;
    bulkDragGhostSnapshots = null;
    bulkDragGhostLayout = null;

    if (selectedStepIds.length < 2 || !selectedStepIdSet.has(draggedId)) {
      onBulkDragSessionChange(null);
      return;
    }

    const block = selectedIdsInRowOrder(stepIds, selectedStepIds);
    bulkDragBlockIds = block;
    onBulkDragSessionChange(block);
    bulkDragGhostSnapshots = captureBulkStepGhostSnapshots(
      dndZoneElement,
      block,
      layoutPx,
      stepInsertZoneWidthPx(),
      stepCellPaddingPx(),
    );
    bulkDragGhostLayout = bulkGhostLayoutEntries(block, bulkDragGhostSnapshots);
  }

  /** @param {PointerEvent} event @param {number} step */
  function prepareStepDrag(event, step) {
    draggedAsDuplicate = event.altKey;
    duplicateDropIndex = step;
    const stepId = stepIds[step];
    onDuplicateDragChange(event.altKey ? stepId : null);

    if (
      (stepInspectionActive && inspectedStepId !== stepId) ||
      !selectedStepIdSet.has(stepId)
    ) {
      onPrepareStepSelection(row, step, stepId);
    }

    prepareBulkDragFromStep(stepId);
  }

  function restoreDuplicateSourceItems() {
    if (!draggedAsDuplicate || !idsBeforeDrag) return;

    dndItems = idsBeforeDrag.map((id, step) => ({
      id,
      multiplierIndex: multiplierIndexForDataStep(step),
    }));
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
  function logicalShellWidthPx(dataStep) {
    if (dataStep < 0) {
      return stepDisplayWidthPx(defaultStepTimingMultiplierIndex);
    }

    return stepDisplayWidthPx(multiplierIndexForDataStep(dataStep));
  }

  let layoutScale = $derived(Math.min(1, contentFitScale));
  let phraseStepCellHeightPx = $derived(phraseStepCellMinHeightPx());
  let phraseStepDropIndicatorHeightPxValue = $derived(phraseStepDropIndicatorHeightPx());
  let phraseRowDndZoneStyle = $derived(
    `min-height: ${phraseStepCellHeightPx}px; height: ${phraseStepCellHeightPx}px;`,
  );

  /** @param {number} px */
  function layoutPx(px) {
    return px * layoutScale;
  }

  /** @param {number} dataStep */
  function shellWidthPx(dataStep) {
    return layoutPx(logicalShellWidthPx(dataStep));
  }

  /** @param {number} dataStep */
  function shellStyleForStep(dataStep) {
    return fixedFlexStyle(shellWidthPx(dataStep));
  }

  /** @type {HTMLDivElement | null} */
  let dndZoneElement = $state(null);
  /** @type {HTMLDivElement | null} */
  let dropIndicatorHostElement = $state(null);

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
    return `left: ${layoutPx(leftPx)}px; width: ${layoutPx(stepInsertZoneWidthPx())}px;`;
  }

  /** @param {number} boundaryCenterPx */
  function boundaryResizeZoneStyle(boundaryCenterPx) {
    return `left: ${layoutPx(boundaryResizeZoneLeftPxAtGridBoundaryPx(boundaryCenterPx))}px; width: ${layoutPx(stepBoundaryResizeZoneWidthPx())}px;`;
  }

  /** @param {number} leftPx */
  function leadingBoundaryInsertZoneStyle(leftPx) {
    return `left: ${layoutPx(leftPx)}px; width: ${layoutPx(stepInsertZoneWidthPx() + stepBoundaryStartResizePx())}px;`;
  }

  function trailingResizeZoneStyle() {
    const trailingHitWidth = stepBoundaryEndResizePx() + stepInsertZoneWidthPx();

    return `right: -${layoutPx(stepInsertZoneWidthPx())}px; width: ${layoutPx(trailingHitWidth)}px;`;
  }

  /** @param {number} boundaryPx */
  function insertLeftAtBoundary(boundaryPx) {
    return insertSlotLeftPxAtGridBoundaryPx(boundaryPx);
  }

  /** @param {CustomEvent} event */
  function handleConsider(event) {
    const trigger = event.detail.info.trigger;

    if (stepIds.length === 0 && trigger === TRIGGERS.DRAG_STARTED) return;
    const shadowIndex = event.detail.items.findIndex(isShadowItem);

    if (
      trigger === TRIGGERS.DRAG_STARTED
      || trigger === TRIGGERS.DRAGGED_ENTERED
      || trigger === TRIGGERS.DRAGGED_ENTERED_ANOTHER
    ) {
      captureDropGeometry();
    }

    syncDropIndicatorFromPointer(shadowIndex);

    if (trigger === TRIGGERS.DRAG_STARTED) {
      beginDragSession();
      idsBeforeDrag = stepIds.slice();
      draggedStepId = event.detail.info.id;
      if (!bulkDragGhostSnapshots || bulkDragGhostSnapshots.size === 0) {
        prepareBulkDragFromStep(event.detail.info.id);
      }
      draggedAsDuplicate = draggedAsDuplicate || duplicateDragStepId === event.detail.info.id;

      if (draggedAsDuplicate) {
        syncDropIndicatorFromPointer(stepIndexFromId(event.detail.info.id));
        duplicateSourceRestoreFrame = requestAnimationFrame(() => {
          duplicateSourceRestoreFrame = 0;
          restoreDuplicateSourceItems();
          scheduleBulkDragGhostRefresh();
        });
      }

      scheduleBulkDragGhostRefresh();

      if (document.activeElement instanceof HTMLElement) {
        document.activeElement.blur();
      }
    } else if (
      trigger === TRIGGERS.DRAGGED_ENTERED
      || trigger === TRIGGERS.DRAGGED_ENTERED_ANOTHER
    ) {
      isDragging = true;
      beginDragPointerTracking();
      draggedStepId = event.detail.info.id;
      draggedAsDuplicate = duplicateDragStepId === event.detail.info.id;
    } else if (trigger === TRIGGERS.DRAGGED_LEFT && idsBeforeDrag === null) {
      isDragging = false;
      draggedStepId = null;
      dropIndicatorVisible = false;
      resetDndItemsToRow();
      clearDragPointerTracking();
    }

    if (bulkDragBlockIds && bulkDragBlockIds.length >= 2 && idsBeforeDrag) {
      if (shadowIndex >= 0) {
        syncDropIndicatorFromPointer(shadowIndex);
      }

      if (draggedAsDuplicate && trigger !== TRIGGERS.DRAG_STARTED) {
        dndItems = event.detail.items;
        scheduleBulkDragGhostRefresh();
        scheduleDropIndicatorRefresh(shadowIndex);
        return;
      }

      dndItems = event.detail.items;
      scheduleBulkDragGhostRefresh();
      scheduleDropIndicatorRefresh(shadowIndex);
      void tick();
      return;
    }

    if (draggedAsDuplicate && idsBeforeDrag && trigger !== TRIGGERS.DRAG_STARTED) {
      if (shadowIndex >= 0) {
        syncDropIndicatorFromPointer(shadowIndex);
      }

      restoreDuplicateSourceItems();
      scheduleDropIndicatorRefresh(shadowIndex);
      return;
    }

    dndItems = event.detail.items;
    scheduleDropIndicatorRefresh(shadowIndex);
  }

  /** @param {CustomEvent} event */
  async function handleFinalize(event) {
    const trigger = event.detail.info.trigger;
    const movedStepId = event.detail.info.id;
    const filtered = withoutShadowItems(event.detail.items);
    const isDuplicateDrop = draggedAsDuplicate || duplicateDragStepId === movedStepId;

    if (trigger === TRIGGERS.DROPPED_INTO_ANOTHER) {
      if (idsBeforeDrag) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
      } else {
        resetDndItemsToRow();
      }

      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    if (trigger === TRIGGERS.DROPPED_OUTSIDE_OF_ANY) {
      if (idsBeforeDrag) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
      } else {
        resetDndItemsToRow();
      }

      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    const isCrossRowDrop = trigger === TRIGGERS.DROPPED_INTO_ZONE
      && !stepIds.includes(movedStepId);
    const inboundBulkDrag = bulkDragStepIds !== null
      && bulkDragStepIds.length >= 2
      && bulkDragStepIds.includes(movedStepId);
    const isBulkDrag = bulkDragBlockIds !== null && bulkDragBlockIds.length >= 2 && idsBeforeDrag;

    if (isBulkDrag) {
      const bulkShadowIndex = bulkDropShadowIndex();

      if (isDuplicateDrop && bulkShadowIndex >= 0) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
        await onBulkStepDuplicateDrop(row, bulkDragBlockIds, blockDuplicateInsertionIndex(
          idsBeforeDrag,
          bulkDragBlockIds,
          bulkShadowIndex,
        ));
      } else if (bulkShadowIndex >= 0) {
        const afterIds = blockMoveOrder(idsBeforeDrag, bulkDragBlockIds, bulkShadowIndex);

        if (afterIds.some((id, index) => id !== idsBeforeDrag[index])) {
          dndItems = dndItemsFromIds(afterIds);
          onReorder(row, afterIds);
          await onBulkMoveCommitted(row, idsBeforeDrag, afterIds);
        } else {
          dndItems = idsBeforeDrag.map((id) => ({ id }));
        }
      } else {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
      }

      idsBeforeDrag = null;
      await tick();
      endDragSession();
      return;
    }

    if (isCrossRowDrop) {
      dndItems = filtered;
      const previewIds = filtered.map((item) => item.id);
      const bulkShadowIndex = bulkDropShadowIndex();

      if (inboundBulkDrag && bulkDragStepIds) {
        if (isDuplicateDrop && bulkShadowIndex >= 0) {
          await onBulkCrossRowDuplicateDrop(
            row,
            movedStepId,
            bulkDragStepIds,
            previewIds,
            bulkShadowIndex,
          );
        } else {
          await onBulkCrossRowMove(
            row,
            movedStepId,
            bulkDragStepIds,
            previewIds,
            bulkShadowIndex,
          );
        }
      } else if (isDuplicateDrop) {
        await onStepDuplicateDrop(row, movedStepId, previewIds, dropIndicatorIndex);
      } else {
        await onCrossRowMove(row, movedStepId, previewIds, dropIndicatorIndex);
      }

      await tick();
      endDragSession();
      return;
    }

    if (filtered.length !== stepIds.length) {
      if (idsBeforeDrag) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
      } else {
        resetDndItemsToRow();
      }

      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    dndItems = filtered;
    const afterIds = filtered.map((item) => item.id);

    if (isDuplicateDrop) {
      const sourceIndex = idsBeforeDrag?.indexOf(movedStepId) ?? -1;

      if (idsBeforeDrag && duplicateDropIndex >= 0 && duplicateDropIndex !== sourceIndex) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
        await onStepDuplicateDrop(row, movedStepId, afterIds, duplicateDropIndex);
      }

      idsBeforeDrag = null;
      await tick();
      endDragSession();
      return;
    }

    const stableAfterIds =
      idsBeforeDrag && dropIndicatorIndex >= 0
        ? blockMoveOrder(idsBeforeDrag, [movedStepId], dropIndicatorIndex)
        : afterIds;

    dndItems = dndItemsFromIds(stableAfterIds);
    onReorder(row, stableAfterIds);

    if (idsBeforeDrag) {
      await onMoveCommitted(row, idsBeforeDrag, stableAfterIds);
      idsBeforeDrag = null;
    }

    await tick();
    endDragSession();
  }

  /** @type {import('svelte-dnd-action').TransformDraggedElementFunction} */
  function transformDraggedElement(element) {
    if (!element) return;

    const dragHeight = `${phraseStepCellHeightPx}px`;

    if (bulkDragBlockIds && bulkDragBlockIds.length >= 2 && bulkDragGhostSnapshots) {
      applyBulkStepDragGhost(
        element,
        bulkDragBlockIds,
        bulkDragGhostSnapshots,
        draggedAsDuplicate,
      );
      element.style.setProperty("height", dragHeight, "important");
      element.style.setProperty("min-height", dragHeight, "important");
      element.style.setProperty("overflow", "visible", "important");
      return;
    }

    element.style.setProperty("opacity", dragGhostOpacity, "important");
    element.style.setProperty("visibility", "visible", "important");
    element.style.setProperty("height", dragHeight, "important");
    element.style.setProperty("min-height", dragHeight, "important");
    element.style.setProperty("overflow", "visible", "important");
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

    if (
      target.closest("[data-step-duration], [data-step-note], [data-step-velocity]")
    ) {
      return;
    }

    const stepId = stepIds[step];

    if (!target.closest("button, input, textarea, select, [role='slider']")) return;

    if (
      (stepInspectionActive && inspectedStepId !== stepId) ||
      !selectedStepIdSet.has(stepId)
    ) {
      onPrepareStepSelection(row, step, stepId);
    }
  }

  /** @param {PointerEvent} event @param {number} step */
  function handleCompactStepPointerDown(event, step) {
    if (!stretchToFit) {
      handleStepControlPointerDown(event, step);
      return;
    }

    if (stepInspectorInteractionDisabled || event.button !== 0) return;

    const target = event.target;

    if (!(target instanceof Element)) return;

    suppressCompactStepClick = false;

    if (event.shiftKey) {
      const element = /** @type {HTMLElement} */ (event.currentTarget);
      compactStepPointerGesture = {
        pointerId: event.pointerId,
        startX: event.clientX,
        startY: event.clientY,
        moved: false,
        step,
        element,
      };
      element.setPointerCapture?.(event.pointerId);
      event.preventDefault();
      event.stopImmediatePropagation();
      return;
    }

    compactStepPointerGesture = null;
  }

  /** @param {PointerEvent} event */
  function handleCompactStepPointerMove(event) {
    if (!compactStepPointerGesture || compactStepPointerGesture.pointerId !== event.pointerId) return;

    const deltaX = event.clientX - compactStepPointerGesture.startX;
    const deltaY = event.clientY - compactStepPointerGesture.startY;

    if (Math.hypot(deltaX, deltaY) >= compactStepMoveThresholdPx) {
      const gesture = compactStepPointerGesture;
      gesture.moved = true;
      suppressCompactStepClick = true;

      if (gesture.element.hasPointerCapture?.(event.pointerId)) {
        gesture.element.releasePointerCapture(event.pointerId);
      }

      compactStepPointerGesture = null;
      onBulkSelectPointerDown(event, {
        clientX: gesture.startX,
        clientY: gesture.startY,
        addToSelection: true,
      });
    }
  }

  /** @param {PointerEvent} event */
  function handleCompactStepPointerEnd(event) {
    if (!compactStepPointerGesture || compactStepPointerGesture.pointerId !== event.pointerId) return;

    const gesture = compactStepPointerGesture;
    compactStepPointerGesture = null;

    if (gesture.element.hasPointerCapture?.(event.pointerId)) {
      gesture.element.releasePointerCapture(event.pointerId);
    }

    suppressCompactStepClick = true;

    if (!gesture.moved && event.type === "pointerup") {
      onBulkSelectPointerDown(event, { toggleStep: true });
    }

    event.preventDefault();
    event.stopImmediatePropagation();
  }

  /** @param {MouseEvent} event @param {number} step */
  function openCompactStepInspector(event, step) {
    if (suppressCompactStepClick) {
      suppressCompactStepClick = false;
      return;
    }

    if (stepInspectorInteractionDisabled || event.defaultPrevented || event.shiftKey) return;

    event.preventDefault();
    event.stopPropagation();
    void onInspectStep(row, step, stepIds[step]);
  }

  /** @param {MouseEvent | PointerEvent} event */
  function shouldIgnoreFullStepInspectorInteraction(event) {
    const target = event.target;

    if (!(target instanceof Element)) return true;

    if (target.closest("[data-step-header]")) {
      return shouldIgnoreStepHeaderInspectorInteraction(event);
    }

    return Boolean(
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [role='slider'], [data-no-inspect], [data-insert-slot], [data-remove-button], [data-multiplier-resize]",
      ),
    );
  }

  /** @param {MouseEvent | PointerEvent} event @param {number} step */
  function retargetOpenStepInspector(event, step) {
    const stepId = stepIds[step];

    if (
      !stepInspectorOpen ||
      stepInspectorInteractionDisabled ||
      event.defaultPrevented ||
      event.shiftKey ||
      inspectedStepId === stepId ||
      shouldIgnoreFullStepInspectorInteraction(event)
    ) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();
    void onInspectStep(row, step, stepId);
  }

  /** @param {MouseEvent | PointerEvent} event */
  function shouldIgnoreStepHeaderInspectorInteraction(event) {
    const target = event.target;

    if (!(target instanceof Element)) return true;

    return Boolean(
      target.closest(
        "button, [data-remove-button], [data-multiplier-resize]",
      ),
    );
  }

  /** @param {MouseEvent} event @param {number} step */
  function openFullStepInspector(event, step) {
    const stepId = stepIds[step];

    if (
      stepInspectorInteractionDisabled ||
      event.defaultPrevented ||
      event.shiftKey ||
      (stepInspectorOpen && inspectedStepId === stepId) ||
      shouldIgnoreFullStepInspectorInteraction(event)
    ) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();
    void onInspectStep(row, step, stepId);
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
    if (stretchToFit && !event.shiftKey) {
      const target = event.target;

      if (target instanceof Element && !target.closest("[data-bulk-step-cell]")) {
        onDismissPhraseBackground();
      }

      return;
    }

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
    const delta = layoutScale > 0 ? (clientX - resizeStartX) / layoutScale : 0;

    return Math.round(
      Math.min(max, Math.max(min, resizeStartWidth + delta)),
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
    if (duplicateSourceRestoreFrame) cancelAnimationFrame(duplicateSourceRestoreFrame);
    if (dropIndicatorRefreshFrame) cancelAnimationFrame(dropIndicatorRefreshFrame);
    clearDragPointerTracking();
    stopDragYLock();
    teardownActiveResize();
    clearCompactStepDrag();
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

    const displayWidth = logicalShellWidthPx(step);

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

  /** @param {MouseEvent} event @param {number} insertStep */
  function handleBoundaryDoubleClick(event, insertStep) {
    event.preventDefault();
    event.stopPropagation();

    if (isDragging || removeBlocked) return;

    const command = boundaryDoubleClickCommand({
      altKey: event.altKey,
      insertStep,
      stepCount: stepIds.length,
      maxStepCount: maxPhraseStepsPerRow,
    });

    if (!command) return;

    if (command.type === "duplicate") {
      onDuplicateStep(row, command.insertStep);
    } else {
      onInsertStep(row, command.insertStep, defaultStepTimingMultiplierIndex);
    }
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

  function compactPxPerQuarter() {
    const pulse = pulseQuartersForIndex(pulseIndex);
    const quartersPerGridColumn = pulse * stepTimingMultiplierQuarterStep;

    if (fitGridColumns > 0) {
      const pxPerGridColumn = layoutPx(stepCellQuarterGridWidthPx());

      return pxPerGridColumn / quartersPerGridColumn;
    }

    return stepCellQuarterGridWidthPx() / quartersPerGridColumn;
  }

  function clearCompactStepDrag() {
    clearResizeListeners();
    clearActiveCursor("ew-resize");
    onCompactTimingPreviewEnd();
    compactStepDrag = null;
  }

  /** @param {{ id?: string, multiplierIndex?: number }} item */
  function multiplierIndexForDndItem(item) {
    if (item.multiplierIndex != null) return item.multiplierIndex;

    const step = stepIndexFromId(item.id ?? "");

    if (step >= 0) return multiplierIndexForDataStep(step);

    return defaultStepTimingMultiplierIndex;
  }

  function multiplierIndexForActiveDrag() {
    if (!draggedStepId) return defaultStepTimingMultiplierIndex;

    const match = withoutShadowItems(dndItems).find((item) => item.id === draggedStepId);

    return multiplierIndexForDndItem(match ?? { id: draggedStepId });
  }

  /** @param {number} multiplierIndex */
  function compactInboundCellWidthPx(multiplierIndex) {
    return layoutPx(stepDisplayWidthPx(multiplierIndex));
  }

  /** @param {{ id: string, multiplierIndex?: number }} item @param {number} index */
  function compactLayoutForItem(item, index) {
    if (isShadowItem(item)) {
      const multiplierIndex = multiplierIndexForActiveDrag();
      const gridColumns = quarterGridColumnsForMultiplierIndex(multiplierIndex);

      return {
        step: -1,
        gridColumns,
        cellWidth: 0,
        isShadow: true,
      };
    }

    const step = stepIndexFromId(item.id);
    const multiplierIndex = multiplierIndexForDndItem(item);
    const gridColumns = quarterGridColumnsForMultiplierIndex(multiplierIndex);

    return {
      step,
      gridColumns,
      cellWidth: compactInboundCellWidthPx(multiplierIndex),
      isShadow: false,
    };
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
  let selectedStepIdSet = $derived(new Set(selectedStepIds));
  let insertMultiplierOptions = $derived(insertStepTimingMultiplierOptions(timingMultiplierOptions));
  let stepInspectorInteractionDisabled = $derived(
    isDragging || removeBlocked || resizingStep >= 0 || compactStepDrag !== null,
  );
  let isBulkDragActive = $derived(
    isDragging && bulkDragBlockIds !== null && bulkDragBlockIds.length >= 2,
  );
  let orderedStepItems = $derived(stepIds.map((id, step) => ({
    id,
    multiplierIndex: multiplierIndexForDataStep(step),
  })));
  let renderedDndItems = $derived(isDragging ? dndItems : orderedStepItems);
  let dndZoneOptions = $derived({
    items: renderedDndItems,
    flipDurationMs: 0,
    type: "phrase-step",
    dropFromOthersDisabled: stepIds.length >= maxPhraseStepsPerRow,
    morphDisabled: true,
    useCursorForDetection: true,
    zoneTabIndex: -1,
    autoAriaDisabled: true,
    dropTargetStyle: { outline: "none" },
    transformDraggedElement,
  });
  let dndPreviewMultiplierIndices = $derived.by(() => {
    if (!isDragging) return null;

    return withoutShadowItems(dndItems).map((item) =>
      item.multiplierIndex ?? multiplierIndexForDataStep(stepIndexFromId(item.id)),
    );
  });
  let compactDragTimingMultipliers = $derived.by(() => {
    if (!compactStepDrag || compactStepDrag.mode !== "boundaryResize") return null;

    const preview = stepTimingMultiplier.slice();
    preview[compactStepDrag.step] =
      compactStepDrag.previewMultiplierIndex ?? defaultStepTimingMultiplierIndex;

    return preview;
  });
  let compactRenderedItems = $derived(isDragging ? dndItems : orderedStepItems);
  let layoutTimingMultipliers = $derived(
    resizePreviewMultipliers
      ?? compactDragTimingMultipliers
      ?? dndPreviewMultiplierIndices
      ?? stepTimingMultiplier,
  );
  let rowStepLayout = $derived(rowStepLayoutsPx(layoutTimingMultipliers));
  let rowGridSpanPx = $derived(rowGridWidthPx(layoutTimingMultipliers));
  let trailingInsertLeftPx = $derived(insertLeftAtBoundary(rowGridSpanPx));
  /** @type {{ cellWidth: number, step: number, gapBefore: boolean }[]} */
  let rowCellLayouts = $derived(renderedDndItems.map((item, index) => {
    const dataStep = isShadowItem(item)
      ? stepIndexFromId(draggedStepId ?? "")
      : stepIndexFromId(item.id);
    const stepIdForLayout = dataStep >= 0 ? stepIds[dataStep] : null;
    const collapsed = isCollapsedDndSlot(item, stepIdForLayout);
    const multiplierIndex = item.multiplierIndex
      ?? (dataStep >= 0 ? multiplierIndexForDataStep(dataStep) : defaultStepTimingMultiplierIndex);

    return {
      cellWidth: collapsed ? 0 : layoutPx(stepDisplayWidthPx(multiplierIndex)),
      step: isShadowItem(item) ? -1 : dataStep,
      gapBefore: index > 0 && !collapsed,
    };
  }));
  let stableDropCellWidths = $derived(stepIds.map((id, step) =>
    layoutPx(stepDisplayWidthPx(multiplierIndexForDataStep(step))),
  ));
</script>

{#snippet stepHeaderRemoveButton(step, dimmed)}
  <button
    type="button"
    data-remove-button
    data-no-marquee
    data-no-long-press
    data-cursor="pointer"
    aria-label="Remove step"
    disabled={removeBlocked}
    class="step-remove-button pointer-events-auto absolute top-0 left-0 z-[80] flex h-5 w-5 items-center justify-center rounded-tl-lg p-0 transition-colors outline-none disabled:pointer-events-none disabled:opacity-50 {dimmed
      ? 'text-text-faint hover:text-text-muted'
      : `text-text-secondary hover:text-text ${accent.textAccentFocus}`}"
    onpointerdowncapture={(event) => event.stopPropagation()}
    onpointerdown={(event) => event.stopPropagation()}
    onmousedown={(event) => event.stopPropagation()}
    onclick={(event) => handleRemoveClick(event, step)}
  >
    <RemoveXIcon class="pointer-events-none h-2.5 w-2.5" />
  </button>
{/snippet}

{#snippet stepHeaderMultiplierLabel(stepDimmed, multiplierLabel)}
  <span
    data-multiplier-label
    class="pointer-events-none font-sans text-xs leading-none font-semibold tabular-nums {stepHeaderLabelClass(
      stepDimmed,
    )}"
    aria-hidden="true"
  >
    {multiplierLabel}
  </span>
{/snippet}

{#snippet stepHeaderTimingArea(step, stepDimmed, multiplierLabel, reorderEnabled)}
  {#if reorderEnabled}
    <div
      use:dragHandle
      use:preventTabFocus
      role="button"
      tabindex="0"
      aria-label="Drag to reorder step"
      data-cursor="grab"
      data-no-inspect
      data-no-marquee
      data-no-long-press
      title="Drag to reorder · Option-drag to duplicate · moves or copies all selected steps"
      class="step-header-drag-handle relative z-0 flex min-h-5 min-w-0 flex-1 items-center justify-end {stepDimmed
        ? 'opacity-80'
        : 'opacity-60'}"
      onpointerdown={(event) => prepareStepDrag(event, step)}
      onpointerup={(event) => retargetOpenStepInspector(event, step)}
    >
      {@render stepHeaderMultiplierLabel(stepDimmed, multiplierLabel)}
    </div>
  {:else}
    <div
      data-no-inspect
      class="flex min-h-5 min-w-0 flex-1 items-center justify-end {stepDimmed
        ? 'opacity-80'
        : 'opacity-60'}"
    >
      {@render stepHeaderMultiplierLabel(stepDimmed, multiplierLabel)}
    </div>
  {/if}
{/snippet}

{#snippet multiplierResizeHandle(step)}
  <div
    class="trailing-multiplier-resize-zone pointer-events-none absolute top-0 bottom-5 z-[60]"
    style={trailingResizeZoneStyle()}
  >
    <button
      type="button"
      data-multiplier-resize
      data-no-long-press
      data-cursor="ew-resize"
      aria-label="Resize final step boundary; double-click to insert; Option-double-click to duplicate"
      title="Double-click to insert · Option-double-click to duplicate"
      disabled={isDragging || removeBlocked}
      class="pointer-events-auto absolute top-0 right-0 bottom-5 left-0 touch-none select-none border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth} disabled:pointer-events-none disabled:opacity-50"
      onpointerdown={(event) => beginMultiplierResize(event, step)}
      onmousedown={(event) => beginMultiplierResize(event, step)}
      ondblclick={(event) => handleBoundaryDoubleClick(event, step + 1)}
    ></button>
    <span
      class="boundary-edge-handle pointer-events-none absolute top-[calc(50%+0.625rem)] z-10 h-7 w-1 -translate-x-1/2 -translate-y-1/2 rounded-full border border-current bg-current opacity-0 shadow-sm transition-opacity duration-100 {accent.textAccent}"
      style:left="{layoutPx(stepBoundaryEndResizePx())}px"
      aria-hidden="true"
    ></span>
  </div>
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
    class="relative z-[65] flex h-5 w-full shrink-0 {isQuarterStep
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
  <div
    class="relative h-full w-full min-w-0 overflow-visible rounded-lg transition-[opacity,box-shadow] duration-150 {stepInspectionMuted
      ? 'opacity-[0.78]'
      : ''} {stepInspectionFocused ? 'z-[2]' : ''} {stepCellPlaybackGlowClass(
      activeGates[step],
      stepDimmed,
    )} {(isStepSelected || stepInspectorHighlighted) && !isDragging ? accent.selectionShell : ''}"
  >
    {@render stepHeaderRemoveButton(step, stepDimmed)}
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
            data-step-header
            class="flex h-5 w-full shrink-0 items-center gap-0 pl-5 pr-1 {stepHeaderClass(stepDimmed)}"
          >
            {@render stepHeaderTimingArea(step, stepDimmed, multiplierLabel, true)}
          </div>
        {:else}
          <div
            data-step-header
            class="flex h-5 w-full shrink-0 items-center gap-0 pl-5 pr-1 {stepHeaderClass(stepDimmed)}"
          >
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
    {#if step === stepIds.length - 1}
      {@render multiplierResizeHandle(step)}
    {/if}
  </div>
{/snippet}

{#snippet largeAddStepButton(label, insertStep)}
  <PlusDragButton
    {accent}
    {muted}
    ariaLabel={label}
    variant="large"
    contentClass={isEmptyRow ? "" : "-translate-x-4"}
    options={insertMultiplierOptions}
    defaultIndex={defaultStepTimingMultiplierIndex}
    onConfirm={(multiplierIndex) => onInsertStep(row, insertStep, multiplierIndex)}
  />
{/snippet}

{#snippet emptyRowDndPlaceholders()}
  {#each renderedDndItems as item, index (item.id)}
    {@const layout = stretchToFit ? compactLayoutForItem(item, index) : layoutForItem(item, index)}
    {@const collapsed = isShadowItem(item)}
    <div
      class="relative shrink-0 pointer-events-none {collapsed ? 'h-0 min-h-0 overflow-hidden' : 'opacity-0'}"
      style={fixedFlexStyle(collapsed ? 0 : layout.cellWidth)}
      style:margin-left="{dndSlotMarginLeftPx(index, collapsed)}px"
      aria-hidden="true"
    ></div>
  {/each}
{/snippet}

{#snippet rowStartAddStepControl()}
  <div
    class="row-start-add-step-control pointer-events-auto flex shrink-0 items-center self-stretch"
    style:margin-left="{layoutPx(stepCellPaddingPx())}px"
  >
    {@render largeAddStepButton("Add first step", 0)}
  </div>
{/snippet}

{#snippet rowEndAddStepControl()}
  <div
    class="row-end-add-step-control pointer-events-auto flex shrink-0 items-center self-stretch"
    style:margin-left="{phraseRowEndAddStepInsetPx()}px"
  >
    {@render largeAddStepButton(
      isEmptyRow ? "Add first step" : "Add step to end of row",
      stepIds.length,
    )}
  </div>
{/snippet}

{#snippet gridInsertSlot(leftPx, insertStep, mode)}
  {@const boundaryCenterPx = leftPx + stepInsertZoneWidthPx() / 2}
  <div
    data-insert-slot
    class="boundary-resize-zone pointer-events-none absolute top-0 bottom-5 z-[60]"
    style={mode === "between"
      ? boundaryResizeZoneStyle(boundaryCenterPx)
      : mode === "leading"
        ? leadingBoundaryInsertZoneStyle(leftPx)
        : insertSlotStyle(leftPx)}
  >
    {#if mode === "between" && insertStep > 0}
      <button
        type="button"
        data-multiplier-resize
        data-no-long-press
        data-cursor="ew-resize"
        aria-label="Resize step boundary; double-click to insert; Option-double-click to duplicate"
        title="Double-click to insert · Option-double-click to duplicate"
        disabled={isDragging || removeBlocked}
        class="pointer-events-auto absolute top-0 right-0 bottom-5 left-0 z-0 touch-none border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth} disabled:pointer-events-none disabled:opacity-50"
        onpointerdown={(event) => beginMultiplierResize(event, insertStep - 1)}
        onmousedown={(event) => beginMultiplierResize(event, insertStep - 1)}
        ondblclick={(event) => handleBoundaryDoubleClick(event, insertStep)}
      ></button>
      <span
        class="boundary-edge-handle pointer-events-none absolute top-[calc(50%+0.625rem)] z-10 h-7 w-1 -translate-x-1/2 -translate-y-1/2 rounded-full border border-current bg-current opacity-0 shadow-sm transition-opacity duration-100 {accent.textAccent}"
        style:left="{layoutPx(stepBoundaryEndResizePx())}px"
        aria-hidden="true"
      ></span>
      <span
        class="boundary-edge-handle pointer-events-none absolute top-[calc(50%+0.625rem)] z-10 h-7 w-1 translate-x-1/2 -translate-y-1/2 rounded-full border border-current bg-current opacity-0 shadow-sm transition-opacity duration-100 {accent.textAccent}"
        style:right="{layoutPx(stepBoundaryStartResizePx())}px"
        aria-hidden="true"
      ></span>
    {:else if mode === "leading" && insertStep === 0}
      <button
        type="button"
        data-no-long-press
        data-cursor="pointer"
        aria-label="First step boundary; double-click to insert"
        title="Double-click to insert"
        disabled={isDragging || removeBlocked}
        class="pointer-events-auto absolute top-0 right-0 bottom-5 left-0 z-0 touch-none border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth} disabled:pointer-events-none disabled:opacity-50"
        onpointerdown={(event) => event.stopPropagation()}
        onmousedown={(event) => event.stopPropagation()}
        ondblclick={(event) => handleBoundaryDoubleClick(event, insertStep)}
      ></button>
      <span
        class="boundary-edge-handle pointer-events-none absolute top-[calc(50%+0.625rem)] z-10 h-7 w-1 -translate-x-1/2 -translate-y-1/2 rounded-full border border-current bg-current opacity-0 shadow-sm transition-opacity duration-100 {accent.textAccent}"
        style:left="{layoutPx(stepInsertZoneWidthPx())}px"
        aria-hidden="true"
      ></span>
    {/if}
    <div
      class="pointer-events-none absolute top-0 bottom-0 left-0 z-10"
      style:width="{layoutPx(stepInsertZoneWidthPx())}px"
    >
      <StepInsertZone
        {accent}
        {muted}
        {timingMultiplierOptions}
        onInsert={mode === "leading"
          ? (multiplierIndex) => onInsertStep(row, insertStep, multiplierIndex)
          : undefined}
      />
    </div>
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

{#snippet compactStepCell(step, stepId)}
  {@const isStepSelected = selectedStepIdSet.has(stepId)}
  {@const stepIsMuted = stepMuted[step]}
  {@const stepIsSkipped = stepSkipped[step]}
  {@const stepDimmed = muted || stepIsSkipped}
  {@const velocityOpacity = compactStepVelocityOpacity(stepVelocity[step], stepIsSkipped)}
  <div
    class="relative h-full min-h-full overflow-hidden rounded-md transition-[box-shadow,filter] duration-75 {muted
      ? 'mp-duration-track-gradient bg-app/95 ring-1 ring-inset ring-border-subtle/90'
      : 'mp-duration-track-gradient bg-surface'} {activeGates[step] && !stepDimmed
      ? accent.playbackGlow
      : ''} {isStepSelected
      ? 'brightness-75 saturate-75'
      : activeGates[step]
        ? 'brightness-125'
        : ''}"
  >
    <div
      class="pointer-events-none absolute inset-0 transition-[background-color,opacity] duration-75 {muted
        ? 'mp-duration-track-gradient bg-surface/70'
        : `mp-duration-fill-gradient ${activeGates[step] ? accent.bgAccentStrong : accent.bgAccent}`}"
      style:opacity={muted ? 1 : activeGates[step] ? 1 : velocityOpacity}
      aria-hidden="true"
    ></div>
    {#if stepIsSkipped}
      <StepSkippedOverlay active={true} />
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
    <div class="pointer-events-none relative z-10 flex h-full min-w-0 items-center justify-center px-1">
      <span
        class="truncate font-sans text-lg leading-none font-black tabular-nums {stepDimmed
          ? 'text-text-muted'
          : 'text-white'}"
      >
        {midiToNoteName(notes[step])}
      </span>
    </div>
  </div>
{/snippet}

<style>
  .boundary-resize-zone:hover .boundary-edge-handle,
  .boundary-resize-zone:focus-within .boundary-edge-handle,
  .trailing-multiplier-resize-zone:hover .boundary-edge-handle,
  .trailing-multiplier-resize-zone:focus-within .boundary-edge-handle {
    opacity: 1;
  }

  :global(.compact-step-row-dragging) > * {
    transition: none !important;
  }

  :global([data-phrase-row-dragging] [data-bulk-step-cell]) {
    transform: none !important;
  }

  .step-drop-indicator {
    width: 3px;
    top: 50%;
    transform: translate(-50%, -50%);
    border-radius: 9999px;
    background: currentColor;
    box-shadow:
      0 0 2px currentColor,
      0 0 6px currentColor,
      0 0 12px color-mix(in srgb, currentColor 72%, transparent);
  }
</style>

<div
  class="flex min-w-0 flex-1 overflow-x-hidden overflow-y-hidden"
  role="presentation"
  style:padding-left="{layoutPx(rowTimingOffsetShiftPx(timingOffsetIndex) + timingOffsetVisualCompensationPx)}px"
  onpointerdown={handleBulkSelectPointerDown}
>
  <div
    class="pointer-events-none shrink-0 self-stretch"
    style:width="{phraseGridOriginLeftOffsetPx()}px"
    aria-hidden="true"
  ></div>
  <div
    class="flex min-w-0 flex-1 items-stretch overflow-x-hidden overflow-y-hidden pt-2 pr-2 pb-2"
    role="presentation"
    style:min-height="{phraseRowMinHeightPx()}px"
  >
  {#if isEmptyRow}
    <div bind:this={dropIndicatorHostElement} class="relative flex min-w-0 flex-1 items-stretch">
      <div
        bind:this={dndZoneElement}
        use:dragHandleZone={dndZoneOptions}
        onconsider={handleConsider}
        onfinalize={handleFinalize}
        data-phrase-row-dragging={isDragging ? true : undefined}
        class="phrase-row-dnd-zone flex min-w-0 flex-1 items-stretch outline-none {stretchToFit && isDragging
          ? 'compact-step-row-dragging'
          : ''}"
        style={phraseRowDndZoneStyle}
      >
        {@render rowStartAddStepControl()}
        {@render emptyRowDndPlaceholders()}
      </div>
      {#if isDragging && dropIndicatorVisible && dropIndicatorIndex >= 0}
        <div
          data-step-drop-indicator
          class="step-drop-indicator pointer-events-none absolute top-1/2 z-[70] {accent.textAccentLight}"
          style:left="{dropIndicatorLeftPx}px"
          style:height="{phraseStepDropIndicatorHeightPxValue}px"
          aria-hidden="true"
        ></div>
      {/if}
    </div>
  {:else if stretchToFit}
    <div class="min-w-0 flex-1 overflow-x-hidden overflow-y-hidden">
      <div class="flex w-max min-w-0 items-stretch">
        <div
          bind:this={dropIndicatorHostElement}
          class="relative flex w-max min-w-0 shrink-0 items-stretch overflow-visible"
          style:min-height="{phraseStepCellMinHeightPx()}px"
          style:height="{phraseStepCellMinHeightPx()}px"
        >
          <div
            bind:this={dndZoneElement}
            use:dragHandleZone={dndZoneOptions}
            onconsider={handleConsider}
            onfinalize={handleFinalize}
            data-phrase-row-dragging={isDragging ? true : undefined}
            class="phrase-row-dnd-zone relative flex w-max shrink-0 items-stretch overflow-visible outline-none {isDragging ? 'compact-step-row-dragging' : ''}"
            style={phraseRowDndZoneStyle}
            style:padding-right="{phraseRowEndAddStepReservePx()}px"
          >
            {#each compactRenderedItems as item, index (item.id)}
              {@const layout = compactLayoutForItem(item, index)}
              {@const stepIdForLayout = layout.step >= 0 ? stepIds[layout.step] : null}
              {@const hideBulkDragSource =
                isBulkDragActive
                && stepIdForLayout
                && bulkDragBlockIds?.includes(stepIdForLayout)
                && !draggedAsDuplicate}
              {@const dimBulkDuplicateSource =
                isBulkDragActive
                && stepIdForLayout
                && bulkDragBlockIds?.includes(stepIdForLayout)
                && draggedAsDuplicate}
              {@const collapsed = layout.isShadow || hideBulkDragSource}
              <!-- svelte-ignore a11y_no_static_element_interactions -->
              <div
                data-bulk-step-cell={layout.step >= 0 ? true : undefined}
                data-compact-step-cell
                data-step-row={layout.step >= 0 ? row : undefined}
                data-step-id={stepIdForLayout ?? undefined}
                data-step-index={layout.step >= 0 ? layout.step : undefined}
                data-step-selected={layout.step >= 0 && selectedStepIdSet.has(stepIds[layout.step])
                  ? true
                  : undefined}
                data-cursor={layout.step >= 0 ? "grab" : undefined}
                class="relative min-w-0 touch-none select-none {collapsed
                  ? 'h-0 min-h-0 overflow-hidden pointer-events-none'
                  : 'h-full'} {dimBulkDuplicateSource ? 'opacity-75' : ''}"
                style={fixedFlexStyle(collapsed ? 0 : layout.cellWidth)}
                style:min-height={collapsed ? undefined : `${phraseStepCellHeightPx}px`}
                style:margin-left="{dndSlotMarginLeftPx(index, collapsed)}px"
                title={layout.isShadow
                  ? undefined
                  : "Click for advanced settings · drag to reorder · Option-drag to duplicate · Shift-drag to select"}
                aria-hidden={collapsed ? true : undefined}
                onclick={(event) => layout.step >= 0 && openCompactStepInspector(event, layout.step)}
                onpointerdowncapture={(event) => layout.step >= 0 && handleCompactStepPointerDown(event, layout.step)}
                onpointermovecapture={handleCompactStepPointerMove}
                onpointerupcapture={handleCompactStepPointerEnd}
                onpointercancelcapture={handleCompactStepPointerEnd}
              >
                {#if !layout.isShadow}
                  <div
                    use:dragHandle
                    data-compact-step-drag-handle
                    data-cursor="grab"
                    class="absolute inset-0 z-50 touch-none select-none"
                    aria-label="Drag to reorder step"
                    onpointerdown={(event) => prepareStepDrag(event, layout.step)}
                  ></div>
                  <div class="pointer-events-auto h-full overflow-visible">
                    {@render compactStepCell(layout.step, stepIds[layout.step])}
                  </div>
                {/if}
              </div>
            {/each}
          </div>
          <div class="row-end-add-step-overlay absolute inset-y-0 right-0 flex items-center">
            {@render rowEndAddStepControl()}
          </div>
          {#if isDragging && dropIndicatorVisible && dropIndicatorIndex >= 0}
            <div
              data-step-drop-indicator
              class="step-drop-indicator pointer-events-none absolute top-1/2 z-[70] {accent.textAccentLight}"
              style:left="{dropIndicatorLeftPx}px"
              style:height="{phraseStepDropIndicatorHeightPxValue}px"
              aria-hidden="true"
            ></div>
          {/if}
        </div>
      </div>
    </div>
  {:else}
    <div class="phrase-row-scrollport min-w-0 flex-1 overflow-x-auto overflow-y-hidden">
      <div class="flex w-max min-w-0 items-stretch">
      <div
        bind:this={dropIndicatorHostElement}
        class="relative flex w-max min-w-0 shrink-0 items-stretch overflow-visible"
        style:min-height="{phraseStepCellMinHeightPx()}px"
        style:height="{phraseStepCellMinHeightPx()}px"
      >
      {@render rowInsertSlots()}

        <div
          bind:this={dndZoneElement}
          use:dragHandleZone={dndZoneOptions}
          onconsider={handleConsider}
          onfinalize={handleFinalize}
          data-phrase-row-dragging={isDragging ? true : undefined}
          class="phrase-row-dnd-zone relative flex w-max shrink-0 items-stretch overflow-visible outline-none"
          style={phraseRowDndZoneStyle}
          style:padding-right="{phraseRowEndAddStepReservePx()}px"
        >
          {#each renderedDndItems as item, index (item.id)}
            {@const layout = layoutForItem(item, index)}
            {@const stepIdForLayout = layout.step >= 0 ? stepIds[layout.step] : null}
            {@const hideBulkDragSource =
              isBulkDragActive
              && stepIdForLayout
              && bulkDragBlockIds?.includes(stepIdForLayout)
              && !draggedAsDuplicate}
            {@const dimBulkDuplicateSource =
              isBulkDragActive
              && stepIdForLayout
              && bulkDragBlockIds?.includes(stepIdForLayout)
              && draggedAsDuplicate}
            {@const collapsed = isShadowItem(item) || hideBulkDragSource}
            <div
              data-bulk-step-cell={layout.step >= 0 ? true : undefined}
              data-step-row={layout.step >= 0 ? row : undefined}
              data-step-id={layout.step >= 0 ? stepIds[layout.step] : undefined}
              data-step-index={layout.step >= 0 ? layout.step : undefined}
              data-step-selected={layout.step >= 0 && selectedStepIdSet.has(stepIds[layout.step]) ? true : undefined}
              class="relative shrink-0 overflow-visible {collapsed
                ? 'h-0 min-h-0 overflow-hidden pointer-events-none'
                : 'h-full'} {dimBulkDuplicateSource ? 'opacity-75' : ''}"
              style={fixedFlexStyle(collapsed ? 0 : layout.cellWidth)}
              style:min-height={collapsed ? undefined : `${phraseStepCellHeightPx}px`}
              style:margin-left="{dndSlotMarginLeftPx(index, collapsed)}px"
              aria-hidden={collapsed ? true : undefined}
              onclick={(event) => layout.step >= 0 && retargetOpenStepInspector(event, layout.step)}
              ondblclick={(event) => layout.step >= 0 && openFullStepInspector(event, layout.step)}
            >
              {#if !isShadowItem(item)}
                <div class="pointer-events-auto h-full overflow-visible">
                  {@render stepCell(layout.step, true)}
                </div>
              {/if}
            </div>
          {/each}
        </div>
        <div class="row-end-add-step-overlay absolute inset-y-0 right-0 flex items-center">
          {@render rowEndAddStepControl()}
        </div>
        {#if isDragging && dropIndicatorVisible && dropIndicatorIndex >= 0}
          <div
            data-step-drop-indicator
            class="step-drop-indicator pointer-events-none absolute top-1/2 z-[70] {accent.textAccentLight}"
            style:left="{dropIndicatorLeftPx}px"
            style:height="{phraseStepDropIndicatorHeightPxValue}px"
            aria-hidden="true"
          ></div>
        {/if}

      </div>
      </div>
    </div>
  {/if}
  </div>
</div>
