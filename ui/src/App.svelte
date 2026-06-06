<script>
  import { onMount } from "svelte";
  import { SvelteMap, SvelteSet } from "svelte/reactivity";
  import { getNativeFunction } from "@juce/index.js";
  import {
    defaultPhraseGrid,
    defaultRowTimingOffsetIndex,
    defaultStepDurationGrid,
    defaultStepTimingMultiplierGrid,
    defaultStepMutedGrid,
    defaultStepSkippedGrid,
    defaultStepProbabilityGrid,
    defaultStepCycleGrid,
    defaultStepCycleOffsetGrid,
    defaultStepVelocityGrid,
    defaultStepNote,
    defaultStepDurationFraction,
    defaultStepVelocity,
  } from "./midiNoteNames.js";
  import RowDisableIcon from "./RowDisableIcon.svelte";
  import RowRandomizeOctaveIcon from "./RowRandomizeOctaveIcon.svelte";
  import RowRandomizeOrderIcon from "./RowRandomizeOrderIcon.svelte";
  import RowRecordIcon from "./RowRecordIcon.svelte";
  import RowReverseOrderIcon from "./RowReverseOrderIcon.svelte";
  import StepGearIcon from "./StepGearIcon.svelte";
  import BipolarKnob from "./BipolarKnob.svelte";
  import PhraseRow from "./PhraseRow.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import PianoRollPreview from "./PianoRollPreview.svelte";
  import RecordPianoKeyboard from "./RecordPianoKeyboard.svelte";
  import {
    defaultStepTimingMultiplierIndex,
    maxPhraseStepsPerRow,
    findSingleMove,
    stepTimingMultiplierCount,
    timingMultiplierOptions,
  } from "./stepCellLayout.js";
  import { sanitizeOrderedIds } from "./dndUtils.js";
  import {
    isStepActiveAtBeat,
    swingSubdivisionOptions,
    swingSubdivisionValues,
  } from "./phraseSchedule.js";
  import DiscreteDragSelect from "./DiscreteDragSelect.svelte";
  import PulseNoteButtonGroup from "./PulseNoteButtonGroup.svelte";
  import ColorsToggle from "./ColorsToggle.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import { defaultPulseIndex, pulseOptions } from "./pulseLayout.js";
  import {
    phraseBeatGuideGlobalLeftPx,
    phraseGridVisualOffsetCompensationPx,
  } from "./phraseRowLayout.js";
  import {
    emeraldRowAccent,
    rowAccentFor,
    rowMutedOverlayClasses,
    rowMuteControlClasses,
    rowPowerToggleOffClasses,
    rowActionIconControlClasses,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";

  let pluginName = $state("MIDI Phrases");
  let grid = $state(defaultPhraseGrid());
  /** @type {boolean[]} */
  let rowMuted = $state([false, false, false, false]);
  let soloRow = -1;
  /** @type {boolean[] | null} */
  let rowSoloRestoreMuted = null;
  /** @type {number[]} */
  let rowTimingOffset = $state([
    defaultRowTimingOffsetIndex,
    defaultRowTimingOffsetIndex,
    defaultRowTimingOffsetIndex,
    defaultRowTimingOffsetIndex,
  ]);
  /** @type {number[]} */
  let rowMidiChannel = $state([1, 2, 3, 4]);
  /** @type {number[][]} */
  let stepDurationFraction = $state(defaultStepDurationGrid());
  /** @type {number[][]} */
  let stepTimingMultiplier = $state(defaultStepTimingMultiplierGrid());
  /** @type {number[][]} */
  let stepVelocity = $state(defaultStepVelocityGrid());
  /** @type {boolean[][]} */
  let stepMuted = $state(defaultStepMutedGrid());
  /** @type {boolean[][]} */
  let stepSkipped = $state(defaultStepSkippedGrid());
  /** @type {number[][]} */
  let stepProbability = $state(defaultStepProbabilityGrid());
  /** @type {number[][]} */
  let stepCycle = $state(defaultStepCycleGrid());
  /** @type {number[][]} */
  let stepCycleOffset = $state(defaultStepCycleOffsetGrid());
  /** @type {boolean[][]} */
  let activeGates = $state(defaultPhraseGrid().map((row) => row.map(() => false)));
  /** @type {string[][]} */
  let stepIds = $state(defaultPhraseGrid().map((row, rowIndex) =>
    row.map((_, step) => `step-${rowIndex}-${step}`),
  ));

  let playbackPollFrameId = 0;
  let slotSelectionInFlight = 0;
  let previousGateSnapshot = defaultPhraseGrid().map((row) => row.map(() => false));
  let activeGateHoldUntil = defaultPhraseGrid().map((row) => row.map(() => 0));

  /** Row index armed for MIDI capture, or null. */
  let recordingRow = $state(null);
  /** Snapshot taken when recording was armed (for undo / cancel). */
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let recordingHistoryBefore = null;
  /** Whether any note was captured in the current recording session. */
  let recordingCapturedNotes = false;
  /** Clears the row on the next captured note. */
  let recordingAwaitingFirstNote = false;
  /** @type {Set<number>} */
  let recordingKeysHeld = $state(new Set());

  const timingOffsetOptions = [
    { index: 0, label: "-.75" },
    { index: 1, label: "-.5" },
    { index: 2, label: "-.25" },
    { index: 3, label: "0" },
    { index: 4, label: ".25" },
    { index: 5, label: ".5" },
    { index: 6, label: ".75" },
  ];


  let nextStepId = defaultPhraseGrid().reduce((count, row) => count + row.length, 0);
  let loopBraceEnabled = $state(false);
  let loopBraceStart = $state(0);
  let loopBraceEnd = $state(8);
  let playbackBeat = $state(-1);
  let standaloneTransportAvailable = $state(false);
  let standalonePlaying = $state(false);
  let standaloneTempoBpm = $state(120);
  let activePatternSlot = $state(0);
  let viewPatternSlot = 0;
  let patternCopySource = $state(-1);
  let activeLoopSlot = $state(-1);
  let loopSlotAssigned = $state(Array.from({ length: 8 }, () => false));
  let loopSlotPattern = Array.from({ length: 8 }, () => 0);
  let pulseIndex = $state(defaultPulseIndex);
  let swingPercent = $state(0);
  let velocityHumanizePercent = $state(0);
  let timingHumanizePercent = $state(0);
  let swingSubdivisionIndex = $state(1);
  let rowColorsEnabled = $state(false);
  let undoStack = $state([]);
  let redoStack = $state([]);
  const selectedStepKeys = new SvelteSet();
  /** @type {string[][]} */
  let selectableStepKeySet = $derived(new Set(allSelectableStepKeys()));
  let selectedStepKeysForGrid = $derived(
    new Set([...selectedStepKeys].filter((key) => selectableStepKeySet.has(key))),
  );
  let selectedStepIdsByRow = $derived(selectedStepIdsByRowForKeys(selectedStepKeysForGrid));
  let globalStepBackView = $state(false);
  let bulkDurationPercent = $state(100);
  let bulkVelocityPercent = $state(100);
  let bulkTransposeSemitones = $state(0);
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let bulkEditGestureBefore = null;
  /** @type {Map<string, number> | null} */
  let bulkTransposeBaselineByKey = null;
  /** @type {{ startX: number, startY: number, currentX: number, currentY: number, addToSelection: boolean, baseKeys: Set<string> } | null} */
  let marqueeSelection = $state(null);

  const historyLimit = 100;
  const stepTriggerFlashMs = 110;
  const historyButtonBaseClasses =
    "flex h-8 w-8 items-center justify-center rounded-md border bg-zinc-900 transition-colors outline-none focus:ring-1 focus:ring-emerald-400 disabled:border-zinc-800 disabled:text-zinc-700";

  function historyButtonClasses(enabled) {
    return `${historyButtonBaseClasses} ${
      enabled
        ? "border-zinc-700 text-zinc-300 hover:border-zinc-600 hover:text-zinc-100"
        : "border-zinc-800 text-zinc-700"
    }`;
  }

  function slotButtonClasses(active, assigned = true, copySource = false) {
    return `flex h-7 w-7 items-center justify-center rounded-sm border text-xs font-semibold transition-colors outline-none focus:ring-1 focus:ring-emerald-400 ${
      active
        ? "border-emerald-400 bg-emerald-400 text-zinc-950"
        : assigned
          ? "border-zinc-700 bg-zinc-900 text-zinc-200 hover:border-zinc-500"
          : "border-zinc-800 bg-zinc-950 text-zinc-600 hover:border-zinc-700 hover:text-zinc-400"
    } ${copySource ? "ring-1 ring-amber-300" : ""}`;
  }

  function clearPatternButtonClasses(enabled) {
    return `flex h-7 w-7 items-center justify-center border-0 bg-transparent transition-colors outline-none focus:ring-1 focus:ring-emerald-400 ${
      enabled ? "text-red-400 hover:text-red-300" : "text-zinc-700"
    }`;
  }

  function outputMuteButtonClasses(active) {
    return `row-span-2 flex h-[calc(1.75rem*1.33)] w-[calc(1.75rem*1.33)] items-center justify-center self-center rounded-sm border text-sm font-semibold transition-colors outline-none focus:ring-1 focus:ring-emerald-400 ${
      active
        ? "border-emerald-400 bg-emerald-400 text-zinc-950"
        : "border-zinc-700 bg-zinc-900 text-zinc-200 hover:border-zinc-500"
    }`;
  }

  function bulkSelectButtonClasses(active, enabled = true) {
    return `flex h-8 min-w-12 items-center justify-center rounded-md border px-3 text-sm font-semibold transition-colors outline-none focus:ring-1 focus:ring-emerald-400 ${
      !enabled
        ? "border-zinc-800 bg-zinc-950 text-zinc-700"
        : active
          ? "border-emerald-400 bg-emerald-400 text-zinc-950"
          : "border-zinc-700 bg-zinc-900 text-zinc-200 hover:border-zinc-500"
    }`;
  }

  function bulkActionIconButtonClasses(enabled = true) {
    return `flex h-8 w-8 items-center justify-center rounded-md border transition-colors outline-none focus:ring-1 focus:ring-emerald-400 ${
      enabled
        ? "border-zinc-700 bg-zinc-900 text-zinc-400 hover:border-zinc-500 hover:text-zinc-100"
        : "border-zinc-800 bg-zinc-950 text-zinc-700"
    }`;
  }

  function brandIconToggleButtonClasses(active, enabled = true) {
    return `flex h-5 w-5 shrink-0 items-center justify-center border-0 bg-transparent p-0 transition-colors outline-none focus:ring-1 focus:ring-emerald-400 ${
      !enabled
        ? "text-zinc-700"
        : active
          ? "text-white"
          : "text-zinc-500 hover:text-zinc-300"
    }`;
  }

  /** UI-only; shifts phrase rows and beat-one guide when any row has a negative offset. */
  let phraseVisualOffsetCompensationPx = $derived(phraseGridVisualOffsetCompensationPx(
    rowTimingOffset,
    pulseIndex,
  ));

  let selectedStepCount = $derived(selectedStepKeysForGrid.size);
  let selectableStepCount = $derived(stepIds.reduce((count, rowStepIds) => count + rowStepIds.length, 0));
  let allStepsSelected = $derived(selectedStepCount === selectableStepCount && selectedStepCount > 0);
  let selectedStepReverseAvailable = $derived.by(() => {
    const selectedByRow = new SvelteMap();

    for (const { row } of selectedStepLocations()) {
      selectedByRow.set(row, (selectedByRow.get(row) ?? 0) + 1);
    }

    return [...selectedByRow.values()].some((count) => count > 1);
  });
  let marqueeLeft = $derived(marqueeSelection
    ? Math.min(marqueeSelection.startX, marqueeSelection.currentX)
    : 0);
  let marqueeTop = $derived(marqueeSelection
    ? Math.min(marqueeSelection.startY, marqueeSelection.currentY)
    : 0);
  let marqueeWidth = $derived(marqueeSelection
    ? Math.abs(marqueeSelection.currentX - marqueeSelection.startX)
    : 0);
  let marqueeHeight = $derived(marqueeSelection
    ? Math.abs(marqueeSelection.currentY - marqueeSelection.startY)
    : 0);
  let marqueeRectStyle = $derived(marqueeSelection
    ? `left: ${marqueeLeft}px; top: ${marqueeTop}px; width: ${marqueeWidth}px; height: ${marqueeHeight}px;`
    : "");

  function createStepId() {
    const id = `step-${nextStepId}`;
    nextStepId += 1;
    return id;
  }

  function cloneMatrix(matrix) {
    return matrix.map((row) => [...row]);
  }

  /** @param {number} row @param {string} stepId */
  function stepSelectionKey(row, stepId) {
    return `${row}:${stepId}`;
  }

  /** @param {string} key */
  function rowFromStepSelectionKey(key) {
    const row = Number.parseInt(key.split(":")[0] ?? "-1", 10);

    return Number.isNaN(row) ? -1 : row;
  }

  function allSelectableStepKeys() {
    const keys = [];

    for (let row = 0; row < stepIds.length; row += 1) {
      for (const stepId of stepIds[row]) {
        keys.push(stepSelectionKey(row, stepId));
      }
    }

    return keys;
  }

  /** @param {Set<string>} keys */
  function selectedStepIdsByRowForKeys(keys) {
    return stepIds.map((rowStepIds, row) =>
      rowStepIds.filter((stepId) => keys.has(stepSelectionKey(row, stepId))),
    );
  }

  /** @param {Set<string>} next */
  function setSelectedStepKeys(next) {
    selectedStepKeys.clear();

    for (const key of next) {
      if (selectableStepKeySet.has(key)) {
        selectedStepKeys.add(key);
      }
    }
  }

  function selectedStepLocations() {
    const locations = [];

    for (const key of selectedStepKeysForGrid) {
      const row = rowFromStepSelectionKey(key);
      const stepId = key.substring(key.indexOf(":") + 1);
      const step = stepIds[row]?.indexOf(stepId) ?? -1;

      if (row >= 0 && step >= 0) {
        locations.push({ row, step, key });
      }
    }

    return locations;
  }

  function selectedStepLocationsByPosition() {
    return selectedStepLocations().sort((left, right) =>
      left.row === right.row ? left.step - right.step : left.row - right.row,
    );
  }

  function selectedStepLocationsGroupedByRow() {
    const groups = new SvelteMap();

    for (const location of selectedStepLocationsByPosition()) {
      const rowLocations = groups.get(location.row) ?? [];
      rowLocations.push(location);
      groups.set(location.row, rowLocations);
    }

    return groups;
  }

  function selectAllStepsForBulkEdit() {
    setSelectedStepKeys(new Set(allSelectableStepKeys()));
    syncBulkControlsFromSelection();
  }

  function toggleGlobalStepBackView() {
    if (selectableStepCount === 0) return;

    globalStepBackView = !globalStepBackView;
  }

  function syncBulkControlsFromSelection() {
    const [first] = selectedStepLocations();

    bulkTransposeSemitones = 0;

    if (!first) {
      return;
    }

    bulkDurationPercent = Math.round(
      Math.min(1, Math.max(0, stepDurationFraction[first.row][first.step] ?? 1)) * 100,
    );
    bulkVelocityPercent = Math.round(
      (Math.min(127, Math.max(0, stepVelocity[first.row][first.step] ?? defaultStepVelocity)) / 127) * 100,
    );
  }

  function marqueeRect() {
    if (!marqueeSelection) {
      return { left: 0, top: 0, right: 0, bottom: 0 };
    }

    const left = Math.min(marqueeSelection.startX, marqueeSelection.currentX);
    const right = Math.max(marqueeSelection.startX, marqueeSelection.currentX);
    const top = Math.min(marqueeSelection.startY, marqueeSelection.currentY);
    const bottom = Math.max(marqueeSelection.startY, marqueeSelection.currentY);

    return { left, top, right, bottom };
  }

  /** @param {DOMRect} left @param {{ left: number, top: number, right: number, bottom: number }} right */
  function rectsIntersect(left, right) {
    return left.left <= right.right &&
      left.right >= right.left &&
      left.top <= right.bottom &&
      left.bottom >= right.top;
  }

  function updateMarqueeSelectionFromPointer() {
    if (!marqueeSelection) return;

    const selectionRect = marqueeRect();
    const hitKeys = new SvelteSet();

    for (const element of document.querySelectorAll("[data-bulk-step-cell]")) {
      if (!(element instanceof HTMLElement)) continue;

      const row = Number.parseInt(element.dataset.stepRow ?? "-1", 10);
      const stepId = element.dataset.stepId;

      if (Number.isNaN(row) || !stepId) continue;
      if (!rectsIntersect(element.getBoundingClientRect(), selectionRect)) continue;

      hitKeys.add(stepSelectionKey(row, stepId));
    }

    const next = marqueeSelection.addToSelection
      ? new SvelteSet(marqueeSelection.baseKeys)
      : new SvelteSet();

    for (const key of hitKeys) {
      next.add(key);
    }

    setSelectedStepKeys(next);
  }

  /** @param {PointerEvent} event */
  function updateMarqueePointer(event) {
    if (!marqueeSelection) return;

    marqueeSelection = {
      ...marqueeSelection,
      currentX: event.clientX,
      currentY: event.clientY,
    };
    updateMarqueeSelectionFromPointer();
  }

  function finishMarqueeSelection() {
    if (!marqueeSelection) return;

    marqueeSelection = null;
    syncBulkControlsFromSelection();
    document.removeEventListener("pointermove", updateMarqueePointer);
    document.removeEventListener("pointerup", finishMarqueeSelection);
    document.removeEventListener("pointercancel", cancelMarqueeSelection);
  }

  function cancelMarqueeSelection() {
    if (!marqueeSelection) return;

    setSelectedStepKeys(new Set(marqueeSelection.baseKeys));
    marqueeSelection = null;
    document.removeEventListener("pointermove", updateMarqueePointer);
    document.removeEventListener("pointerup", finishMarqueeSelection);
    document.removeEventListener("pointercancel", cancelMarqueeSelection);
  }

  /** @param {PointerEvent} event */
  function beginStepMarqueeSelection(event) {
    if (event.button !== 0 || marqueeSelection) return;

    const target = event.target;

    if (!(target instanceof Element)) return;

    if (
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [role='slider'], [data-bulk-step-cell], [data-no-marquee], [data-no-long-press], [data-insert-slot], [data-remove-button], [data-multiplier-resize]",
      )
    ) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();

    marqueeSelection = {
      startX: event.clientX,
      startY: event.clientY,
      currentX: event.clientX,
      currentY: event.clientY,
      addToSelection: event.shiftKey,
      baseKeys: event.shiftKey ? new Set(selectedStepKeysForGrid) : new Set(),
    };

    updateMarqueeSelectionFromPointer();
    document.addEventListener("pointermove", updateMarqueePointer);
    document.addEventListener("pointerup", finishMarqueeSelection);
    document.addEventListener("pointercancel", cancelMarqueeSelection);
  }

  function createHistorySnapshot() {
    return {
      grid: cloneMatrix(grid),
      rowMuted: [...rowMuted],
      soloRow,
      rowSoloRestoreMuted: rowSoloRestoreMuted ? [...rowSoloRestoreMuted] : null,
      rowTimingOffset: [...rowTimingOffset],
      rowMidiChannel: [...rowMidiChannel],
      stepDurationFraction: cloneMatrix(stepDurationFraction),
      stepTimingMultiplier: cloneMatrix(stepTimingMultiplier),
      stepVelocity: cloneMatrix(stepVelocity),
      stepMuted: cloneMatrix(stepMuted),
      stepSkipped: cloneMatrix(stepSkipped),
      stepProbability: cloneMatrix(stepProbability),
      stepCycle: cloneMatrix(stepCycle),
      stepCycleOffset: cloneMatrix(stepCycleOffset),
      stepIds: cloneMatrix(stepIds),
      nextStepId,
      pulseIndex,
      swingPercent,
      velocityHumanizePercent,
      timingHumanizePercent,
      swingSubdivisionIndex,
      loopBraceEnabled,
      loopBraceStart,
      loopBraceEnd,
    };
  }

  function cloneSnapshot(snapshot) {
    return {
      ...snapshot,
      grid: cloneMatrix(snapshot.grid),
      rowMuted: [...snapshot.rowMuted],
      rowSoloRestoreMuted: snapshot.rowSoloRestoreMuted ? [...snapshot.rowSoloRestoreMuted] : null,
      rowTimingOffset: [...snapshot.rowTimingOffset],
      rowMidiChannel: [...snapshot.rowMidiChannel],
      stepDurationFraction: cloneMatrix(snapshot.stepDurationFraction),
      stepTimingMultiplier: cloneMatrix(snapshot.stepTimingMultiplier),
      stepVelocity: cloneMatrix(snapshot.stepVelocity),
      stepMuted: cloneMatrix(snapshot.stepMuted),
      stepSkipped: cloneMatrix(snapshot.stepSkipped),
      stepProbability: cloneMatrix(snapshot.stepProbability),
      stepCycle: cloneMatrix(snapshot.stepCycle),
      stepCycleOffset: cloneMatrix(snapshot.stepCycleOffset),
      stepIds: cloneMatrix(snapshot.stepIds),
    };
  }

  function snapshotsEqual(left, right) {
    return JSON.stringify(left) === JSON.stringify(right);
  }

  function assignSnapshot(snapshot) {
    const next = cloneSnapshot(snapshot);

    grid = next.grid;
    rowMuted = next.rowMuted;
    soloRow = next.soloRow;
    rowSoloRestoreMuted = next.rowSoloRestoreMuted;
    rowTimingOffset = next.rowTimingOffset;
    rowMidiChannel = next.rowMidiChannel;
    stepDurationFraction = next.stepDurationFraction;
    stepTimingMultiplier = next.stepTimingMultiplier;
    stepVelocity = next.stepVelocity;
    stepMuted = next.stepMuted;
    stepSkipped = next.stepSkipped;
    stepProbability = next.stepProbability;
    stepCycle = next.stepCycle;
    stepCycleOffset = next.stepCycleOffset;
    stepIds = next.stepIds;
    nextStepId = next.nextStepId;
    pulseIndex = next.pulseIndex;
    swingPercent = next.swingPercent;
    velocityHumanizePercent = next.velocityHumanizePercent;
    timingHumanizePercent = next.timingHumanizePercent;
    swingSubdivisionIndex = next.swingSubdivisionIndex;
    loopBraceEnabled = next.loopBraceEnabled;
    loopBraceStart = next.loopBraceStart;
    loopBraceEnd = next.loopBraceEnd;
  }

  function assignPatternState(state, updateSlotSelection = true) {
    if (!state || typeof state !== "object") return;

    const statePatternSlot = Number.parseInt(String(state.patternSlot ?? activePatternSlot), 10);
    if (updateSlotSelection && !Number.isNaN(statePatternSlot)) {
      activePatternSlot = Math.min(7, Math.max(0, statePatternSlot));
    } else if (!Number.isNaN(statePatternSlot)) {
      viewPatternSlot = Math.min(7, Math.max(0, statePatternSlot));
    }

    grid = cloneMatrix(state.phraseNotes ?? grid);
    rowMuted = [...(state.phraseRowMuted ?? rowMuted)].map(Boolean);
    soloRow = -1;
    rowSoloRestoreMuted = null;
    rowTimingOffset = [...(state.phraseRowTimingOffset ?? rowTimingOffset)];
    rowMidiChannel = [...(state.phraseRowMidiChannel ?? rowMidiChannel)];
    stepDurationFraction = cloneMatrix(state.phraseStepDurationFraction ?? stepDurationFraction);
    stepTimingMultiplier = cloneMatrix(state.phraseStepTimingMultiplier ?? stepTimingMultiplier);
    stepVelocity = cloneMatrix(state.phraseStepVelocity ?? stepVelocity);
    stepMuted = cloneMatrix(state.phraseStepMuted ?? stepMuted).map((row) => row.map(Boolean));
    stepSkipped = cloneMatrix(state.phraseStepSkipped ?? stepSkipped).map((row) => row.map(Boolean));
    stepProbability = cloneMatrix(state.phraseStepProbability ?? stepProbability);
    stepCycle = cloneMatrix(state.phraseStepCycle ?? stepCycle);
    stepCycleOffset = cloneMatrix(state.phraseStepCycleOffset ?? stepCycleOffset);
    loopBraceEnabled = Boolean(Number.parseInt(String(state.loopBraceEnabled ?? 0), 10));
    loopBraceStart = Number.parseFloat(String(state.loopBraceStart ?? 0));
    loopBraceEnd = Number.parseFloat(String(state.loopBraceEnd ?? 8));
    activeGates = grid.map((row) => row.map(() => false));
    const slotForIds = activePatternSlot >= 0 ? activePatternSlot : viewPatternSlot;
    stepIds = grid.map((row, rowIndex) => row.map((_, step) => `step-${slotForIds}-${rowIndex}-${step}`));
    nextStepId = grid.reduce((count, row) => count + row.length, 0);
    setSelectedStepKeys(new Set());
    bulkTransposeSemitones = 0;
    undoStack = [];
    redoStack = [];
  }

  function pushHistoryEntry(label, before, after) {
    if (snapshotsEqual(before, after)) return;

    undoStack = [
      ...undoStack,
      {
        label,
        before: cloneSnapshot(before),
        after: cloneSnapshot(after),
      },
    ].slice(-historyLimit);
    redoStack = [];
  }

  async function commitHistory(label, mutation) {
    const before = createHistorySnapshot();
    await mutation();
    const after = createHistorySnapshot();

    pushHistoryEntry(label, before, after);
  }

  function snapshotWithRowOrder(snapshot, row, targetIds, currentIds) {
    const next = cloneSnapshot(snapshot);
    const idToIndex = Object.fromEntries(currentIds.map((id, index) => [id, index]));
    const reorder = (values) => targetIds.map((id) => values[idToIndex[id]]);

    next.grid[row] = reorder(next.grid[row]);
    next.stepDurationFraction[row] = reorder(next.stepDurationFraction[row]);
    next.stepTimingMultiplier[row] = reorder(next.stepTimingMultiplier[row]);
    next.stepVelocity[row] = reorder(next.stepVelocity[row]);
    next.stepMuted[row] = reorder(next.stepMuted[row]);
    next.stepSkipped[row] = reorder(next.stepSkipped[row]);
    next.stepProbability[row] = reorder(next.stepProbability[row]);
    next.stepCycle[row] = reorder(next.stepCycle[row]);
    next.stepCycleOffset[row] = reorder(next.stepCycleOffset[row]);
    next.stepIds[row] = [...targetIds];

    return next;
  }

  async function syncSnapshotToNative(snapshot, previousSnapshot) {
    if (nativeFunctionAvailable("replacePhraseRow")) {
      const replacePhraseRow = getNativeFunction("replacePhraseRow");

      for (let row = 0; row < snapshot.grid.length; row += 1) {
        await replacePhraseRow(
          row,
          snapshot.grid[row],
          snapshot.stepTimingMultiplier[row],
          snapshot.stepDurationFraction[row],
          snapshot.stepVelocity[row],
          snapshot.stepMuted[row].map((muted) => (muted ? 1 : 0)),
          snapshot.stepSkipped[row].map((skipped) => (skipped ? 1 : 0)),
          snapshot.stepProbability[row],
          snapshot.stepCycle[row],
          snapshot.stepCycleOffset[row],
        );
      }
    }

    for (let row = 0; row < snapshot.grid.length; row += 1) {
      await pushRowMutedValue(row, snapshot.rowMuted[row]);
      await pushRowTimingOffset(row);
      await pushRowMidiChannel(row);
    }

    if (nativeFunctionAvailable("setPulseIndex")) {
      await getNativeFunction("setPulseIndex")(snapshot.pulseIndex);
    }

    if (nativeFunctionAvailable("setSwingPercent")) {
      await getNativeFunction("setSwingPercent")(snapshot.swingPercent);
    }

    if (nativeFunctionAvailable("setVelocityHumanizePercent")) {
      await getNativeFunction("setVelocityHumanizePercent")(snapshot.velocityHumanizePercent);
    }

    if (nativeFunctionAvailable("setTimingHumanizePercent")) {
      await getNativeFunction("setTimingHumanizePercent")(snapshot.timingHumanizePercent);
    }

    if (nativeFunctionAvailable("setSwingSubdivisionIndex")) {
      await getNativeFunction("setSwingSubdivisionIndex")(snapshot.swingSubdivisionIndex);
    }

    if (snapshot.loopBraceStart > previousSnapshot.loopBraceStart) {
      await pushLoopBraceEnd(snapshot.loopBraceEnd);
      await pushLoopBraceStart(snapshot.loopBraceStart);
    } else {
      await pushLoopBraceStart(snapshot.loopBraceStart);
      await pushLoopBraceEnd(snapshot.loopBraceEnd);
    }

    await pushLoopBraceEnabled(snapshot.loopBraceEnabled);
  }

  async function applyHistorySnapshot(snapshot) {
    const previousSnapshot = createHistorySnapshot();
    assignSnapshot(snapshot);
    await syncSnapshotToNative(snapshot, previousSnapshot);
  }

  async function undo() {
    const entry = undoStack[undoStack.length - 1];

    if (!entry) return;

    undoStack = undoStack.slice(0, -1);
    redoStack = [...redoStack, entry];
    await applyHistorySnapshot(entry.before);
  }

  async function redo() {
    const entry = redoStack[redoStack.length - 1];

    if (!entry) return;

    redoStack = redoStack.slice(0, -1);
    undoStack = [...undoStack, entry];
    await applyHistorySnapshot(entry.after);
  }

  /** @param {number} row @param {string[]} orderedIds */
  function reorderRowByIds(row, orderedIds) {
    const validIds = sanitizeOrderedIds(orderedIds, stepIds[row]);

    if (validIds.length !== stepIds[row].length) return;

    const idToIndex = Object.fromEntries(stepIds[row].map((id, index) => [id, index]));
    /** @param {unknown[]} array */
    const reorder = (array) => validIds.map((id) => array[idToIndex[id]]);

    grid[row] = reorder(grid[row]);
    stepDurationFraction[row] = reorder(stepDurationFraction[row]);
    stepTimingMultiplier[row] = reorder(stepTimingMultiplier[row]);
    stepVelocity[row] = reorder(stepVelocity[row]);
    stepMuted[row] = reorder(stepMuted[row]);
    stepSkipped[row] = reorder(stepSkipped[row]);
    stepProbability[row] = reorder(stepProbability[row]);
    stepCycle[row] = reorder(stepCycle[row]);
    stepCycleOffset[row] = reorder(stepCycleOffset[row]);
    activeGates[row] = reorder(activeGates[row]);
    stepIds[row] = validIds;

  }

  /** JUCE wraps each withInitialisationData value as [payload]. */
  function unwrapJuceInit(key) {
    const raw = window.__JUCE__?.initialisationData?.[key];

    if (!Array.isArray(raw) || raw.length === 0) return null;

    if (raw.length === 1 && Array.isArray(raw[0])) return raw[0];

    return raw;
  }

  function nativeFunctionAvailable(name) {
    return window.__JUCE__?.initialisationData?.__juce__functions?.includes?.(name) ?? false;
  }

  function currentUiTimeMs() {
    return window.performance?.now?.() ?? Date.now();
  }

  function inactiveGateMatrix() {
    return grid.map((row) => row.map(() => false));
  }

  function gateSnapshotForBeat(beat) {
    const nextActiveGates = grid.map((row) => row.map(() => false));

    if (beat >= 0) {
      for (let row = 0; row < grid.length; row += 1) {
        for (let step = 0; step < grid[row].length; step += 1) {
          nextActiveGates[row][step] = isStepActiveAtBeat({
            beat,
            step,
            rowNotes: grid[row],
            rowMuted: rowMuted[row],
            rowTimingOffset: rowTimingOffset[row],
            stepDurationFraction: stepDurationFraction[row],
            stepTimingMultiplier: stepTimingMultiplier[row],
            stepVelocity: stepVelocity[row],
            stepMuted: stepMuted[row],
            stepSkipped: stepSkipped[row],
            stepProbability: stepProbability[row],
            stepCycle: stepCycle[row],
            stepCycleOffset: stepCycleOffset[row],
            pulseIndex,
            swingPercent,
            swingSubdivisionIndex,
          });
        }
      }
    }

    return nextActiveGates;
  }

  function updateActiveGatesForBeat(beat) {
    const rawGates = gateSnapshotForBeat(beat);

    if (beat < 0) {
      previousGateSnapshot = rawGates;
      activeGateHoldUntil = inactiveGateMatrix().map((row) => row.map(() => 0));
      activeGates = rawGates;
      return;
    }

    const now = currentUiTimeMs();
    const nextHoldUntil = rawGates.map((rowGates, row) =>
      rowGates.map((active, step) => {
        const heldUntil = activeGateHoldUntil[row]?.[step] ?? 0;
        const wasActive = previousGateSnapshot[row]?.[step] ?? false;

        return active && !wasActive
          ? Math.max(heldUntil, now + stepTriggerFlashMs)
          : heldUntil;
      }),
    );
    const nextActiveGates = rawGates.map((rowGates, row) =>
      rowGates.map((active, step) => active || (nextHoldUntil[row]?.[step] ?? 0) > now),
    );

    previousGateSnapshot = rawGates;
    activeGateHoldUntil = nextHoldUntil;
    activeGates = nextActiveGates;
  }

  if (window.__JUCE__?.initialisationData?.pluginName?.[0]) {
    pluginName = window.__JUCE__.initialisationData.pluginName[0];
  }

  function loadGridFromInitialisation() {
    const init = unwrapJuceInit("phraseNotes");

    if (!Array.isArray(init) || init.length === 0) return;

    const defaults = defaultPhraseGrid();
    const nextGrid = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];

      if (!Array.isArray(rowData)) {
        nextGrid[row] = [...defaults[row]];
        continue;
      }

      nextGrid[row] = rowData.map((value, step) => {
        const midi = Number.parseInt(String(value), 10);

        if (Number.isNaN(midi)) return defaults[row][step] ?? defaults[row][0];

        return Math.min(127, Math.max(0, midi));
      });
    }

    grid = nextGrid;
    activeGates = nextGrid.map((row) => row.map(() => false));
    stepIds = nextGrid.map((row, rowIndex) => row.map((_, step) => `step-${rowIndex}-${step}`));
    nextStepId = nextGrid.reduce((count, row) => count + row.length, 0);
  }

  function loadRowMutedFromInitialisation() {
    const init = unwrapJuceInit("phraseRowMuted");

    if (!Array.isArray(init)) return;

    const next = [false, false, false, false];

    for (let row = 0; row < 4; row += 1) {
      next[row] = Boolean(init[row]);
    }

    rowMuted = next;
    soloRow = -1;
    rowSoloRestoreMuted = null;
  }

  function loadRowTimingOffsetFromInitialisation() {
    const init = unwrapJuceInit("phraseRowTimingOffset");

    if (!Array.isArray(init)) return;

    const next = [3, 3, 3, 3];

    for (let row = 0; row < 4; row += 1) {
      const value = Number.parseInt(String(init[row]), 10);

      if (Number.isNaN(value)) continue;

      next[row] = Math.min(6, Math.max(0, value));
    }

    rowTimingOffset = next;
  }

  function loadRowMidiChannelFromInitialisation() {
    const init = unwrapJuceInit("phraseRowMidiChannel");

    if (!Array.isArray(init)) return;

    const next = [1, 2, 3, 4];

    for (let row = 0; row < 4; row += 1) {
      const value = Number.parseInt(String(init[row]), 10);

      if (Number.isNaN(value)) continue;

      next[row] = Math.min(16, Math.max(1, value));
    }

    rowMidiChannel = next;
  }

  function loadStepDurationFromInitialisation() {
    const init = unwrapJuceInit("phraseStepDurationFraction");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepDurationGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const raw = rowData?.[step] ?? defaults[row][step];
        const parsed = Number.parseFloat(String(raw));

        next[row][step] = Number.isNaN(parsed)
          ? defaults[row][step] ?? defaults[row][0]
          : Math.min(1, Math.max(0, parsed));
      }
    }

    stepDurationFraction = next;
  }

  function loadStepTimingMultiplierFromInitialisation() {
    const init = unwrapJuceInit("phraseStepTimingMultiplier");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepTimingMultiplierGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const value = Number.parseInt(String(rowData?.[step] ?? defaults[row][step]), 10);

        next[row][step] = Number.isNaN(value)
          ? defaults[row][step] ?? defaults[row][0]
          : Math.min(stepTimingMultiplierCount - 1, Math.max(0, value));
      }
    }

    stepTimingMultiplier = next;
  }

  function loadStepVelocityFromInitialisation() {
    const init = unwrapJuceInit("phraseStepVelocity");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepVelocityGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const value = Number.parseInt(String(rowData?.[step] ?? defaults[row][step]), 10);

        next[row][step] = Number.isNaN(value)
          ? defaults[row][step] ?? defaults[row][0]
          : Math.min(127, Math.max(0, value));
      }
    }

    stepVelocity = next;
  }

  function loadStepMutedFromInitialisation() {
    const init = unwrapJuceInit("phraseStepMuted");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepMutedGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const raw = rowData?.[step] ?? defaults[row][step];
        next[row][step] = raw === true || raw === 1 || raw === "1";
      }
    }

    stepMuted = next;
  }

  function loadStepSkippedFromInitialisation() {
    const init = unwrapJuceInit("phraseStepSkipped");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepSkippedGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const raw = rowData?.[step] ?? defaults[row][step];
        next[row][step] = raw === true || raw === 1 || raw === "1";
      }
    }

    stepSkipped = next;
  }

  function loadStepProbabilityFromInitialisation() {
    const init = unwrapJuceInit("phraseStepProbability");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepProbabilityGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const value = Number.parseInt(String(rowData?.[step] ?? defaults[row][step]), 10);
        next[row][step] = Number.isNaN(value)
          ? defaults[row][step] ?? 100
          : Math.min(100, Math.max(0, value));
      }
    }

    stepProbability = next;
  }

  function loadStepCycleFromInitialisation() {
    const init = unwrapJuceInit("phraseStepCycle");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepCycleGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const value = Number.parseInt(String(rowData?.[step] ?? defaults[row][step]), 10);
        next[row][step] = Number.isNaN(value)
          ? defaults[row][step] ?? 1
          : Math.min(64, Math.max(1, value));
      }
    }

    stepCycle = next;
  }

  function loadStepCycleOffsetFromInitialisation() {
    const init = unwrapJuceInit("phraseStepCycleOffset");

    if (!Array.isArray(init)) return;

    const defaults = defaultStepCycleOffsetGrid();
    const next = [];

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];
      const stepCount = grid[row]?.length ?? defaults[row].length;
      next[row] = [];

      for (let step = 0; step < stepCount; step += 1) {
        const cycle = stepCycle[row]?.[step] ?? defaults[row][step] ?? 1;
        const value = Number.parseInt(String(rowData?.[step] ?? defaults[row][step]), 10);
        next[row][step] = Number.isNaN(value)
          ? defaults[row][step] ?? 0
          : Math.min(Math.max(0, cycle - 1), Math.max(0, value));
      }
    }

    stepCycleOffset = next;
  }

  async function pushMovePhraseStep(row, fromStep, toStep) {
    if (!nativeFunctionAvailable("movePhraseStep")) return;

    const movePhraseStep = getNativeFunction("movePhraseStep");
    await movePhraseStep(row, fromStep, toStep);
  }

  /** @param {number} row @param {string[]} beforeIds @param {string[]} afterIds */
  async function commitRowMove(row, beforeIds, afterIds) {
    const move = findSingleMove(beforeIds, afterIds);

    if (!move || move.from === move.to) return;

    const after = createHistorySnapshot();
    const before = snapshotWithRowOrder(after, row, beforeIds, afterIds);
    pushHistoryEntry("Move step", before, after);

    await pushMovePhraseStep(row, move.from, move.to);
  }

  async function pushNote(row, step) {
    if (!nativeFunctionAvailable("setPhraseNote")) return;

    const setPhraseNote = getNativeFunction("setPhraseNote");
    await setPhraseNote(row, step, grid[row][step]);
  }

  async function pushRowMutedValue(row, muted) {
    if (!nativeFunctionAvailable("setPhraseRowMuted")) return;

    const setPhraseRowMuted = getNativeFunction("setPhraseRowMuted");
    await setPhraseRowMuted(row, muted ? 1 : 0);
  }

  async function applyRowMutedState(nextMuted) {
    const previousMuted = rowMuted;
    rowMuted = nextMuted;

    for (let row = 0; row < nextMuted.length; row += 1) {
      if (previousMuted[row] !== nextMuted[row]) {
        await pushRowMutedValue(row, nextMuted[row]);
      }
    }
  }

  async function pushCurrentPhraseRow(row) {
    if (nativeFunctionAvailable("replacePhraseRow")) {
      const replacePhraseRow = getNativeFunction("replacePhraseRow");
      await replacePhraseRow(
        row,
        grid[row],
        stepTimingMultiplier[row],
        stepDurationFraction[row],
        stepVelocity[row],
        stepMuted[row].map((muted) => (muted ? 1 : 0)),
        stepSkipped[row].map((skipped) => (skipped ? 1 : 0)),
        stepProbability[row],
        stepCycle[row],
        stepCycleOffset[row],
      );
      return;
    }

    for (let step = 0; step < grid[row].length; step += 1) {
      await pushNote(row, step);
    }
  }

  async function pushRowTimingOffset(row) {
    if (!nativeFunctionAvailable("setPhraseRowTimingOffset")) return;

    const setPhraseRowTimingOffset = getNativeFunction("setPhraseRowTimingOffset");
    await setPhraseRowTimingOffset(row, rowTimingOffset[row]);
  }

  async function pushRowMidiChannel(row) {
    if (!nativeFunctionAvailable("setPhraseRowMidiChannel")) return;

    const setPhraseRowMidiChannel = getNativeFunction("setPhraseRowMidiChannel");
    await setPhraseRowMidiChannel(row, rowMidiChannel[row]);
  }

  async function pushRowColorsEnabled() {
    if (!nativeFunctionAvailable("setRowColorsEnabled")) return;

    const setRowColorsEnabled = getNativeFunction("setRowColorsEnabled");
    await setRowColorsEnabled(rowColorsEnabled ? 1 : 0);
  }

  async function pushStepTimingMultiplier(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepTimingMultiplier")) return;

    const setPhraseStepTimingMultiplier = getNativeFunction("setPhraseStepTimingMultiplier");
    await setPhraseStepTimingMultiplier(row, step, stepTimingMultiplier[row][step]);
  }

  async function pushStepDurationFraction(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepDurationFraction")) return;

    const setPhraseStepDurationFraction = getNativeFunction("setPhraseStepDurationFraction");
    await setPhraseStepDurationFraction(row, step, stepDurationFraction[row][step]);
  }

  async function pushStepVelocity(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepVelocity")) return;

    const setPhraseStepVelocity = getNativeFunction("setPhraseStepVelocity");
    await setPhraseStepVelocity(row, step, stepVelocity[row][step]);
  }

  async function pushStepMuted(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepMuted")) return;

    const setPhraseStepMuted = getNativeFunction("setPhraseStepMuted");
    await setPhraseStepMuted(row, step, stepMuted[row][step] ? 1 : 0);
  }

  async function pushStepSkipped(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepSkipped")) return;

    const setPhraseStepSkipped = getNativeFunction("setPhraseStepSkipped");
    await setPhraseStepSkipped(row, step, stepSkipped[row][step] ? 1 : 0);
  }

  async function pushStepProbability(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepProbability")) return;

    const setPhraseStepProbability = getNativeFunction("setPhraseStepProbability");
    await setPhraseStepProbability(row, step, stepProbability[row][step]);
  }

  async function pushStepCycle(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepCycle")) return;

    const setPhraseStepCycle = getNativeFunction("setPhraseStepCycle");
    await setPhraseStepCycle(row, step, stepCycle[row][step]);
  }

  async function pushStepCycleOffset(row, step) {
    if (!nativeFunctionAvailable("setPhraseStepCycleOffset")) return;

    const setPhraseStepCycleOffset = getNativeFunction("setPhraseStepCycleOffset");
    await setPhraseStepCycleOffset(row, step, stepCycleOffset[row][step]);
  }

  async function setPhraseNoteValue(row, step, midi) {
    await commitHistory("Change note", async () => {
      grid[row][step] = Math.min(127, Math.max(0, midi));
      await pushNote(row, step);
    });
  }

  async function toggleRowMute(row, soloRequested = false) {
    await commitHistory(soloRequested ? "Solo row" : "Toggle row mute", async () => {
      if (soloRequested) {
        if (soloRow === row && rowSoloRestoreMuted) {
          const restoreMuted = [...rowSoloRestoreMuted];
          soloRow = -1;
          rowSoloRestoreMuted = null;
          await applyRowMutedState(restoreMuted);
          return;
        }

        if (!rowSoloRestoreMuted) {
          rowSoloRestoreMuted = [...rowMuted];
        }

        soloRow = row;
        await applyRowMutedState(rowMuted.map((_, index) => index !== row));
        return;
      }

      const nextMuted = [...rowMuted];
      nextMuted[row] = !nextMuted[row];
      await applyRowMutedState(nextMuted);
    });
  }

  function randomStepOrder(stepCount) {
    const stepOrder = Array.from({ length: stepCount }, (_, step) => step);

    for (let index = stepOrder.length - 1; index > 0; index -= 1) {
      const swapIndex = Math.floor(Math.random() * (index + 1));
      [stepOrder[index], stepOrder[swapIndex]] = [stepOrder[swapIndex], stepOrder[index]];
    }

    if (stepCount > 1 && stepOrder.every((sourceStep, step) => sourceStep === step)) {
      const swapIndex = 1 + Math.floor(Math.random() * (stepCount - 1));
      [stepOrder[0], stepOrder[swapIndex]] = [stepOrder[swapIndex], stepOrder[0]];
    }

    return stepOrder;
  }

  function stepPayloadAt(row, step) {
    return {
      note: grid[row][step],
      durationFraction: stepDurationFraction[row][step],
      timingMultiplier: stepTimingMultiplier[row][step],
      velocity: stepVelocity[row][step],
      muted: stepMuted[row][step],
      skipped: stepSkipped[row][step],
      probability: stepProbability[row][step],
      cycle: stepCycle[row][step],
      cycleOffset: stepCycleOffset[row][step],
      activeGate: activeGates[row]?.[step] ?? false,
    };
  }

  function writeStepPayload(row, step, payload) {
    grid[row][step] = payload.note;
    stepDurationFraction[row][step] = payload.durationFraction;
    stepTimingMultiplier[row][step] = payload.timingMultiplier;
    stepVelocity[row][step] = payload.velocity;
    stepMuted[row][step] = payload.muted;
    stepSkipped[row][step] = payload.skipped;
    stepProbability[row][step] = payload.probability;
    stepCycle[row][step] = payload.cycle;
    stepCycleOffset[row][step] = payload.cycleOffset;
    activeGates[row][step] = payload.activeGate;
  }

  async function pushRowsForRowSet(rows) {
    for (const row of rows) {
      await pushCurrentPhraseRow(row);
    }
  }

  async function shuffleSelectedSteps() {
    const locations = selectedStepLocationsByPosition();

    if (locations.length <= 1) return;

    await commitHistory("Shuffle selected steps", async () => {
      const payloads = locations.map(({ row, step }) => stepPayloadAt(row, step));
      const stepOrder = randomStepOrder(locations.length);
      const changedRows = new SvelteSet(locations.map(({ row }) => row));

      for (let index = 0; index < locations.length; index += 1) {
        const { row, step } = locations[index];
        writeStepPayload(row, step, payloads[stepOrder[index]]);
      }

      await pushRowsForRowSet(changedRows);
    });

    syncBulkControlsFromSelection();
  }

  async function randomizeSelectedStepOctaves() {
    const locations = selectedStepLocationsByPosition();

    if (locations.length === 0) return;

    await commitHistory("Randomize selected octaves", async () => {
      const changedRows = new SvelteSet(locations.map(({ row }) => row));

      for (const { row, step } of locations) {
        const shift = Math.random() < 0.5 ? -12 : 12;
        grid[row][step] = Math.min(127, Math.max(0, grid[row][step] + shift));
      }

      await pushRowsForRowSet(changedRows);
    });

    syncBulkControlsFromSelection();
  }

  async function reverseSelectedStepsByRow() {
    const groups = [...selectedStepLocationsGroupedByRow().entries()]
      .filter(([, locations]) => locations.length > 1);

    if (groups.length === 0) return;

    await commitHistory("Reverse selected steps", async () => {
      const changedRows = new SvelteSet();

      for (const [row, locations] of groups) {
        const payloads = locations
          .map(({ step }) => stepPayloadAt(row, step))
          .reverse();

        for (let index = 0; index < locations.length; index += 1) {
          writeStepPayload(row, locations[index].step, payloads[index]);
        }

        changedRows.add(row);
      }

      await pushRowsForRowSet(changedRows);
    });

    syncBulkControlsFromSelection();
  }

  async function selectRowTimingOffset(row, offsetIndex) {
    await commitHistory("Change row timing", async () => {
      rowTimingOffset[row] = offsetIndex;
      await pushRowTimingOffset(row);
    });
  }

  async function selectStepTimingMultiplier(row, step, multiplierIndex) {
    await commitHistory("Change step timing", async () => {
      const next = cloneMatrix(stepTimingMultiplier);
      next[row][step] = multiplierIndex;
      stepTimingMultiplier = next;
      await pushStepTimingMultiplier(row, step);
    });
  }

  async function selectStepDurationFraction(row, step, fraction) {
    await commitHistory("Change duration", async () => {
      stepDurationFraction[row][step] = Math.min(1, Math.max(0, fraction));
      await pushStepDurationFraction(row, step);
    });
  }

  async function setStepVelocity(row, step, value) {
    await commitHistory("Change velocity", async () => {
      stepVelocity[row][step] = Math.min(127, Math.max(0, value));
      await pushStepVelocity(row, step);
    });
  }

  function clampBulkPercent(value) {
    const parsed = Number.parseInt(String(value), 10);

    return Number.isNaN(parsed) ? 0 : Math.min(100, Math.max(0, parsed));
  }

  function clampTransposeSemitones(value) {
    const parsed = Number.parseInt(String(value), 10);

    return Number.isNaN(parsed) ? 0 : Math.min(48, Math.max(-48, parsed));
  }

  function formatSemitoneValue(value) {
    const rounded = Math.round(value);

    return rounded > 0 ? `+${rounded}` : String(rounded);
  }

  async function pushRowsForSelectedLocations(locations) {
    const rows = [...new Set(locations.map((location) => location.row))];

    for (const row of rows) {
      await pushCurrentPhraseRow(row);
    }
  }

  function beginBulkEditGesture() {
    if (!bulkEditGestureBefore) {
      bulkEditGestureBefore = createHistorySnapshot();
    }
  }

  function resetBulkEditGesture() {
    bulkEditGestureBefore = null;
    bulkTransposeBaselineByKey = null;
  }

  async function commitBulkEditGesture(label, syncFn) {
    const before = bulkEditGestureBefore;
    resetBulkEditGesture();

    if (!before) return;

    await syncFn();
    const after = createHistorySnapshot();
    pushHistoryEntry(label, before, after);
  }

  function previewBulkDurationPercent(value) {
    const clamped = clampBulkPercent(value);
    bulkDurationPercent = clamped;
    const locations = selectedStepLocations();

    if (locations.length === 0) return;

    const fraction = clamped / 100;

    for (const { row, step } of locations) {
      stepDurationFraction[row][step] = fraction;
    }
  }

  async function commitBulkDurationPercent(value) {
    const clamped = clampBulkPercent(value);
    const locations = selectedStepLocations();

    if (locations.length === 0) {
      resetBulkEditGesture();
      return;
    }

    previewBulkDurationPercent(clamped);

    await commitBulkEditGesture("Bulk duration", async () => {
      await pushRowsForSelectedLocations(locations);
    });
  }

  function previewBulkVelocityPercent(value) {
    const clamped = clampBulkPercent(value);
    bulkVelocityPercent = clamped;
    const locations = selectedStepLocations();

    if (locations.length === 0) return;

    const velocity = Math.round((clamped / 100) * 127);

    for (const { row, step } of locations) {
      stepVelocity[row][step] = velocity;
    }
  }

  async function commitBulkVelocityPercent(value) {
    const clamped = clampBulkPercent(value);
    const locations = selectedStepLocations();

    if (locations.length === 0) {
      resetBulkEditGesture();
      return;
    }

    previewBulkVelocityPercent(clamped);

    await commitBulkEditGesture("Bulk velocity", async () => {
      await pushRowsForSelectedLocations(locations);
    });
  }

  function previewBulkTransposeSemitones(value) {
    const clamped = clampTransposeSemitones(value);
    const locations = selectedStepLocations();

    if (locations.length === 0) return;

    if (!bulkTransposeBaselineByKey) {
      bulkTransposeBaselineByKey = new Map();

      for (const { row, step, key } of locations) {
        bulkTransposeBaselineByKey.set(key, grid[row][step]);
      }
    }

    bulkTransposeSemitones = clamped;

    for (const { row, step, key } of locations) {
      const baseline = bulkTransposeBaselineByKey.get(key) ?? grid[row][step];
      grid[row][step] = Math.min(127, Math.max(0, baseline + clamped));
    }
  }

  async function commitBulkTransposeSemitones(value) {
    const clamped = clampTransposeSemitones(value);
    const locations = selectedStepLocations();

    if (locations.length === 0) {
      resetBulkEditGesture();
      return;
    }

    previewBulkTransposeSemitones(clamped);

    await commitBulkEditGesture("Bulk transpose", async () => {
      await pushRowsForSelectedLocations(locations);
    });
  }

  async function setStepMuted(row, step, muted) {
    await commitHistory("Toggle step mute", async () => {
      stepMuted[row][step] = muted;
      let clearedSkip = false;

      if (muted && stepSkipped[row][step]) {
        stepSkipped[row][step] = false;
        clearedSkip = true;
      }

      await pushStepMuted(row, step);

      if (clearedSkip) {
        await pushStepSkipped(row, step);
      }
    });
  }

  async function setStepSkipped(row, step, skipped) {
    await commitHistory("Toggle step skip", async () => {
      stepSkipped[row][step] = skipped;
      let clearedMute = false;

      if (skipped && stepMuted[row][step]) {
        stepMuted[row][step] = false;
        clearedMute = true;
      }

      await pushStepSkipped(row, step);

      if (clearedMute) {
        await pushStepMuted(row, step);
      }
    });
  }

  async function setStepProbability(row, step, probability) {
    await commitHistory("Change probability", async () => {
      stepProbability[row][step] = Math.min(100, Math.max(0, probability));
      await pushStepProbability(row, step);
    });
  }

  async function setStepCycle(row, step, cycle) {
    await commitHistory("Change cycle", async () => {
      const nextCycle = Math.min(64, Math.max(1, cycle));
      stepCycle[row][step] = nextCycle;
      stepCycleOffset[row][step] = Math.min(stepCycleOffset[row][step], nextCycle - 1);
      await pushStepCycle(row, step);
      await pushStepCycleOffset(row, step);
    });
  }

  async function setStepCycleOffset(row, step, cycleOffset) {
    await commitHistory("Change cycle offset", async () => {
      const maxOffset = Math.max(0, (stepCycle[row][step] ?? 1) - 1);
      stepCycleOffset[row][step] = Math.min(maxOffset, Math.max(0, cycleOffset));
      await pushStepCycleOffset(row, step);
    });
  }

  async function removeStep(row, step) {
    if (step < 0 || step >= grid[row].length) return;

    await commitHistory("Remove step", async () => {
      grid[row].splice(step, 1);
      stepDurationFraction[row].splice(step, 1);
      stepTimingMultiplier[row].splice(step, 1);
      stepVelocity[row].splice(step, 1);
      stepMuted[row].splice(step, 1);
      stepSkipped[row].splice(step, 1);
      stepProbability[row].splice(step, 1);
      stepCycle[row].splice(step, 1);
      stepCycleOffset[row].splice(step, 1);
      activeGates[row].splice(step, 1);
      stepIds[row].splice(step, 1);


      if (!nativeFunctionAvailable("removePhraseStep")) return;

      const removePhraseStep = getNativeFunction("removePhraseStep");
      await removePhraseStep(row, step);
    });
  }

  async function insertStep(row, step) {
    await commitHistory("Insert step", async () => {
      const defaultDurations = defaultStepDurationGrid();
      const defaultMultipliers = defaultStepTimingMultiplierGrid();
      const defaultVelocities = defaultStepVelocityGrid();
      const defaultMuted = defaultStepMutedGrid();
      const defaultSkipped = defaultStepSkippedGrid();
      const defaultProbability = defaultStepProbabilityGrid();
      const defaultCycle = defaultStepCycleGrid();
      const defaultCycleOffset = defaultStepCycleOffsetGrid();

      grid[row].splice(step, 0, defaultStepNote);
      stepDurationFraction[row].splice(step, 0, defaultDurations[row]?.[0] ?? 1);
      stepTimingMultiplier[row].splice(
        step,
        0,
        defaultMultipliers[row]?.[0] ?? defaultStepTimingMultiplierIndex,
      );
      stepVelocity[row].splice(step, 0, defaultVelocities[row]?.[0] ?? 100);
      stepMuted[row].splice(step, 0, defaultMuted[row]?.[0] ?? false);
      stepSkipped[row].splice(step, 0, defaultSkipped[row]?.[0] ?? false);
      stepProbability[row].splice(step, 0, defaultProbability[row]?.[0] ?? 100);
      stepCycle[row].splice(step, 0, defaultCycle[row]?.[0] ?? 1);
      stepCycleOffset[row].splice(step, 0, defaultCycleOffset[row]?.[0] ?? 0);
      activeGates[row].splice(step, 0, false);
      stepIds[row].splice(step, 0, createStepId());


      if (!nativeFunctionAvailable("insertPhraseStep")) return;

      const insertPhraseStep = getNativeFunction("insertPhraseStep");
      await insertPhraseStep(row, step);
    });
  }

  async function duplicateStep(row, step) {
    if (step <= 0) return;

    await commitHistory("Duplicate step", async () => {
      const source = step - 1;

      grid[row].splice(step, 0, grid[row][source]);
      stepDurationFraction[row].splice(step, 0, stepDurationFraction[row][source]);
      stepTimingMultiplier[row].splice(step, 0, stepTimingMultiplier[row][source]);
      stepVelocity[row].splice(step, 0, stepVelocity[row][source]);
      stepMuted[row].splice(step, 0, stepMuted[row][source]);
      stepSkipped[row].splice(step, 0, stepSkipped[row][source]);
      stepProbability[row].splice(step, 0, stepProbability[row][source]);
      stepCycle[row].splice(step, 0, stepCycle[row][source]);
      stepCycleOffset[row].splice(step, 0, stepCycleOffset[row][source]);
      activeGates[row].splice(step, 0, false);
      stepIds[row].splice(step, 0, createStepId());


      if (!nativeFunctionAvailable("duplicatePhraseStep")) return;

      const duplicatePhraseStep = getNativeFunction("duplicatePhraseStep");
      await duplicatePhraseStep(row, step);
    });
  }

  function buildDefaultRecordedStep() {
    return {
      duration: defaultStepDurationFraction,
      multiplier: defaultStepTimingMultiplierIndex,
      velocity: defaultStepVelocity,
      muted: false,
      skipped: false,
      probability: 100,
      cycle: 1,
      cycleOffset: 0,
    };
  }

  async function commitRecordedNote(midi) {
    if (recordingRow === null) return;

    const note = Math.min(127, Math.max(0, Math.round(midi)));

    if (grid[recordingRow].length >= maxPhraseStepsPerRow) {
      return;
    }

    applyRecordedNoteToUiRow(recordingRow, note);
    recordingCapturedNotes = true;

    if (nativeFunctionAvailable("injectPhraseRowRecordedNote")) {
      await getNativeFunction("injectPhraseRowRecordedNote")(note);
    }
  }

  function applyRecordedNoteToUiRow(row, midi) {
    const defs = buildDefaultRecordedStep();
    const note = Math.min(127, Math.max(0, Math.round(midi)));

    if (recordingAwaitingFirstNote) {
      recordingAwaitingFirstNote = false;
    }

    if (grid[row].length < maxPhraseStepsPerRow) {
      grid[row] = [...grid[row], note];
      stepDurationFraction[row] = [...stepDurationFraction[row], defs.duration];
      stepTimingMultiplier[row] = [...stepTimingMultiplier[row], defs.multiplier];
      stepVelocity[row] = [...stepVelocity[row], defs.velocity];
      stepMuted[row] = [...stepMuted[row], defs.muted];
      stepSkipped[row] = [...stepSkipped[row], defs.skipped];
      stepProbability[row] = [...stepProbability[row], defs.probability];
      stepCycle[row] = [...stepCycle[row], defs.cycle];
      stepCycleOffset[row] = [...stepCycleOffset[row], defs.cycleOffset];
      stepIds[row] = [...stepIds[row], createStepId()];
      activeGates[row] = [...activeGates[row], false];
    }

  }

  async function disarmRowRecordingNative() {
    if (nativeFunctionAvailable("setPhraseRowRecording")) {
      await getNativeFunction("setPhraseRowRecording")(-1);
    }
  }

  async function cancelRowRecording() {
    if (recordingRow === null) return;

    recordingRow = null;
    recordingAwaitingFirstNote = false;
    recordingCapturedNotes = false;
    recordingKeysHeld = new Set();

    await disarmRowRecordingNative();

    if (recordingHistoryBefore !== null) {
      const restore = recordingHistoryBefore;
      recordingHistoryBefore = null;
      assignSnapshot(restore);
      const previousSnapshot = createHistorySnapshot();
      await syncSnapshotToNative(restore, previousSnapshot);
    } else {
      recordingHistoryBefore = null;
    }

    return row;
  }

  async function startRowRecording(row) {
    if (recordingRow !== null && recordingRow !== row) {
      await cancelRowRecording();
    }

    recordingRow = row;
    recordingHistoryBefore = createHistorySnapshot();
    recordingCapturedNotes = false;
    recordingAwaitingFirstNote = true;

    if (nativeFunctionAvailable("setPhraseRowRecording")) {
      await getNativeFunction("setPhraseRowRecording")(row);
    }
  }

  async function finishRowRecording() {
    if (recordingRow === null) return;

    const row = recordingRow;
    const before = recordingHistoryBefore;
    const hadNotes = recordingCapturedNotes;

    recordingRow = null;
    recordingAwaitingFirstNote = false;
    recordingCapturedNotes = false;
    recordingHistoryBefore = null;
    recordingKeysHeld = new Set();

    await disarmRowRecordingNative();

    if (hadNotes && before !== null) {
      const after = createHistorySnapshot();
      pushHistoryEntry("Record row", before, after);
    } else if (before !== null) {
      assignSnapshot(before);
      const previousSnapshot = createHistorySnapshot();
      await syncSnapshotToNative(before, previousSnapshot);
    }
  }

  async function toggleRowRecording(row) {
    if (recordingRow === row) {
      await finishRowRecording();
    } else {
      await startRowRecording(row);
    }
  }

  async function pollRowRecordingNotes() {
    if (recordingRow === null) {
      return;
    }

    if (nativeFunctionAvailable("getPhraseRowRecordingKeysHeld")) {
      try {
        const held = await getNativeFunction("getPhraseRowRecordingKeysHeld")();
        const keys = Array.isArray(held) ? held : [];
        recordingKeysHeld = new Set(
          keys
            .map((value) => Number.parseInt(String(value), 10))
            .filter((midi) => !Number.isNaN(midi)),
        );
      } catch {
        // Native bridge unavailable during teardown.
      }
    }

    if (!nativeFunctionAvailable("drainPhraseRowRecordedNotes")) {
      return;
    }

    try {
      const drained = await getNativeFunction("drainPhraseRowRecordedNotes")();
      const notes = Array.isArray(drained) ? drained : [];

      for (const raw of notes) {
        const midi = Number.parseInt(String(raw), 10);

        if (Number.isNaN(midi)) continue;

        applyRecordedNoteToUiRow(recordingRow, midi);
        recordingCapturedNotes = true;
      }
    } catch {
      // Native bridge unavailable during teardown.
    }
  }

  /** @param {number} midi */
  function onRecordPianoNotePress(midi) {
    void commitRecordedNote(midi);
  }

  async function pollPlaybackActivity() {
    const beatNativeName = nativeFunctionAvailable("getPlaybackBeat")
      ? "getPlaybackBeat"
      : nativeFunctionAvailable("getLoopPlaybackBeat")
        ? "getLoopPlaybackBeat"
        : null;

    if (beatNativeName) {
      const getPlaybackBeat = getNativeFunction(beatNativeName);

      try {
        const result = await getPlaybackBeat();
        const beat = Number.parseFloat(String(result));
        playbackBeat = Number.isNaN(beat) ? -1 : beat;
      } catch {
        playbackBeat = -1;
      }
    } else {
      playbackBeat = -1;
    }

    updateActiveGatesForBeat(playbackBeat);

    await pollCurrentSlotState();
    await pollRowRecordingNotes();

    playbackPollFrameId = requestAnimationFrame(pollPlaybackActivity);
  }

  async function pollCurrentSlotState() {
    if (!nativeFunctionAvailable("getCurrentSlotState")) return;
    if (slotSelectionInFlight > 0) return;

    try {
      const state = await getNativeFunction("getCurrentSlotState")();

      if (!state || typeof state !== "object") return;

      const nextPatternSlot = Number.parseInt(String(state.currentPatternSlot ?? activePatternSlot), 10);
      const nextViewPatternSlot = Number.parseInt(String(state.viewPatternSlot ?? viewPatternSlot), 10);
      const nextLoopSlot = Number.parseInt(String(state.currentLoopSlot ?? -1), 10);
      const clampedPatternSlot = Number.isNaN(nextPatternSlot)
        ? activePatternSlot
        : nextPatternSlot >= -1 && nextPatternSlot < 8
          ? nextPatternSlot
          : activePatternSlot;
      const clampedViewPatternSlot = Number.isNaN(nextViewPatternSlot)
        ? viewPatternSlot
        : Math.min(7, Math.max(0, nextViewPatternSlot));
      const clampedLoopSlot = Number.isNaN(nextLoopSlot) || nextLoopSlot < 0 || nextLoopSlot >= 8 ? -1 : nextLoopSlot;

      viewPatternSlot = clampedViewPatternSlot;

      if (clampedPatternSlot !== activePatternSlot) {
        activePatternSlot = clampedPatternSlot;
        activeLoopSlot = clampedLoopSlot;

        if (nativeFunctionAvailable("getPatternSlotState")) {
          const patternSlotForGrid =
            clampedPatternSlot >= 0 ? clampedPatternSlot : clampedViewPatternSlot;
          const patternState = await getNativeFunction("getPatternSlotState")(patternSlotForGrid);
          assignPatternState(patternState, clampedPatternSlot >= 0);
        }

        if (clampedPatternSlot < 0) loopBraceEnabled = false;

        return;
      }

      const loopSlotChanged = clampedLoopSlot !== activeLoopSlot;
      activeLoopSlot = clampedLoopSlot;

      if (
        clampedLoopSlot >= 0 &&
        loopSlotChanged &&
        nativeFunctionAvailable("getPatternSlotState")
      ) {
        const patternSlotForGrid =
          activePatternSlot >= 0 ? activePatternSlot : clampedViewPatternSlot;
        const patternState = await getNativeFunction("getPatternSlotState")(patternSlotForGrid);
        assignPatternState(patternState, activePatternSlot >= 0);
      }
    } catch {
      // Native bridge unavailable during teardown.
    }
  }

  function loadPulseFromInitialisation() {
    const init = unwrapJuceInit("pulseIndex");

    if (init === null) return;

    const raw = Array.isArray(init) ? init[0] : init;
    const value = Number.parseInt(String(raw), 10);

    if (!Number.isNaN(value)) {
      pulseIndex = Math.min(pulseOptions.length - 1, Math.max(0, value));
    }
  }

  async function applyPulseIndex(nextIndex) {
    await commitHistory("Change pulse", async () => {
      pulseIndex = Math.min(pulseOptions.length - 1, Math.max(0, nextIndex));

      if (!nativeFunctionAvailable("setPulseIndex")) return;

      const result = await getNativeFunction("setPulseIndex")(pulseIndex);
      const confirmed = Number.parseInt(String(result), 10);

      if (!Number.isNaN(confirmed)) {
        pulseIndex = Math.min(pulseOptions.length - 1, Math.max(0, confirmed));
      }
    });
  }

  function clampPercent(value) {
    const parsed = Number.parseInt(String(value), 10);

    return Number.isNaN(parsed) ? 0 : Math.min(100, Math.max(0, parsed));
  }

  function loadHumanizeControlsFromInitialisation() {
    const swingInit = unwrapJuceInit("swingPercent");
    const velocityInit = unwrapJuceInit("velocityHumanizePercent");
    const timingInit = unwrapJuceInit("timingHumanizePercent");
    const subdivisionInit = unwrapJuceInit("swingSubdivisionIndex");

    if (swingInit !== null) swingPercent = clampPercent(Array.isArray(swingInit) ? swingInit[0] : swingInit);
    if (velocityInit !== null)
      velocityHumanizePercent = clampPercent(Array.isArray(velocityInit) ? velocityInit[0] : velocityInit);
    if (timingInit !== null)
      timingHumanizePercent = clampPercent(Array.isArray(timingInit) ? timingInit[0] : timingInit);

    if (subdivisionInit !== null) {
      const raw = Array.isArray(subdivisionInit) ? subdivisionInit[0] : subdivisionInit;
      const value = Number.parseInt(String(raw), 10);

      if (!Number.isNaN(value)) {
        swingSubdivisionIndex = Math.min(swingSubdivisionValues.length - 1, Math.max(0, value));
      }
    }
  }

  async function applyGlobalPercent(next, nativeName, assign) {
    await commitHistory("Change percent", async () => {
      const clamped = clampPercent(next);
      assign(clamped);

      if (!nativeFunctionAvailable(nativeName)) return;

      const result = await getNativeFunction(nativeName)(clamped);
      const confirmed = clampPercent(result);
      assign(confirmed);
    });
  }

  async function applySwingSubdivisionIndex(nextIndex) {
    await commitHistory("Change swing subdivision", async () => {
      swingSubdivisionIndex = Math.min(swingSubdivisionValues.length - 1, Math.max(0, nextIndex));

      if (!nativeFunctionAvailable("setSwingSubdivisionIndex")) return;

      const result = await getNativeFunction("setSwingSubdivisionIndex")(swingSubdivisionIndex);
      const confirmed = Number.parseInt(String(result), 10);

      if (!Number.isNaN(confirmed)) {
        swingSubdivisionIndex = Math.min(swingSubdivisionValues.length - 1, Math.max(0, confirmed));
      }
    });
  }

  function loadLoopBraceFromInitialisation() {
    const enabledInit = unwrapJuceInit("loopBraceEnabled");

    if (enabledInit !== null) {
      const raw = Array.isArray(enabledInit) ? enabledInit[0] : enabledInit;
      loopBraceEnabled = Boolean(Number.parseInt(String(raw), 10));
    }

    const startInit = unwrapJuceInit("loopBraceStart");

    if (startInit !== null) {
      const raw = Array.isArray(startInit) ? startInit[0] : startInit;
      const value = Number.parseFloat(String(raw));

      if (!Number.isNaN(value)) loopBraceStart = Math.max(0, value);
    }

    const endInit = unwrapJuceInit("loopBraceEnd");

    if (endInit !== null) {
      const raw = Array.isArray(endInit) ? endInit[0] : endInit;
      const value = Number.parseFloat(String(raw));

      if (!Number.isNaN(value)) loopBraceEnd = Math.max(loopBraceStart + 0.5, value);
    }
  }

  function loadRowColorsFromInitialisation() {
    const init = unwrapJuceInit("rowColorsEnabled");

    rowColorsEnabled = init === true || init === 1 || init === "1";
  }

  async function pushLoopBraceEnabled(enabled) {
    if (!nativeFunctionAvailable("setLoopBraceEnabled")) return;

    const setLoopBraceEnabled = getNativeFunction("setLoopBraceEnabled");
    await setLoopBraceEnabled(enabled ? 1 : 0);
  }

  async function pushLoopBraceStart(start) {
    if (!nativeFunctionAvailable("setLoopBraceStart")) return;

    const setLoopBraceStart = getNativeFunction("setLoopBraceStart");
    await setLoopBraceStart(start);
  }

  async function pushLoopBraceEnd(end) {
    if (!nativeFunctionAvailable("setLoopBraceEnd")) return;

    const setLoopBraceEnd = getNativeFunction("setLoopBraceEnd");
    await setLoopBraceEnd(end);
  }

  /** @param {{ enabled?: boolean, start?: number, end?: number }} next */
  async function updateLoopBrace(next) {
    await commitHistory("Change loop brace", async () => {
      const previousStart = loopBraceStart;
      const nextStart = next.start ?? loopBraceStart;
      const nextEnd = next.end ?? loopBraceEnd;

      if (next.enabled !== undefined) {
        loopBraceEnabled = next.enabled;
        await pushLoopBraceEnabled(next.enabled);
      }

      loopBraceStart = nextStart;
      loopBraceEnd = nextEnd;

      if (next.start !== undefined && next.end !== undefined) {
        if (nextStart > previousStart) {
          await pushLoopBraceEnd(nextEnd);
          await pushLoopBraceStart(nextStart);
        } else {
          await pushLoopBraceStart(nextStart);
          await pushLoopBraceEnd(nextEnd);
        }
        return;
      }

      if (next.start !== undefined) await pushLoopBraceStart(nextStart);
      if (next.end !== undefined) await pushLoopBraceEnd(nextEnd);
    });
  }

  function loadStandaloneTransportFromInitialisation() {
    const available = unwrapJuceInit("standaloneTransportAvailable");
    const playing = unwrapJuceInit("standaloneTransportPlaying");
    const tempo = unwrapJuceInit("standaloneTempoBpm");

    standaloneTransportAvailable = Boolean(Number.parseInt(String(available ?? 0), 10));
    standalonePlaying = Boolean(Number.parseInt(String(playing ?? 0), 10));

    const parsedTempo = Number.parseFloat(String(tempo ?? 120));
    if (!Number.isNaN(parsedTempo)) standaloneTempoBpm = Math.min(300, Math.max(20, parsedTempo));
  }

  function loadSlotStateFromInitialisation() {
    const patternInit = unwrapJuceInit("currentPatternSlot");
    const loopInit = unwrapJuceInit("currentLoopSlot");
    const assignedInit = unwrapJuceInit("loopSlotAssigned");
    const patternRefInit = unwrapJuceInit("loopSlotPattern");

    const viewInit = unwrapJuceInit("viewPatternSlot");

    if (viewInit !== null) {
      const raw = Array.isArray(viewInit) ? viewInit[0] : viewInit;
      const value = Number.parseInt(String(raw), 10);
      if (!Number.isNaN(value)) viewPatternSlot = Math.min(7, Math.max(0, value));
    }

    if (patternInit !== null) {
      const raw = Array.isArray(patternInit) ? patternInit[0] : patternInit;
      const value = Number.parseInt(String(raw), 10);
      if (!Number.isNaN(value) && value >= -1 && value < 8) activePatternSlot = value;
    }

    if (loopInit !== null) {
      const raw = Array.isArray(loopInit) ? loopInit[0] : loopInit;
      const value = Number.parseInt(String(raw), 10);
      if (!Number.isNaN(value)) activeLoopSlot = value >= 0 && value < 8 ? value : -1;
    }

    if (Array.isArray(assignedInit)) {
      loopSlotAssigned = Array.from({ length: 8 }, (_, index) =>
        Boolean(Number.parseInt(String(assignedInit[index] ?? 0), 10)),
      );
    }

    if (Array.isArray(patternRefInit)) {
      loopSlotPattern = Array.from({ length: 8 }, (_, index) => {
        const value = Number.parseInt(String(patternRefInit[index] ?? 0), 10);
        return Number.isNaN(value) ? 0 : Math.min(7, Math.max(0, value));
      });
    }
  }

  async function toggleStandaloneTransport() {
    if (!nativeFunctionAvailable("setStandaloneTransportPlaying")) return;

    const nextPlaying = !standalonePlaying;
    standalonePlaying = nextPlaying;

    await getNativeFunction("setStandaloneTransportPlaying")(nextPlaying ? 1 : 0);
  }

  async function setStandaloneTempoFromInput(event) {
    const nextTempo = Number.parseFloat(event.currentTarget.value);
    if (Number.isNaN(nextTempo)) return;

    standaloneTempoBpm = Math.min(300, Math.max(20, nextTempo));

    if (!nativeFunctionAvailable("setStandaloneTempoBpm")) return;

    const result = await getNativeFunction("setStandaloneTempoBpm")(standaloneTempoBpm);
    const confirmedTempo = Number.parseFloat(String(result));
    if (!Number.isNaN(confirmedTempo)) standaloneTempoBpm = confirmedTempo;
  }

  async function selectPatternSlot(slot) {
    const nextSlot = Math.min(7, Math.max(0, slot));
    const keepActiveLoop =
      activeLoopSlot >= 0 &&
      loopSlotAssigned[activeLoopSlot] &&
      loopSlotPattern[activeLoopSlot] === nextSlot;

    activePatternSlot = nextSlot;

    if (!keepActiveLoop) {
      activeLoopSlot = -1;
      loopBraceEnabled = false;
    }

    if (!nativeFunctionAvailable("setCurrentPatternSlot")) return;

    slotSelectionInFlight += 1;

    try {
      const state = await getNativeFunction("setCurrentPatternSlot")(nextSlot);
      assignPatternState(state);
    } finally {
      slotSelectionInFlight = Math.max(0, slotSelectionInFlight - 1);
    }
  }

  function beginPatternCopy(slot) {
    patternCopySource = Math.min(7, Math.max(0, slot));
  }

  function cancelPatternCopy() {
    patternCopySource = -1;
  }

  async function copyPatternToSlot(slot) {
    const source = patternCopySource;
    const nextSlot = Math.min(7, Math.max(0, slot));

    if (source < 0 || source === nextSlot || !nativeFunctionAvailable("copyPatternSlot")) return;

    await getNativeFunction("copyPatternSlot")(source, nextSlot);
    patternCopySource = -1;
    await selectPatternSlot(nextSlot);
  }

  async function handlePatternSlotClick(event, slot) {
    const nextSlot = Math.min(7, Math.max(0, slot));

    if (event.shiftKey) {
      if (patternCopySource === nextSlot) {
        cancelPatternCopy();
      } else {
        beginPatternCopy(nextSlot);
      }
      return;
    }

    if (patternCopySource >= 0) {
      await copyPatternToSlot(nextSlot);
      return;
    }

    await selectPatternSlot(nextSlot);
  }

  async function deactivateOutput() {
    if (!nativeFunctionAvailable("deactivatePatternOutput")) return;

    cancelPatternCopy();
    activePatternSlot = -1;
    activeLoopSlot = -1;
    loopBraceEnabled = false;

    slotSelectionInFlight += 1;

    try {
      const slotState = await getNativeFunction("deactivatePatternOutput")();
      const nextView = Number.parseInt(String(slotState?.viewPatternSlot ?? viewPatternSlot), 10);

      if (!Number.isNaN(nextView)) viewPatternSlot = Math.min(7, Math.max(0, nextView));

      if (nativeFunctionAvailable("getPatternSlotState")) {
        const patternState = await getNativeFunction("getPatternSlotState")(viewPatternSlot);
        assignPatternState(patternState, false);
      }
    } finally {
      slotSelectionInFlight = Math.max(0, slotSelectionInFlight - 1);
    }
  }

  async function clearSelectedPatternSlot() {
    if (!nativeFunctionAvailable("clearPatternSlot")) return;

    cancelPatternCopy();
    const slot = activePatternSlot >= 0 ? activePatternSlot : viewPatternSlot;
    const state = await getNativeFunction("clearPatternSlot")(slot);
    assignPatternState(state, activePatternSlot >= 0);
  }

  /** @param {number} slot */
  async function activateLoopSlot(slot) {
    if (!loopSlotAssigned[slot] || !nativeFunctionAvailable("selectLoopSlot")) return;

    activeLoopSlot = slot;
    activePatternSlot = loopSlotPattern[slot] ?? activePatternSlot;
    slotSelectionInFlight += 1;

    try {
      const state = await getNativeFunction("selectLoopSlot")(slot);
      assignPatternState(state);
    } finally {
      slotSelectionInFlight = Math.max(0, slotSelectionInFlight - 1);
    }
  }

  async function handleLoopSlotClick(event, slot) {
    const nextSlot = Math.min(7, Math.max(0, slot));
    cancelPatternCopy();

    if (event.shiftKey) {
      if (!nativeFunctionAvailable("saveCurrentBraceToLoopSlot")) return;

      activeLoopSlot = nextSlot;
      slotSelectionInFlight += 1;

      try {
        const state = await getNativeFunction("saveCurrentBraceToLoopSlot")(nextSlot);
        loopSlotAssigned[nextSlot] = true;
        assignPatternState(state);
        loopSlotPattern[nextSlot] = activePatternSlot >= 0 ? activePatternSlot : viewPatternSlot;
      } finally {
        slotSelectionInFlight = Math.max(0, slotSelectionInFlight - 1);
      }

      return;
    }

    await activateLoopSlot(nextSlot);
  }

  function loadInitialStateFromJuce() {
    loadGridFromInitialisation();
    loadRowMutedFromInitialisation();
    loadRowTimingOffsetFromInitialisation();
    loadRowMidiChannelFromInitialisation();
    loadStepDurationFromInitialisation();
    loadStepTimingMultiplierFromInitialisation();
    loadStepVelocityFromInitialisation();
    loadStepMutedFromInitialisation();
    loadStepSkippedFromInitialisation();
    loadStepProbabilityFromInitialisation();
    loadStepCycleFromInitialisation();
    loadStepCycleOffsetFromInitialisation();
    loadPulseFromInitialisation();
    loadHumanizeControlsFromInitialisation();
    loadLoopBraceFromInitialisation();
    loadRowColorsFromInitialisation();
    loadStandaloneTransportFromInitialisation();
    loadSlotStateFromInitialisation();
  }

  loadInitialStateFromJuce();

  onMount(() => {
    const handleKeydown = (event) => {
      if (!event.metaKey || event.altKey || event.ctrlKey) return;
      if (event.key.toLowerCase() !== "z") return;

      event.preventDefault();

      if (event.shiftKey) {
        redo();
      } else {
        undo();
      }
    };

    window.addEventListener("keydown", handleKeydown);
    playbackPollFrameId = requestAnimationFrame(pollPlaybackActivity);

    return () => {
      window.removeEventListener("keydown", handleKeydown);
      cancelAnimationFrame(playbackPollFrameId);
      document.removeEventListener("pointermove", updateMarqueePointer);
      document.removeEventListener("pointerup", finishMarqueeSelection);
      document.removeEventListener("pointercancel", cancelMarqueeSelection);
    };
  });
</script>

<main class="flex h-full flex-col overflow-hidden p-6">
  <div class="shrink-0 -mx-6">
  <header class="flex items-end gap-6 px-6 pb-6">
    <div class="shrink-0 self-start">
      <div class="flex items-center gap-1.5">
        <p class="text-xs font-medium uppercase tracking-widest text-emerald-400">ofsound</p>
        <ColorsToggle
          accent={emeraldRowAccent}
          enabled={rowColorsEnabled}
          onChange={async (next) => {
            rowColorsEnabled = next;
            await pushRowColorsEnabled();
          }}
        />
        <button
          type="button"
          aria-label={globalStepBackView
            ? "Show front of all steps"
            : "Show advanced settings for all steps"}
          aria-pressed={globalStepBackView}
          title={globalStepBackView
            ? "Show front of all steps"
            : "Show advanced settings for all steps"}
          disabled={selectableStepCount === 0}
          data-cursor="pointer"
          class={brandIconToggleButtonClasses(
            globalStepBackView,
            selectableStepCount > 0,
          )}
          onclick={toggleGlobalStepBackView}
        >
          <StepGearIcon class="pointer-events-none h-4 w-4" />
        </button>
      </div>
      <h1 class="whitespace-nowrap text-xl font-semibold tracking-tight text-zinc-100">
        {pluginName}
      </h1>
    </div>

    <div class="flex min-w-0 flex-1 flex-wrap items-end gap-x-6 gap-y-3">
        <div class="flex flex-col items-start gap-1">
          <span class="text-xs font-semibold leading-none text-zinc-500">Pulse</span>
          <PulseNoteButtonGroup
            accent={emeraldRowAccent}
            value={pulseIndex}
            onValueChange={applyPulseIndex}
          />
        </div>
        <div class="flex items-end">
          <div class="flex items-end gap-3">
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-zinc-500">Swing</span>
              <StepNumberDragInput
                boxed
                compact
                accent={emeraldRowAccent}
                value={swingPercent}
                min={0}
                max={100}
                resetValue={0}
                ariaLabel="Swing"
                onValueChange={(value) =>
                  applyGlobalPercent(value, "setSwingPercent", (next) => {
                    swingPercent = next;
                  })}
              />
            </div>
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-zinc-500">Sub</span>
              <DiscreteDragSelect
                compact
                accent={emeraldRowAccent}
                options={swingSubdivisionOptions}
                value={swingSubdivisionIndex}
                resetValue={1}
                ariaLabel="Swing subdivision"
                onValueChange={applySwingSubdivisionIndex}
              />
            </div>
          </div>
          <div class="w-5 shrink-0" aria-hidden="true"></div>
          <div class="flex items-end gap-3">
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-zinc-500">Vel %</span>
            <StepNumberDragInput
              boxed
              compact
              accent={emeraldRowAccent}
              value={velocityHumanizePercent}
              min={0}
              max={100}
              resetValue={0}
              ariaLabel="Velocity humanize percent"
              onValueChange={(value) =>
                applyGlobalPercent(value, "setVelocityHumanizePercent", (next) => {
                  velocityHumanizePercent = next;
                })}
            />
          </div>
          <div class="flex flex-col items-start gap-1">
            <span class="text-xs font-semibold leading-none text-zinc-500">Time %</span>
            <StepNumberDragInput
              boxed
              compact
              accent={emeraldRowAccent}
              value={timingHumanizePercent}
              min={0}
              max={100}
              resetValue={0}
              ariaLabel="Timing humanize percent"
              onValueChange={(value) =>
                applyGlobalPercent(value, "setTimingHumanizePercent", (next) => {
                  timingHumanizePercent = next;
                })}
            />
          </div>
          </div>
        </div>
        <div class="flex items-end gap-3 border-l border-zinc-800 pl-5">
          <div class="flex flex-col items-start gap-1">
            <span class="text-xs font-semibold leading-none text-zinc-500">Steps</span>
            <button
              type="button"
              aria-label="Select all steps for bulk editing"
              aria-pressed={allStepsSelected}
              title="Select all steps"
              data-cursor="pointer"
              class={bulkSelectButtonClasses(
                allStepsSelected,
                selectableStepCount > 0,
              )}
              onclick={selectAllStepsForBulkEdit}
            >
              All
            </button>
          </div>
          <div class="flex flex-col items-start gap-1">
            <span class="text-xs font-semibold leading-none text-zinc-500">Ops</span>
            <div class="flex items-center gap-1">
              <button
                type="button"
                aria-label="Reverse selected steps by row"
                title="Reverse selected steps by row"
                disabled={!selectedStepReverseAvailable}
                data-cursor="pointer"
                class={bulkActionIconButtonClasses(selectedStepReverseAvailable)}
                onclick={reverseSelectedStepsByRow}
              >
                <RowReverseOrderIcon class="pointer-events-none h-5 w-5" />
              </button>
              <button
                type="button"
                aria-label="Shuffle selected steps"
                title="Shuffle selected steps across rows"
                disabled={selectedStepCount <= 1}
                data-cursor="pointer"
                class={bulkActionIconButtonClasses(selectedStepCount > 1)}
                onclick={shuffleSelectedSteps}
              >
                <RowRandomizeOrderIcon class="pointer-events-none h-5 w-5" />
              </button>
              <button
                type="button"
                aria-label="Randomize selected step octaves"
                title="Randomize selected step octaves"
                disabled={selectedStepCount === 0}
                data-cursor="pointer"
                class={bulkActionIconButtonClasses(selectedStepCount > 0)}
                onclick={randomizeSelectedStepOctaves}
              >
                <RowRandomizeOctaveIcon class="pointer-events-none h-5 w-5" />
              </button>
            </div>
          </div>
          <div class="flex flex-col items-start gap-1">
            <span class="text-xs font-semibold leading-none text-zinc-500">Dur %</span>
            <StepNumberDragInput
              boxed
              compact
              deferCommit
              accent={emeraldRowAccent}
              value={bulkDurationPercent}
              min={0}
              max={100}
              resetValue={100}
              ariaLabel="Bulk step duration percent"
              disabled={selectedStepCount === 0}
              onGestureStart={beginBulkEditGesture}
              onValuePreview={previewBulkDurationPercent}
              onValueCommit={commitBulkDurationPercent}
            />
          </div>
          <div class="flex flex-col items-start gap-1">
            <span class="text-xs font-semibold leading-none text-zinc-500">Vel %</span>
            <StepNumberDragInput
              boxed
              compact
              deferCommit
              accent={emeraldRowAccent}
              value={bulkVelocityPercent}
              min={0}
              max={100}
              resetValue={100}
              ariaLabel="Bulk step velocity percent"
              disabled={selectedStepCount === 0}
              onGestureStart={beginBulkEditGesture}
              onValuePreview={previewBulkVelocityPercent}
              onValueCommit={commitBulkVelocityPercent}
            />
          </div>
          <div class="flex flex-col items-start gap-1">
            <span class="text-xs font-semibold leading-none text-zinc-500">Trans</span>
            <StepNumberDragInput
              boxed
              compact
              deferCommit
              accent={emeraldRowAccent}
              value={bulkTransposeSemitones}
              min={-48}
              max={48}
              resetValue={0}
              formatValue={formatSemitoneValue}
              ariaLabel="Bulk step transpose semitones"
              disabled={selectedStepCount === 0}
              onGestureStart={beginBulkEditGesture}
              onValuePreview={previewBulkTransposeSemitones}
              onValueCommit={commitBulkTransposeSemitones}
            />
          </div>
        </div>
        <div class="flex items-center gap-1">
          <button
            type="button"
            aria-label="Undo"
            title="Undo"
            disabled={undoStack.length === 0}
            data-cursor="pointer"
            class={historyButtonClasses(undoStack.length > 0)}
            onclick={undo}
          >
            <svg
              class="h-4 w-4"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <path d="M9 14 4 9l5-5" />
              <path d="M4 9h10a6 6 0 0 1 0 12h-2" />
            </svg>
          </button>
          <button
            type="button"
            aria-label="Redo"
            title="Redo"
            disabled={redoStack.length === 0}
            data-cursor="pointer"
            class={historyButtonClasses(redoStack.length > 0)}
            onclick={redo}
          >
            <svg
              class="h-4 w-4"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              stroke-width="2"
              stroke-linecap="round"
              stroke-linejoin="round"
              aria-hidden="true"
            >
              <path d="m15 14 5-5-5-5" />
              <path d="M20 9H10a6 6 0 0 0 0 12h2" />
            </svg>
          </button>
        </div>
    </div>

    <div class="flex shrink-0 items-end gap-3">
      <div
        class="grid grid-cols-[4rem_auto_auto_auto] items-center gap-x-2 gap-y-1.5"
      >
        <span class="text-right text-xs font-semibold leading-none text-zinc-500">Patterns</span>
        <div class="flex items-center gap-1">
          {#each Array.from({ length: 8 }, (_, index) => index) as slot (slot)}
            <button
              type="button"
              aria-label={patternCopySource === slot
                ? `Pattern ${slot + 1} selected as copy source`
                : `Select pattern ${slot + 1}`}
              aria-pressed={activePatternSlot === slot}
              title={patternCopySource === slot
                ? "Copy source selected"
                : "Shift-click to copy from this pattern"}
              data-cursor="pointer"
              class={slotButtonClasses(activePatternSlot === slot, true, patternCopySource === slot)}
              onclick={(event) => handlePatternSlotClick(event, slot)}
            >
              {slot + 1}
            </button>
          {/each}
        </div>
        <button
          type="button"
          aria-label="Clear selected pattern"
          title="Clear pattern shown in the grid"
          data-cursor="pointer"
          class={clearPatternButtonClasses(true)}
          onclick={clearSelectedPatternSlot}
        >
          <RemoveXIcon class="pointer-events-none h-3 w-3" />
        </button>
        <button
          type="button"
          aria-label="Mute output"
          aria-pressed={activePatternSlot < 0}
          title="Mute output until a pattern or loop is selected (MIDI note 16)"
          data-cursor="pointer"
          class={outputMuteButtonClasses(activePatternSlot < 0)}
          onclick={deactivateOutput}
        >
          M
        </button>

        <span class="text-right text-xs font-semibold leading-none text-zinc-500">Loops</span>
        <div class="flex items-center gap-1">
          {#each Array.from({ length: 8 }, (_, index) => index) as slot (slot)}
            <button
              type="button"
              aria-label={loopSlotAssigned[slot]
                ? `Select loop ${slot + 1}`
                : `Save current brace to loop ${slot + 1}`}
              aria-pressed={activeLoopSlot === slot}
              title="Click to select, Shift-click to save current brace"
              data-cursor="pointer"
              class={slotButtonClasses(activeLoopSlot === slot, loopSlotAssigned[slot])}
              onclick={(event) => handleLoopSlotClick(event, slot)}
            >
              {slot + 1}
            </button>
          {/each}
        </div>
        <div aria-hidden="true"></div>
      </div>
      {#if standaloneTransportAvailable}
        <div class="flex items-center gap-2">
          <button
            type="button"
            aria-label={standalonePlaying ? "Stop standalone transport" : "Start standalone transport"}
            aria-pressed={standalonePlaying}
            data-cursor="pointer"
            class="h-8 min-w-16 rounded-md border px-3 text-sm font-semibold transition-colors outline-none focus:ring-1 focus:ring-emerald-400 {standalonePlaying
              ? 'border-emerald-500 bg-emerald-500 text-zinc-950'
              : 'border-zinc-700 bg-zinc-900 text-zinc-200 hover:border-zinc-600'}"
            onclick={toggleStandaloneTransport}
          >
            {standalonePlaying ? "Stop" : "Play"}
          </button>
          <label class="flex items-center gap-1.5 text-xs font-medium uppercase text-zinc-500">
            BPM
            <input
              type="number"
              min="20"
              max="300"
              step="1"
              value={Math.round(standaloneTempoBpm)}
              class="h-8 w-[4.5rem] rounded-md border border-zinc-700 bg-zinc-950 px-2 text-sm font-semibold text-zinc-100 outline-none focus:border-emerald-500 focus:ring-1 focus:ring-emerald-500"
              onchange={setStandaloneTempoFromInput}
            />
          </label>
        </div>
      {/if}
    </div>
  </header>
  <div class="h-0.5 w-full bg-zinc-500/40" role="separator" aria-hidden="true"></div>
  </div>

  <section class="mt-4 flex min-h-0 flex-1 flex-col overflow-hidden">
    <div class="w-full shrink-0">
      <div class="relative flex flex-col gap-5">
        <div
          class="pointer-events-none absolute top-0 bottom-0 z-0 w-px bg-zinc-600/70"
          style:left="{phraseBeatGuideGlobalLeftPx(phraseVisualOffsetCompensationPx)}px"
          aria-hidden="true"
          title="Beat one"
        ></div>
        {#each grid as _row, row (row)}
          {@const rowAccent = rowAccentFor(row, rowColorsEnabled)}
          <div class="relative z-10 flex min-w-0 flex-1 items-center gap-1">
            <div class="relative flex shrink-0 items-center gap-1">
              {#if rowMuted[row]}
                <div class={rowMutedOverlayClasses} aria-hidden="true"></div>
              {/if}
              <button
                type="button"
                aria-label={rowMuted[row] ? "Turn row on" : "Turn row off"}
                aria-pressed={!rowMuted[row]}
                data-cursor="pointer"
                class="{rowMuteControlClasses} {rowMuted[row]
                  ? rowPowerToggleOffClasses
                  : rowAccent.textAccent}"
                onclick={(event) => toggleRowMute(row, event.shiftKey)}
                title="Shift-click to solo row"
              >
                <RowDisableIcon class="h-9 w-9" />
              </button>
              <button
                type="button"
                aria-label={recordingRow === row ? "Stop row recording" : "Record row from MIDI"}
                aria-pressed={recordingRow === row}
                data-cursor="pointer"
                class="{rowActionIconControlClasses} {rowMuted[row]
                  ? 'text-red-950'
                  : recordingRow === row
                    ? 'text-red-400'
                    : 'text-red-500 hover:text-red-400'}"
                onclick={() => toggleRowRecording(row)}
                title={recordingRow === row
                  ? "Stop recording (notes fill this row as 1× steps)"
                  : "Record row from MIDI keyboard (first note replaces row)"}
              >
                <RowRecordIcon
                  class="pointer-events-none h-9 w-9"
                  recording={recordingRow === row}
                />
              </button>
              <BipolarKnob
                accent={rowAccent}
                options={timingOffsetOptions}
                value={rowTimingOffset[row]}
                resetValue={defaultRowTimingOffsetIndex}
                ariaLabel="Row timing offset"
                muted={rowMuted[row]}
                onValueChange={(offsetIndex) => selectRowTimingOffset(row, offsetIndex)}
              />
            </div>
            <PhraseRow
              {row}
              muted={rowMuted[row]}
              accent={rowAccent}
              timingOffsetIndex={rowTimingOffset[row]}
              timingOffsetVisualCompensationPx={phraseVisualOffsetCompensationPx}
              {pulseIndex}
              stepIds={stepIds[row]}
              notes={grid[row]}
              stepDurationFraction={stepDurationFraction[row]}
              stepTimingMultiplier={stepTimingMultiplier[row]}
              stepVelocity={stepVelocity[row]}
              stepMuted={stepMuted[row]}
              stepSkipped={stepSkipped[row]}
              stepProbability={stepProbability[row]}
              stepCycle={stepCycle[row]}
              stepCycleOffset={stepCycleOffset[row]}
              activeGates={activeGates[row]}
              globalStepBackView={globalStepBackView}
              selectedStepIds={selectedStepIdsByRow[row]}
              {timingMultiplierOptions}
              onReorder={reorderRowByIds}
              onMoveCommitted={commitRowMove}
              onRemoveStep={removeStep}
              onInsertStep={insertStep}
              onDuplicateStep={duplicateStep}
              onNoteChange={setPhraseNoteValue}
              onMultiplierChange={selectStepTimingMultiplier}
              onDurationChange={selectStepDurationFraction}
              onVelocityChange={setStepVelocity}
              onStepMuteChange={setStepMuted}
              onStepSkipChange={setStepSkipped}
              onStepProbabilityChange={setStepProbability}
              onStepCycleChange={setStepCycle}
              onStepCycleOffsetChange={setStepCycleOffset}
              onBulkSelectPointerDown={beginStepMarqueeSelection}
              onBulkSelectBackgroundDoubleClick={selectAllStepsForBulkEdit}
            />
          </div>
        {/each}
      </div>
    </div>

    {#if recordingRow !== null}
      <RecordPianoKeyboard
        row={recordingRow}
        accent={rowAccentFor(recordingRow, rowColorsEnabled)}
        heldKeys={recordingKeysHeld}
        onNotePress={onRecordPianoNotePress}
      />
    {:else}
      <PianoRollPreview
        notes={grid}
        {rowColorsEnabled}
        {rowMuted}
        {rowTimingOffset}
        {stepDurationFraction}
        {stepTimingMultiplier}
        {stepVelocity}
        {stepMuted}
        {stepSkipped}
        stepProbability={stepProbability}
        stepCycle={stepCycle}
        stepCycleOffset={stepCycleOffset}
        {pulseIndex}
        {swingPercent}
        {swingSubdivisionIndex}
        loopEnabled={loopBraceEnabled}
        loopStart={loopBraceStart}
        loopEnd={loopBraceEnd}
        {playbackBeat}
        onLoopBraceChange={updateLoopBrace}
      />
    {/if}
  </section>
  {#if marqueeSelection}
    <div
      class="pointer-events-none fixed z-[9999] rounded-sm border border-emerald-300 bg-emerald-300/15 shadow-[0_0_0_1px_rgba(16,185,129,0.25)]"
      style={marqueeRectStyle}
      aria-hidden="true"
    ></div>
  {/if}
</main>
