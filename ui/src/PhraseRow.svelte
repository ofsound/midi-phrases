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
  import { compactStepVelocityOpacity } from "./compactStepVisuals.js";
  import {
    compactStepMoveThresholdPx,
  } from "./compactStepInteraction.js";
  import { clearActiveCursor, setActiveCursor } from "./cursor.js";
  import { preventTabFocus } from "./preventTabFocus.js";
  import {
    isShadowItem,
    placementIndicatorLeftPx,
    withoutShadowItems,
  } from "./dndUtils.js";
  import {
    blockDuplicateInsertionIndex,
    blockMoveOrder,
    selectedIdsInRowOrder,
  } from "./bulkStepDrag.js";
  import {
    applyBulkStepDragGhost,
    bulkGhostLayoutEntries,
    captureBulkStepGhostSnapshots,
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
    phraseRowEndStepTailPaddingPx,
    phraseRowMinHeightPx,
    phraseStepCellMinHeightPx,
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
    compactStepShellPaddingPercent,
    compactStepShellTrailingPaddingPercent,
    phraseRowEndAddStepQuarterGridColumns,
    rowGridWidthPx,
    rowQuarterGridColumns,
    insertSlotLeftPxAtGridBoundaryPx,
    boundaryResizeZoneLeftPxAtGridBoundaryPx,
    rowStepLayoutsPx,
    rowTimingOffsetShiftPx,
    stepBoundaryResizeZoneWidthPx,
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
   * @property {number} [fitGridStartColumn]
   * @property {string | null} [inspectedStepId]
   * @property {string | null} [stepInspectorHighlightedId]
   * @property {(value: number, delta: number) => number} [stepNoteValue]
   * @property {number} [defaultStepNote]
   * @property {(row: number, orderedIds: string[]) => void} [onReorder]
   * @property {(row: number, beforeIds: string[], afterIds: string[]) => void | Promise<void>} [onMoveCommitted]
   * @property {(row: number, beforeIds: string[], afterIds: string[]) => void | Promise<void>} [onBulkMoveCommitted]
   * @property {(row: number, blockIds: string[], insertionIndex: number) => void | Promise<void>} [onBulkStepDuplicateDrop]
   * @property {(targetRow: number, stepId: string, orderedTargetIds: string[]) => void | Promise<void>} [onCrossRowMove]
   * @property {(targetRow: number, stepId: string, orderedTargetIds: string[], insertionIndex?: number) => void | Promise<void>} [onStepDuplicateDrop]
   * @property {(stepId: string | null) => void} [onDuplicateDragChange]
   * @property {string | null} [duplicateDragStepId]
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
   * @property {(event: PointerEvent, origin?: { clientX?: number, clientY?: number }) => void} [onBulkSelectPointerDown]
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
    fitGridStartColumn = 0,
    inspectedStepId = null,
    stepInspectorHighlightedId = null,
    stepNoteValue = (value, delta) => value + delta,
    defaultStepNote = 60,
    onReorder = () => {},
    onMoveCommitted = () => {},
    onBulkMoveCommitted = () => {},
    onBulkStepDuplicateDrop = () => {},
    onCrossRowMove = () => {},
    onStepDuplicateDrop = () => {},
    onDuplicateDragChange = () => {},
    duplicateDragStepId = null,
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
  const defaultStepProbability = 100;
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
  /** @type {{ pointerId: number, startX: number, startY: number, moved: boolean, step: number } | null} */
  let compactStepPointerGesture = null;
  /** @type {HTMLDivElement | null} */
  let compactGridElement = $state(null);
  let compactGridWidthPx = $state(0);
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
    if (duplicateSourceRestoreFrame) {
      cancelAnimationFrame(duplicateSourceRestoreFrame);
      duplicateSourceRestoreFrame = 0;
    }

    isDragging = false;
    draggedStepId = null;
    clearDndZoneTransforms();
    blockRemoveTemporarily();
    draggedAsDuplicate = false;
    duplicateDropIndex = -1;
    dropIndicatorIndex = -1;
    onDuplicateDragChange(null);
    bulkDragBlockIds = null;
    bulkDragGhostSnapshots = null;
    bulkDragGhostLayout = null;
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

  /** @param {string} draggedId */
  function prepareBulkDragFromStep(draggedId) {
    bulkDragBlockIds = null;
    bulkDragGhostSnapshots = null;
    bulkDragGhostLayout = null;

    if (selectedStepIds.length < 2 || !selectedStepIdSet.has(draggedId)) return;

    const block = selectedIdsInRowOrder(stepIds, selectedStepIds);
    bulkDragBlockIds = block;
    bulkDragGhostSnapshots = captureBulkStepGhostSnapshots(
      dndZoneElement,
      block,
      layoutPx,
      stretchToFit ? 0 : stepInsertZoneWidthPx(),
      stretchToFit ? 0 : stepCellPaddingPx(),
    );
    bulkDragGhostLayout = bulkGhostLayoutEntries(block, bulkDragGhostSnapshots);
  }

  /** @param {PointerEvent} event @param {number} step */
  function prepareStepDrag(event, step) {
    draggedAsDuplicate = event.altKey;
    duplicateDropIndex = step;
    const stepId = stepIds[step];
    onDuplicateDragChange(event.altKey ? stepId : null);
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

  let layoutScale = $derived(stretchToFit ? 1 : Math.min(1, contentFitScale));

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

  /** @param {number} boundaryPx */
  function insertLeftAtBoundary(boundaryPx) {
    return insertSlotLeftPxAtGridBoundaryPx(boundaryPx);
  }

  /** @param {CustomEvent} event */
  function handleConsider(event) {
    if (reorderDisabled) return;

    const trigger = event.detail.info.trigger;
    const shadowIndex = event.detail.items.findIndex(isShadowItem);

    dropIndicatorIndex = shadowIndex;

    if (trigger === TRIGGERS.DRAG_STARTED) {
      beginDragSession();
      idsBeforeDrag = stepIds.slice();
      draggedStepId = event.detail.info.id;
      if (!bulkDragGhostSnapshots) {
        prepareBulkDragFromStep(event.detail.info.id);
      }
      draggedAsDuplicate = draggedAsDuplicate || duplicateDragStepId === event.detail.info.id;

      if (draggedAsDuplicate) {
        duplicateDropIndex = stepIndexFromId(event.detail.info.id);
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
      draggedStepId = event.detail.info.id;
      draggedAsDuplicate = duplicateDragStepId === event.detail.info.id;
    } else if (trigger === TRIGGERS.DRAGGED_LEFT && idsBeforeDrag === null) {
      isDragging = false;
      draggedStepId = null;
    }

    if (bulkDragBlockIds && bulkDragBlockIds.length >= 2 && idsBeforeDrag) {
      if (shadowIndex >= 0) {
        duplicateDropIndex = shadowIndex;
      }

      if (draggedAsDuplicate && trigger !== TRIGGERS.DRAG_STARTED) {
        restoreDuplicateSourceItems();
        scheduleBulkDragGhostRefresh();
        return;
      }

      dndItems = event.detail.items;
      scheduleBulkDragGhostRefresh();
      return;
    }

    if (draggedAsDuplicate && idsBeforeDrag && trigger !== TRIGGERS.DRAG_STARTED) {
      if (shadowIndex >= 0) {
        duplicateDropIndex = shadowIndex;
      }

      restoreDuplicateSourceItems();
      return;
    }

    dndItems = event.detail.items;
  }

  /** @param {CustomEvent} event */
  async function handleFinalize(event) {
    const trigger = event.detail.info.trigger;
    const movedStepId = event.detail.info.id;
    const filtered = withoutShadowItems(event.detail.items);
    const isDuplicateDrop = draggedAsDuplicate || duplicateDragStepId === movedStepId;

    if (trigger === TRIGGERS.DROPPED_INTO_ANOTHER) {
      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    if (trigger === TRIGGERS.DROPPED_OUTSIDE_OF_ANY && idsBeforeDrag) {
      dndItems = idsBeforeDrag.map((id) => ({ id }));
      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    const isCrossRowDrop = trigger === TRIGGERS.DROPPED_INTO_ZONE
      && !stepIds.includes(movedStepId);
    const isBulkDrag = bulkDragBlockIds !== null && bulkDragBlockIds.length >= 2 && idsBeforeDrag;

    if (isBulkDrag && (isCrossRowDrop || trigger === TRIGGERS.DROPPED_INTO_ANOTHER)) {
      dndItems = idsBeforeDrag.map((id) => ({ id }));
      idsBeforeDrag = null;
      endDragSession();
      return;
    }

    if (isBulkDrag) {
      if (isDuplicateDrop && duplicateDropIndex >= 0) {
        dndItems = idsBeforeDrag.map((id) => ({ id }));
        await onBulkStepDuplicateDrop(row, bulkDragBlockIds, blockDuplicateInsertionIndex(
          idsBeforeDrag,
          bulkDragBlockIds,
          duplicateDropIndex,
        ));
      } else if (duplicateDropIndex >= 0) {
        const afterIds = blockMoveOrder(idsBeforeDrag, bulkDragBlockIds, duplicateDropIndex);

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
      if (isDuplicateDrop) {
        await onStepDuplicateDrop(row, movedStepId, filtered.map((item) => item.id));
      } else {
        await onCrossRowMove(row, movedStepId, filtered.map((item) => item.id));
      }
      await tick();
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

    if (bulkDragBlockIds && bulkDragBlockIds.length >= 2 && bulkDragGhostSnapshots) {
      applyBulkStepDragGhost(
        element,
        bulkDragBlockIds,
        bulkDragGhostSnapshots,
        draggedAsDuplicate,
      );
      return;
    }

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

    if (
      target.closest(
        "[data-remove-button], [data-insert-slot], [data-multiplier-resize], [data-no-inspect], [data-step-inspector-toggle], [aria-label='Drag to reorder step']",
      )
    ) {
      return;
    }

    suppressCompactStepClick = false;
    compactStepPointerGesture = {
      pointerId: event.pointerId,
      startX: event.clientX,
      startY: event.clientY,
      moved: false,
      marqueeStarted: false,
      step,
    };

    if (event.shiftKey) {
      compactStepPointerGesture = null;
      onBulkSelectPointerDown(event);
      event.preventDefault();
      event.stopPropagation();
      return;
    }

    handleStepControlPointerDown(event, step);
  }

  /** @param {PointerEvent} event */
  function handleCompactStepPointerMove(event) {
    if (!compactStepPointerGesture || compactStepPointerGesture.pointerId !== event.pointerId) return;

    const deltaX = event.clientX - compactStepPointerGesture.startX;
    const deltaY = event.clientY - compactStepPointerGesture.startY;

    if (
      !compactStepPointerGesture.marqueeStarted
      && Math.hypot(deltaX, deltaY) >= compactStepMoveThresholdPx
    ) {
      compactStepPointerGesture.marqueeStarted = true;
      compactStepPointerGesture.moved = true;
      onBulkSelectPointerDown(event, {
        clientX: compactStepPointerGesture.startX,
        clientY: compactStepPointerGesture.startY,
      });
      compactStepPointerGesture = null;
      return;
    }

    if (Math.hypot(deltaX, deltaY) >= compactStepMoveThresholdPx) {
      compactStepPointerGesture.moved = true;
    }
  }

  /** @param {PointerEvent} event */
  function handleCompactStepPointerEnd(event) {
    if (!compactStepPointerGesture || compactStepPointerGesture.pointerId !== event.pointerId) return;

    suppressCompactStepClick = compactStepPointerGesture.moved;
    compactStepPointerGesture = null;
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

    if (compactGridWidthPx > 0 && fitGridColumns > 0) {
      const pxPerGridColumn = compactGridWidthPx / fitGridColumns;

      return pxPerGridColumn / quartersPerGridColumn;
    }

    return stepCellQuarterGridWidthPx() / quartersPerGridColumn;
  }

  /** @param {HTMLElement} node */
  function compactGridAttachment(node) {
    compactGridElement = node;
    compactGridWidthPx = node.clientWidth;

    const observer = new ResizeObserver(() => {
      compactGridWidthPx = node.clientWidth;
    });
    observer.observe(node);

    return () => {
      observer.disconnect();

      if (compactGridElement === node) {
        compactGridElement = null;
      }
    };
  }

  function clearCompactStepDrag() {
    clearResizeListeners();
    clearActiveCursor("ew-resize");
    onCompactTimingPreviewEnd();
    compactStepDrag = null;
  }

  /** @param {number} gridColumns */
  function compactStepFlexStyle(gridColumns) {
    return `flex: ${gridColumns} 1 0%; min-width: 0;`;
  }

  /** @param {number} step @param {number} [gridColumns] */
  function compactCellWidthPx(step, gridColumns = undefined) {
    const cols = gridColumns ?? quarterGridColumnsForMultiplierIndex(
      layoutTimingMultipliers[step] ?? defaultStepTimingMultiplierIndex,
    );

    if (compactGridWidthPx <= 0 || fitGridColumns <= 0) return 0;

    return (cols / fitGridColumns) * compactGridWidthPx;
  }

  /** @param {{ id: string, multiplierIndex?: number }} item @param {number} index */
  function compactLayoutForItem(item, index) {
    if (isShadowItem(item)) {
      const draggedStep = stepIndexFromId(draggedStepId ?? "");
      const gridColumns = draggedStep >= 0
        ? quarterGridColumnsForMultiplierIndex(multiplierIndexForDataStep(draggedStep))
        : 1;

      return {
        step: -1,
        gridColumns,
        cellWidth: bulkDragGhostLayout && bulkDragGhostLayout.length >= 2
          ? bulkDragGhostLayout.reduce((sum, ghost) => sum + ghost.widthPx + ghost.gapBeforePx, 0)
          : compactCellWidthPx(draggedStep, gridColumns),
        isShadow: true,
      };
    }

    const step = stepIndexFromId(item.id);
    const multiplierIndex = item.multiplierIndex ?? multiplierIndexForDataStep(step);
    const gridColumns = quarterGridColumnsForMultiplierIndex(multiplierIndex);

    return {
      step,
      gridColumns,
      cellWidth: compactCellWidthPx(step, gridColumns),
      isShadow: false,
    };
  }

  /** @param {{ id: string, multiplierIndex?: number }} item @param {number} index */
  function compactItemFlexStyle(item, index) {
    const layout = compactLayoutForItem(item, index);

    if (!layout.isShadow) return compactStepFlexStyle(layout.gridColumns);

    if (bulkDragBlockIds && bulkDragBlockIds.length >= 2) {
      const totalColumns = bulkDragBlockIds.reduce((sum, id) => {
        const step = stepIndexFromId(id);

        if (step < 0) return sum;

        return sum + quarterGridColumnsForMultiplierIndex(multiplierIndexForDataStep(step));
      }, 0);

      return compactStepFlexStyle(Math.max(1, totalColumns));
    }

    return compactStepFlexStyle(layout.gridColumns);
  }

  let compactDropIndicatorLeftPx = $derived.by(() => {
    if (!stretchToFit || !isDragging || dropIndicatorIndex < 0) return 0;
    if (compactGridWidthPx <= 0 || fitGridColumns <= 0) return 0;

    const leadingPx = (fitGridStartColumn / fitGridColumns) * compactGridWidthPx;
    const stepWidths = compactRenderedItems.map((item, index) => compactLayoutForItem(item, index).cellWidth);

    return leadingPx + placementIndicatorLeftPx(stepWidths, dropIndicatorIndex, 0, 0);
  });

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
  let reorderDisabled = $derived(stepIds.length === 0);
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
  let compactRowStepColumns = $derived(rowQuarterGridColumns(layoutTimingMultipliers));
  let compactAddStepColumns = $derived(phraseRowEndAddStepQuarterGridColumns());
  let compactTrailingGridColumns = $derived(
    Math.max(0, fitGridColumns - fitGridStartColumn - compactRowStepColumns - compactAddStepColumns),
  );
  let layoutTimingMultipliers = $derived(
    resizePreviewMultipliers
      ?? compactDragTimingMultipliers
      ?? dndPreviewMultiplierIndices
      ?? stepTimingMultiplier,
  );
  let rowStepLayout = $derived(rowStepLayoutsPx(layoutTimingMultipliers));
  let rowGridSpanPx = $derived(rowGridWidthPx(layoutTimingMultipliers));
  let scaledRowGridSpanPx = $derived(layoutPx(rowGridSpanPx));
  let trailingInsertLeftPx = $derived(insertLeftAtBoundary(rowGridSpanPx));
  let trailingAddStepLeftPx = $derived(scaledRowGridSpanPx + layoutPx(phraseRowEndAddStepInsetPx()));
  /** @type {{ cellWidth: number, step: number, gapBefore: boolean }[]} */
  let rowCellLayouts = $derived(renderedDndItems.map((item, index) => {
    const dataStep = isShadowItem(item)
      ? stepIndexFromId(draggedStepId ?? "")
      : stepIndexFromId(item.id);
    const multiplierIndex = item.multiplierIndex
      ?? (dataStep >= 0 ? multiplierIndexForDataStep(dataStep) : defaultStepTimingMultiplierIndex);

    return {
      cellWidth: layoutPx(stepDisplayWidthPx(multiplierIndex)),
      step: isShadowItem(item) ? -1 : dataStep,
      gapBefore: index > 0,
    };
  }));
  let dropIndicatorLeftPx = $derived(placementIndicatorLeftPx(
    rowCellLayouts.map((layout) => layout.cellWidth),
    dropIndicatorIndex,
    layoutPx(stepCellPaddingPx()),
    layoutPx(stepInsertZoneWidthPx()),
  ));
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
      class="flex min-h-5 min-w-0 flex-1 items-center justify-end {stepDimmed
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
  <div
    class="relative h-full w-full min-w-0 overflow-visible rounded-lg transition-[opacity,box-shadow] duration-150 {stepInspectionMuted
      ? 'opacity-[0.78]'
      : ''} {stepInspectionFocused ? 'z-[2]' : ''} {stepCellPlaybackGlowClass(
      activeGates[step],
      stepDimmed,
    )} {(isStepSelected || stepInspectorHighlighted) && !isDragging ? accent.selectionShell : ''}"
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
            data-step-header
            class="flex h-5 w-full shrink-0 items-center gap-0 px-1 {stepHeaderClass(stepDimmed)}"
          >
            {@render stepHeaderRemoveButton(step, stepDimmed)}
            {@render stepHeaderTimingArea(step, stepDimmed, multiplierLabel, true)}
          </div>
        {:else}
          <div
            data-step-header
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
    options={insertMultiplierOptions}
    defaultIndex={defaultStepTimingMultiplierIndex}
    onConfirm={(multiplierIndex) => onInsertStep(row, insertStep, multiplierIndex)}
  />
{/snippet}

{#snippet gridInsertSlot(leftPx, insertStep, mode)}
  {@const boundaryCenterPx = leftPx + stepInsertZoneWidthPx() / 2}
  <div
    data-insert-slot
    class="pointer-events-none absolute top-0 bottom-0 z-[60]"
    style={mode === "between"
      ? boundaryResizeZoneStyle(boundaryCenterPx)
      : insertSlotStyle(leftPx)}
  >
    {#if mode === "between" && insertStep > 0}
      <button
        type="button"
        data-multiplier-resize
        data-no-long-press
        data-cursor="ew-resize"
        aria-label="Resize step timing multiplier"
        disabled={isDragging || removeBlocked}
        class="pointer-events-auto absolute inset-0 z-0 touch-none border-0 bg-transparent p-0 outline-none {accent.ringFocusWithWidth} disabled:pointer-events-none disabled:opacity-50"
        onpointerdown={(event) => beginMultiplierResize(event, insertStep - 1)}
        onmousedown={(event) => beginMultiplierResize(event, insertStep - 1)}
      ></button>
    {/if}
    <StepInsertZone
      {accent}
      {muted}
      {timingMultiplierOptions}
      onInsert={mode === "leading" || mode === "between"
        ? (multiplierIndex) => onInsertStep(row, insertStep, multiplierIndex)
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
  {@const stepDimmed = muted || stepIsSkipped}
  {@const velocityOpacity = compactStepVelocityOpacity(stepVelocity[step], stepIsSkipped)}
  {@const stepMultiplierIndex = layoutTimingMultipliers[step] ?? defaultStepTimingMultiplierIndex}
  {@const shellPaddingPercent = compactStepShellPaddingPercent(stepMultiplierIndex)}
  {@const trailingPaddingPercent = compactStepShellTrailingPaddingPercent(stepMultiplierIndex)}
  <div
    class="relative h-full overflow-hidden rounded-md transition-[box-shadow,filter] duration-75 {muted
      ? 'bg-app/95 ring-1 ring-inset ring-border-subtle/90'
      : 'bg-surface'} {activeGates[step] && !stepDimmed
      ? accent.playbackGlow
      : ''} {isStepSelected
      ? 'brightness-75 saturate-75'
      : activeGates[step]
        ? 'brightness-125'
        : ''}"
    style:margin-left="{shellPaddingPercent}%"
    style:margin-right="{step === stepIds.length - 1 ? trailingPaddingPercent : shellPaddingPercent}%"
    style:min-height="{phraseStepCellMinHeightPx()}px"
  >
    <div
      class="pointer-events-none absolute inset-0 transition-[background-color,opacity] duration-75 {muted
        ? 'bg-surface/70'
        : activeGates[step]
          ? accent.bgAccentStrong
          : accent.bgAccent}"
      style:opacity={muted ? 1 : activeGates[step] ? 1 : velocityOpacity}
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
      class="pointer-events-none relative z-10 flex h-full min-w-0 items-center justify-center px-1"
    >
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
        class="compact-step-reorder-handle pointer-events-auto inline-flex max-w-full touch-none items-center justify-center rounded-sm px-2 py-1 outline-none {stepIsSkipped
          ? 'opacity-45'
          : muted
            ? 'opacity-80'
            : ''} {accent.ringFocusWithWidth}"
        onpointerdown={(event) => prepareStepDrag(event, step)}
        onpointerup={(event) => retargetOpenStepInspector(event, step)}
      >
        <span
          class="pointer-events-none truncate font-sans text-lg leading-none font-black tabular-nums {stepDimmed
            ? 'text-text-muted'
            : 'text-white'}"
        >
          {midiToNoteName(notes[step])}
        </span>
      </div>
    </div>
  </div>
{/snippet}

<style>
  .compact-step-reorder-handle {
    transform-origin: center;
    transition: transform 75ms ease;
  }

  .compact-step-reorder-handle:hover,
  .compact-step-reorder-handle:focus-visible {
    transform: scale(1.12);
  }

  :global(.compact-step-row-dragging) > * {
    transition: none !important;
  }

  :global(.compact-step-row-dragging) .compact-step-reorder-handle {
    transform: none;
  }

  .compact-step-skipped-overlay {
    background: linear-gradient(
      to bottom right,
      transparent calc(50% - 1px),
      color-mix(in srgb, var(--color-text) 72%, transparent) calc(50% - 1px),
      color-mix(in srgb, var(--color-text) 72%, transparent) calc(50% + 1px),
      transparent calc(50% + 1px)
    );
  }

  .step-drop-indicator {
    width: 3px;
    transform: translateX(-50%);
    border-radius: 9999px;
    background: currentColor;
    box-shadow:
      0 0 3px currentColor,
      0 0 8px currentColor,
      0 0 16px color-mix(in srgb, currentColor 78%, transparent);
    transition: left 55ms cubic-bezier(0.2, 0.75, 0.25, 1);
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
    class="flex min-w-0 flex-1 items-stretch pt-2 pr-2 pb-2 {stretchToFit || layoutScale < 1
      ? 'overflow-hidden'
      : 'overflow-x-auto'}"
    role="presentation"
    style:min-height="{phraseRowMinHeightPx()}px"
  >
  {#if isEmptyRow}
    <div class="relative flex shrink-0 items-center" style:padding-left="{layoutPx(phraseRowEndAddStepInsetPx())}px">
      {@render largeAddStepButton("Add first step", 0)}
    </div>
  {:else if stretchToFit}
    <div
      class="relative flex min-w-0 flex-1 items-stretch"
      {@attach compactGridAttachment}
    >
      {#if fitGridStartColumn > 0}
        <div
          class="pointer-events-none shrink-0"
          style={compactStepFlexStyle(fitGridStartColumn)}
          aria-hidden="true"
        ></div>
      {/if}
      <div
        bind:this={dndZoneElement}
        use:dragHandleZone={dndZoneOptions}
        onconsider={handleConsider}
        onfinalize={handleFinalize}
        data-phrase-row-dragging={isDragging ? true : undefined}
        class="flex min-h-0 min-w-0 items-stretch outline-none {isDragging ? 'compact-step-row-dragging' : ''}"
        style={compactStepFlexStyle(compactRowStepColumns)}
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
          <!-- svelte-ignore a11y_click_events_have_key_events -->
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
            class="relative min-w-0 {layout.isShadow ? 'pointer-events-none' : ''} {hideBulkDragSource
              ? 'opacity-0 pointer-events-none'
              : dimBulkDuplicateSource
                ? 'opacity-75'
                : ''}"
            style={compactItemFlexStyle(item, index)}
            title={layout.isShadow
              ? undefined
              : "Click for advanced settings · drag center to reorder · Option-drag to duplicate"}
            aria-hidden={layout.isShadow ? true : undefined}
            onclick={(event) => layout.step >= 0 && openCompactStepInspector(event, layout.step)}
            onpointerdowncapture={(event) => layout.step >= 0 && handleCompactStepPointerDown(event, layout.step)}
            onpointermovecapture={handleCompactStepPointerMove}
            onpointerupcapture={handleCompactStepPointerEnd}
            onpointercancelcapture={handleCompactStepPointerEnd}
          >
            {#if layout.isShadow}
              {#if bulkDragGhostLayout && bulkDragGhostLayout.length >= 2}
                <div
                  class="flex h-full min-w-0 items-stretch opacity-35"
                  style:min-height="{phraseStepCellMinHeightPx()}px"
                  aria-hidden="true"
                >
                  {#each bulkDragGhostLayout as ghost (ghost.stepId)}
                    {@const ghostStep = stepIndexFromId(ghost.stepId)}
                    {@const ghostColumns = ghostStep >= 0
                      ? quarterGridColumnsForMultiplierIndex(
                          layoutTimingMultipliers[ghostStep] ?? defaultStepTimingMultiplierIndex,
                        )
                      : 1}
                    <div
                      class="h-full shrink-0 rounded-md border-2 border-dashed border-border-subtle bg-surface/60"
                      style={compactStepFlexStyle(ghostColumns)}
                    ></div>
                  {/each}
                </div>
              {:else}
                <div
                  class="shrink-0 rounded-md border-2 border-dashed border-border-subtle/80 bg-surface/40 opacity-35"
                  style:min-height="{phraseStepCellMinHeightPx()}px"
                  aria-hidden="true"
                ></div>
              {/if}
            {:else}
              {@render compactStepCell(layout.step, stepIds[layout.step])}
            {/if}
          </div>
        {/each}
      </div>
      {#if compactTrailingGridColumns > 0}
        <div
          class="pointer-events-none shrink-0"
          style={compactStepFlexStyle(compactTrailingGridColumns)}
          aria-hidden="true"
        ></div>
      {/if}
      <div
        class="pointer-events-auto flex shrink-0 items-center self-stretch"
        style={compactStepFlexStyle(compactAddStepColumns)}
      >
        {@render largeAddStepButton("Add step to end of row", stepIds.length)}
      </div>
      {#if isDragging && dropIndicatorIndex >= 0}
        <div
          data-step-drop-indicator
          class="step-drop-indicator pointer-events-none absolute -top-1 -bottom-1 z-[70] {accent.textAccentLight}"
          style:left="{compactDropIndicatorLeftPx}px"
          aria-hidden="true"
        ></div>
      {/if}
    </div>
  {:else}
    <div
      class="relative w-max min-w-0 shrink-0 self-stretch overflow-visible"
      style:min-width="{scaledRowGridSpanPx}px"
      style:padding-right="{layoutPx(phraseRowEndStepTailPaddingPx())}px"
    >
      {@render rowInsertSlots()}

      {#if reorderDisabled}
        <div class="relative flex w-max shrink-0 items-stretch overflow-visible">
          {#each stepIds as stepId, step (stepId)}
            <!-- svelte-ignore a11y_click_events_have_key_events -->
            <!-- svelte-ignore a11y_no_static_element_interactions -->
            <div
              data-bulk-step-cell
              data-step-row={row}
              data-step-id={stepId}
              data-step-index={step}
              data-step-selected={selectedStepIdSet.has(stepId) ? true : undefined}
              class="relative shrink-0 overflow-visible"
              style={shellStyleForStep(step)}
              style:margin-left={step === 0
                ? `${layoutPx(stepCellPaddingPx())}px`
                : `${layoutPx(stepInsertZoneWidthPx())}px`}
              onclick={(event) => retargetOpenStepInspector(event, step)}
              ondblclick={(event) => openFullStepInspector(event, step)}
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
            {@const shadowShellWidth =
              isShadowItem(item) && bulkDragGhostLayout && bulkDragGhostLayout.length >= 2
                ? bulkDragGhostLayout.reduce(
                    (sum, ghost) => sum + ghost.widthPx + ghost.gapBeforePx,
                    0,
                  )
                : layout.cellWidth}
            <div
              data-bulk-step-cell={layout.step >= 0 ? true : undefined}
              data-step-row={layout.step >= 0 ? row : undefined}
              data-step-id={layout.step >= 0 ? stepIds[layout.step] : undefined}
              data-step-index={layout.step >= 0 ? layout.step : undefined}
              data-step-selected={layout.step >= 0 && selectedStepIdSet.has(stepIds[layout.step]) ? true : undefined}
              class="relative shrink-0 overflow-visible {isShadowItem(item) ? 'pointer-events-none' : ''} {hideBulkDragSource
                ? 'opacity-0 pointer-events-none'
                : dimBulkDuplicateSource
                  ? 'opacity-75'
                  : ''}"
              style={fixedFlexStyle(shadowShellWidth)}
              style:margin-left={index === 0
                ? `${layoutPx(stepCellPaddingPx())}px`
                : `${layoutPx(stepInsertZoneWidthPx())}px`}
              aria-hidden={isShadowItem(item) ? true : undefined}
              onclick={(event) => layout.step >= 0 && retargetOpenStepInspector(event, layout.step)}
              ondblclick={(event) => layout.step >= 0 && openFullStepInspector(event, layout.step)}
            >
              {#if isShadowItem(item)}
                {#if bulkDragGhostLayout && bulkDragGhostLayout.length >= 2}
                  <div class="flex shrink-0 items-stretch opacity-35" aria-hidden="true">
                    {#each bulkDragGhostLayout as ghost (ghost.stepId)}
                      <div
                        class="shrink-0 rounded-lg border-2 border-dashed border-border-subtle bg-surface/60"
                        style={fixedFlexStyle(ghost.widthPx)}
                        style:margin-left="{ghost.gapBeforePx}px"
                      ></div>
                    {/each}
                  </div>
                {:else}
                  <div class="shrink-0" style={fixedFlexStyle(layout.cellWidth)}></div>
                {/if}
              {:else}
                <div class="pointer-events-auto h-full overflow-visible">
                  {@render stepCell(layout.step, true)}
                </div>
              {/if}
            </div>
          {/each}
        </div>
        {#if isDragging && dropIndicatorIndex >= 0}
          <div
            data-step-drop-indicator
            class="step-drop-indicator pointer-events-none absolute -top-1 -bottom-1 z-[70] {accent.textAccentLight}"
            style:left="{dropIndicatorLeftPx}px"
            aria-hidden="true"
          ></div>
        {/if}
      {/if}

      {@render trailingAddStep()}
    </div>
  {/if}
  </div>
</div>
