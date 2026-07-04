<script>
  import PianoRollKeyboardLabels from "./PianoRollKeyboardLabels.svelte";
  import { midiToNoteName } from "./midiNoteNames.js";
  import { fittedPitchRangeForNotes } from "./pianoRollViewport.js";
  import { defaultPulseIndex } from "./pulseLayout.js";
  import { isBlackKey } from "./phraseSchedule.js";
  import { doubleClick } from "./doubleClickAction.js";
  import {
    emeraldRowAccent,
  } from "./rowAccentTheme.js";
  import {
    beatLineQuarters,
    insertStepIndexFromRollX,
    measureLineQuarters,
    midiFromPitchDragDelta,
    pianoRollNoteDragTooltipVisible,
    rollLengthQuartersForCycle,
    shapeVelocityUpdatesFromStroke,
    stepAtRollX,
    velocityYInRoll,
  } from "./rowPianoRollShape.js";
  import {
    buildRowRollTimeline,
    precedingStepExpansionForNoteDrag,
    precedingStepResizeForNoteDrag,
    timingMultiplierIndexAfterRollResize,
  } from "./rowPianoRollTimeline.js";
  import RowPianoRollModeIcon from "./RowPianoRollModeIcon.svelte";
  import RowShapeDrawIcon from "./RowShapeDrawIcon.svelte";
  import BulkStepEditControls from "./BulkStepEditControls.svelte";
  import { durationBarFillOpacity } from "./compactStepVisuals.js";
  import CompactStepResizeHandle from "./CompactStepResizeHandle.svelte";
  import StepSkippedOverlay from "./StepSkippedOverlay.svelte";
  import {
    defaultRowTimingOffsetIndex,
    defaultStepTimingMultiplierIndex,
    maxPhraseStepsPerRow,
    stepTimingMultiplierCount,
  } from "./stepCellLayout.js";
  import { scaledPx } from "./uiScale.svelte.js";

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {string[]} [stepIds]
   * @property {number[]} [notes]
   * @property {number[]} [stepDurationFraction]
   * @property {number[]} [stepTimingMultiplier]
   * @property {number[]} [stepVelocity]
   * @property {boolean[]} [stepMuted]
   * @property {boolean[]} [stepSkipped]
   * @property {boolean[]} [activeGates]
   * @property {number} [rowTimingOffset]
   * @property {number} [pulseIndex]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {string | null} [inspectedStepId]
   * @property {string[]} [selectedStepIds]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(row: number, step: number, stepId: string) => void | Promise<void>} [onInspectStep]
   * @property {(row: number, step: number, midi: number) => void} [onNotePreview]
   * @property {(row: number, step: number, midi: number) => void | Promise<void>} [onNoteCommit]
   * @property {(row: number, fromStep: number, toStep: number) => void | Promise<void>} [onStepMove]
   * @property {(row: number, step: number, multiplierIndex: number) => void | Promise<void>} [onStepResize]
   * @property {(row: number, step: number) => void} [onStepBulkGestureStart]
   * @property {(row: number, step: number, stepId: string) => void | Promise<void>} [onOpenAdvancedInspector]
   * @property {(row: number, updates: { step: number, velocity: number }[]) => void | Promise<void>} [onShapeVelocitiesCommit]
   * @property {(row: number, step: number, multiplierIndex?: number) => void | Promise<void>} [onInsertStep]
   * @property {(row: number, step: number) => void | Promise<void>} [onDuplicateStep]
   * @property {(event: PointerEvent, origin?: { toggleStep?: boolean }) => void} [onBulkSelectPointerDown]
   * @property {number} [bulkDurationPercent]
   * @property {number} [bulkVelocityPercent]
   * @property {number} [bulkLengthDelta]
   * @property {number} [bulkTransposeSemitones]
   * @property {string} [bulkPitchAriaLabel]
   * @property {boolean} [bulkReverseAvailable]
   * @property {boolean} [bulkSkipActive]
   * @property {boolean} [bulkMuteActive]
   * @property {() => void | Promise<void>} [onBulkReverse]
   * @property {() => void | Promise<void>} [onBulkShuffle]
   * @property {() => void | Promise<void>} [onBulkRandomizeOctaves]
   * @property {() => void | Promise<void>} [onBulkRandomizeLengths]
   * @property {() => void | Promise<void>} [onBulkToggleSkip]
   * @property {() => void | Promise<void>} [onBulkToggleMute]
   * @property {() => void} [onBulkGestureStart]
   * @property {(value: number) => void} [onBulkLengthPreview]
   * @property {(value: number) => void | Promise<void>} [onBulkLengthCommit]
   * @property {(value: number) => void} [onBulkDurationPreview]
   * @property {(value: number) => void | Promise<void>} [onBulkDurationCommit]
   * @property {(value: number) => void} [onBulkVelocityPreview]
   * @property {(value: number) => void | Promise<void>} [onBulkVelocityCommit]
   * @property {(value: number) => void} [onBulkTransposePreview]
   * @property {(value: number) => void | Promise<void>} [onBulkTransposeCommit]
   */

  /** @type {Props} */
  let {
    row = 0,
    stepIds = [],
    notes = [],
    stepDurationFraction = [],
    stepTimingMultiplier = [],
    stepVelocity = [],
    stepMuted = [],
    stepSkipped = [],
    activeGates = [],
    rowTimingOffset = defaultRowTimingOffsetIndex,
    pulseIndex = defaultPulseIndex,
    scaleRoot = 0,
    scaleModeIndex = 0,
    inspectedStepId = null,
    selectedStepIds = [],
    accent,
    onInspectStep = () => {},
    onNotePreview = () => {},
    onNoteCommit = () => {},
    onStepMove = () => {},
    onStepResize = () => {},
    onStepBulkGestureStart = () => {},
    onOpenAdvancedInspector = () => {},
    onInsertStep = () => {},
    onDuplicateStep = () => {},
    onShapeVelocitiesCommit = () => {},
    onBulkSelectPointerDown = () => {},
    bulkDurationPercent = 0,
    bulkVelocityPercent = 0,
    bulkLengthDelta = 0,
    bulkTransposeSemitones = 0,
    bulkPitchAriaLabel = "Bulk step pitch semitones",
    bulkReverseAvailable = false,
    bulkSkipActive = false,
    bulkMuteActive = false,
    onBulkReverse = () => {},
    onBulkShuffle = () => {},
    onBulkRandomizeOctaves = () => {},
    onBulkRandomizeLengths = () => {},
    onBulkToggleSkip = () => {},
    onBulkToggleMute = () => {},
    onBulkGestureStart = () => {},
    onBulkLengthPreview = () => {},
    onBulkLengthCommit = () => {},
    onBulkDurationPreview = () => {},
    onBulkDurationCommit = () => {},
    onBulkVelocityPreview = () => {},
    onBulkVelocityCommit = () => {},
    onBulkTransposePreview = () => {},
    onBulkTransposeCommit = () => {},
  } = $props();

  const basePxPerQuarter = 28;
  const baseKeyboardWidthPx = 44;
  const baseRulerHeightPx = 28;

  /** @type {HTMLElement | null} */
  let gridElement = $state(null);
  /** @type {HTMLElement | null} */
  let verticalScrollElement = $state(null);
  let viewportHeightPx = $state(0);
  let gridViewportWidthPx = $state(0);
  let drag = $state(null);
  /** @type {'roll' | 'velocity'} */
  let shapeDrawMode = $state("roll");
  /** @type {{ pointerId: number, mode: 'velocity', points: { x: number, y: number }[] } | null} */
  let shapeStroke = $state(null);
  let rowAccent = $derived(accent ?? emeraldRowAccent);
  let bulkEffectiveStepCount = $derived(
    selectedStepIds.length > 0 ? selectedStepIds.length : stepIds.length,
  );
  /** CSS color variable resolved from the row accent's bgAccent class (e.g. "bg-accent" → "--color-accent"). */
  let boundaryAccentVar = $derived(
    `var(--color-${rowAccent.bgAccent.replace('bg-', '')})`
  );
  let isShapeDrawMode = $derived(shapeDrawMode === "velocity");
  let selectedStepIdSet = $derived(new Set(selectedStepIds));

  let displayedTimingMultipliers = $derived.by(() => {
    if (drag?.mode === "resize") {
      return stepTimingMultiplier.map((value, step) =>
        step === drag.step ? drag.previewMultiplierIndex : value,
      );
    }

    if (
      drag?.mode === "move" &&
      drag.previewPreviousStep >= 0 &&
      drag.previewPreviousMultiplierIndex !== drag.originalPreviousMultiplierIndex
    ) {
      return stepTimingMultiplier.map((value, step) =>
        step === drag.previewPreviousStep ? drag.previewPreviousMultiplierIndex : value,
      );
    }

    return stepTimingMultiplier;
  });
  let timeline = $derived(
    buildRowRollTimeline(displayedTimingMultipliers, stepSkipped, pulseIndex, rowTimingOffset),
  );
  let rollLengthQuarters = $derived(
    rollLengthQuartersForCycle(timeline.timelineLengthQuarters),
  );
  let pxPerQuarter = $derived.by(() => {
    if (drag?.mode === "resize") return drag.resizePxPerQuarter;
    if (drag?.mode === "move" && drag.previewPreviousStep >= 0) {
      return drag.dragPxPerQuarter;
    }

    const floor = scaledPx(basePxPerQuarter);

    if (gridViewportWidthPx <= 0 || rollLengthQuarters <= 0) return floor;

    return Math.max(floor, gridViewportWidthPx / rollLengthQuarters);
  });
  let rollWidthPx = $derived(Math.max(1, rollLengthQuarters * pxPerQuarter));
  let keyboardWidthPx = $derived(scaledPx(baseKeyboardWidthPx));
  let rulerHeightPx = $derived(scaledPx(baseRulerHeightPx));
  let measureLines = $derived(measureLineQuarters(rollLengthQuarters));
  let beatLines = $derived(beatLineQuarters(rollLengthQuarters));
  let visiblePitchRange = $derived.by(() => {
    if (drag?.mode === "move") {
      return drag.lockedPitchRange;
    }

    return fittedPitchRangeForNotes(notes);
  });
  let pitchSpan = $derived(visiblePitchRange.maxMidi - visiblePitchRange.minMidi + 1);
  let rowHeightPx = $derived(
    viewportHeightPx > 0 ? viewportHeightPx / pitchSpan : scaledPx(16),
  );
  let rollHeightPx = $derived(pitchSpan * rowHeightPx);
  let pitchRows = $derived(Array.from({ length: pitchSpan }, (_, index) => visiblePitchRange.maxMidi - index));

  let shapePreviewByStep = $derived.by(() => {
    if (!shapeStroke || shapeStroke.points.length === 0) return null;

    const updates = shapeVelocityUpdatesFromStroke(
      shapeStroke.points,
      timeline.slots,
      pxPerQuarter,
      rollHeightPx,
    );

    return new Map(updates.map((update) => [update.step, update.velocity]));
  });
  let stepNotes = $derived.by(() => {
    return notes.map((midi, step) => {
      const displayStep =
        drag?.mode === "move" && drag.step === step ? drag.targetStep : step;
      const slot = timeline.slots[displayStep] ?? timeline.slots[step];
      const durationFraction = stepDurationFraction[step] ?? 1;
      const shapePreviewValue = shapePreviewByStep?.get(step);
      const shapePreviewVelocity = shapeStroke?.mode === "velocity" ? shapePreviewValue : undefined;
      const noteLengthQuarters = slot.lengthQuarters * durationFraction;
      const fullStepWidthPx = Math.max(1, slot.lengthQuarters * pxPerQuarter);

      return {
        step,
        stepId: stepIds[step],
        midi,
        leftPx: slot.startQuarters * pxPerQuarter,
        fullStepWidthPx,
        durationWidthPx: Math.min(
          fullStepWidthPx,
          Math.max(scaledPx(12), noteLengthQuarters * pxPerQuarter - 2),
        ),
        velocity: shapePreviewVelocity ?? stepVelocity[step] ?? 100,
        muted: stepMuted[step],
        skipped: stepSkipped[step],
        dimmed: stepMuted[step] || stepSkipped[step],
        active: activeGates[step] ?? false,
      };
    });
  });
  /** @param {HTMLElement} node */
  function gridAttachment(node) {
    gridElement = node;
    gridViewportWidthPx = node.clientWidth;

    const resizeObserver = new ResizeObserver(() => {
      gridViewportWidthPx = node.clientWidth;
    });
    resizeObserver.observe(node);

    return () => {
      resizeObserver.disconnect();
      if (gridElement === node) gridElement = null;
    };
  }

  /** @param {HTMLElement} node */
  function verticalScrollAttachment(node) {
    verticalScrollElement = node;
    viewportHeightPx = node.clientHeight;

    const resizeObserver = new ResizeObserver(() => {
      viewportHeightPx = node.clientHeight;
    });
    resizeObserver.observe(node);

    return () => {
      resizeObserver.disconnect();
      if (verticalScrollElement === node) verticalScrollElement = null;
    };
  }

  /** @param {number} midi */
  function pitchTopPx(midi) {
    return (visiblePitchRange.maxMidi - midi) * rowHeightPx;
  }

  /**
   * @param {number} previewStep
   * @param {number} previewMidi
   */
  function fittedPitchRangeForDragPreview(previewStep, previewMidi) {
    return fittedPitchRangeForNotes(
      notes.map((midi, step) => (step === previewStep ? previewMidi : midi)),
    );
  }

  /** @param {string} stepId */
  function isMultiPitchDrag(stepId) {
    return selectedStepIds.length > 1 && selectedStepIdSet.has(stepId);
  }

  /** @param {number} velocity */
  function velocityBarHeightPx(velocity) {
    const clamped = Math.min(127, Math.max(0, Math.round(velocity)));

    return Math.max(1, (clamped / 127) * rollHeightPx);
  }

  /** @param {number} midi */
  function pitchRowClass(midi) {
    return isBlackKey(midi)
      ? "bg-piano-roll-black-key border-b border-piano-roll-black-key-line"
      : "bg-piano-roll-white-key border-b border-piano-roll-white-key-line";
  }

  /** @param {number} quarter */
  function quarterLeftPx(quarter) {
    return quarter * pxPerQuarter;
  }

  /** @param {number} measureQuarter */
  function measureNumberForQuarter(measureQuarter) {
    return measureQuarter / 4 + 1;
  }

  /** @param {PointerEvent} event */
  function rollXFromPointer(event) {
    if (!gridElement) return 0;

    const rect = gridElement.getBoundingClientRect();
    return Math.max(0, Math.min(rollWidthPx, event.clientX - rect.left + gridElement.scrollLeft));
  }

  /** @param {PointerEvent} event @param {HTMLElement} surface */
  function rollPointFromPointer(event, surface) {
    const rect = surface.getBoundingClientRect();

    return {
      x: Math.max(0, Math.min(rollWidthPx, event.clientX - rect.left)),
      y: Math.max(0, Math.min(rollHeightPx, event.clientY - rect.top)),
    };
  }

  /** @param {'roll' | 'velocity'} mode */
  function setShapeDrawMode(mode) {
    if (shapeDrawMode === mode) return;

    shapeDrawMode = mode;
    shapeStroke = null;
    drag = null;
  }

  /** @param {'roll' | 'velocity'} mode */
  function shapeModeButtonClasses(mode) {
    const active = shapeDrawMode === mode;

    return `flex h-10 shrink-0 items-center justify-center gap-1.5 rounded-md border px-3 text-xs font-medium transition-colors outline-none ${
      active
        ? `${rowAccent.borderActive} ${rowAccent.bgAccentStrong} text-control-primary-text`
        : "border-border bg-surface text-text-muted hover:border-border-strong hover:text-text"
    } ${rowAccent.ringFocusWithWidth || "focus-visible:ring-1 focus-visible:ring-focus-ring"}`;
  }

  /** @param {PointerEvent} event */
  function beginShapeDraw(event) {
    if (!isShapeDrawMode || shapeStroke || drag) return;

    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    const surface = /** @type {HTMLElement} */ (event.currentTarget);
    shapeStroke = {
      pointerId: event.pointerId,
      mode: "velocity",
      points: [rollPointFromPointer(event, surface)],
    };
  }

  /** @param {PointerEvent} event */
  function moveShapeDraw(event) {
    if (!shapeStroke || event.pointerId !== shapeStroke.pointerId) return;

    const surface = /** @type {HTMLElement} */ (event.currentTarget);
    const point = rollPointFromPointer(event, surface);
    const lastPoint = shapeStroke.points[shapeStroke.points.length - 1];

    if (Math.hypot(point.x - lastPoint.x, point.y - lastPoint.y) < 1) return;

    shapeStroke = {
      ...shapeStroke,
      points: [...shapeStroke.points, point],
    };
  }

  /** @param {PointerEvent} event */
  async function endShapeDraw(event) {
    if (!shapeStroke || event.pointerId !== shapeStroke.pointerId) return;

    event.currentTarget.releasePointerCapture(event.pointerId);

    const finished = shapeStroke;
    shapeStroke = null;

    const updates = shapeVelocityUpdatesFromStroke(
      finished.points,
      timeline.slots,
      pxPerQuarter,
      rollHeightPx,
    );

    if (updates.length === 0) return;

    const changedUpdates = updates.filter(
      (update) => stepVelocity[update.step] !== update.velocity,
    );

    if (changedUpdates.length === 0) return;

    await onShapeVelocitiesCommit(row, changedUpdates);
  }

  /** @param {PointerEvent} event @param {number} baseMidi @param {number} startY @param {number} pitchDragRowHeightPx */
  function midiFromPointerDelta(event, baseMidi, startY, pitchDragRowHeightPx) {
    const deltaRows = Math.round((startY - event.clientY) / Math.max(1, pitchDragRowHeightPx));

    return midiFromPitchDragDelta(baseMidi, deltaRows, scaleRoot, scaleModeIndex);
  }

  /** @param {PointerEvent} event @param {any} note */
  function beginNoteDrag(event, note) {
    if (isShapeDrawMode) return;

    if (event.shiftKey) {
      onBulkSelectPointerDown(event, { toggleStep: true });
      return;
    }

    void onInspectStep(row, note.step, note.stepId);

    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    const originalSlots = timeline.slots.map((slot) => ({ ...slot }));
    const previewPreviousStep = note.step - 1;
    const originalPreviousMultiplierIndex =
      previewPreviousStep >= 0
        ? stepTimingMultiplier[previewPreviousStep] ?? defaultStepTimingMultiplierIndex
        : defaultStepTimingMultiplierIndex;

    drag = {
      mode: "move",
      pointerId: event.pointerId,
      row,
      step: note.step,
      startX: event.clientX,
      startY: event.clientY,
      baseMidi: note.midi,
      previewMidi: note.midi,
      targetStep: note.step,
      originalSlots,
      dragPxPerQuarter: pxPerQuarter,
      previewPreviousStep,
      originalPreviousMultiplierIndex,
      previewPreviousMultiplierIndex: originalPreviousMultiplierIndex,
      pitchDragRowHeightPx: rowHeightPx,
      lockedPitchRange: { ...visiblePitchRange },
      didDrag: false,
      multiPitchDrag: isMultiPitchDrag(note.stepId),
    };

    onStepBulkGestureStart(row, note.step);
  }

  /** @param {PointerEvent} event */
  function moveNoteDrag(event) {
    if (!drag || drag.mode !== "move" || event.pointerId !== drag.pointerId) return;

    if (drag.multiPitchDrag) {
      const nextMidi = midiFromPointerDelta(
        event,
        drag.baseMidi,
        drag.startY,
        drag.pitchDragRowHeightPx,
      );

      if (nextMidi === drag.previewMidi) return;

      drag = {
        ...drag,
        didDrag: true,
        previewMidi: nextMidi,
        lockedPitchRange: fittedPitchRangeForDragPreview(drag.step, nextMidi),
      };
      onNotePreview(row, drag.step, nextMidi);
      return;
    }

    const dragDeltaX = event.clientX - drag.startX;
    const nextMidi = midiFromPointerDelta(
      event,
      drag.baseMidi,
      drag.startY,
      drag.pitchDragRowHeightPx,
    );
    let targetStep = stepAtRollX(
      rollXFromPointer(event),
      drag.originalSlots,
      drag.dragPxPerQuarter,
    );
    let previewPreviousMultiplierIndex = drag.originalPreviousMultiplierIndex;

    if (dragDeltaX < 0 && drag.previewPreviousStep >= 0) {
      const precedingSlot = drag.originalSlots[drag.previewPreviousStep];
      const draggedSlot = drag.originalSlots[drag.step];
      const draggedStartXPx = Math.max(
        0,
        draggedSlot.startQuarters * drag.dragPxPerQuarter + dragDeltaX,
      );
      targetStep = stepAtRollX(
        draggedStartXPx,
        drag.originalSlots,
        drag.dragPxPerQuarter,
      );
      const distanceToPrecedingStartPx =
        (draggedSlot.startQuarters - precedingSlot.startQuarters) * drag.dragPxPerQuarter;
      const precedingResize = precedingStepResizeForNoteDrag(
        drag.originalPreviousMultiplierIndex,
        dragDeltaX,
        drag.dragPxPerQuarter,
        distanceToPrecedingStartPx,
        pulseIndex,
      );

      previewPreviousMultiplierIndex = precedingResize.multiplierIndex;

      if (!precedingResize.crossedPrecedingStart) {
        targetStep = drag.step;
      }
    } else if (dragDeltaX > 0 && drag.previewPreviousStep >= 0) {
      const draggedSlot = drag.originalSlots[drag.step];
      const draggedStartXPx =
        draggedSlot.startQuarters * drag.dragPxPerQuarter + dragDeltaX;
      targetStep = stepAtRollX(
        draggedStartXPx,
        drag.originalSlots,
        drag.dragPxPerQuarter,
      );
      const precedingExpansion = precedingStepExpansionForNoteDrag(
        drag.originalPreviousMultiplierIndex,
        dragDeltaX,
        drag.dragPxPerQuarter,
        pulseIndex,
      );

      previewPreviousMultiplierIndex = precedingExpansion.multiplierIndex;

      if (!precedingExpansion.exceededMaximum) {
        targetStep = drag.step;
      }
    }

    drag = {
      ...drag,
      didDrag: true,
      previewMidi: nextMidi,
      targetStep: targetStep < 0 ? drag.step : targetStep,
      previewPreviousMultiplierIndex,
      lockedPitchRange: fittedPitchRangeForDragPreview(drag.step, nextMidi),
    };
    onNotePreview(row, drag.step, nextMidi);
  }

  /** @param {PointerEvent} event */
  async function endNoteDrag(event) {
    if (!drag || drag.mode !== "move" || event.pointerId !== drag.pointerId) return;

    event.currentTarget.releasePointerCapture(event.pointerId);

    const finished = drag;
    drag = null;

    if (!finished.didDrag) return;

    await onNoteCommit(row, finished.step, finished.previewMidi);

    if (finished.multiPitchDrag) return;

    if (finished.targetStep !== finished.step) {
      await onStepMove(row, finished.step, finished.targetStep);
    } else if (
      finished.previewPreviousStep >= 0 &&
      finished.previewPreviousMultiplierIndex !== finished.originalPreviousMultiplierIndex
    ) {
      await onStepResize(
        row,
        finished.previewPreviousStep,
        finished.previewPreviousMultiplierIndex,
      );
    }
  }

  /** @param {PointerEvent} event @param {any} note @param {"start" | "end"} edge */
  function beginStepResize(event, note, edge) {
    if (isShapeDrawMode) return;

    if (event.shiftKey) {
      onBulkSelectPointerDown(event);
      return;
    }

    const resizedStep = edge === "start" ? note.step - 1 : note.step;

    if (resizedStep < 0) return;

    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    const initialMultiplierIndex =
      stepTimingMultiplier[resizedStep] ?? defaultStepTimingMultiplierIndex;
    drag = {
      mode: "resize",
      pointerId: event.pointerId,
      step: resizedStep,
      edge,
      startX: event.clientX,
      initialMultiplierIndex,
      previewMultiplierIndex: initialMultiplierIndex,
      resizePxPerQuarter: pxPerQuarter,
    };

    void onInspectStep(row, note.step, note.stepId);
  }

  /** @param {PointerEvent} event */
  function moveStepResize(event) {
    if (!drag || drag.mode !== "resize" || event.pointerId !== drag.pointerId) return;

    const previewMultiplierIndex = timingMultiplierIndexAfterRollResize(
      drag.initialMultiplierIndex,
      event.clientX - drag.startX,
      drag.resizePxPerQuarter,
      pulseIndex,
    );

    if (previewMultiplierIndex === drag.previewMultiplierIndex) return;

    drag = { ...drag, previewMultiplierIndex };
  }

  /** @param {PointerEvent} event */
  async function endStepResize(event) {
    if (!drag || drag.mode !== "resize" || event.pointerId !== drag.pointerId) return;

    event.currentTarget.releasePointerCapture(event.pointerId);

    const finished = drag;
    drag = null;

    if (finished.previewMultiplierIndex === finished.initialMultiplierIndex) return;

    await onStepResize(row, finished.step, finished.previewMultiplierIndex);
  }

  /** @param {PointerEvent} event */
  function cancelStepResize(event) {
    if (!drag || drag.mode !== "resize" || event.pointerId !== drag.pointerId) return;

    event.currentTarget.releasePointerCapture(event.pointerId);
    drag = null;
  }

  /** @param {KeyboardEvent} event @param {any} note @param {"start" | "end"} edge */
  function resizeStepWithKeyboard(event, note, edge) {
    if (isShapeDrawMode) return;

    if (event.key !== "ArrowLeft" && event.key !== "ArrowRight") return;

    event.preventDefault();
    event.stopPropagation();

    const resizedStep = edge === "start" ? note.step - 1 : note.step;

    if (resizedStep < 0) return;

    const initialMultiplierIndex =
      stepTimingMultiplier[resizedStep] ?? defaultStepTimingMultiplierIndex;
    const nextMultiplierIndex = Math.min(
      stepTimingMultiplierCount - 1,
      Math.max(
        0,
        initialMultiplierIndex + (event.key === "ArrowLeft" ? -1 : 1),
      ),
    );

    if (nextMultiplierIndex !== initialMultiplierIndex) {
      void onStepResize(row, resizedStep, nextMultiplierIndex);
    }
  }

  /** @param {MouseEvent} event @param {any} note */
  function openAdvancedInspector(event, note) {
    event.preventDefault();
    event.stopPropagation();
    drag = null;
    void onOpenAdvancedInspector(row, note.step, note.stepId);
  }

  /** @param {PointerEvent} event @param {any} note */
  function handleNoteOutlinePointerDown(event, note) {
    if (isShapeDrawMode) return;
    if (event.target !== event.currentTarget) return;

    if (event.shiftKey) {
      onBulkSelectPointerDown(event, { toggleStep: true });
      return;
    }

    void onInspectStep(row, note.step, note.stepId);
  }

  /** @param {PointerEvent} event */
  function shouldIgnoreRollInsertDoubleClick(event) {
    const target = event.target;

    if (!(target instanceof Element)) return true;

    return Boolean(
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [role='slider'], [data-bulk-step-cell], [data-piano-roll-boundary]",
      ),
    );
  }

  /** @param {PointerEvent} event @param {number} insertStep */
  function handleBoundaryDoubleClick(event, insertStep) {
    event.preventDefault();
    event.stopPropagation();

    const stationaryBoundaryResize =
      drag?.mode === "resize"
      && drag.previewMultiplierIndex === drag.initialMultiplierIndex;

    if (
      isShapeDrawMode
      || (drag && !stationaryBoundaryResize)
      || insertStep <= 0
      || insertStep > stepIds.length
      || stepIds.length >= maxPhraseStepsPerRow
    ) {
      return;
    }

    if (stationaryBoundaryResize) {
      const target = /** @type {HTMLElement} */ (event.currentTarget);

      if (target.hasPointerCapture(event.pointerId)) {
        target.releasePointerCapture(event.pointerId);
      }

      drag = null;
    }

    void onDuplicateStep(row, insertStep);
  }

  /** @param {PointerEvent} event */
  function handleRollBackgroundDoubleClick(event) {
    if (isShapeDrawMode || drag || stepIds.length >= maxPhraseStepsPerRow) return;

    const insertStep = insertStepIndexFromRollX(
      rollXFromPointer(event),
      timeline.slots,
      pxPerQuarter,
    );

    void onInsertStep(row, insertStep, defaultStepTimingMultiplierIndex);
  }

  let rollInsertDoubleClickOptions = $derived({
    disabled: isShapeDrawMode,
    shouldIgnore: shouldIgnoreRollInsertDoubleClick,
    onDoubleClick: handleRollBackgroundDoubleClick,
  });

  /** @param {number} insertStep */
  function boundaryDoubleClickOptions(insertStep) {
    return {
      disabled: isShapeDrawMode,
      onDoubleClick: (event) => handleBoundaryDoubleClick(event, insertStep),
    };
  }

</script>

<section class="step-inspector step-inspector--piano-roll flex min-h-0 w-full flex-1 overflow-hidden bg-app/90">
  <aside class="inspector-sidebar flex h-full min-h-0 w-[13rem] shrink-0 flex-col gap-2 pt-0 pr-3 pb-2 pl-0">
    <div class="inspector-bulk-controls flex min-h-0 w-full flex-1 flex-col">
      <BulkStepEditControls
        layout="sidebar"
        inspectorEmbedded
        className="min-h-0 flex-1"
      accent={rowAccent}
      requireSelection={false}
      totalStepCount={stepIds.length}
      selectedStepCount={selectedStepIds.length}
      reverseAvailable={bulkReverseAvailable}
      skipActive={bulkSkipActive}
      muteActive={bulkMuteActive}
      durationPercent={bulkDurationPercent}
      velocityPercent={bulkVelocityPercent}
      lengthDelta={bulkLengthDelta}
      transposeSemitones={bulkTransposeSemitones}
      pitchAriaLabel={bulkPitchAriaLabel}
      onReverse={onBulkReverse}
      onShuffle={onBulkShuffle}
      onRandomizeOctaves={onBulkRandomizeOctaves}
      onRandomizeLengths={onBulkRandomizeLengths}
      onToggleSkip={onBulkToggleSkip}
      onToggleMute={onBulkToggleMute}
      onGestureStart={onBulkGestureStart}
      onLengthPreview={onBulkLengthPreview}
      onLengthCommit={onBulkLengthCommit}
      onDurationPreview={onBulkDurationPreview}
      onDurationCommit={onBulkDurationCommit}
      onVelocityPreview={onBulkVelocityPreview}
      onVelocityCommit={onBulkVelocityCommit}
      onTransposePreview={onBulkTransposePreview}
      onTransposeCommit={onBulkTransposeCommit}
    />
    </div>
  </aside>

  <div class="inspector-main grid min-h-0 min-w-0 flex-1 grid-rows-[auto_minmax(0,1fr)] content-start gap-y-2 overflow-hidden pb-2">
    <div class="flex w-full items-center justify-center bg-surface/15 px-4 py-1.25">
      <div
        class="flex shrink-0 gap-1.5"
        role="radiogroup"
        aria-label="Piano roll interaction mode"
      >
      <button
        type="button"
        role="radio"
        data-cursor="pointer"
        aria-label="Edit notes on the piano roll"
        aria-checked={shapeDrawMode === "roll"}
        title="Move and resize steps on the piano roll"
        class={shapeModeButtonClasses("roll")}
        onclick={() => setShapeDrawMode("roll")}
      >
        <RowPianoRollModeIcon class="pointer-events-none h-4 w-4 shrink-0" />
        Roll
      </button>
      <button
        type="button"
        role="radio"
        data-cursor="pointer"
        aria-label="Draw velocity shape across steps"
        aria-checked={shapeDrawMode === "velocity"}
        title="Draw a freeform line to set step velocities"
        class={shapeModeButtonClasses("velocity")}
        onclick={() => setShapeDrawMode("velocity")}
      >
        <RowShapeDrawIcon class="pointer-events-none h-4 w-4 shrink-0" />
        Velocity
      </button>
      </div>
    </div>

  <div class="flex min-h-0 min-w-0 overflow-hidden">
  <div
    class="flex h-full min-h-0 min-w-0 flex-1 overflow-hidden rounded-xl border border-border-subtle bg-app/80"
    role="group"
    aria-label="Monophonic piano roll"
    onpointerdown={onBulkSelectPointerDown}
  >
    <div
      class="shrink-0 border-r border-border-subtle bg-surface/90"
      style:width="{keyboardWidthPx}px"
      style:padding-top="{rulerHeightPx}px"
    >
      <div class="relative overflow-visible" style:height="{rollHeightPx}px">
        {#each pitchRows as midi (midi)}
          <div
            class="absolute right-0 left-0 {pitchRowClass(midi)}"
            style:top="{pitchTopPx(midi)}px"
            style:height="{rowHeightPx}px"
          ></div>
        {/each}
        <PianoRollKeyboardLabels {pitchRows} {rowHeightPx} {pitchTopPx} />
      </div>
    </div>

    <div
      {@attach gridAttachment}
      class="min-w-0 flex-1 overflow-x-hidden overflow-y-hidden"
    >
      <div class="relative flex h-full flex-col" style:width="{rollWidthPx}px">
        <div
          class="relative border-b border-border-subtle bg-surface/95"
          style:height="{rulerHeightPx}px"
        >
          {#each measureLines as measureQuarter (measureQuarter)}
            <div
              class="absolute top-0 bottom-0 border-l border-piano-roll-bar-line"
              style:left="{quarterLeftPx(measureQuarter)}px"
            >
              <span class="absolute top-1 left-1 text-[9px] font-medium text-text-muted">
                {measureNumberForQuarter(measureQuarter)}
              </span>
            </div>
          {/each}
        </div>

        <div
          {@attach verticalScrollAttachment}
          class="h-0 min-h-0 flex-1 overflow-x-hidden overflow-y-auto overscroll-contain"
        >
          <div
            class="relative touch-none select-none"
            role="group"
            aria-label="Focused row piano roll"
            style:height="{rollHeightPx}px"
            use:doubleClick={rollInsertDoubleClickOptions}
            title="Double-click empty grid space to insert a step"
          >
            {#each pitchRows as midi (midi)}
              <div
                class="pointer-events-none absolute right-0 left-0 {pitchRowClass(midi)}"
                style:top="{pitchTopPx(midi)}px"
                style:height="{rowHeightPx}px"
              ></div>
            {/each}

            {#each beatLines as quarter (quarter)}
              {@const isMeasureLine = quarter % 4 === 0}
              <div
                class="pointer-events-none absolute top-0 bottom-0 border-l {isMeasureLine
                  ? 'border-piano-roll-bar-line'
                  : 'border-piano-roll-beat-line'}"
                style:left="{quarterLeftPx(quarter)}px"
                aria-hidden="true"
              ></div>
            {/each}
            <div
              class="pointer-events-none absolute top-0 right-0 bottom-0 border-r border-piano-roll-bar-line"
              aria-hidden="true"
            ></div>

            {#if shapeDrawMode === "velocity"}
              <div
                class="pointer-events-none absolute inset-0 z-25"
                aria-hidden="true"
              >
                <div
                  class="absolute right-0 left-0 border-t border-dashed border-text-muted/20"
                  style:top="{velocityYInRoll(64, rollHeightPx)}px"
                ></div>
                {#each stepNotes as note (note.stepId)}
                  {@const barHeightPx = velocityBarHeightPx(note.velocity)}
                  {@const velocityPreview =
                    shapeStroke?.mode === "velocity" && shapePreviewByStep?.has(note.step)}
                  <div
                    class="absolute bottom-0"
                    style:left="{note.leftPx}px"
                    style:width="{note.fullStepWidthPx}px"
                    style:height="{rollHeightPx}px"
                  >
                    <div
                      class="absolute right-0 bottom-0 left-0 rounded-t-sm border-2 border-x border-t {rowAccent.borderFocus} {velocityPreview
                        ? rowAccent.velocityBarFillPreview
                        : rowAccent.velocityBarFill}"
                      style:height="{barHeightPx}px"
                    >
                      <span
                        class="absolute top-0.5 left-1/2 -translate-x-1/2 text-[9px] font-semibold leading-none tabular-nums {rowAccent.textAccentStrong}"
                      >
                        {note.velocity}
                      </span>
                    </div>
                  </div>
                {/each}
              </div>
            {/if}

            <!-- Draggable step boundary vertical lines -->
            {#if !isShapeDrawMode}
            {#each stepNotes as note, index (note.stepId + '-boundary')}
              {#if index > 0}
                <div
                  data-piano-roll-boundary
                  role="presentation"
                  class="step-boundary-interactive-zone absolute top-0 bottom-0 z-25 flex items-center justify-center touch-none select-none cursor-ew-resize w-4 -ml-2"
                  style:left="{note.leftPx}px"
                  style:--boundary-accent={boundaryAccentVar}
                  use:doubleClick={boundaryDoubleClickOptions(note.step)}
                  onpointerdown={(event) => beginStepResize(event, note, "start")}
                  onpointermove={moveStepResize}
                  onpointerup={endStepResize}
                  onpointercancel={cancelStepResize}
                >
                  <div
                    class="step-boundary-line pointer-events-none h-full"
                  ></div>
                </div>
              {/if}
              {#if index === stepNotes.length - 1}
                <div
                  data-piano-roll-boundary
                  role="presentation"
                  class="step-boundary-interactive-zone absolute top-0 bottom-0 z-25 flex items-center justify-center touch-none select-none cursor-ew-resize w-4 -ml-2"
                  style:left="{note.leftPx + note.fullStepWidthPx}px"
                  style:--boundary-accent={boundaryAccentVar}
                  use:doubleClick={boundaryDoubleClickOptions(note.step + 1)}
                  onpointerdown={(event) => beginStepResize(event, note, "end")}
                  onpointermove={moveStepResize}
                  onpointerup={endStepResize}
                  onpointercancel={cancelStepResize}
                >
                  <div
                    class="step-boundary-line pointer-events-none h-full"
                  ></div>
                </div>
              {/if}
            {/each}
            {/if}

            {#each stepNotes as note (note.stepId)}
              {@const selected = selectedStepIdSet.has(note.stepId)}
              {@const inspected = inspectedStepId === note.stepId}
              {@const highlighted = selected || inspected}
              {@const playbackActive = note.active && !note.dimmed}
              {@const noteFillOpacity =
                highlighted || playbackActive ? 1 : durationBarFillOpacity(note.velocity)}
              {@const displayMidi = drag?.mode === "move" && drag.step === note.step ? drag.previewMidi : note.midi}
              {@const displayLabel = midiToNoteName(displayMidi)}
              <div
                data-bulk-step-cell
                data-step-row={row}
                data-step-id={note.stepId}
                class="absolute z-[26] transition-[opacity,box-shadow] duration-150 {isShapeDrawMode
                  ? 'pointer-events-none'
                  : ''} {playbackActive ? rowAccent.playbackGlow : ''}"
                role="group"
                aria-label={`Step ${note.step + 1}`}
                title="Click to select · Shift-click to add or remove from selection · Double-click for advanced settings"
                style:left="{note.leftPx}px"
                style:top="{pitchTopPx(displayMidi) + 1}px"
                style:width="{note.fullStepWidthPx}px"
                style:height="{Math.max(8, rowHeightPx - 2)}px"
                onpointerdown={(event) => handleNoteOutlinePointerDown(event, note)}
                ondblclick={(event) => {
                  if (isShapeDrawMode) return;
                  openAdvancedInspector(event, note);
                }}
              >
                <div
                  class="pointer-events-none absolute inset-0 rounded-sm border-2 {highlighted
                    ? rowAccent.borderFocus
                    : playbackActive
                      ? rowAccent.borderActive
                      : rowAccent.selectionBorder} {note.dimmed && !playbackActive && !highlighted
                    ? 'opacity-50'
                    : 'opacity-95'}"
                  aria-hidden="true"
                ></div>
                {#if note.skipped}
                  <StepSkippedOverlay active={true} />
                {/if}
                <button
                  type="button"
                  data-cursor="grab"
                  aria-label={`Move ${midiToNoteName(note.midi)} step ${note.step + 1}; click to select, Shift-click to add or remove from selection`}
                  aria-pressed={selected}
                  title="Click to select · Drag to move · Shift-click to add or remove from selection · Double-click for advanced settings"
                  class="absolute top-0 left-0 flex h-full items-center rounded-sm border px-1 text-[10px] font-semibold leading-none text-text-inverse tabular-nums outline-none transition-[border-color,box-shadow,opacity] {rowAccent.ringFocusWithWidth || 'focus-visible:ring-1 focus-visible:ring-focus-ring'} {playbackActive
                    ? rowAccent.pianoNoteActive
                    : highlighted
                      ? rowAccent.pianoNoteSelected
                      : rowAccent.pianoNoteIdle} {note.dimmed ? 'opacity-35' : ''}"
                  style:width="{note.durationWidthPx}px"
                  style:opacity={note.dimmed ? undefined : noteFillOpacity}
                  onpointerdown={(event) => {
                    if (isShapeDrawMode) return;
                    beginNoteDrag(event, note);
                  }}
                  onpointermove={moveNoteDrag}
                  onpointerup={endNoteDrag}
                  onpointercancel={endNoteDrag}
                >
                  <span class="pointer-events-none truncate">{displayLabel}</span>
                </button>
                {#if note.step > 0}
                  <CompactStepResizeHandle
                    edge="start"
                    disabled={isShapeDrawMode}
                    ariaLabel={`Resize start of step ${note.step + 1}`}
                    title="Drag to move this boundary by resizing the preceding step"
                    ringFocusClass={rowAccent.ringFocusWithWidth ||
                      "focus-visible:ring-1 focus-visible:ring-focus-ring"}
                    onPointerDown={(event) => beginStepResize(event, note, "start")}
                    onPointerMove={moveStepResize}
                    onPointerUp={endStepResize}
                    onPointerCancel={cancelStepResize}
                    onKeyDown={(event) => resizeStepWithKeyboard(event, note, "start")}
                  />
                {/if}
                <CompactStepResizeHandle
                    edge="end"
                    disabled={isShapeDrawMode}
                    ariaLabel={`Resize end of step ${note.step + 1}`}
                    title="Drag to resize this step"
                    ringFocusClass={rowAccent.ringFocusWithWidth ||
                      "focus-visible:ring-1 focus-visible:ring-focus-ring"}
                    onPointerDown={(event) => beginStepResize(event, note, "end")}
                    onPointerMove={moveStepResize}
                    onPointerUp={endStepResize}
                    onPointerCancel={cancelStepResize}
                    onKeyDown={(event) => resizeStepWithKeyboard(event, note, "end")}
                  />
              </div>
            {/each}

            {#if isShapeDrawMode}
              <div
                class="absolute inset-0 z-40 touch-none select-none"
                data-cursor="crosshair"
                aria-hidden="true"
                onpointerdown={beginShapeDraw}
                onpointermove={moveShapeDraw}
                onpointerup={endShapeDraw}
                onpointercancel={endShapeDraw}
              ></div>
            {/if}

            {#if drag?.mode === "move" && drag.didDrag}
              {@const draggedNote = stepNotes[drag.step]}
              {@const dragTooltipLabel = midiToNoteName(drag.previewMidi ?? draggedNote?.midi ?? 60)}
              {#if draggedNote && pianoRollNoteDragTooltipVisible(rowHeightPx, draggedNote.durationWidthPx, dragTooltipLabel)}
                <div
                  class="note-drag-tooltip pointer-events-none absolute z-50 rounded-md border border-border-strong bg-surface-raised px-2.5 py-1 text-base font-bold tabular-nums text-text shadow-lg"
                  style:left="{draggedNote.leftPx + draggedNote.durationWidthPx / 2}px"
                  style:top="{pitchTopPx(drag.previewMidi ?? draggedNote.midi) + 1}px"
                  aria-hidden="true"
                >
                  {dragTooltipLabel}
                </div>
              {/if}
            {/if}
          </div>
        </div>
      </div>
    </div>
  </div>
  </div>
  </div>
</section>

<style>
  .step-boundary-line {
    width: 0;
    border-left: 1px dotted color-mix(in srgb, var(--boundary-accent, var(--color-accent)) 35%, transparent);
    transition:
      border-color 100ms,
      box-shadow 100ms,
      opacity 100ms;
  }

  .step-boundary-interactive-zone:hover .step-boundary-line {
    border-left-style: solid;
    border-left-color: var(--boundary-accent, var(--color-accent));
    box-shadow: 0 0 4px color-mix(in srgb, var(--boundary-accent, var(--color-accent)) 45%, transparent);
  }

  .note-drag-tooltip {
    white-space: nowrap;
    transform: translate(-50%, calc(-100% - 8px));
    animation: note-tooltip-in 80ms ease-out;
  }

  @keyframes note-tooltip-in {
    from {
      opacity: 0;
      transform: translate(-50%, calc(-100% - 4px));
    }
    to {
      opacity: 1;
      transform: translate(-50%, calc(-100% - 8px));
    }
  }
</style>
