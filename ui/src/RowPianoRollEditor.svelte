<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { defaultPulseIndex } from "./pulseLayout.js";
  import { isBlackKey } from "./phraseSchedule.js";
  import {
    emeraldRowAccent,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";
  import { shapeNoteUpdatesFromStroke } from "./rowPianoRollShape.js";
  import RowShapeDrawIcon from "./RowShapeDrawIcon.svelte";
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
   * @property {(row: number, step: number, fraction: number) => void | Promise<void>} [onDurationChange]
   * @property {(row: number, updates: { step: number, midi: number }[]) => void | Promise<void>} [onShapeNotesCommit]
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
    onDurationChange = () => {},
    onShapeNotesCommit = () => {},
    onClose = () => {},
  } = $props();

  const basePxPerStepColumn = 72;
  const baseKeyboardWidthPx = 54;
  const baseRulerHeightPx = 24;
  const minimumVisibleSemitones = 12;
  const durationSnapFractions = [0, 0.25, 0.5, 0.75, 1];
  const durationSnapStep = 0.25;

  /** @type {HTMLElement | null} */
  let gridElement = $state(null);
  /** @type {HTMLElement | null} */
  let verticalScrollElement = $state(null);
  let viewportHeightPx = $state(0);
  let gridViewportWidthPx = $state(0);
  let drag = $state(null);
  let shapeDrawMode = $state(false);
  /** @type {{ pointerId: number, points: { x: number, y: number }[] } | null} */
  let shapeStroke = $state(null);
  let rowAccent = $derived(accent ?? emeraldRowAccent);

  // Equal-width step columns: bar length is duration fraction (0.25–1), not musical time.
  let stepCount = $derived(Math.max(1, notes.length));
  let pxPerStep = $derived.by(() => {
    const floor = scaledPx(basePxPerStepColumn);

    if (gridViewportWidthPx <= 0 || stepCount <= 0) return floor;

    return Math.max(floor, gridViewportWidthPx / stepCount);
  });
  let rollWidthPx = $derived(Math.max(1, stepCount * pxPerStep));
  let keyboardWidthPx = $derived(scaledPx(baseKeyboardWidthPx));
  let rulerHeightPx = $derived(scaledPx(baseRulerHeightPx));
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

    const updates = shapeNoteUpdatesFromStroke(
      shapeStroke.points,
      stepCount,
      pxPerStep,
      rowHeightPx,
      visiblePitchRange.maxMidi,
    );

    return new Map(updates.map((update) => [update.step, update.midi]));
  });
  let stepNotes = $derived.by(() => {
    return notes.map((midi, step) => {
      const displayStep =
        drag?.mode === "move" && drag.step === step ? drag.targetStep : step;
      const start = displayStep;
      const previewFraction =
        drag?.mode === "resize" && drag.step === step ? drag.previewFraction : null;
      const durationFraction =
        previewFraction ?? stepDurationFraction[step] ?? 1;
      const shapePreviewMidi = shapePreviewByStep?.get(step);

      return {
        step,
        stepId: stepIds[step],
        midi: shapePreviewMidi ?? midi,
        start,
        end: start + durationFraction,
        velocity: stepVelocity[step] ?? 100,
        muted: stepMuted[step] || stepSkipped[step],
      };
    });
  });
  let shapeStrokePolyline = $derived(
    shapeStroke?.points.map((point) => `${point.x},${point.y}`).join(" ") ?? "",
  );
  let shapePreviewDots = $derived.by(() => {
    if (!shapePreviewByStep) return [];

    return [...shapePreviewByStep.entries()].map(([step, midi]) => ({
      step,
      midi,
      x: (step + 0.5) * pxPerStep,
      y: pitchTopPx(midi) + rowHeightPx / 2,
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

  /** @param {number} stepPosition */
  function stepLeftPx(stepPosition) {
    return stepPosition * pxPerStep;
  }

  /** @param {number} start @param {number} end */
  function noteWidthPx(start, end) {
    return Math.max(scaledPx(12), (end - start) * pxPerStep - 2);
  }

  /** @param {number} midi */
  function pitchRowClass(midi) {
    return isBlackKey(midi)
      ? "bg-app/85 border-b border-border-subtle/80"
      : "bg-surface-muted/70 border-b border-border/50";
  }

  /** @param {number} position */
  function snapToDurationGrid(position) {
    return Math.round(position / durationSnapStep) * durationSnapStep;
  }

  /** @param {number} position */
  function stepAtDisplayPosition(position) {
    if (stepCount <= 0) return -1;

    const snapped = Math.floor(snapToDurationGrid(position));
    return Math.min(stepCount - 1, Math.max(0, snapped));
  }

  /** @param {PointerEvent} event */
  function stepPositionFromPointer(event) {
    if (!gridElement) return 0;

    const rect = gridElement.getBoundingClientRect();
    return Math.max(0, (event.clientX - rect.left + gridElement.scrollLeft) / pxPerStep);
  }

  /** @param {PointerEvent} event @param {HTMLElement} surface */
  function rollPointFromPointer(event, surface) {
    const rect = surface.getBoundingClientRect();

    return {
      x: Math.max(0, Math.min(rollWidthPx, event.clientX - rect.left)),
      y: Math.max(0, Math.min(rollHeightPx, event.clientY - rect.top)),
    };
  }

  function toggleShapeDrawMode() {
    shapeDrawMode = !shapeDrawMode;
    shapeStroke = null;
    drag = null;
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

    const updates = shapeNoteUpdatesFromStroke(
      finished.points,
      stepCount,
      pxPerStep,
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

  /** @param {number} value */
  function clampDurationFraction(value) {
    return Math.min(1, Math.max(0, value));
  }

  /** @param {number} fraction */
  function snapDurationFraction(fraction) {
    return durationSnapFractions.reduce((closest, snapValue) =>
      Math.abs(snapValue - fraction) < Math.abs(closest - fraction) ? snapValue : closest,
    );
  }

  /** @param {number} endPosition @param {number} startPosition */
  function durationFractionFromSnappedEnd(endPosition, startPosition) {
    const minEnd = startPosition + durationSnapStep;
    const snappedEnd = Math.max(minEnd, snapToDurationGrid(endPosition));
    const rawFraction = snappedEnd - startPosition;

    return snapDurationFraction(clampDurationFraction(rawFraction));
  }

  /** @param {PointerEvent} event @param {any} note @param {"move" | "resize"} mode */
  function beginNoteDrag(event, note, mode) {
    event.preventDefault();
    event.stopPropagation();
    event.currentTarget.setPointerCapture(event.pointerId);

    drag = {
      mode,
      pointerId: event.pointerId,
      row,
      step: note.step,
      startY: event.clientY,
      startPosition: stepPositionFromPointer(event),
      baseStartPosition: note.start,
      baseMidi: note.midi,
      baseFraction: stepDurationFraction[note.step] ?? 1,
      previewMidi: note.midi,
      previewFraction: stepDurationFraction[note.step] ?? 1,
      targetStep: note.step,
      didDrag: false,
    };

    void onInspectStep(row, note.step, note.stepId);
  }

  /** @param {PointerEvent} event */
  function moveNoteDrag(event) {
    if (!drag || event.pointerId !== drag.pointerId) return;

    drag.didDrag = true;

    if (drag.mode === "move") {
      const nextMidi = midiFromPointerDelta(event, drag.baseMidi, drag.startY);
      const targetStep = stepAtDisplayPosition(stepPositionFromPointer(event));

      drag.previewMidi = nextMidi;
      drag.targetStep = targetStep < 0 ? drag.step : targetStep;
      onNotePreview(row, drag.step, nextMidi);
      return;
    }

    drag.previewFraction = durationFractionFromSnappedEnd(
      stepPositionFromPointer(event),
      drag.baseStartPosition,
    );
  }

  /** @param {PointerEvent} event */
  async function endNoteDrag(event) {
    if (!drag || event.pointerId !== drag.pointerId) return;

    event.currentTarget.releasePointerCapture(event.pointerId);

    const finished = drag;
    drag = null;

    if (!finished.didDrag) return;

    if (finished.mode === "move") {
      await onNoteCommit(row, finished.step, finished.previewMidi);

      if (finished.targetStep !== finished.step) {
        await onStepMove(row, finished.step, finished.targetStep);
      }

      return;
    }

    await onDurationChange(row, finished.step, finished.previewFraction);
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
        aria-pressed={shapeDrawMode}
        title="Draw a freeform line to set step pitches"
        class="flex h-8 items-center gap-1.5 rounded-md border px-2.5 text-xs font-medium transition-colors outline-none {shapeDrawMode
          ? `${rowAccent.borderActive} ${rowAccent.bgAccent}/15 ${rowAccent.textAccent}`
          : 'border-border bg-surface text-text-muted hover:border-border-strong hover:text-text'} {rowAccent.ringFocusWithWidth || 'focus-visible:ring-1 focus-visible:ring-focus-ring'}"
        onclick={toggleShapeDrawMode}
      >
        <RowShapeDrawIcon
          class="pointer-events-none h-4 w-4 {shapeDrawMode ? toggleIconActiveClasses : toggleIconRestClasses}"
        />
        Shape
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

  <div class="flex min-h-0 flex-1 overflow-hidden rounded-lg border border-border-subtle bg-app/80">
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
          {#each stepNotes as note (note.stepId)}
            <div
              class="absolute top-0 bottom-0 border-l border-border/80"
              style:left="{stepLeftPx(note.start)}px"
            >
              <span class="absolute top-1 left-1 text-[9px] font-medium text-text-muted">{note.step + 1}</span>
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

            {#each stepIds as stepId, stepIndex (stepId)}
              <div
                class="pointer-events-none absolute top-0 bottom-0 border-l border-border/50"
                style:left="{stepLeftPx(stepIndex)}px"
                aria-hidden="true"
              ></div>
            {/each}
            <div
              class="pointer-events-none absolute top-0 bottom-0 border-r border-border/50"
              style:left="{stepLeftPx(stepCount)}px"
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
              <div
                class="absolute z-20 {shapeDrawMode ? 'pointer-events-none' : ''}"
                style:left="{stepLeftPx(note.start)}px"
                style:top="{pitchTopPx(displayMidi) + 1}px"
                style:width="{noteWidthPx(note.start, note.end)}px"
                style:height="{Math.max(8, rowHeightPx - 2)}px"
              >
                <button
                  type="button"
                  data-cursor="grab"
                  aria-label={`Move ${midiToNoteName(note.midi)} step ${note.step + 1}`}
                  aria-pressed={selected}
                  class="flex h-full w-full items-center rounded-sm border px-1 pr-3 text-[10px] font-semibold leading-none text-text-inverse tabular-nums outline-none transition-[border-color,box-shadow,opacity] {rowAccent.ringFocusWithWidth || 'focus-visible:ring-1 focus-visible:ring-focus-ring'} {selected
                    ? rowAccent.pianoNoteActive
                    : rowAccent.pianoNoteIdle} {note.muted ? 'opacity-35' : ''}"
                  onpointerdown={(event) => {
                    if (shapeDrawMode) return;
                    beginNoteDrag(event, note, "move");
                  }}
                  onpointermove={moveNoteDrag}
                  onpointerup={endNoteDrag}
                  onpointercancel={endNoteDrag}
                >
                  <span class="pointer-events-none truncate">{midiToNoteName(displayMidi)}</span>
                </button>
                <button
                  type="button"
                  data-cursor="ew-resize"
                  aria-label={`Resize step ${note.step + 1}`}
                  class="absolute top-0 right-0 bottom-0 w-2 rounded-r-sm border-0 border-l border-border/70 bg-text/15 p-0 outline-none focus-visible:ring-1 focus-visible:ring-focus-ring"
                  onpointerdown={(event) => {
                    if (shapeDrawMode) return;
                    beginNoteDrag(event, note, "resize");
                  }}
                  onpointermove={moveNoteDrag}
                  onpointerup={endNoteDrag}
                  onpointercancel={endNoteDrag}
                ></button>
              </div>
            {/each}
          </div>
        </div>
      </div>
    </div>
  </div>
</section>
