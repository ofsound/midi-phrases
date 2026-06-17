<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { defaultPulseIndex } from "./pulseLayout.js";
  import { isBlackKey } from "./phraseSchedule.js";
  import { durationFractionFromRailX } from "./rowPianoRollDuration.js";
  import {
    emeraldRowAccent,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";
  import {
    beatLineQuarters,
    measureLineQuarters,
    rollLengthQuartersForCycle,
    shapeNoteUpdatesFromStroke,
    shapeVelocityUpdatesFromStroke,
    stepAtRollX,
    stepSlotCenterXPx,
    velocityYInRoll,
  } from "./rowPianoRollShape.js";
  import {
    buildRowRollTimeline,
    precedingStepExpansionForNoteDrag,
    precedingStepResizeForNoteDrag,
    timingMultiplierIndexAfterRollResize,
  } from "./rowPianoRollTimeline.js";
  import RowShapeDrawIcon from "./RowShapeDrawIcon.svelte";
  import {
    defaultStepTimingMultiplierIndex,
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
   * @property {number} [rowTimingOffset]
   * @property {number} [pulseIndex]
   * @property {string | null} [inspectedStepId]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(row: number, step: number, stepId: string) => void | Promise<void>} [onInspectStep]
   * @property {(row: number, step: number, midi: number) => void} [onNotePreview]
   * @property {(row: number, step: number, midi: number) => void | Promise<void>} [onNoteCommit]
   * @property {(row: number, fromStep: number, toStep: number) => void | Promise<void>} [onStepMove]
   * @property {(row: number, step: number, multiplierIndex: number) => void | Promise<void>} [onStepResize]
   * @property {(row: number, step: number, fraction: number) => void | Promise<void>} [onDurationCommit]
   * @property {(row: number, updates: { step: number, midi: number }[]) => void | Promise<void>} [onShapeNotesCommit]
   * @property {(row: number, updates: { step: number, velocity: number }[]) => void | Promise<void>} [onShapeVelocitiesCommit]
   * @property {() => void} [onClose]
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
    rowTimingOffset = 3,
    pulseIndex = defaultPulseIndex,
    inspectedStepId = null,
    accent,
    onInspectStep = () => {},
    onNotePreview = () => {},
    onNoteCommit = () => {},
    onStepMove = () => {},
    onStepResize = () => {},
    onDurationCommit = () => {},
    onShapeNotesCommit = () => {},
    onShapeVelocitiesCommit = () => {},
    onClose = () => {},
  } = $props();

  const basePxPerQuarter = 28;
  const baseKeyboardWidthPx = 44;
  const baseRulerHeightPx = 28;
  const minimumVisibleSemitones = 12;

  /** @type {HTMLElement | null} */
  let gridElement = $state(null);
  /** @type {HTMLElement | null} */
  let verticalScrollElement = $state(null);
  let viewportHeightPx = $state(0);
  let gridViewportWidthPx = $state(0);
  let drag = $state(null);
  /** @type {'note' | 'velocity' | null} */
  let shapeDrawMode = $state(null);
  /** @type {{ pointerId: number, mode: 'note' | 'velocity', points: { x: number, y: number }[] } | null} */
  let shapeStroke = $state(null);
  let rowAccent = $derived(accent ?? emeraldRowAccent);
  let noteShapeDrawActive = $derived(shapeDrawMode === "note");
  let velocityShapeDrawActive = $derived(shapeDrawMode === "velocity");

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
    let minMidi = 60;
    let maxMidi = 72;

    if (notes.length > 0) {
      minMidi = Math.min(...notes);
      maxMidi = Math.max(...notes);
    }

    minMidi = Math.max(0, minMidi - 2);
    maxMidi = Math.min(127, maxMidi + 2);

    const span = maxMidi - minMidi + 1;
    if (span < minimumVisibleSemitones) {
      const extra = minimumVisibleSemitones - span;
      minMidi = Math.max(0, minMidi - Math.floor(extra / 2));
      maxMidi = Math.min(127, maxMidi + Math.ceil(extra / 2));
    }

    return { minMidi, maxMidi };
  });
  let pitchSpan = $derived(visiblePitchRange.maxMidi - visiblePitchRange.minMidi + 1);
  let rowHeightPx = $derived(
    viewportHeightPx > 0
      ? Math.min(scaledPx(22), Math.max(scaledPx(13), viewportHeightPx / pitchSpan))
      : scaledPx(16),
  );
  let rollHeightPx = $derived(pitchSpan * rowHeightPx);
  let pitchRows = $derived(Array.from({ length: pitchSpan }, (_, index) => visiblePitchRange.maxMidi - index));
  let shapePreviewByStep = $derived.by(() => {
    if (!shapeStroke || shapeStroke.points.length === 0) return null;

    if (shapeStroke.mode === "velocity") {
      const updates = shapeVelocityUpdatesFromStroke(
        shapeStroke.points,
        timeline.slots,
        pxPerQuarter,
        rollHeightPx,
      );

      return new Map(updates.map((update) => [update.step, update.velocity]));
    }

    const updates = shapeNoteUpdatesFromStroke(
      shapeStroke.points,
      timeline.slots,
      pxPerQuarter,
      rowHeightPx,
      visiblePitchRange.maxMidi,
    );
    return new Map(updates.map((update) => [update.step, update.midi]));
  });
  let stepNotes = $derived.by(() => {
    return notes.map((midi, step) => {
      const displayStep =
        drag?.mode === "move" && drag.step === step ? drag.targetStep : step;
      const slot = timeline.slots[displayStep] ?? timeline.slots[step];
      const durationFraction =
        drag?.mode === "duration" && drag.step === step
          ? drag.previewFraction
          : stepDurationFraction[step] ?? 1;
      const shapePreviewValue = shapePreviewByStep?.get(step);
      const shapePreviewMidi = shapeStroke?.mode === "note" ? shapePreviewValue : undefined;
      const shapePreviewVelocity = shapeStroke?.mode === "velocity" ? shapePreviewValue : undefined;
      const noteLengthQuarters = slot.lengthQuarters * durationFraction;
      const fullStepWidthPx = Math.max(1, slot.lengthQuarters * pxPerQuarter);

      return {
        step,
        stepId: stepIds[step],
        midi: shapePreviewMidi ?? midi,
        leftPx: slot.startQuarters * pxPerQuarter,
        fullStepWidthPx,
        durationWidthPx: Math.min(
          fullStepWidthPx,
          Math.max(scaledPx(12), noteLengthQuarters * pxPerQuarter - 2),
        ),
        velocity: shapePreviewVelocity ?? stepVelocity[step] ?? 100,
        muted: stepMuted[step] || stepSkipped[step],
      };
    });
  });
  let shapeStrokePolyline = $derived(
    shapeStroke?.points.map((point) => `${point.x},${point.y}`).join(" ") ?? "",
  );
  let shapePreviewDots = $derived.by(() => {
    if (!shapePreviewByStep) return [];

    return [...shapePreviewByStep.entries()].map(([step, value]) => ({
      step,
      value,
      x: stepSlotCenterXPx(timeline.slots[step], pxPerQuarter),
      y:
        shapeStroke?.mode === "velocity"
          ? velocityYInRoll(value, rollHeightPx)
          : pitchTopPx(value) + rowHeightPx / 2,
    }));
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

  /** @param {'note' | 'velocity'} mode */
  function toggleShapeDrawMode(mode) {
    shapeDrawMode = shapeDrawMode === mode ? null : mode;
    shapeStroke = null;
    drag = null;
  }

  /** @param {'note' | 'velocity'} mode */
  function shapeDrawButtonClasses(mode) {
    const active = shapeDrawMode === mode;

    return `flex h-8 items-center gap-1.5 rounded-md border px-2.5 text-xs font-medium transition-colors outline-none ${
      active
        ? `${rowAccent.borderActive} ${rowAccent.bgAccent}/15 ${rowAccent.textAccent}`
        : "border-border bg-surface text-text-muted hover:border-border-strong hover:text-text"
    } ${rowAccent.ringFocusWithWidth || "focus-visible:ring-1 focus-visible:ring-focus-ring"}`;
  }

  /** @param {PointerEvent} event */
  function beginShapeDraw(event) {
    if (!shapeDrawMode || shapeStroke || drag) return;

    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    const surface = /** @type {HTMLElement} */ (event.currentTarget);
    shapeStroke = {
      pointerId: event.pointerId,
      mode: shapeDrawMode,
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

    if (finished.mode === "velocity") {
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
      return;
    }

    const updates = shapeNoteUpdatesFromStroke(
      finished.points,
      timeline.slots,
      pxPerQuarter,
      rowHeightPx,
      visiblePitchRange.maxMidi,
    );

    if (updates.length === 0) return;

    const changedUpdates = updates.filter((update) => notes[update.step] !== update.midi);

    if (changedUpdates.length === 0) return;

    await onShapeNotesCommit(row, changedUpdates);
  }

  /** @param {PointerEvent} event @param {number} baseMidi @param {number} startY */
  function midiFromPointerDelta(event, baseMidi, startY) {
    const deltaRows = Math.round((startY - event.clientY) / rowHeightPx);

    return Math.min(127, Math.max(0, baseMidi + deltaRows));
  }

  /** @param {PointerEvent} event @param {any} note */
  function beginNoteDrag(event, note) {
    if (event.shiftKey) {
      beginDurationDrag(event, note);
      return;
    }

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
      didDrag: false,
    };

    void onInspectStep(row, note.step, note.stepId);
  }

  /** @param {PointerEvent} event */
  function moveNoteDrag(event) {
    if (!drag || drag.mode !== "move" || event.pointerId !== drag.pointerId) return;

    const dragDeltaX = event.clientX - drag.startX;
    const nextMidi = midiFromPointerDelta(event, drag.baseMidi, drag.startY);
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

  /** @param {PointerEvent} event */
  function moveNoteInteraction(event) {
    if (drag?.mode === "duration") {
      moveDurationDrag(event);
      return;
    }

    moveNoteDrag(event);
  }

  /** @param {PointerEvent} event */
  async function endNoteInteraction(event) {
    if (drag?.mode === "duration") {
      await endDurationDrag(event);
      return;
    }

    await endNoteDrag(event);
  }

  /** @param {PointerEvent} event */
  async function cancelNoteInteraction(event) {
    if (drag?.mode === "duration") {
      cancelDurationDrag(event);
      return;
    }

    await endNoteDrag(event);
  }

  /** @param {PointerEvent} event @param {any} note */
  function beginStepResize(event, note) {
    if (event.shiftKey) {
      event.preventDefault();
      event.stopPropagation();
      return;
    }

    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    const initialMultiplierIndex =
      stepTimingMultiplier[note.step] ?? defaultStepTimingMultiplierIndex;
    drag = {
      mode: "resize",
      pointerId: event.pointerId,
      step: note.step,
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

  /** @param {KeyboardEvent} event @param {any} note */
  function resizeStepWithKeyboard(event, note) {
    if (event.key !== "ArrowLeft" && event.key !== "ArrowRight") return;

    event.preventDefault();
    event.stopPropagation();

    const initialMultiplierIndex =
      stepTimingMultiplier[note.step] ?? defaultStepTimingMultiplierIndex;
    const nextMultiplierIndex = Math.min(
      stepTimingMultiplierCount - 1,
      Math.max(
        0,
        initialMultiplierIndex + (event.key === "ArrowLeft" ? -1 : 1),
      ),
    );

    if (nextMultiplierIndex !== initialMultiplierIndex) {
      void onStepResize(row, note.step, nextMultiplierIndex);
    }
  }

  /** @param {PointerEvent} event @param {any} note @param {HTMLElement | null} [rail] */
  function beginDurationDrag(
    event,
    note,
    rail = /** @type {HTMLElement} */ (event.currentTarget).parentElement,
  ) {
    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    if (!rail) return;

    const rect = rail.getBoundingClientRect();
    const initialFraction = stepDurationFraction[note.step] ?? 1;
    const previewFraction = durationFractionFromRailX(event.clientX, rect.left, rect.width);

    drag = {
      mode: "duration",
      pointerId: event.pointerId,
      step: note.step,
      railLeftPx: rect.left,
      railWidthPx: rect.width,
      initialFraction,
      previewFraction,
    };

    void onInspectStep(row, note.step, note.stepId);
  }

  /** @param {PointerEvent} event */
  function moveDurationDrag(event) {
    if (!drag || drag.mode !== "duration" || event.pointerId !== drag.pointerId) return;

    const previewFraction = durationFractionFromRailX(
      event.clientX,
      drag.railLeftPx,
      drag.railWidthPx,
    );

    if (Math.abs(previewFraction - drag.previewFraction) < 0.0001) return;

    drag = { ...drag, previewFraction };
  }

  /** @param {PointerEvent} event */
  async function endDurationDrag(event) {
    if (!drag || drag.mode !== "duration" || event.pointerId !== drag.pointerId) return;

    event.currentTarget.releasePointerCapture(event.pointerId);

    const finished = drag;
    drag = null;

    if (Math.abs(finished.previewFraction - finished.initialFraction) < 0.0001) return;

    await onDurationCommit(row, finished.step, finished.previewFraction);
  }

  /** @param {PointerEvent} event */
  function cancelDurationDrag(event) {
    if (!drag || drag.mode !== "duration" || event.pointerId !== drag.pointerId) return;

    event.currentTarget.releasePointerCapture(event.pointerId);
    drag = null;
  }

  /** @param {PointerEvent} event @param {any} note */
  function beginOutlineDurationDrag(event, note) {
    if (!event.shiftKey || event.target !== event.currentTarget) return;

    beginDurationDrag(event, note, /** @type {HTMLElement} */ (event.currentTarget));
  }

  /** @param {PointerEvent} event */
  function moveOutlineDurationDrag(event) {
    if (!event.currentTarget.hasPointerCapture(event.pointerId)) return;

    moveDurationDrag(event);
  }

  /** @param {PointerEvent} event */
  async function endOutlineDurationDrag(event) {
    if (!event.currentTarget.hasPointerCapture(event.pointerId)) return;

    await endDurationDrag(event);
  }

  /** @param {PointerEvent} event */
  function cancelOutlineDurationDrag(event) {
    if (!event.currentTarget.hasPointerCapture(event.pointerId)) return;

    cancelDurationDrag(event);
  }

</script>

<section class="flex min-h-0 w-full flex-1 flex-col gap-2 bg-app/90 px-6 py-4">
  <div class="flex shrink-0 items-center justify-between gap-3">
    <div class="flex min-w-0 items-baseline gap-3">
      <span class="text-xs font-semibold uppercase tracking-widest {rowAccent.textAccent}">
        Row {row + 1}
      </span>
      <span class="truncate text-xs text-text-faint">monophonic piano roll</span>
    </div>
    <div class="flex shrink-0 items-center gap-2">
      <button
        type="button"
        data-cursor="pointer"
        aria-label="Draw note shape across steps"
        aria-pressed={noteShapeDrawActive}
        title="Draw a freeform line to set step pitches"
        class={shapeDrawButtonClasses("note")}
        onclick={() => toggleShapeDrawMode("note")}
      >
        <RowShapeDrawIcon
          class="pointer-events-none h-4 w-4 {noteShapeDrawActive ? toggleIconActiveClasses : toggleIconRestClasses}"
        />
        Shape
      </button>
      <button
        type="button"
        data-cursor="pointer"
        aria-label="Draw velocity shape across steps"
        aria-pressed={velocityShapeDrawActive}
        title="Draw a freeform line to set step velocities"
        class={shapeDrawButtonClasses("velocity")}
        onclick={() => toggleShapeDrawMode("velocity")}
      >
        <RowShapeDrawIcon
          class="pointer-events-none h-4 w-4 {velocityShapeDrawActive ? toggleIconActiveClasses : toggleIconRestClasses}"
        />
        Velocity Shape
      </button>
      <button
        type="button"
        data-cursor="pointer"
        aria-label="Close row piano roll"
        class="flex h-8 w-8 shrink-0 items-center justify-center rounded-md border border-border bg-surface text-lg leading-none text-text-muted transition-colors outline-none hover:border-border-strong hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={onClose}
      >
        X
      </button>
    </div>
  </div>

  <div class="flex min-h-0 flex-1 overflow-hidden rounded-xl border border-border-subtle bg-app/80">
    <div
      class="shrink-0 border-r border-border-subtle bg-surface/90"
      style:width="{keyboardWidthPx}px"
      style:padding-top="{rulerHeightPx}px"
    >
      <div class="relative" style:height="{rollHeightPx}px">
        {#each pitchRows as midi (midi)}
          <div
            class="absolute right-0 left-0 flex items-center justify-end pr-1.5 {pitchRowClass(midi)}"
            style:top="{pitchTopPx(midi)}px"
            style:height="{rowHeightPx}px"
          >
            {#if midi % 12 === 0}
              <span class="text-[9px] font-medium text-text-muted">{midiToNoteName(midi)}</span>
            {/if}
          </div>
        {/each}
      </div>
    </div>

    <div
      {@attach gridAttachment}
      class="min-w-0 flex-1 overflow-x-auto overflow-y-hidden"
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
          class="h-0 min-h-0 flex-1 overflow-y-auto overscroll-contain"
        >
          <div
            class="relative touch-none select-none"
            role="group"
            aria-label="Focused row piano roll"
            style:height="{rollHeightPx}px"
            data-cursor={shapeDrawMode ? "crosshair" : undefined}
            onpointerdown={shapeDrawMode ? beginShapeDraw : undefined}
            onpointermove={shapeDrawMode ? moveShapeDraw : undefined}
            onpointerup={shapeDrawMode ? endShapeDraw : undefined}
            onpointercancel={shapeDrawMode ? endShapeDraw : undefined}
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
              class="pointer-events-none absolute top-0 bottom-0 border-r border-piano-roll-bar-line"
              style:left="{rollWidthPx}px"
              aria-hidden="true"
            ></div>

            {#if shapeStroke && shapeStrokePolyline}
              <svg
                class="pointer-events-none absolute inset-0 z-30 overflow-visible"
                width={rollWidthPx}
                height={rollHeightPx}
                aria-hidden="true"
              >
                <polyline
                  points={shapeStrokePolyline}
                  fill="none"
                  stroke="currentColor"
                  stroke-width="2"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  class="text-link-hover/90"
                />
                {#each shapePreviewDots as dot (dot.step)}
                  <circle
                    cx={dot.x}
                    cy={dot.y}
                    r="3.5"
                    class="fill-link-hover/90"
                  />
                {/each}
              </svg>
            {/if}

            {#each stepNotes as note (note.stepId)}
              {@const selected = inspectedStepId === note.stepId}
              {@const displayMidi = drag?.mode === "move" && drag.step === note.step ? drag.previewMidi : note.midi}
              {@const displayLabel = shapeDrawMode === "velocity" ? note.velocity : midiToNoteName(displayMidi)}
              <div
                class="absolute z-20 {shapeDrawMode ? 'pointer-events-none' : ''}"
                role="group"
                aria-label={`Step ${note.step + 1}`}
                style:left="{note.leftPx}px"
                style:top="{pitchTopPx(displayMidi) + 1}px"
                style:width="{note.fullStepWidthPx}px"
                style:height="{Math.max(8, rowHeightPx - 2)}px"
                onpointerdown={(event) => beginOutlineDurationDrag(event, note)}
                onpointermove={moveOutlineDurationDrag}
                onpointerup={endOutlineDurationDrag}
                onpointercancel={cancelOutlineDurationDrag}
              >
                <div
                  class="pointer-events-none absolute inset-0 rounded-sm border border-current {rowAccent.textAccent} {note.muted
                    ? 'opacity-25'
                    : selected
                      ? 'opacity-90'
                      : 'opacity-50'}"
                  aria-hidden="true"
                ></div>
                <button
                  type="button"
                  data-cursor="grab"
                  aria-label={`Move ${midiToNoteName(note.midi)} step ${note.step + 1}; Shift-drag to set duration`}
                  aria-pressed={selected}
                  title="Drag to move · Shift-click or Shift-drag to set duration"
                  class="absolute top-0 left-0 flex h-full items-center rounded-sm border px-1 text-[10px] font-semibold leading-none text-text-inverse tabular-nums outline-none transition-[border-color,box-shadow,opacity] {rowAccent.ringFocusWithWidth || 'focus-visible:ring-1 focus-visible:ring-focus-ring'} {selected
                    ? rowAccent.pianoNoteActive
                    : rowAccent.pianoNoteIdle} {note.muted ? 'opacity-35' : ''}"
                  style:width="{note.durationWidthPx}px"
                  onpointerdown={(event) => {
                    if (shapeDrawMode) return;
                    beginNoteDrag(event, note);
                  }}
                  onpointermove={moveNoteInteraction}
                  onpointerup={endNoteInteraction}
                  onpointercancel={cancelNoteInteraction}
                >
                  <span class="pointer-events-none truncate">{displayLabel}</span>
                </button>
                <button
                  type="button"
                  data-cursor="horizontal-drag"
                  aria-label={`Resize step ${note.step + 1}`}
                  title="Drag left or right to resize step"
                  class="absolute top-0 right-0 z-30 flex h-full w-2 touch-none items-center justify-end rounded-sm outline-none {rowAccent.ringFocusWithWidth || 'focus-visible:ring-1 focus-visible:ring-focus-ring'}"
                  onpointerdown={(event) => beginStepResize(event, note)}
                  onpointermove={moveStepResize}
                  onpointerup={endStepResize}
                  onpointercancel={cancelStepResize}
                  onkeydown={(event) => resizeStepWithKeyboard(event, note)}
                >
                  <span
                    class="pointer-events-none h-[70%] w-px rounded-full bg-current {rowAccent.textAccent} opacity-70"
                    aria-hidden="true"
                  ></span>
                </button>
              </div>
            {/each}
          </div>
        </div>
      </div>
    </div>
  </div>
</section>
