<script>
  import { onDestroy, onMount } from "svelte";
  import { midiToNoteName } from "./midiNoteNames.js";
  import { clearActiveCursor, setActiveCursor } from "./cursor.js";
  import { beatFromClientX, clampLoopBrace, loopBraceSnapQuarters } from "./loopBraceLayout.js";
  import { defaultPulseIndex } from "./pulseLayout.js";
  import { applyNoteBandpass } from "./noteBandpass.js";
  import { noteBandpassPreview } from "./noteBandpassPreview.svelte.js";
  import { pagedPlaybackScrollLeft } from "./pianoRollAutoScroll.js";
  import { fittedPitchRangeForSchedule } from "./pianoRollViewport.js";
  import {
    buildPhraseScheduleBeforeBandpass,
    buildPhraseScheduleWindowBeforeBandpass,
    DEFAULT_PREVIEW_LENGTH_QUARTERS,
    isBlackKey,
    isScheduledNoteActiveAtBeat,
    isScheduledNoteActiveAtPatternBeat,
    mapPlaybackBeatForPianoRoll,
    patternRepeatLengthQuarters,
  } from "./phraseSchedule.js";
  import { scaledPx } from "./uiScale.svelte.js";
  import {
    pianoRollKeyboardBlackRowClass,
    pianoRollKeyboardWhiteRowClass,
  } from "./pianoKeyboardTheme.js";

  
  /**
   * @typedef {Object} Props
   * @property {any} [notes]
   * @property {any} [rowMuted]
   * @property {any} [rowTimingOffset]
   * @property {any} [stepDurationFraction]
   * @property {any} [stepTimingMultiplier]
   * @property {any} [stepVelocity]
   * @property {any} [stepMuted]
   * @property {any} [stepSkipped]
   * @property {any} [stepProbability]
   * @property {any} [stepCycle]
   * @property {any} [stepCycleOffset]
   * @property {any} [pulseIndex]
   * @property {number} [swingPercent]
   * @property {number} [swingSubdivisionIndex]
   * @property {number} [combinationModeMask]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {number} [noteBandpassLowMidi]
   * @property {number} [noteBandpassHighMidi]
   * @property {boolean} [octavizerDown8vaEnabled]
   * @property {boolean} [octavizerUp8vaEnabled]
   * @property {number} [octavizerDown8vaRelativeVelocity]
   * @property {number} [octavizerUp8vaRelativeVelocity]
   * @property {boolean} [shimmerEnabled]
   * @property {number} [shimmerDelayMultiplierIndex]
   * @property {number} [shimmerFeedbackPercent]
   * @property {number} [shimmerMixPercent]
   * @property {boolean} [rowColorsEnabled]
   * @property {any} [lengthQuarters]
   * @property {boolean} [loopEnabled]
   * @property {number} [loopStart]
   * @property {number} [loopEnd]
   * @property {any} [playbackBeat]
   * @property {(next: { enabled?: boolean, start?: number, end?: number }) => void | Promise<void>} [onLoopBraceChange]
   */

  /** @type {Props} */
  let {
    notes = [],
    rowMuted = [],
    rowTimingOffset = [],
    stepDurationFraction = [],
    stepTimingMultiplier = [],
    stepVelocity = [],
    stepMuted = [],
    stepSkipped = [],
    stepProbability = [],
    stepCycle = [],
    stepCycleOffset = [],
    pulseIndex = defaultPulseIndex,
    swingPercent = 0,
    swingSubdivisionIndex = 1,
    combinationModeMask = 0,
    scaleRoot = 0,
    scaleModeIndex = 0,
    noteBandpassLowMidi = 36,
    noteBandpassHighMidi = 108,
    octavizerDown8vaEnabled = false,
    octavizerUp8vaEnabled = false,
    octavizerDown8vaRelativeVelocity = 0,
    octavizerUp8vaRelativeVelocity = 0,
    shimmerEnabled = false,
    shimmerDelayMultiplierIndex = 3,
    shimmerFeedbackPercent = 70,
    shimmerMixPercent = 100,
    rowColorsEnabled = false,
    lengthQuarters = DEFAULT_PREVIEW_LENGTH_QUARTERS,
    loopEnabled = false,
    loopStart = 0,
    loopEnd = 8,
    playbackBeat = -1,
    onLoopBraceChange = () => {}
  } = $props();

  const basePxPerQuarter = 28;
  const baseFallbackRowHeightPx = 11;
  const baseMaxRowHeightPx = 16;
  const baseKeyboardWidthPx = 44;
  const baseRulerHeightPx = 28;
  const renderOverscanQuarters = 8;

  const notePalettes = [
    { fill: "--theme-note-row-1-fill", border: "--theme-note-row-1-border", activeFill: "--theme-note-row-1-active-fill", activeBorder: "--theme-note-row-1-active-border", glow: "--theme-note-row-1-glow" },
    { fill: "--theme-note-row-2-fill", border: "--theme-note-row-2-border", activeFill: "--theme-note-row-2-active-fill", activeBorder: "--theme-note-row-2-active-border", glow: "--theme-note-row-2-glow" },
    { fill: "--theme-note-row-3-fill", border: "--theme-note-row-3-border", activeFill: "--theme-note-row-3-active-fill", activeBorder: "--theme-note-row-3-active-border", glow: "--theme-note-row-3-glow" },
    { fill: "--theme-note-row-4-fill", border: "--theme-note-row-4-border", activeFill: "--theme-note-row-4-active-fill", activeBorder: "--theme-note-row-4-active-border", glow: "--theme-note-row-4-glow" },
  ];

  /** @type {HTMLElement | null} */
  let scrollElement = $state(null);
  /** @type {HTMLElement | null} */
  let gridScrollElement = $state(null);
  /** @type {HTMLElement | null} */
  let verticalScrollElement = $state(null);
  /** @type {HTMLCanvasElement | null} */
  let staticCanvas = $state(null);
  /** @type {HTMLCanvasElement | null} */
  let activeCanvas = $state(null);
  let syncingHorizontalScroll = false;
  let viewportWidthPx = $state(0);
  let viewportScrollLeftPx = $state(0);
  let verticalViewportHeightPx = $state(0);
  /** @type {"move" | "start" | "end" | null} */
  let dragMode = $state(null);
  let dragPointerId = -1;
  let dragStartBeat = 0;
  let dragAnchorStart = 0;
  let dragAnchorEnd = 0;
  let dragDisplayStart = $state(0);
  let dragDisplayEnd = $state(8);
  let themeRevision = $state(0);
  let displayStart = $derived(dragMode === null ? loopStart : dragDisplayStart);
  let displayEnd = $derived(dragMode === null ? loopEnd : dragDisplayEnd);
  let pxPerQuarter = $derived(scaledPx(basePxPerQuarter));
  let fallbackRowHeightPx = $derived(scaledPx(baseFallbackRowHeightPx));
  let maxRowHeightPx = $derived(scaledPx(baseMaxRowHeightPx));
  let keyboardWidthPx = $derived(scaledPx(baseKeyboardWidthPx));
  let rulerHeightPx = $derived(scaledPx(baseRulerHeightPx));

  let displayBandpassLow = $derived(noteBandpassPreview.low ?? noteBandpassLowMidi);
  let displayBandpassHigh = $derived(noteBandpassPreview.high ?? noteBandpassHighMidi);
  let visibleStartQuarter = $derived(viewportScrollLeftPx / pxPerQuarter);
  let visibleEndQuarter = $derived(
    viewportWidthPx > 0
      ? (viewportScrollLeftPx + viewportWidthPx) / pxPerQuarter
      : Math.min(lengthQuarters, 32),
  );
  let renderWindowStart = $derived(
    Math.max(0, Math.floor(visibleStartQuarter - renderOverscanQuarters)),
  );
  let renderWindowEnd = $derived(
    Math.min(lengthQuarters, Math.ceil(visibleEndQuarter + renderOverscanQuarters)),
  );
  let renderWindowLeftPx = $derived(renderWindowStart * pxPerQuarter);
  let renderWindowWidthPx = $derived(
    Math.max(1, (renderWindowEnd - renderWindowStart) * pxPerQuarter),
  );

  let scheduledBeforeBandpass = $derived(
    buildPhraseScheduleWindowBeforeBandpass({
      notes,
      rowMuted,
      rowTimingOffset,
      stepDurationFraction,
      stepTimingMultiplier,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      pulseIndex,
      swingPercent,
      swingSubdivisionIndex,
      combinationModeMask,
      lengthQuarters,
      scaleRoot,
      scaleModeIndex,
      octavizerDown8vaEnabled,
      octavizerUp8vaEnabled,
      octavizerDown8vaRelativeVelocity,
      octavizerUp8vaRelativeVelocity,
      shimmerEnabled,
      shimmerDelayMultiplierIndex,
      shimmerFeedbackPercent,
      shimmerMixPercent,
      windowStartQuarters: renderWindowStart,
      windowEndQuarters: renderWindowEnd,
    }),
  );

  let scheduled = $derived(
    applyNoteBandpass(scheduledBeforeBandpass, displayBandpassLow, displayBandpassHigh),
  );

  let fullScheduledBeforeBandpass = $derived(
    buildPhraseScheduleBeforeBandpass({
      notes,
      rowMuted,
      rowTimingOffset,
      stepDurationFraction,
      stepTimingMultiplier,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      pulseIndex,
      swingPercent,
      swingSubdivisionIndex,
      combinationModeMask,
      lengthQuarters,
      scaleRoot,
      scaleModeIndex,
      octavizerDown8vaEnabled,
      octavizerUp8vaEnabled,
      octavizerDown8vaRelativeVelocity,
      octavizerUp8vaRelativeVelocity,
      shimmerEnabled,
      shimmerDelayMultiplierIndex,
      shimmerFeedbackPercent,
      shimmerMixPercent,
    }),
  );

  let fullScheduled = $derived(
    applyNoteBandpass(fullScheduledBeforeBandpass, displayBandpassLow, displayBandpassHigh),
  );

  let pitchRange = $derived(fittedPitchRangeForSchedule(fullScheduled));
  let pitchSpan = $derived(pitchRange.maxMidi - pitchRange.minMidi + 1);
  let rowHeightPx = $derived(
    verticalViewportHeightPx > 0
      ? Math.min(maxRowHeightPx, verticalViewportHeightPx / pitchSpan)
      : fallbackRowHeightPx,
  );
  let rollWidthPx = $derived(lengthQuarters * pxPerQuarter);
  let rollHeightPx = $derived(pitchSpan * rowHeightPx);
  let noteVerticalInsetPx = $derived(Math.min(1, Math.max(0, (rowHeightPx - 1) / 2)));
  let noteHeightPx = $derived(Math.max(1, rowHeightPx - noteVerticalInsetPx * 2));
  let noteCornerRadiusPx = $derived(Math.min(2, noteHeightPx / 2));
  let showKeyboardLabels = $derived(rowHeightPx >= 8);
  let loopSpan = $derived(Math.max(loopBraceSnapQuarters, displayEnd - displayStart));
  let loopLeftPx = $derived(displayStart * pxPerQuarter);
  let loopWidthPx = $derived(loopSpan * pxPerQuarter);
  let patternRepeatLength = $derived(
    patternRepeatLengthQuarters({
      stepTimingMultiplier,
      rowMuted,
      stepSkipped,
      stepCycle,
      pulseIndex,
    }),
  );
  let displayPlaybackBeat = $derived(
    mapPlaybackBeatForPianoRoll(playbackBeat, {
      loopEnabled,
      patternRepeatLengthQuarters: patternRepeatLength,
      previewLengthQuarters: lengthQuarters,
    }),
  );
  let showPlaybackPlayhead = $derived(displayPlaybackBeat >= 0);
  let playbackPlayheadLeftPx = $derived(displayPlaybackBeat * pxPerQuarter);

  /** @param {HTMLElement} node */
  function scrollElementAttachment(node) {
    scrollElement = node;

    return () => {
      if (scrollElement === node) {
        scrollElement = null;
      }
    };
  }

  /** @param {HTMLElement} node */
  function gridScrollElementAttachment(node) {
    gridScrollElement = node;

    if (scrollElement) {
      node.scrollLeft = scrollElement.scrollLeft;
    }

    updateViewportMetrics(node);

    const resizeObserver = new ResizeObserver(() => {
      updateViewportMetrics(node);
    });
    resizeObserver.observe(node);

    return () => {
      resizeObserver.disconnect();

      if (gridScrollElement === node) {
        gridScrollElement = null;
      }
    };
  }

  /** @param {HTMLElement} node */
  function verticalScrollElementAttachment(node) {
    verticalScrollElement = node;
    updateVerticalViewportMetrics(node);

    const resizeObserver = new ResizeObserver(() => {
      updateVerticalViewportMetrics(node);
    });
    resizeObserver.observe(node);

    return () => {
      resizeObserver.disconnect();

      if (verticalScrollElement === node) {
        verticalScrollElement = null;
      }
    };
  }

  /** @param {Event} event */
  function syncGridHorizontalScroll(event) {
    if (syncingHorizontalScroll || !gridScrollElement || !scrollElement) return;

    syncingHorizontalScroll = true;
    gridScrollElement.scrollLeft = /** @type {HTMLElement} */ (event.currentTarget).scrollLeft;
    updateViewportMetrics(gridScrollElement);
    syncingHorizontalScroll = false;
  }

  /** @param {Event} event */
  function syncRulerHorizontalScroll(event) {
    if (syncingHorizontalScroll || !gridScrollElement || !scrollElement) return;

    syncingHorizontalScroll = true;
    const currentTarget = /** @type {HTMLElement} */ (event.currentTarget);
    scrollElement.scrollLeft = currentTarget.scrollLeft;
    updateViewportMetrics(currentTarget);
    syncingHorizontalScroll = false;
  }

  /** @param {number} midi */
  function pitchTopPx(midi) {
    return (pitchRange.maxMidi - midi) * rowHeightPx;
  }

  /** @param {number} start @param {number} end */
  function noteWidthPx(start, end) {
    return Math.max(1, (end - start) * pxPerQuarter - 1);
  }

  /** @param {number} beat */
  function formatBeat(beat) {
    return Number.isInteger(beat) ? String(beat) : beat.toFixed(1);
  }

  /** @param {number} midi */
  function keyboardRowClass(midi) {
    return isBlackKey(midi)
      ? pianoRollKeyboardBlackRowClass
      : pianoRollKeyboardWhiteRowClass;
  }

  /** @param {{ enabled?: boolean, start?: number, end?: number }} next */
  async function commitLoopBrace(next) {
    const clamped = clampLoopBrace(
      next.start ?? loopStart,
      next.end ?? loopEnd,
      lengthQuarters,
    );

    await onLoopBraceChange({
      enabled: next.enabled ?? loopEnabled,
      start: clamped.start,
      end: clamped.end,
    });
  }

  /** @param {PointerEvent} event @param {"move" | "start" | "end"} mode */
  function beginLoopDrag(event, mode) {
    if (!scrollElement) return;

    dragDisplayStart = loopStart;
    dragDisplayEnd = loopEnd;
    dragMode = mode;
    dragPointerId = event.pointerId;
    dragStartBeat = beatFromClientX(event.clientX, scrollElement, pxPerQuarter);
    dragAnchorStart = loopStart;
    dragAnchorEnd = loopEnd;

    setActiveCursor(mode === "move" ? "grabbing" : "ew-resize");
    event.currentTarget?.setPointerCapture?.(event.pointerId);
    event.preventDefault();
  }

  /** @param {PointerEvent} event */
  function moveLoopDrag(event) {
    if (dragMode === null || event.pointerId !== dragPointerId || !scrollElement) return;

    const beat = beatFromClientX(event.clientX, scrollElement, pxPerQuarter);

    if (dragMode === "move") {
      const delta = beat - dragStartBeat;

      if (delta === 0) return;

      const span = dragAnchorEnd - dragAnchorStart;
      let nextStart = dragAnchorStart + delta;
      let nextEnd = nextStart + span;

      if (nextStart < 0) {
        nextStart = 0;
        nextEnd = span;
      }

      if (nextEnd > lengthQuarters) {
        nextEnd = lengthQuarters;
        nextStart = nextEnd - span;
      }

      dragDisplayStart = nextStart;
      dragDisplayEnd = nextEnd;
      return;
    }

    if (dragMode === "start") {
      dragDisplayStart = Math.min(beat, displayEnd - loopBraceSnapQuarters);
      return;
    }

    dragDisplayEnd = Math.max(beat, displayStart + loopBraceSnapQuarters);
  }

  /** @param {PointerEvent} event */
  async function endLoopDrag(event) {
    if (dragMode === null || event.pointerId !== dragPointerId) return;

    event.currentTarget?.releasePointerCapture?.(event.pointerId);

    const mode = dragMode;
    const nextStart = displayStart;
    const nextEnd = displayEnd;
    dragMode = null;
    dragPointerId = -1;
    clearActiveCursor(mode === "move" ? "grabbing" : "ew-resize");

    await commitLoopBrace({ start: nextStart, end: nextEnd });

    if (mode === "move" && !loopEnabled) {
      await commitLoopBrace({ enabled: true, start: nextStart, end: nextEnd });
    }
  }

  async function toggleLoopEnabled() {
    await commitLoopBrace({ enabled: !loopEnabled });
  }

  function fitPitchRange() {
    if (verticalScrollElement) {
      verticalScrollElement.scrollTop = 0;
    }
  }

  /** @param {KeyboardEvent} event */
  async function handleLoopKeydown(event) {
    if (!loopEnabled) return;

    let delta = 0;

    if (event.key === "ArrowLeft") delta = -loopBraceSnapQuarters;
    if (event.key === "ArrowRight") delta = loopBraceSnapQuarters;

    if (delta === 0) return;

    event.preventDefault();

    const span = displayEnd - displayStart;
    let nextStart = displayStart + delta;
    let nextEnd = nextStart + span;

    if (nextStart < 0) {
      nextStart = 0;
      nextEnd = span;
    }

    if (nextEnd > lengthQuarters) {
      nextEnd = lengthQuarters;
      nextStart = nextEnd - span;
    }

    await commitLoopBrace({ start: nextStart, end: nextEnd });
  }

  let pitchRows = $derived(Array.from({ length: pitchSpan }, (_, index) => pitchRange.maxMidi - index));
  let barLines = $derived(Array.from({ length: Math.floor(lengthQuarters / 4) + 1 }, (_, bar) => bar * 4));

  /** @param {HTMLCanvasElement} node */
  function staticCanvasAttachment(node) {
    staticCanvas = node;

    return () => {
      if (staticCanvas === node) {
        staticCanvas = null;
      }
    };
  }

  /** @param {HTMLCanvasElement} node */
  function activeCanvasAttachment(node) {
    activeCanvas = node;

    return () => {
      if (activeCanvas === node) {
        activeCanvas = null;
      }
    };
  }

  /** @param {HTMLElement} node */
  function updateViewportMetrics(node) {
    viewportWidthPx = node.clientWidth;
    viewportScrollLeftPx = node.scrollLeft;
  }

  /** @param {HTMLElement} node */
  function updateVerticalViewportMetrics(node) {
    verticalViewportHeightPx = node.clientHeight;
  }

  /** @param {number} scrollLeft */
  function setHorizontalScrollLeft(scrollLeft) {
    if (!gridScrollElement) return;

    const nextScrollLeft = Math.min(
      Math.max(0, scrollLeft),
      Math.max(0, gridScrollElement.scrollWidth - gridScrollElement.clientWidth),
    );

    syncingHorizontalScroll = true;
    gridScrollElement.scrollLeft = nextScrollLeft;

    if (scrollElement) {
      scrollElement.scrollLeft = nextScrollLeft;
    }

    updateViewportMetrics(gridScrollElement);
    syncingHorizontalScroll = false;
  }

  /**
   * @param {HTMLCanvasElement} canvas
   * @param {number} widthPx
   * @param {number} heightPx
   */
  function prepareCanvas(canvas, widthPx, heightPx) {
    const dpr = window.devicePixelRatio || 1;
    const deviceWidth = Math.max(1, Math.round(widthPx * dpr));
    const deviceHeight = Math.max(1, Math.round(heightPx * dpr));

    if (canvas.width !== deviceWidth) canvas.width = deviceWidth;
    if (canvas.height !== deviceHeight) canvas.height = deviceHeight;

    const context = canvas.getContext("2d");

    if (!context) return null;

    context.setTransform(dpr, 0, 0, dpr, 0, 0);
    context.clearRect(0, 0, widthPx, heightPx);

    return context;
  }

  /** @param {CanvasRenderingContext2D} context */
  function roundedRect(context, x, y, width, height, radius) {
    const r = Math.max(0, Math.min(radius, width / 2, height / 2));

    context.beginPath();
    context.moveTo(x + r, y);
    context.lineTo(x + width - r, y);
    context.quadraticCurveTo(x + width, y, x + width, y + r);
    context.lineTo(x + width, y + height - r);
    context.quadraticCurveTo(x + width, y + height, x + width - r, y + height);
    context.lineTo(x + r, y + height);
    context.quadraticCurveTo(x, y + height, x, y + height - r);
    context.lineTo(x, y + r);
    context.quadraticCurveTo(x, y, x + r, y);
    context.closePath();
  }

  /** @param {number} row */
  function notePaletteForRow(row) {
    if (!rowColorsEnabled || row <= 0) return notePalettes[0];

    return notePalettes[row] ?? notePalettes[0];
  }

  /** @param {string} name */
  function themeColor(name) {
    return getComputedStyle(document.documentElement).getPropertyValue(name).trim() || "CanvasText";
  }

  /** @param {{ fill: string, border: string, activeFill: string, activeBorder: string, glow: string }} palette */
  function resolvedNotePalette(palette) {
    return {
      fill: themeColor(palette.fill),
      border: themeColor(palette.border),
      activeFill: themeColor(palette.activeFill),
      activeBorder: themeColor(palette.activeBorder),
      glow: themeColor(palette.glow),
    };
  }

  function drawStaticCanvas() {
    themeRevision;

    if (!staticCanvas) return;

    const context = prepareCanvas(staticCanvas, renderWindowWidthPx, rollHeightPx);

    if (!context) return;

    const windowLeftPx = renderWindowStart * pxPerQuarter;

    context.fillStyle = themeColor("--theme-piano-roll-bg");
    context.fillRect(0, 0, renderWindowWidthPx, rollHeightPx);

    for (const midi of pitchRows) {
      context.fillStyle = isBlackKey(midi)
        ? themeColor("--theme-piano-roll-black-key")
        : themeColor("--theme-piano-roll-white-key");
      context.fillRect(0, pitchTopPx(midi), renderWindowWidthPx, rowHeightPx);
      context.strokeStyle = isBlackKey(midi)
        ? themeColor("--theme-piano-roll-black-key-line")
        : themeColor("--theme-piano-roll-white-key-line");
      context.lineWidth = 1;
      context.beginPath();
      context.moveTo(0, pitchTopPx(midi) + rowHeightPx - 0.5);
      context.lineTo(renderWindowWidthPx, pitchTopPx(midi) + rowHeightPx - 0.5);
      context.stroke();
    }

    if (loopEnabled) {
      const left = displayStart * pxPerQuarter - windowLeftPx;
      const width = loopSpan * pxPerQuarter;

      if (left < renderWindowWidthPx && left + width > 0) {
        context.fillStyle = themeColor("--theme-piano-roll-loop-fill");
        context.strokeStyle = themeColor("--theme-piano-roll-loop-border");
        context.lineWidth = 1;
        context.fillRect(left, 0, width, rollHeightPx);
        context.strokeRect(left + 0.5, 0.5, Math.max(0, width - 1), Math.max(0, rollHeightPx - 1));
      }
    }

    const firstQuarter = Math.ceil(renderWindowStart);
    const lastQuarter = Math.floor(renderWindowEnd);

    for (let quarter = firstQuarter; quarter <= lastQuarter; quarter += 1) {
      const x = quarter * pxPerQuarter - windowLeftPx + 0.5;

      context.strokeStyle = quarter % 4 === 0
        ? themeColor("--theme-piano-roll-bar-line")
        : themeColor("--theme-piano-roll-beat-line");
      context.lineWidth = 1;
      context.beginPath();
      context.moveTo(x, 0);
      context.lineTo(x, rollHeightPx);
      context.stroke();
    }

    for (const note of scheduled) {
      if (note.velocity <= 0) continue;

      const palette = resolvedNotePalette(notePaletteForRow(note.row));
      const opacity = Math.max(0.2, note.velocity / 127);
      const x = note.start * pxPerQuarter - windowLeftPx;
      const y = pitchTopPx(note.midi) + noteVerticalInsetPx;
      const width = noteWidthPx(note.start, note.end);
      const height = noteHeightPx;

      context.globalAlpha = opacity;
      context.fillStyle = palette.fill;
      roundedRect(context, x, y, width, height, noteCornerRadiusPx);
      context.fill();
      context.globalAlpha = Math.min(1, opacity + 0.15);
      context.strokeStyle = palette.border;
      context.lineWidth = 1;
      context.stroke();
      context.globalAlpha = 1;
    }
  }

  function drawActiveCanvas() {
    themeRevision;

    if (!activeCanvas) return;

    const context = prepareCanvas(activeCanvas, renderWindowWidthPx, rollHeightPx);

    if (!context || !showPlaybackPlayhead) return;

    const windowLeftPx = renderWindowStart * pxPerQuarter;

    for (const note of scheduled) {
      const noteIsActive = loopEnabled
        ? isScheduledNoteActiveAtBeat(note, displayPlaybackBeat)
        : isScheduledNoteActiveAtPatternBeat(note, displayPlaybackBeat, patternRepeatLength);

      if (note.velocity <= 0 || !noteIsActive) continue;

      const palette = resolvedNotePalette(notePaletteForRow(note.row));
      const x = note.start * pxPerQuarter - windowLeftPx;
      const y = pitchTopPx(note.midi) + noteVerticalInsetPx;
      const width = noteWidthPx(note.start, note.end);
      const height = noteHeightPx;

      context.shadowColor = palette.glow;
      context.shadowBlur = 8;
      context.fillStyle = palette.activeFill;
      roundedRect(context, x, y, width, height, noteCornerRadiusPx);
      context.fill();
      context.shadowBlur = 0;
      context.strokeStyle = palette.activeBorder;
      context.lineWidth = 1;
      context.stroke();
    }
  }

  $effect(() => {
    drawStaticCanvas();
  });

  $effect(() => {
    drawActiveCanvas();
  });

  $effect(() => {
    pitchRange;
    fitPitchRange();
  });

  $effect(() => {
    if (!gridScrollElement || viewportWidthPx <= 0 || dragMode !== null) return;

    const viewportStart = gridScrollElement.scrollLeft / pxPerQuarter;
    const viewportEnd = (gridScrollElement.scrollLeft + viewportWidthPx) / pxPerQuarter;

    if (displayStart >= viewportStart && displayStart <= viewportEnd) return;

    setHorizontalScrollLeft((displayStart - 2) * pxPerQuarter);
  });

  $effect(() => {
    if (!gridScrollElement || viewportWidthPx <= 0 || !showPlaybackPlayhead || dragMode !== null) {
      return;
    }

    const nextScrollLeft = pagedPlaybackScrollLeft(
      playbackPlayheadLeftPx,
      gridScrollElement.scrollLeft,
      viewportWidthPx,
      gridScrollElement.scrollWidth - gridScrollElement.clientWidth,
    );

    if (nextScrollLeft === gridScrollElement.scrollLeft) return;

    setHorizontalScrollLeft(nextScrollLeft);
  });

  onMount(() => {
    const observer = new MutationObserver((mutations) => {
      if (mutations.some((mutation) => mutation.attributeName === "data-theme")) {
        themeRevision += 1;
      }
    });

    observer.observe(document.documentElement, {
      attributes: true,
      attributeFilter: ["data-theme"],
    });

    return () => {
      observer.disconnect();
    };
  });

  onDestroy(() => {
    clearActiveCursor();
  });
</script>

<section class="flex min-h-0 w-full flex-1 flex-col">
  <div class="mb-2 flex shrink-0 items-baseline justify-between gap-3">
    <div class="flex items-center gap-3">
      <button
        type="button"
        data-cursor="pointer"
        class="rounded-md border px-2 py-0.5 text-[10px] font-medium uppercase tracking-wide transition-colors outline-none focus-visible:ring-1 focus-visible:ring-focus-ring {loopEnabled
          ? 'border-accent-strong/50 bg-accent-strong/15 text-accent'
          : 'border-border bg-surface text-text-muted hover:border-border-strong hover:text-text-secondary'}"
        onclick={toggleLoopEnabled}
      >
        Loop {loopEnabled ? "on" : "off"}
      </button>
      <button
        type="button"
        data-cursor="pointer"
        title="Fit notes"
        class="rounded-md border border-border bg-surface px-2 py-0.5 text-[10px] font-medium uppercase tracking-wide text-text-muted transition-colors outline-none hover:border-border-strong hover:text-text-secondary focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={fitPitchRange}
      >
        Fit
      </button>
    </div>
    <p class="text-xs text-text-faint">
      loop {formatBeat(displayStart)}–{formatBeat(displayEnd)} ({formatBeat(loopSpan)} beats)
    </p>
  </div>

  <div
    class="flex h-0 min-h-0 flex-1 flex-col overflow-hidden rounded-xl border border-border-subtle bg-app/80"
  >
    <div class="flex shrink-0">
      <div
        class="shrink-0 border-r border-b border-border-subtle bg-surface/90"
        style:width="{keyboardWidthPx}px"
        style:height="{rulerHeightPx}px"
      ></div>

      <div
        {@attach scrollElementAttachment}
        class="min-w-0 flex-1 overflow-x-auto overflow-y-hidden border-b border-border-subtle"
        onscroll={syncGridHorizontalScroll}
      >
        <div class="relative bg-surface/95" style:width="{rollWidthPx}px" style:height="{rulerHeightPx}px">
          {#each barLines as bar (bar)}
            <div
              class="pointer-events-none absolute top-0 bottom-0 border-l border-border/80"
              style:left="{bar * pxPerQuarter}px"
            >
              <span class="absolute top-1 left-1 text-[9px] font-medium text-text-muted">{bar}</span>
            </div>
          {/each}

          <!-- svelte-ignore a11y_no_noninteractive_tabindex -->
          <!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
          <div
            tabindex="0"
            role="group"
            aria-label="Loop brace"
            class="absolute inset-y-1 z-20 touch-none select-none outline-none"
            style:left="{loopLeftPx}px"
            style:width="{loopWidthPx}px"
            onkeydown={handleLoopKeydown}
            onpointermove={moveLoopDrag}
            onpointerup={endLoopDrag}
            onpointercancel={endLoopDrag}
          >
            <div
              class="absolute inset-x-0 top-0 h-2 rounded-sm border transition-colors {loopEnabled
                ? 'border-accent/80 bg-accent/90'
                : 'border-border-strong/70 bg-surface-subtle/60'}"
            >
              <button
                type="button"
                aria-label="Loop start"
                data-cursor="ew-resize"
                class="absolute top-1/2 left-0 z-30 h-4 w-2.5 -translate-x-1/2 -translate-y-1/2 rounded-sm border border-border-subtle/40 bg-text-secondary shadow-sm"
                onpointerdown={(event) => beginLoopDrag(event, "start")}
              ></button>

              <button
                type="button"
                aria-label="Move loop brace"
                data-cursor="grab"
                class="absolute inset-x-2 inset-y-0"
                onpointerdown={(event) => beginLoopDrag(event, "move")}
                ondblclick={toggleLoopEnabled}
              ></button>

              <button
                type="button"
                aria-label="Loop end"
                data-cursor="ew-resize"
                class="absolute top-1/2 right-0 z-30 h-4 w-2.5 translate-x-1/2 -translate-y-1/2 rounded-sm border border-border-subtle/40 bg-text-secondary shadow-sm"
                onpointerdown={(event) => beginLoopDrag(event, "end")}
              ></button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div
      {@attach verticalScrollElementAttachment}
      class="h-0 min-h-0 flex-1 overflow-y-auto overscroll-contain"
    >
      <div class="flex">
        <div
          class="shrink-0 border-r border-border-subtle bg-surface/90"
          style:width="{keyboardWidthPx}px"
        >
          <div class="relative" style:height="{rollHeightPx}px">
            {#each pitchRows as midi (midi)}
              <div
                class="absolute right-0 left-0 flex items-center justify-end pr-1.5 {keyboardRowClass(midi)}"
                style:top="{pitchTopPx(midi)}px"
                style:height="{rowHeightPx}px"
              >
                {#if showKeyboardLabels && midi % 12 === 0}
                  <span class="text-[9px] font-medium text-text-muted">{midiToNoteName(midi)}</span>
                {/if}
              </div>
            {/each}
          </div>
        </div>

        <div
          {@attach gridScrollElementAttachment}
          class="min-w-0 flex-1 overflow-x-auto overflow-y-hidden"
          onscroll={syncRulerHorizontalScroll}
        >
          <div class="relative" style:width="{rollWidthPx}px">
            <div class="relative" style:height="{rollHeightPx}px">
              <canvas
                {@attach staticCanvasAttachment}
                class="pointer-events-none absolute top-0"
                aria-hidden="true"
                style:left="{renderWindowLeftPx}px"
                style:width="{renderWindowWidthPx}px"
                style:height="{rollHeightPx}px"
              ></canvas>

              <canvas
                {@attach activeCanvasAttachment}
                class="pointer-events-none absolute top-0 z-20"
                aria-hidden="true"
                style:left="{renderWindowLeftPx}px"
                style:width="{renderWindowWidthPx}px"
                style:height="{rollHeightPx}px"
              ></canvas>

              {#if showPlaybackPlayhead}
                <div
                  class="pointer-events-none absolute top-0 bottom-0 z-40 w-px bg-text/90 shadow-[0_0_6px_color-mix(in_srgb,var(--color-text)_35%,transparent)]"
                  style:left="{playbackPlayheadLeftPx}px"
                ></div>
              {/if}
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</section>
