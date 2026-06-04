<script>
  import { onMount } from "svelte";
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
  import BipolarKnob from "./BipolarKnob.svelte";
  import MidiChannelStepper from "./MidiChannelStepper.svelte";
  import PhraseRow from "./PhraseRow.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import PianoRollPreview from "./PianoRollPreview.svelte";
  import RecordPianoKeyboard from "./RecordPianoKeyboard.svelte";
  import {
    defaultStepTimingMultiplierIndex,
    maxPhraseStepsPerRow,
    findSingleMove,
    phraseFullestRowReference,
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

  let pluginName = "MIDI Phrases";
  let version = "0.0.1";
  let grid = defaultPhraseGrid();
  /** @type {boolean[]} */
  let rowMuted = [false, false, false, false];
  let soloRow = -1;
  /** @type {boolean[] | null} */
  let rowSoloRestoreMuted = null;
  /** @type {number[]} */
  let rowTimingOffset = [
    defaultRowTimingOffsetIndex,
    defaultRowTimingOffsetIndex,
    defaultRowTimingOffsetIndex,
    defaultRowTimingOffsetIndex,
  ];
  /** @type {number[]} */
  let rowMidiChannel = [1, 1, 1, 1];
  /** @type {number[][]} */
  let stepDurationFraction = defaultStepDurationGrid();
  /** @type {number[][]} */
  let stepTimingMultiplier = defaultStepTimingMultiplierGrid();
  /** @type {number[][]} */
  let stepVelocity = defaultStepVelocityGrid();
  /** @type {boolean[][]} */
  let stepMuted = defaultStepMutedGrid();
  /** @type {boolean[][]} */
  let stepSkipped = defaultStepSkippedGrid();
  /** @type {number[][]} */
  let stepProbability = defaultStepProbabilityGrid();
  /** @type {number[][]} */
  let stepCycle = defaultStepCycleGrid();
  /** @type {number[][]} */
  let stepCycleOffset = defaultStepCycleOffsetGrid();
  /** @type {boolean[][]} */
  let activeGates = defaultPhraseGrid().map((row) => row.map(() => false));
  /** @type {string[][]} */
  let stepIds = defaultPhraseGrid().map((row, rowIndex) =>
    row.map((_, step) => `step-${rowIndex}-${step}`),
  );

  let playbackPollFrameId = 0;
  let slotSelectionInFlight = 0;

  /** Row index armed for MIDI capture, or null. */
  let recordingRow = null;
  /** Snapshot taken when recording was armed (for undo / cancel). */
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let recordingHistoryBefore = null;
  /** Whether any note was captured in the current recording session. */
  let recordingCapturedNotes = false;
  /** Clears the row on the next captured note. */
  let recordingAwaitingFirstNote = false;
  /** @type {Set<number>} */
  let recordingKeysHeld = new Set();

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
  let loopBraceEnabled = false;
  let loopBraceStart = 0;
  let loopBraceEnd = 8;
  let playbackBeat = -1;
  let standaloneTransportAvailable = false;
  let standalonePlaying = false;
  let standaloneTempoBpm = 120;
  let activePatternSlot = 0;
  let patternCopySource = -1;
  let activeLoopSlot = -1;
  let loopSlotAssigned = Array.from({ length: 8 }, () => false);
  let loopSlotPattern = Array.from({ length: 8 }, () => 0);
  let pulseIndex = defaultPulseIndex;
  let swingPercent = 0;
  let velocityHumanizePercent = 0;
  let timingHumanizePercent = 0;
  let swingSubdivisionIndex = 1;
  let rowColorsEnabled = false;
  let undoStack = [];
  let redoStack = [];

  const historyLimit = 100;
  const historyButtonBaseClasses =
    "flex h-8 w-8 items-center justify-center rounded-md border bg-zinc-900 transition-colors outline-none focus:ring-1 focus:ring-emerald-400 disabled:cursor-default disabled:border-zinc-800 disabled:text-zinc-700";

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
    return `flex h-7 w-7 items-center justify-center rounded-sm border text-sm font-semibold transition-colors outline-none focus:ring-1 focus:ring-emerald-400 ${
      enabled
        ? "border-zinc-700 bg-zinc-900 text-zinc-400 hover:border-red-500 hover:text-red-300"
        : "border-zinc-800 bg-zinc-950 text-zinc-700"
    }`;
  }

  /** UI-only; shifts phrase rows and beat-one guide when any row has a negative offset. */
  $: phraseVisualOffsetCompensationPx = phraseGridVisualOffsetCompensationPx(
    rowTimingOffset,
    pulseIndex,
  );

  /** Fullest row for solo-step width (gap-compensated to match dense rows). */
  $: phraseReferenceRow = phraseFullestRowReference(stepTimingMultiplier);

  function createStepId() {
    const id = `step-${nextStepId}`;
    nextStepId += 1;
    return id;
  }

  function cloneMatrix(matrix) {
    return matrix.map((row) => [...row]);
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

  function assignPatternState(state) {
    if (!state || typeof state !== "object") return;

    const statePatternSlot = Number.parseInt(String(state.patternSlot ?? activePatternSlot), 10);
    if (!Number.isNaN(statePatternSlot)) {
      activePatternSlot = Math.min(7, Math.max(0, statePatternSlot));
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
    stepIds = grid.map((row, rowIndex) => row.map((_, step) => `step-${activePatternSlot}-${rowIndex}-${step}`));
    nextStepId = grid.reduce((count, row) => count + row.length, 0);
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

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
    stepMuted = stepMuted;
    stepSkipped = stepSkipped;
    stepProbability = stepProbability;
    stepCycle = stepCycle;
    stepCycleOffset = stepCycleOffset;
    activeGates = activeGates;
    stepIds = stepIds;
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

  $: {
    const nextActiveGates = grid.map((row) => row.map(() => false));

    if (playbackBeat >= 0) {
      for (let row = 0; row < grid.length; row += 1) {
        for (let step = 0; step < grid[row].length; step += 1) {
          nextActiveGates[row][step] = isStepActiveAtBeat({
            beat: playbackBeat,
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

    activeGates = nextActiveGates;
  }

  if (window.__JUCE__?.initialisationData?.pluginName?.[0]) {
    pluginName = window.__JUCE__.initialisationData.pluginName[0];
  }

  if (window.__JUCE__?.initialisationData?.version?.[0]) {
    version = window.__JUCE__.initialisationData.version[0];
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

    const next = [1, 1, 1, 1];

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

  async function pushReversePhraseRowSteps(row) {
    if (!nativeFunctionAvailable("reversePhraseRowSteps")) return;

    const reversePhraseRowSteps = getNativeFunction("reversePhraseRowSteps");
    await reversePhraseRowSteps(row);
  }

  async function pushReorderPhraseRowSteps(row, stepOrder) {
    if (!nativeFunctionAvailable("reorderPhraseRowSteps")) return;

    const reorderPhraseRowSteps = getNativeFunction("reorderPhraseRowSteps");
    await reorderPhraseRowSteps(row, stepOrder);
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
      grid = grid;
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

  async function reverseRowStepOrder(row) {
    if (!grid[row] || grid[row].length <= 1) return;

    await commitHistory("Reverse row", async () => {
      applyRowStepOrder(
        row,
        grid[row].map((_, step) => grid[row].length - 1 - step),
      );

      await pushReversePhraseRowSteps(row);
    });
  }

  function applyRowStepOrder(row, stepOrder) {
    const reorder = (values) => stepOrder.map((sourceStep) => values[sourceStep]);

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
    stepIds[row] = reorder(stepIds[row]);

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
    stepMuted = stepMuted;
    stepSkipped = stepSkipped;
    stepProbability = stepProbability;
    stepCycle = stepCycle;
    stepCycleOffset = stepCycleOffset;
    activeGates = activeGates;
    stepIds = stepIds;
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

  async function randomizeRowStepOrder(row) {
    if (!grid[row] || grid[row].length <= 1) return;

    await commitHistory("Randomize row", async () => {
      const stepOrder = randomStepOrder(grid[row].length);
      applyRowStepOrder(row, stepOrder);
      await pushReorderPhraseRowSteps(row, stepOrder);
    });
  }

  async function randomizeRowOctaves(row) {
    if (!grid[row] || grid[row].length === 0) return;

    await commitHistory("Randomize row octaves", async () => {
      grid[row] = grid[row].map((midi) => {
        const shift = Math.random() < 0.5 ? -12 : 12;
        return Math.min(127, Math.max(0, midi + shift));
      });
      grid = grid;
      await pushCurrentPhraseRow(row);
    });
  }

  async function selectRowTimingOffset(row, offsetIndex) {
    await commitHistory("Change row timing", async () => {
      rowTimingOffset[row] = offsetIndex;
      rowTimingOffset = rowTimingOffset;
      await pushRowTimingOffset(row);
    });
  }

  async function selectRowMidiChannel(row, channel) {
    await commitHistory("Change MIDI channel", async () => {
      rowMidiChannel[row] = Math.min(16, Math.max(1, channel));
      rowMidiChannel = rowMidiChannel;
      await pushRowMidiChannel(row);
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
      stepDurationFraction = stepDurationFraction;
      await pushStepDurationFraction(row, step);
    });
  }

  async function setStepVelocity(row, step, value) {
    await commitHistory("Change velocity", async () => {
      stepVelocity[row][step] = Math.min(127, Math.max(0, value));
      stepVelocity = stepVelocity;
      await pushStepVelocity(row, step);
    });
  }

  async function setStepMuted(row, step, muted) {
    await commitHistory("Toggle step mute", async () => {
      stepMuted[row][step] = muted;
      let clearedSkip = false;

      if (muted && stepSkipped[row][step]) {
        stepSkipped[row][step] = false;
        clearedSkip = true;
        stepSkipped = stepSkipped;
      }

      stepMuted = stepMuted;
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
        stepMuted = stepMuted;
      }

      stepSkipped = stepSkipped;
      await pushStepSkipped(row, step);

      if (clearedMute) {
        await pushStepMuted(row, step);
      }
    });
  }

  async function setStepProbability(row, step, probability) {
    await commitHistory("Change probability", async () => {
      stepProbability[row][step] = Math.min(100, Math.max(0, probability));
      stepProbability = stepProbability;
      await pushStepProbability(row, step);
    });
  }

  async function setStepCycle(row, step, cycle) {
    await commitHistory("Change cycle", async () => {
      const nextCycle = Math.min(64, Math.max(1, cycle));
      stepCycle[row][step] = nextCycle;
      stepCycleOffset[row][step] = Math.min(stepCycleOffset[row][step], nextCycle - 1);
      stepCycle = stepCycle;
      stepCycleOffset = stepCycleOffset;
      await pushStepCycle(row, step);
      await pushStepCycleOffset(row, step);
    });
  }

  async function setStepCycleOffset(row, step, cycleOffset) {
    await commitHistory("Change cycle offset", async () => {
      const maxOffset = Math.max(0, (stepCycle[row][step] ?? 1) - 1);
      stepCycleOffset[row][step] = Math.min(maxOffset, Math.max(0, cycleOffset));
      stepCycleOffset = stepCycleOffset;
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

      grid = grid;
      stepDurationFraction = stepDurationFraction;
      stepTimingMultiplier = stepTimingMultiplier;
      stepVelocity = stepVelocity;
      stepMuted = stepMuted;
      stepSkipped = stepSkipped;
      stepProbability = stepProbability;
      stepCycle = stepCycle;
      stepCycleOffset = stepCycleOffset;
      activeGates = activeGates;
      stepIds = stepIds;

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

      grid = grid;
      stepDurationFraction = stepDurationFraction;
      stepTimingMultiplier = stepTimingMultiplier;
      stepVelocity = stepVelocity;
      stepMuted = stepMuted;
      stepSkipped = stepSkipped;
      stepProbability = stepProbability;
      stepCycle = stepCycle;
      stepCycleOffset = stepCycleOffset;
      activeGates = activeGates;
      stepIds = stepIds;

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

      grid = grid;
      stepDurationFraction = stepDurationFraction;
      stepTimingMultiplier = stepTimingMultiplier;
      stepVelocity = stepVelocity;
      stepMuted = stepMuted;
      stepSkipped = stepSkipped;
      stepProbability = stepProbability;
      stepCycle = stepCycle;
      stepCycleOffset = stepCycleOffset;
      activeGates = activeGates;
      stepIds = stepIds;

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

    if (
      !recordingAwaitingFirstNote &&
      grid[recordingRow].length >= maxPhraseStepsPerRow
    ) {
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
      grid[row] = [note];
      stepDurationFraction[row] = [defs.duration];
      stepTimingMultiplier[row] = [defs.multiplier];
      stepVelocity[row] = [defs.velocity];
      stepMuted[row] = [defs.muted];
      stepSkipped[row] = [defs.skipped];
      stepProbability[row] = [defs.probability];
      stepCycle[row] = [defs.cycle];
      stepCycleOffset[row] = [defs.cycleOffset];
      stepIds[row] = [createStepId()];
      activeGates[row] = [false];
    } else if (grid[row].length < maxPhraseStepsPerRow) {
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

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
    stepMuted = stepMuted;
    stepSkipped = stepSkipped;
    stepProbability = stepProbability;
    stepCycle = stepCycle;
    stepCycleOffset = stepCycleOffset;
    stepIds = stepIds;
    activeGates = activeGates;
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
      const nextLoopSlot = Number.parseInt(String(state.currentLoopSlot ?? -1), 10);
      const clampedPatternSlot = Number.isNaN(nextPatternSlot)
        ? activePatternSlot
        : Math.min(7, Math.max(0, nextPatternSlot));
      const clampedLoopSlot = Number.isNaN(nextLoopSlot) || nextLoopSlot < 0 || nextLoopSlot >= 8 ? -1 : nextLoopSlot;

      if (clampedPatternSlot !== activePatternSlot) {
        activePatternSlot = clampedPatternSlot;
        activeLoopSlot = clampedLoopSlot;

        if (nativeFunctionAvailable("getPatternSlotState")) {
          const patternState = await getNativeFunction("getPatternSlotState")(clampedPatternSlot);
          assignPatternState(patternState);
        }

        return;
      }

      activeLoopSlot = clampedLoopSlot;
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

    if (patternInit !== null) {
      const raw = Array.isArray(patternInit) ? patternInit[0] : patternInit;
      const value = Number.parseInt(String(raw), 10);
      if (!Number.isNaN(value)) activePatternSlot = Math.min(7, Math.max(0, value));
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
    activePatternSlot = nextSlot;
    activeLoopSlot = -1;

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

  async function clearSelectedPatternSlot() {
    if (!nativeFunctionAvailable("clearPatternSlot")) return;

    cancelPatternCopy();
    const state = await getNativeFunction("clearPatternSlot")(activePatternSlot);
    assignPatternState(state);
  }

  async function handleLoopSlotClick(event, slot) {
    const nextSlot = Math.min(7, Math.max(0, slot));
    cancelPatternCopy();

    if (event.shiftKey) {
      if (!nativeFunctionAvailable("saveCurrentBraceToLoopSlot")) return;

      await getNativeFunction("saveCurrentBraceToLoopSlot")(nextSlot);
      loopSlotAssigned[nextSlot] = true;
      loopSlotPattern[nextSlot] = activePatternSlot;
      loopSlotAssigned = loopSlotAssigned;
      loopSlotPattern = loopSlotPattern;
      return;
    }

    if (!loopSlotAssigned[nextSlot] || !nativeFunctionAvailable("selectLoopSlot")) return;

    activeLoopSlot = nextSlot;
    activePatternSlot = loopSlotPattern[nextSlot] ?? activePatternSlot;
    slotSelectionInFlight += 1;

    try {
      const state = await getNativeFunction("selectLoopSlot")(nextSlot);
      assignPatternState(state);
    } finally {
      slotSelectionInFlight = Math.max(0, slotSelectionInFlight - 1);
    }
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
    };
  });
</script>

<main class="flex h-full flex-col overflow-hidden p-6">
  <div class="shrink-0 -mx-6">
  <header class="flex items-end gap-6 px-6 pb-6">
    <div class="shrink-0 self-start">
      <p class="text-xs font-medium uppercase tracking-widest text-emerald-400">ofsound</p>
      <h1 class="whitespace-nowrap text-xl font-semibold tracking-tight text-zinc-100">
        {pluginName} <span class="text-sm font-medium text-zinc-500">v{version}</span>
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
        <ColorsToggle
          accent={emeraldRowAccent}
          enabled={rowColorsEnabled}
          onChange={(next) => {
            rowColorsEnabled = next;
          }}
        />
        <div class="flex items-center gap-1">
          <button
            type="button"
            aria-label="Undo"
            title="Undo"
            disabled={undoStack.length === 0}
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
      <div class="flex flex-col items-end gap-1.5">
        <div class="flex items-center gap-2">
          <span class="w-16 text-right text-xs font-semibold leading-none text-zinc-500">Patterns</span>
          <div class="flex items-center gap-1">
            {#each Array.from({ length: 8 }, (_, index) => index) as slot}
              <button
                type="button"
                aria-label={patternCopySource === slot
                  ? `Pattern ${slot + 1} selected as copy source`
                  : `Select pattern ${slot + 1}`}
                aria-pressed={activePatternSlot === slot}
                title={patternCopySource === slot
                  ? "Copy source selected"
                  : "Shift-click to copy from this pattern"}
                class={slotButtonClasses(activePatternSlot === slot, true, patternCopySource === slot)}
                onclick={(event) => handlePatternSlotClick(event, slot)}
              >
                {slot + 1}
              </button>
            {/each}
            <button
              type="button"
              aria-label="Clear selected pattern"
              title="Clear selected pattern"
              class={clearPatternButtonClasses(true)}
              onclick={clearSelectedPatternSlot}
            >
              x
            </button>
          </div>
        </div>
        <div class="flex items-center gap-2">
          <span class="w-16 text-right text-xs font-semibold leading-none text-zinc-500">Loops</span>
          <div class="flex items-center gap-1">
            {#each Array.from({ length: 8 }, (_, index) => index) as slot}
              <button
                type="button"
                aria-label={loopSlotAssigned[slot]
                  ? `Select loop ${slot + 1}`
                  : `Save current brace to loop ${slot + 1}`}
                aria-pressed={activeLoopSlot === slot}
                title="Click to select, Shift-click to save current brace"
                class={slotButtonClasses(activeLoopSlot === slot, loopSlotAssigned[slot])}
                onclick={(event) => handleLoopSlotClick(event, slot)}
              >
                {slot + 1}
              </button>
            {/each}
            <div class="h-7 w-7" aria-hidden="true"></div>
          </div>
        </div>
      </div>
      {#if standaloneTransportAvailable}
        <div class="flex items-center gap-2">
          <button
            type="button"
            aria-label={standalonePlaying ? "Stop standalone transport" : "Start standalone transport"}
            aria-pressed={standalonePlaying}
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
        {#each grid as _row, row}
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
                class="{rowMuteControlClasses} {rowMuted[row]
                  ? rowPowerToggleOffClasses
                  : rowAccent.textAccent}"
                onclick={(event) => toggleRowMute(row, event.shiftKey)}
                title="Shift-click to solo row"
              >
                <RowDisableIcon class="h-9 w-9" />
              </button>
              <div class="flex h-9 w-10 shrink-0 items-center justify-center">
                <MidiChannelStepper
                  accent={rowAccent}
                  value={rowMidiChannel[row]}
                  resetValue={1}
                  ariaLabel="Row {row + 1} MIDI channel"
                  muted={rowMuted[row]}
                  onValueChange={(channel) => selectRowMidiChannel(row, channel)}
                />
              </div>
              <BipolarKnob
                accent={rowAccent}
                options={timingOffsetOptions}
                value={rowTimingOffset[row]}
                resetValue={defaultRowTimingOffsetIndex}
                ariaLabel="Row timing offset"
                muted={rowMuted[row]}
                onValueChange={(offsetIndex) => selectRowTimingOffset(row, offsetIndex)}
              />
              <button
                type="button"
                aria-label={recordingRow === row ? "Stop row recording" : "Record row from MIDI"}
                aria-pressed={recordingRow === row}
                class="{rowActionIconControlClasses} {rowMuted[row]
                  ? 'text-zinc-600'
                  : recordingRow === row
                    ? `${rowAccent.textAccent} ${toggleIconActiveClasses}`
                    : `${toggleIconRestClasses} hover:text-zinc-300`}"
                onclick={() => toggleRowRecording(row)}
                title={recordingRow === row
                  ? "Stop recording (notes fill this row as 1× steps)"
                  : "Record row from MIDI keyboard (first note replaces row)"}
              >
                <RowRecordIcon
                  class="pointer-events-none h-6 w-6"
                  recording={recordingRow === row}
                />
              </button>
              <button
                type="button"
                aria-label="Reverse row step order"
                class="{rowActionIconControlClasses} {rowMuted[row]
                  ? 'text-zinc-600'
                  : `${toggleIconRestClasses} hover:text-zinc-300`}"
                onclick={() => reverseRowStepOrder(row)}
                title="Reverse row step order"
              >
                <RowReverseOrderIcon class="pointer-events-none h-6 w-6" />
              </button>
              <button
                type="button"
                aria-label="Randomize row step order"
                class="{rowActionIconControlClasses} {rowMuted[row]
                  ? 'text-zinc-600'
                  : `${toggleIconRestClasses} hover:text-zinc-300`}"
                onclick={() => randomizeRowStepOrder(row)}
                title="Randomize row step order"
              >
                <RowRandomizeOrderIcon class="pointer-events-none h-6 w-6" />
              </button>
              <button
                type="button"
                aria-label="Randomize row octaves"
                class="{rowActionIconControlClasses} {rowMuted[row]
                  ? 'text-zinc-600'
                  : `${toggleIconRestClasses} hover:text-zinc-300`}"
                onclick={() => randomizeRowOctaves(row)}
                title="Randomize row octaves"
              >
                <RowRandomizeOctaveIcon class="pointer-events-none h-6 w-6" />
              </button>
            </div>
            <PhraseRow
              {row}
              muted={rowMuted[row]}
              accent={rowAccent}
              timingOffsetIndex={rowTimingOffset[row]}
              timingOffsetVisualCompensationPx={phraseVisualOffsetCompensationPx}
              {phraseReferenceRow}
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
</main>
