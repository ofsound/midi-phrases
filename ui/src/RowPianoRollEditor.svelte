<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { loopBraceSnapQuarters } from "./loopBraceLayout.js";
  import { defaultPulseIndex } from "./pulseLayout.js";
  import { isBlackKey, rowStepLayout, rowTimingOffsetQuarters } from "./phraseSchedule.js";
  import { timingMultiplierAtIndex } from "./stepCellLayout.js";
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
    onClose = () => {},
  } = $props();

  const basePxPerQuarter = 72;
  const baseKeyboardWidthPx = 54;
  const baseRulerHeightPx = 24;
  const minimumVisibleSemitones = 12;

  /** @type {HTMLElement | null} */
  let gridElement = $state(null);
  /** @type {HTMLElement | null} */
  let verticalScrollElement = $state(null);
  let viewportHeightPx = $state(0);
  let drag = $state(null);

  let pxPerQuarter = $derived(scaledPx(basePxPerQuarter));
  let keyboardWidthPx = $derived(scaledPx(baseKeyboardWidthPx));
  let rulerHeightPx = $derived(scaledPx(baseRulerHeightPx));
  let rowLayout = $derived(rowStepLayout(stepTimingMultiplier, pulseIndex, stepSkipped));
  let offsetQuarters = $derived(rowTimingOffsetQuarters(rowTimingOffset, pulseIndex));
  let leftPaddingQuarters = $derived(Math.max(0, -offsetQuarters));
  let rowLengthQuarters = $derived(Math.max(loopBraceSnapQuarters, rowLayout.cycleLengthQuarters));
  let rollWidthPx = $derived(Math.max(1, (rowLengthQuarters + Math.abs(offsetQuarters)) * pxPerQuarter));
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
  let stepNotes = $derived.by(() => {
    return notes.map((midi, step) => {
      const stepLength = rowLayout.stepLengthQuarters[step] ?? timingMultiplierAtIndex(stepTimingMultiplier[step]) ?? 1;
      const duration = Math.max(0.05, stepLength * Math.min(1, Math.max(0.05, stepDurationFraction[step] ?? 1)));
      const start = (rowLayout.stepStartQuarters[step] ?? 0) + offsetQuarters + leftPaddingQuarters;
      const previewFraction = drag?.mode === "resize" && drag.row === row && drag.step === step
        ? drag.previewFraction
        : null;
      const displayDuration = previewFraction === null
        ? duration
        : Math.max(0.05, stepLength * previewFraction);

      return {
        step,
        stepId: stepIds[step],
        midi,
        start,
        end: start + displayDuration,
        stepLength,
        velocity: stepVelocity[step] ?? 100,
        muted: stepMuted[step] || stepSkipped[step],
      };
    });
  });

  /** @param {HTMLElement} node */
  function gridAttachment(node) {
    gridElement = node;

    return () => {
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

  /** @param {number} beat */
  function beatLeftPx(beat) {
    return beat * pxPerQuarter;
  }

  /** @param {number} start @param {number} end */
  function noteWidthPx(start, end) {
    return Math.max(scaledPx(12), (end - start) * pxPerQuarter - 2);
  }

  /** @param {number} midi */
  function pitchRowClass(midi) {
    return isBlackKey(midi)
      ? "bg-app/85 border-b border-border-subtle/80"
      : "bg-surface-muted/70 border-b border-border/50";
  }

  /** @param {number} beat */
  function stepAtBeat(beat) {
    if (stepNotes.length === 0) return -1;

    let bestStep = 0;
    let bestDistance = Number.POSITIVE_INFINITY;

    for (const note of stepNotes) {
      const distance = Math.abs(note.start - beat);

      if (distance < bestDistance) {
        bestDistance = distance;
        bestStep = note.step;
      }
    }

    return bestStep;
  }

  /** @param {PointerEvent} event */
  function beatFromPointer(event) {
    if (!gridElement) return 0;

    const rect = gridElement.getBoundingClientRect();
    return Math.max(0, (event.clientX - rect.left + gridElement.scrollLeft) / pxPerQuarter);
  }

  /** @param {PointerEvent} event @param {number} baseMidi @param {number} startY */
  function midiFromPointerDelta(event, baseMidi, startY) {
    const deltaRows = Math.round((startY - event.clientY) / rowHeightPx);

    return Math.min(127, Math.max(0, baseMidi + deltaRows));
  }

  /** @param {number} value */
  function clampDurationFraction(value) {
    return Math.min(1, Math.max(0.05, value));
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
      startBeat: beatFromPointer(event),
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
      const targetStep = stepAtBeat(beatFromPointer(event));

      drag.previewMidi = nextMidi;
      drag.targetStep = targetStep < 0 ? drag.step : targetStep;
      onNotePreview(row, drag.step, nextMidi);
      return;
    }

    const beatDelta = beatFromPointer(event) - drag.startBeat;
    const stepLength = stepNotes[drag.step]?.stepLength ?? 1;
    drag.previewFraction = clampDurationFraction(drag.baseFraction + beatDelta / stepLength);
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

  /** @param {PointerEvent} event */
  function closeOnBackgroundPointerDown(event) {
    if (event.target !== event.currentTarget) return;

    onClose();
  }
</script>

<section class="flex min-h-0 w-full flex-1 flex-col gap-2 bg-app/90 px-6 py-4">
  <div class="flex shrink-0 items-center justify-between gap-3">
    <div class="flex min-w-0 items-baseline gap-3">
      <span class="text-xs font-semibold uppercase tracking-widest {accent?.textAccent ?? 'text-accent'}">
        Row {row + 1}
      </span>
      <span class="truncate text-xs text-text-faint">monophonic piano roll</span>
    </div>
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
              style:left="{beatLeftPx(note.start)}px"
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
            onpointerdown={closeOnBackgroundPointerDown}
          >
            {#each pitchRows as midi (midi)}
              <div
                class="pointer-events-none absolute right-0 left-0 {pitchRowClass(midi)}"
                style:top="{pitchTopPx(midi)}px"
                style:height="{rowHeightPx}px"
              ></div>
            {/each}

            {#each stepNotes as note (note.stepId)}
              {@const selected = inspectedStepId === note.stepId}
              {@const displayMidi = drag?.mode === "move" && drag.step === note.step ? drag.previewMidi : note.midi}
              <div
                class="absolute z-20"
                style:left="{beatLeftPx(note.start)}px"
                style:top="{pitchTopPx(displayMidi) + 1}px"
                style:width="{noteWidthPx(note.start, note.end)}px"
                style:height="{Math.max(8, rowHeightPx - 2)}px"
              >
                <button
                  type="button"
                  data-cursor="grab"
                  aria-label={`Move ${midiToNoteName(note.midi)} step ${note.step + 1}`}
                  aria-pressed={selected}
                  class="flex h-full w-full items-center rounded-sm border px-1 pr-3 text-[10px] font-semibold leading-none tabular-nums outline-none transition-[border-color,box-shadow,opacity] focus-visible:ring-1 {accent?.ringFocus ?? 'focus-visible:ring-focus-ring'} {selected
                    ? `${accent?.selectionBorder ?? 'border-accent'} ${accent?.selectionRing ?? 'ring-1 ring-accent'}`
                    : 'border-border-strong/70'} {note.muted ? 'opacity-35' : ''}"
                  style:background="color-mix(in srgb, var(--color-accent) 28%, var(--color-surface) 72%)"
                  onpointerdown={(event) => beginNoteDrag(event, note, "move")}
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
                  onpointerdown={(event) => beginNoteDrag(event, note, "resize")}
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
