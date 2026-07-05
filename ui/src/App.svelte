<script>
  import { onMount, tick } from "svelte";
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
    defaultStepNoteForScaleRoot,
    defaultStepDurationFraction,
    defaultStepVelocity,
  } from "./midiNoteNames.js";
  import {
    cycleMaskFromLegacyOffset,
    defaultStepCycleMask,
    normalizeCyclePattern,
    normalizeEditorCyclePattern,
  } from "./cyclePattern.js";
  import RowDisableIcon from "./RowDisableIcon.svelte";
  import RowEditPencilIcon from "./RowEditPencilIcon.svelte";
  import RowRecordIcon from "./RowRecordIcon.svelte";
  import BulkStepEditControls from "./BulkStepEditControls.svelte";
  import ScaleModeDialog from "./ScaleModeDialog.svelte";
  import SeedModePanel from "./SeedModePanel.svelte";
  import BipolarKnob from "./BipolarKnob.svelte";
  import PhraseRow from "./PhraseRow.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import PianoRollPreview from "./PianoRollPreview.svelte";
  import RowPianoRollEditor from "./RowPianoRollEditor.svelte";
  import CombinationModeRail from "./CombinationModeRail.svelte";
  import RecordModePanel from "./RecordModePanel.svelte";
  import RecordPianoKeyboard from "./RecordPianoKeyboard.svelte";
  import StepInspector from "./StepInspector.svelte";
  import {
    defaultStepTimingMultiplierIndex,
    maxPhraseStepsPerRow,
    findSingleMove,
    compactPhraseGridLayout,
    stepTimingMultiplierCount,
    timingMultiplierOptions,
    timingMultiplierAtIndex,
    timingMultiplierIndicesInRange,
    clampTimingMultiplierDelta,
    clampTimingMultiplierValue,
    timingMultiplierIndexForValue,
    timingOffsetValues,
    rowTimingOffsetCount,
    clampRowTimingOffsetIndex,
    rowTimingOffsetIndicesWithDelta,
    rowTimingOffsetIndicesWithSingleValue,
  } from "./stepCellLayout.js";
  import { sanitizeOrderedIds } from "./dndUtils.js";
  import {
    blockCrossRowInsertionIndex,
    blockDuplicateInsertionIndex,
    duplicateBlockInRow,
  } from "./bulkStepDrag.js";
  import {
    duplicateBlockFromRowsToRow,
    duplicateStepBetweenRows,
    moveBlockFromRowsToRow,
    moveStepBetweenRows,
  } from "./crossRowStepMove.js";
  import {
    clampNoteBandpass,
    defaultNoteBandpassHighMidi,
    defaultNoteBandpassLowMidi,
  } from "./noteBandpass.js";
  import { setNoteBandpassPreviewApplyListener } from "./noteBandpassPreview.svelte.js";
  import {
    clampOctavizerRelativeVelocity,
    defaultOctavizerRelativeVelocity,
  } from "./octavizer.js";
  import {
    clampHundredScalePercent,
    clampStepProbabilityPercent,
    clampSignedRelativePercent,
    defaultStepProbabilityValue,
    maxPercentValue,
  } from "./percentLimits.js";
  import {
    clampShimmerDelayMultiplierIndex,
    clampShimmerFeedbackPercent,
    clampShimmerMixPercent,
    defaultShimmerDelayMultiplierIndex,
    defaultShimmerFeedbackPercent,
    defaultShimmerMixPercent,
  } from "./shimmer.js";
  import {
    clampVelocityTiltAmount,
    clampVelocityTiltPivotMidi,
    defaultVelocityTiltAmount,
    defaultVelocityTiltPivotMidi,
  } from "./velocityTilt.js";
  import {
    clampGlobalTransposeSemitones,
    defaultGlobalTransposeSemitones,
  } from "./globalTranspose.js";
  import {
    combinationModeMaskBits,
    combinationModes,
    combinationSyncDivisionOptions,
    defaultCombinationSyncDivisionIndex,
    isStepActiveAtBeat,
    stepTriggerCountAtBeat,
    swingSubdivisionOptions,
    swingSubdivisionValues,
  } from "./phraseSchedule.js";
  import DiscreteDragSelect from "./DiscreteDragSelect.svelte";
  import PulseDragInput from "./PulseDragInput.svelte";
  import ColorsToggle from "./ColorsToggle.svelte";
  import ThemeModeToggle from "./ThemeModeToggle.svelte";
  import StepViewModeToggle from "./StepViewModeToggle.svelte";
  import FullscreenIcon from "./FullscreenIcon.svelte";
  import MidiPhrasesLogo from "./MidiPhrasesLogo.svelte";
  import UiScaleDragInput from "./UiScaleDragInput.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import SaplingIcon from "./SaplingIcon.svelte";
  import {
    applySeedingRowSettingsUpdate,
    createDefaultSeedModeRowSettings,
    defaultSeedModeState,
    generateSeededPhraseRowsFromSeedModeState,
    hasSeedingRowTargets,
    mergeSeededPhraseRows,
    normalizeSeedModeState,
    phraseRowsFromGridState,
    randomSeedingSeed,
    refreshSeedingSeedsForRows,
    reshuffleSeedingAspectSeedUpdate,
    seedingTimingMultiplierMaxIndex,
    seedingTimingMultiplierMinIndex,
    seedingRhythmStepMax,
  } from "./seeding.js";
  import { defaultPulseIndex, pulseOptions } from "./pulseLayout.js";
  import {
    phraseRowEndAddStepReservePx,
    phraseRowsContentFitScale,
    phraseGridVisualOffsetCompensationPx,
    phraseRowHeaderGapPx,
  } from "./phraseRowLayout.js";
  import {
    interfaceAccent,
    rowAccentColorVar,
    rowAccentFor,
    rowMutedOverlayClasses,
    rowMuteControlClasses,
    rowPowerToggleOffClasses,
    toggleIconActiveClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";
  import {
    clampScaleModeIndex,
    clampScaleRoot,
    defaultScaleModeIndex,
    defaultScaleRoot,
    isChromaticScaleMode,
    isMidiInScale,
    keyCenters,
    scaleModes,
    scaleName,
    scaleDegreeDelta,
    snapMidiToScale,
    transposeMidiByScaleDegrees,
  } from "./scaleUtils.js";
  import { applyThemeMode, defaultThemeMode, storedThemeMode } from "./themeMode.js";
  import {
    currentUiScaleMinimumSize,
    resolveInitialUiScalePercent,
    scaledPx,
    setUiScalePercent,
    setUiViewportSize,
    uiScaleState,
  } from "./uiScale.svelte.js";

  /** @type {HTMLElement | null} */
  let appRoot = $state(null);
  let pluginName = $state("MIDI Phrases");
  let grid = $state(defaultPhraseGrid());
  let rowHeaderRecordIconSizePx = $derived(scaledPx(15));
  let rowHeaderPianoRollIconSizePx = $derived(scaledPx(16));
  let rowHeaderSeedIconSizePx = $derived(scaledPx(19));
  let rowHeaderRecordIconStyle = $derived(`width: ${rowHeaderRecordIconSizePx}px; height: ${rowHeaderRecordIconSizePx}px;`);
  let rowHeaderPianoRollIconStyle = $derived(`width: ${rowHeaderPianoRollIconSizePx}px; height: ${rowHeaderPianoRollIconSizePx}px;`);
  let rowHeaderSeedIconStyle = $derived(`width: ${rowHeaderSeedIconSizePx}px; height: ${rowHeaderSeedIconSizePx}px;`);
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
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let rowTimingOffsetGestureBefore = null;
  /** @type {number[] | null} */
  let rowTimingOffsetGestureBaseline = null;
  let rowTimingOffsetPreviewSyncPromise = Promise.resolve();
  /** @type {number[]} */
  let rowMidiChannel = $state([1, 2, 3, 4]);
  /** @type {number[][]} */
  let stepDurationFraction = $state(defaultStepDurationGrid());
  /** @type {number[][]} */
  let stepTimingMultiplier = $state(defaultStepTimingMultiplierGrid());
  let stretchStepsToFit = $state(false);
  let phraseGridFieldWidth = $state(0);
  /** @type {{ row: number, multipliers: number[] } | null} */
  let compactTimingPreview = $state(null);
  let layoutStepTimingMultiplier = $derived.by(() => {
    if (compactTimingPreview === null) return stepTimingMultiplier;

    return stepTimingMultiplier.map((rowMultipliers, rowIndex) =>
      rowIndex === compactTimingPreview.row
        ? compactTimingPreview.multipliers
        : rowMultipliers,
    );
  });
  let compactGridLayout = $derived(
    compactPhraseGridLayout(layoutStepTimingMultiplier, rowTimingOffset),
  );
  let phraseStepsContentFitScale = $derived(
    phraseRowsContentFitScale(
      layoutStepTimingMultiplier,
      rowTimingOffset,
      phraseGridFieldWidth,
      phraseVisualOffsetCompensationPx,
      phraseRowEndAddStepReservePx(),
    ),
  );
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

  let nextStepId = 0;

  function createStepId() {
    const id = `step-${nextStepId}`;
    nextStepId += 1;
    return id;
  }

  function createInitialStepIds() {
    nextStepId = 0;

    return defaultPhraseGrid().map((row) => row.map(() => createStepId()));
  }

  /** @param {string[][]} rows */
  function createStepIdsForGrid(rows) {
    return rows.map((row) => row.map(() => createStepId()));
  }

  /** @type {string[][]} */
  let stepIds = $state(createInitialStepIds());
  let duplicateDragStepId = $state(null);
  /** @type {string[] | null} */
  let bulkDragStepIds = $state(null);
  let stepTimingMultiplierById = $derived.by(() => {
    const byId = new SvelteMap();

    stepIds.forEach((ids, row) => {
      ids.forEach((id, step) => {
        byId.set(id, stepTimingMultiplier[row]?.[step] ?? defaultStepTimingMultiplierIndex);
      });
    });

    return byId;
  });

  let playbackPollTimerId = 0;
  let playbackPollInFlight = false;
  /** Poll slot/recording native state every N playback frames (~4 Hz at 16 ms). */
  let playbackSlowPollCounter = 0;
  const playbackSlowPollInterval = 15;
  let slotSelectionInFlight = 0;
  let previousGateSnapshot = defaultPhraseGrid().map((row) => row.map(() => false));
  let activeGateHoldUntil = defaultPhraseGrid().map((row) => row.map(() => 0));
  let bulkPreviewSyncPromise = Promise.resolve();
  const bulkPreviewSyncThrottleMs = 100;
  /** @type {Map<string, { row: number, step: number, pushStep: (row: number, step: number) => Promise<void> }>} */
  let bulkPreviewPendingSyncs = new Map();
  let bulkPreviewSyncTimerId = 0;
  let bulkPreviewLastSyncAt = 0;

  /** Row index armed for MIDI capture, or null. */
  let recordingRow = $state(null);
  /** Per-row loop length used by MIDI/keyboard record overdub. */
  let recordCycleLengthPulses = $state([16, 16, 16, 16]);
  let recordingProgress = $state(0);
  /** Step shown in the lower inspector panel, or null. */
  /** @type {{ row: number, stepId: string } | null} */
  let inspectedStep = $state(null);
  /** Step whose row is shown in the lower piano-roll editor, or null. */
  /** @type {{ row: number, stepId: string } | null} */
  let rowPianoRollStep = $state(null);
  /** Snapshot taken when recording was armed (for undo / cancel). */
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let recordingHistoryBefore = null;
  /** Whether any note was captured in the current recording session. */
  let recordingCapturedNotes = false;
  /** Clears the row on the next captured note. */
  let recordingAwaitingFirstNote = false;
  let recordingCaptureNativePromise = Promise.resolve();
  /** @type {Set<number>} */
  let recordingKeysHeld = $state(new Set());

  /** @param {number} quarters */
  function formatRowTimingOffsetLabel(quarters) {
    if (quarters === 0) return "0";

    const abs = Math.abs(quarters);
    let text;

    if (Number.isInteger(abs)) {
      text = String(abs);
    } else {
      text = abs.toFixed(2).replace(/0+$/, "").replace(/\.$/, "");
      if (text.startsWith("0.")) text = text.slice(1);
    }

    return quarters < 0 ? `-${text}` : text;
  }

  const timingOffsetOptions = timingOffsetValues.map((quarters, index) => ({
    index,
    label: formatRowTimingOffsetLabel(quarters),
  }));


  let loopBraceEnabled = $state(false);
  let loopBraceStart = $state(0);
  let loopBraceEnd = $state(8);
  let playbackBeat = $state(-1);
  let standaloneTransportAvailable = $state(false);
  let standalonePlaying = $state(false);
  let standaloneTempoBpm = $state(120);
  let activePatternSlot = $state(0);
  let viewPatternSlot = $state(0);
  let patternCopySource = $state(-1);
  let patternClearArmed = $state(false);
  let activeLoopSlot = $state(-1);
  let loopSlotAssigned = $state(Array.from({ length: 8 }, () => false));
  let loopSlotPattern = Array.from({ length: 8 }, () => 0);
  let combinationModeMask = $state(0);
  let combinationSyncDivisionIndex = $state(defaultCombinationSyncDivisionIndex);
  let combinationChangePhaseContinue = $state(false);
  let scaleRoot = $state(defaultScaleRoot);
  let scaleModeIndex = $state(defaultScaleModeIndex);
  let noteBandpassLowMidi = $state(defaultNoteBandpassLowMidi);
  let noteBandpassHighMidi = $state(defaultNoteBandpassHighMidi);
  let velocityTiltPivotMidi = $state(defaultVelocityTiltPivotMidi);
  let velocityTiltAmount = $state(defaultVelocityTiltAmount);
  let globalTransposeSemitones = $state(defaultGlobalTransposeSemitones);
  let octavizerDown8vaEnabled = $state(false);
  let octavizerUp8vaEnabled = $state(false);
  let octavizerDown8vaRelativeVelocity = $state(defaultOctavizerRelativeVelocity);
  let octavizerUp8vaRelativeVelocity = $state(defaultOctavizerRelativeVelocity);
  let shimmerEnabled = $state(false);
  let shimmerDelayMultiplierIndex = $state(defaultShimmerDelayMultiplierIndex);
  let shimmerFeedbackPercent = $state(defaultShimmerFeedbackPercent);
  let shimmerMixPercent = $state(defaultShimmerMixPercent);
  let scaleDialogOpen = $state(false);
  let seedModeActive = $state(false);
  let seedModeRhythmStep = $state(defaultSeedModeState.rhythmStep);
  let seedModeRowSettings = $state(createDefaultSeedModeRowSettings());
  let seedModeRowTargets = $state([...defaultSeedModeState.rowTargets]);
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let seedModeGestureBefore = null;
  let seedModeApplyVersion = 0;
  let pulseIndex = $state(defaultPulseIndex);
  let swingPercent = $state(0);
  let velocityHumanizePercent = $state(0);
  let timingHumanizePercent = $state(0);
  let swingSubdivisionIndex = $state(1);
  let rowColorsEnabled = $state(true);
  // Keep the row color toggle plumbing wired while the visible control is parked.
  const showRowColorsToggle = false;
  let themeMode = $state(defaultThemeMode);
  let projectName = $state("Untitled Project");
  let projectDescription = $state("");
  let projectCreatedAt = $state("");
  let projectModifiedAt = $state("");
  let projectFileName = $state("");
  let hasPreviousProject = $state(false);
  let hasNextProject = $state(false);
  let projectOperationBusy = $state(false);
  let projectOperationError = $state("");
  let undoStack = $state([]);
  let redoStack = $state([]);
  let nextUndoHistoryLabel = $derived(undoStack[undoStack.length - 1]?.label ?? "");
  let nextRedoHistoryLabel = $derived(redoStack[redoStack.length - 1]?.label ?? "");
  let undoHistoryLabel = $derived(nextUndoHistoryLabel ? `Undo ${nextUndoHistoryLabel}` : "Undo");
  let redoHistoryLabel = $derived(nextRedoHistoryLabel ? `Redo ${nextRedoHistoryLabel}` : "Redo");
  const selectedStepKeys = new SvelteSet();
  /** @type {string[][]} */
  let selectableStepKeySet = $derived(new Set(allSelectableStepKeys()));
  let selectedStepKeysForGrid = $derived(
    new Set([...selectedStepKeys].filter((key) => selectableStepKeySet.has(key))),
  );
  let selectedStepIdsByRow = $derived(selectedStepIdsByRowForKeys(selectedStepKeysForGrid));
  let selectedStepIdsForDragGhost = $derived.by(() => {
    const ids = [];

    for (let row = 0; row < stepIds.length; row += 1) {
      for (const stepId of stepIds[row]) {
        if (selectedStepKeysForGrid.has(stepSelectionKey(row, stepId))) {
          ids.push(stepId);
        }
      }
    }

    return ids;
  });

  /** @param {string[]} ids */
  function stepLocationsForIds(ids) {
    const seen = new SvelteSet();
    const locations = [];

    for (const id of ids) {
      if (seen.has(id)) continue;

      for (let row = 0; row < stepIds.length; row += 1) {
        const step = stepIds[row].indexOf(id);

        if (step < 0) continue;

        seen.add(id);
        locations.push({ id, row, step });
        break;
      }
    }

    return locations;
  }
  let bulkDurationPercent = $state(0);
  let bulkVelocityPercent = $state(0);
  let bulkLengthDelta = $state(0);
  let bulkTransposeSemitones = $state(0);
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let bulkEditGestureBefore = null;
  /** @type {ReturnType<typeof createHistorySnapshot> | null} */
  let processingParamGestureBefore = null;
  /** @type {Map<string, number> | null} */
  let bulkDurationBaselineByKey = null;
  /** @type {Map<string, number> | null} */
  let bulkVelocityBaselineByKey = null;
  /** @type {Map<string, number> | null} */
  let bulkLengthBaselineByKey = null;
  /** @type {Map<string, number> | null} */
  let bulkTransposeBaselineByKey = null;
  /** @type {{ startX: number, startY: number, currentX: number, currentY: number, addToSelection: boolean, baseKeys: Set<string> } | null} */
  let marqueeSelection = $state(null);
  let lastRowGapPointerDownTime = 0;
  let lastRowGapPointerDownX = 0;
  let lastRowGapPointerDownY = 0;
  let editorFullscreen = $state(false);
  let editorFullscreenBusy = $state(false);

  const historyLimit = 100;

  /** @param {HTMLElement} node */
  function appRootAttachment(node) {
    appRoot = node;

    return () => {
      if (appRoot === node) appRoot = null;
    };
  }

  /** @param {HTMLElement} node */
  function phraseGridFieldAttachment(node) {
    phraseGridFieldWidth = node.clientWidth;

    const observer = new ResizeObserver(() => {
      phraseGridFieldWidth = node.clientWidth;
    });
    observer.observe(node);

    return () => {
      observer.disconnect();
    };
  }
  const rowGapDoubleClickIntervalMs = 400;
  const rowGapDoubleClickMaxDistancePx = 16;
  const stepTriggerFlashMs = 110;
  const historyButtonBaseClasses =
    "flex h-8 w-8 shrink-0 items-center justify-center rounded-md border p-0 text-sm font-semibold transition-[border-color,color,box-shadow,filter] outline-none focus:ring-1 focus:ring-focus-ring disabled:border-border-subtle disabled:text-text-faint";

  function historyButtonClasses(enabled) {
    return `${historyButtonBaseClasses} ${
      enabled
        ? "mp-control-gradient border-border text-text-secondary hover:border-border-strong hover:text-text"
        : "mp-control-gradient-muted border-border-subtle text-text-faint"
    }`;
  }

  /** @param {string} next */
  function setThemeMode(next) {
    themeMode = applyThemeMode(next);
  }

  async function syncEditorScaleMinimumToNative() {
    if (!nativeFunctionAvailable("setEditorScaleMinimum")) return;

    const minimumSize = currentUiScaleMinimumSize({ standaloneTransportAvailable });
    await getNativeFunction("setEditorScaleMinimum")(minimumSize.widthPx, minimumSize.heightPx);
  }

  async function syncProjectUiScaleToNative() {
    if (!nativeFunctionAvailable("setProjectUiScalePercent")) return;

    await getNativeFunction("setProjectUiScalePercent")(uiScaleState.percent);
  }

  function setExplicitUiScalePercent(next) {
    setUiScalePercent(next);
    void syncEditorScaleMinimumToNative();
    void syncProjectUiScaleToNative();
  }

  function editorFullscreenButtonClasses() {
    return `flex h-8 w-8 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors focus-visible:ring-1 focus-visible:ring-focus-ring ${
      editorFullscreen ? "text-text" : "text-text-muted hover:text-text-secondary"
    } ${editorFullscreenBusy ? "opacity-60" : ""}`;
  }

  function editorFullscreenEnabledFromNativeResult(result) {
    const payload = Array.isArray(result) && result.length === 1 ? result[0] : result;

    if (typeof payload === "boolean") return payload;
    if (typeof payload === "number") return payload !== 0;
    if (payload && typeof payload === "object" && "enabled" in payload) return Boolean(payload.enabled);

    return null;
  }

  async function refreshEditorFullscreenState() {
    if (!nativeFunctionAvailable("getEditorFullscreenState")) return;

    const result = await getNativeFunction("getEditorFullscreenState")();
    const enabled = editorFullscreenEnabledFromNativeResult(result);

    if (enabled !== null) editorFullscreen = enabled;
  }

  async function setEditorFullscreen(next) {
    if (!nativeFunctionAvailable("setEditorFullscreen") || editorFullscreenBusy) return;

    editorFullscreenBusy = true;
    const previous = editorFullscreen;
    editorFullscreen = next;

    try {
      const result = await getNativeFunction("setEditorFullscreen")(next ? 1 : 0);
      const enabled = editorFullscreenEnabledFromNativeResult(result);
      editorFullscreen = enabled ?? next;
    } catch {
      editorFullscreen = previous;
    } finally {
      editorFullscreenBusy = false;
      queuePlaybackUiRefresh();
    }
  }

  function toggleEditorFullscreen() {
    void setEditorFullscreen(!editorFullscreen);
  }

  function setNoteBandpassState(lowMidi, highMidi) {
    const next = clampNoteBandpass(lowMidi, highMidi);
    noteBandpassLowMidi = next.low;
    noteBandpassHighMidi = next.high;
  }

  function handleNoteBandpassChange(lowMidi, highMidi) {
    setNoteBandpassState(lowMidi, highMidi);
  }

  function previewNoteBandpass(lowMidi, highMidi) {
    if (!nativeFunctionAvailable("setPatternNoteBandpass")) return;

    const next = clampNoteBandpass(lowMidi, highMidi);
    void getNativeFunction("setPatternNoteBandpass")(next.low, next.high);
  }

  setNoteBandpassPreviewApplyListener(previewNoteBandpass);

  async function commitNoteBandpass(lowMidi, highMidi) {
    const previousLow = noteBandpassLowMidi;
    const previousHigh = noteBandpassHighMidi;
    setNoteBandpassState(lowMidi, highMidi);

    if (nativeFunctionAvailable("setPatternNoteBandpass")) {
      const confirmed = await getNativeFunction("setPatternNoteBandpass")(
        noteBandpassLowMidi,
        noteBandpassHighMidi,
      );

      if (Array.isArray(confirmed) && confirmed.length >= 2) {
        setNoteBandpassState(
          Number.parseInt(String(confirmed[0]), 10),
          Number.parseInt(String(confirmed[1]), 10),
        );
      } else if (confirmed && typeof confirmed === "object") {
        setNoteBandpassState(
          Number.parseInt(String(confirmed.noteBandpassLowMidi ?? confirmed.lowMidi), 10),
          Number.parseInt(String(confirmed.noteBandpassHighMidi ?? confirmed.highMidi), 10),
        );
      }
    }

    const after = createHistorySnapshot();
    if (after.noteBandpassLowMidi === previousLow && after.noteBandpassHighMidi === previousHigh) {
      return;
    }

    const before = cloneSnapshot(after);
    before.noteBandpassLowMidi = previousLow;
    before.noteBandpassHighMidi = previousHigh;
    pushHistoryEntry("Note bandpass", before, after);
  }

  function setOctavizerState({
    down8vaEnabled = octavizerDown8vaEnabled,
    up8vaEnabled = octavizerUp8vaEnabled,
    down8vaRelativeVelocity = octavizerDown8vaRelativeVelocity,
    up8vaRelativeVelocity = octavizerUp8vaRelativeVelocity,
  } = {}) {
    octavizerDown8vaEnabled = Boolean(down8vaEnabled);
    octavizerUp8vaEnabled = Boolean(up8vaEnabled);
    octavizerDown8vaRelativeVelocity = clampOctavizerRelativeVelocity(down8vaRelativeVelocity);
    octavizerUp8vaRelativeVelocity = clampOctavizerRelativeVelocity(up8vaRelativeVelocity);
  }

  function setVelocityTiltState({
    pivotMidi = velocityTiltPivotMidi,
    amount = velocityTiltAmount,
  } = {}) {
    velocityTiltPivotMidi = clampVelocityTiltPivotMidi(pivotMidi);
    velocityTiltAmount = clampVelocityTiltAmount(amount);
  }

  function setGlobalTransposeState(value) {
    globalTransposeSemitones = clampGlobalTransposeSemitones(value);
  }

  function setShimmerState({
    enabled = shimmerEnabled,
    delayMultiplierIndex = shimmerDelayMultiplierIndex,
    feedbackPercent = shimmerFeedbackPercent,
    mixPercent = shimmerMixPercent,
  } = {}) {
    shimmerEnabled = Boolean(enabled);
    shimmerDelayMultiplierIndex = clampShimmerDelayMultiplierIndex(delayMultiplierIndex);
    shimmerFeedbackPercent = clampShimmerFeedbackPercent(feedbackPercent);
    shimmerMixPercent = clampShimmerMixPercent(mixPercent);
  }

  async function syncVelocityTiltToNative() {
    if (nativeFunctionAvailable("setPatternVelocityTiltPivotMidi")) {
      const confirmed = await getNativeFunction("setPatternVelocityTiltPivotMidi")(
        velocityTiltPivotMidi,
      );
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        velocityTiltPivotMidi = clampVelocityTiltPivotMidi(parsed);
      }
    }

    if (nativeFunctionAvailable("setPatternVelocityTiltAmount")) {
      const confirmed = await getNativeFunction("setPatternVelocityTiltAmount")(velocityTiltAmount);
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        velocityTiltAmount = clampVelocityTiltAmount(parsed);
      }
    }
  }

  async function syncGlobalTransposeToNative() {
    if (!nativeFunctionAvailable("setPatternGlobalTransposeSemitones")) return;

    const confirmed = await getNativeFunction("setPatternGlobalTransposeSemitones")(
      globalTransposeSemitones,
    );
    const parsed = Number.parseInt(String(confirmed), 10);

    if (!Number.isNaN(parsed)) setGlobalTransposeState(parsed);
  }

  async function syncOctavizerToNative() {
    if (nativeFunctionAvailable("setPatternOctavizerDown8vaEnabled")) {
      const confirmed = await getNativeFunction("setPatternOctavizerDown8vaEnabled")(
        octavizerDown8vaEnabled ? 1 : 0,
      );
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        octavizerDown8vaEnabled = parsed !== 0;
      }
    }

    if (nativeFunctionAvailable("setPatternOctavizerUp8vaEnabled")) {
      const confirmed = await getNativeFunction("setPatternOctavizerUp8vaEnabled")(
        octavizerUp8vaEnabled ? 1 : 0,
      );
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        octavizerUp8vaEnabled = parsed !== 0;
      }
    }

    if (nativeFunctionAvailable("setPatternOctavizerDown8vaRelativeVelocity")) {
      const confirmed = await getNativeFunction("setPatternOctavizerDown8vaRelativeVelocity")(
        octavizerDown8vaRelativeVelocity,
      );
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        octavizerDown8vaRelativeVelocity = clampOctavizerRelativeVelocity(parsed);
      }
    }

    if (nativeFunctionAvailable("setPatternOctavizerUp8vaRelativeVelocity")) {
      const confirmed = await getNativeFunction("setPatternOctavizerUp8vaRelativeVelocity")(
        octavizerUp8vaRelativeVelocity,
      );
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        octavizerUp8vaRelativeVelocity = clampOctavizerRelativeVelocity(parsed);
      }
    }
  }

  async function syncShimmerToNative() {
    if (nativeFunctionAvailable("setPatternShimmerEnabled")) {
      const confirmed = await getNativeFunction("setPatternShimmerEnabled")(shimmerEnabled ? 1 : 0);
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        shimmerEnabled = parsed !== 0;
      }
    }

    if (nativeFunctionAvailable("setPatternShimmerDelayMultiplierIndex")) {
      const confirmed = await getNativeFunction("setPatternShimmerDelayMultiplierIndex")(
        shimmerDelayMultiplierIndex,
      );
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        shimmerDelayMultiplierIndex = clampShimmerDelayMultiplierIndex(parsed);
      }
    }

    if (nativeFunctionAvailable("setPatternShimmerFeedbackPercent")) {
      const confirmed = await getNativeFunction("setPatternShimmerFeedbackPercent")(
        shimmerFeedbackPercent,
      );
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        shimmerFeedbackPercent = clampShimmerFeedbackPercent(parsed);
      }
    }

    if (nativeFunctionAvailable("setPatternShimmerMixPercent")) {
      const confirmed = await getNativeFunction("setPatternShimmerMixPercent")(shimmerMixPercent);
      const parsed = Number.parseInt(String(confirmed), 10);

      if (!Number.isNaN(parsed)) {
        shimmerMixPercent = clampShimmerMixPercent(parsed);
      }
    }
  }

  async function handleOctavizerDown8vaToggle(enabled) {
    const previousEnabled = octavizerDown8vaEnabled;
    setOctavizerState({ down8vaEnabled: enabled });
    await syncOctavizerToNative();

    const after = createHistorySnapshot();
    if (after.octavizerDown8vaEnabled === previousEnabled) return;

    const before = cloneSnapshot(after);
    before.octavizerDown8vaEnabled = previousEnabled;
    pushHistoryEntry("Octavizer -8va", before, after);
  }

  async function handleOctavizerUp8vaToggle(enabled) {
    const previousEnabled = octavizerUp8vaEnabled;
    setOctavizerState({ up8vaEnabled: enabled });
    await syncOctavizerToNative();

    const after = createHistorySnapshot();
    if (after.octavizerUp8vaEnabled === previousEnabled) return;

    const before = cloneSnapshot(after);
    before.octavizerUp8vaEnabled = previousEnabled;
    pushHistoryEntry("Octavizer 8va", before, after);
  }

  function beginProcessingParamGesture() {
    if (!processingParamGestureBefore) {
      processingParamGestureBefore = createHistorySnapshot();
    }
  }

  function resetProcessingParamGesture() {
    processingParamGestureBefore = null;
  }

  function previewOctavizerDown8vaRelativeVelocity(value) {
    setOctavizerState({ down8vaRelativeVelocity: value });
  }

  function previewOctavizerUp8vaRelativeVelocity(value) {
    setOctavizerState({ up8vaRelativeVelocity: value });
  }

  function previewShimmerDelay(value) {
    setShimmerState({ delayMultiplierIndex: value });
  }

  function previewShimmerFeedback(value) {
    setShimmerState({ feedbackPercent: value });
  }

  function previewShimmerMix(value) {
    setShimmerState({ mixPercent: value });
  }

  function previewVelocityTiltPivot(value) {
    const next = clampVelocityTiltPivotMidi(value);
    setVelocityTiltState({ pivotMidi: next });

    if (!nativeFunctionAvailable("setPatternVelocityTiltPivotMidi")) return;

    void getNativeFunction("setPatternVelocityTiltPivotMidi")(next);
  }

  function previewVelocityTiltAmount(value) {
    const next = clampVelocityTiltAmount(value);
    setVelocityTiltState({ amount: next });

    if (!nativeFunctionAvailable("setPatternVelocityTiltAmount")) return;

    void getNativeFunction("setPatternVelocityTiltAmount")(next);
  }

  function previewGlobalTranspose(value) {
    const next = clampGlobalTransposeSemitones(value);
    setGlobalTransposeState(next);

    if (!nativeFunctionAvailable("setPatternGlobalTransposeSemitones")) return;

    void getNativeFunction("setPatternGlobalTransposeSemitones")(next);
  }

  async function commitProcessingParam(label, applyState, syncFn, changed) {
    const before = processingParamGestureBefore ?? createHistorySnapshot();
    resetProcessingParamGesture();

    applyState();
    await syncFn();

    const after = createHistorySnapshot();

    if (!changed(before, after)) return;

    pushHistoryEntry(label, before, after);
  }

  async function commitOctavizerDown8vaRelativeVelocity(value) {
    await commitProcessingParam(
      "Octavizer -8va velocity",
      () => setOctavizerState({ down8vaRelativeVelocity: value }),
      syncOctavizerToNative,
      (before, after) =>
        after.octavizerDown8vaRelativeVelocity !== before.octavizerDown8vaRelativeVelocity,
    );
  }

  async function commitOctavizerUp8vaRelativeVelocity(value) {
    await commitProcessingParam(
      "Octavizer 8va velocity",
      () => setOctavizerState({ up8vaRelativeVelocity: value }),
      syncOctavizerToNative,
      (before, after) =>
        after.octavizerUp8vaRelativeVelocity !== before.octavizerUp8vaRelativeVelocity,
    );
  }

  async function commitVelocityTiltPivot(value) {
    await commitProcessingParam(
      "Velocity tilt pivot",
      () => setVelocityTiltState({ pivotMidi: value }),
      syncVelocityTiltToNative,
      (before, after) => after.velocityTiltPivotMidi !== before.velocityTiltPivotMidi,
    );
  }

  async function commitVelocityTiltAmount(value) {
    await commitProcessingParam(
      "Velocity tilt",
      () => setVelocityTiltState({ amount: value }),
      syncVelocityTiltToNative,
      (before, after) => after.velocityTiltAmount !== before.velocityTiltAmount,
    );
  }

  async function commitVelocityTiltXY(pivotMidi, amount) {
    await commitProcessingParam(
      "Velocity tilt XY",
      () => setVelocityTiltState({ pivotMidi, amount }),
      syncVelocityTiltToNative,
      (before, after) =>
        after.velocityTiltPivotMidi !== before.velocityTiltPivotMidi
        || after.velocityTiltAmount !== before.velocityTiltAmount,
    );
  }

  async function commitGlobalTranspose(value) {
    await commitProcessingParam(
      "Global transpose",
      () => setGlobalTransposeState(value),
      syncGlobalTransposeToNative,
      (before, after) =>
        after.globalTransposeSemitones !== before.globalTransposeSemitones,
    );
  }

  async function handleShimmerToggle(enabled) {
    const previousEnabled = shimmerEnabled;
    setShimmerState({ enabled });
    await syncShimmerToNative();

    const after = createHistorySnapshot();
    if (after.shimmerEnabled === previousEnabled) return;

    const before = cloneSnapshot(after);
    before.shimmerEnabled = previousEnabled;
    pushHistoryEntry("Shimmer", before, after);
  }

  async function commitShimmerDelay(value) {
    await commitProcessingParam(
      "Shimmer delay",
      () => setShimmerState({ delayMultiplierIndex: value }),
      syncShimmerToNative,
      (before, after) =>
        after.shimmerDelayMultiplierIndex !== before.shimmerDelayMultiplierIndex,
    );
  }

  async function commitShimmerFeedback(value) {
    await commitProcessingParam(
      "Shimmer feedback",
      () => setShimmerState({ feedbackPercent: value }),
      syncShimmerToNative,
      (before, after) => after.shimmerFeedbackPercent !== before.shimmerFeedbackPercent,
    );
  }

  async function commitShimmerMix(value) {
    await commitProcessingParam(
      "Shimmer mix",
      () => setShimmerState({ mixPercent: value }),
      syncShimmerToNative,
      (before, after) => after.shimmerMixPercent !== before.shimmerMixPercent,
    );
  }

  async function toggleCombinationMode(modeIndex) {
    const bit = 1 << modeIndex;
    const enabled = (combinationModeMask & bit) === 0;
    const nextMask = enabled ? combinationModeMask | bit : combinationModeMask & ~bit;

    combinationModeMask = nextMask;

    if (!nativeFunctionAvailable("setCombinationModeEnabled")) return;

    const confirmed = await getNativeFunction("setCombinationModeEnabled")(
      modeIndex,
      enabled ? 1 : 0,
    );
    const parsed = Number.parseInt(String(confirmed), 10);

    if (!Number.isNaN(parsed)) {
      combinationModeMask = parsed & combinationModeMaskBits;
    }
  }

  async function applyCombinationSyncDivisionIndex(nextIndex) {
    await commitHistory("Change combination sync", async () => {
      combinationSyncDivisionIndex = Math.min(
        combinationSyncDivisionOptions.length - 1,
        Math.max(0, nextIndex),
      );

      if (!nativeFunctionAvailable("setCombinationSyncDivisionIndex")) return;

      const result = await getNativeFunction("setCombinationSyncDivisionIndex")(
        combinationSyncDivisionIndex,
      );
      const confirmed = Number.parseInt(String(result), 10);

      if (!Number.isNaN(confirmed)) {
        combinationSyncDivisionIndex = Math.min(
          combinationSyncDivisionOptions.length - 1,
          Math.max(0, confirmed),
        );
      }
    });
  }

  async function applyCombinationChangePhaseContinue(enabled) {
    await commitHistory("Change combination phase", async () => {
      combinationChangePhaseContinue = enabled;

      if (!nativeFunctionAvailable("setCombinationChangePhaseContinue")) return;

      const result = await getNativeFunction("setCombinationChangePhaseContinue")(
        combinationChangePhaseContinue ? 1 : 0,
      );
      const confirmed = Number.parseInt(String(result), 10);

      if (!Number.isNaN(confirmed)) {
        combinationChangePhaseContinue = confirmed !== 0;
      }
    });
  }

  function slotButtonClasses(active, assigned = true, copySource = false, copyTarget = false) {
    if (copySource) {
      return "flex h-[2.1rem] w-[2.1rem] items-center justify-center rounded-sm border border-accent text-sm font-semibold leading-none text-control-primary-text outline-none focus:ring-1 focus:ring-focus-ring mp-slot-copy-source";
    }

    const base = `flex h-[2.1rem] w-[2.1rem] items-center justify-center rounded-sm border text-sm font-semibold leading-none transition-[border-color,color,box-shadow,filter] outline-none focus:ring-1 focus:ring-focus-ring ${
      active
        ? "mp-slot-active border-accent text-control-primary-text"
        : assigned
          ? "mp-control-gradient border-border text-text hover:border-border-strong"
          : "mp-control-gradient-muted border-border-subtle text-text-faint hover:border-border hover:text-text-secondary"
    }`;

    return copyTarget ? `${base} mp-slot-copy-target` : base;
  }

  function clearPatternButtonClasses(enabled, armed = false) {
    return `flex h-[1.75rem] w-[1.35rem] shrink-0 items-center justify-center border-0 bg-transparent transition-colors outline-none focus:ring-1 focus:ring-focus-ring ${
      armed
        ? "mp-pattern-clear-armed text-danger"
        : enabled
          ? "text-danger hover:text-danger"
          : "text-text-faint"
    }`;
  }

  function outputMuteButtonClasses(active) {
    return `flex h-[calc(2.1rem*1.33)] w-[calc(2.1rem*1.33)] shrink-0 items-center justify-center rounded-sm border text-sm font-semibold leading-none transition-[border-color,color,box-shadow,filter] outline-none focus:ring-1 focus:ring-focus-ring ${
      active
        ? "border-accent bg-accent text-control-primary-text"
        : "mp-control-gradient border-border text-text hover:border-border-strong"
    }`;
  }

  function stepNoteByCurrentScale(value, delta) {
    const base = Math.min(127, Math.max(0, Math.round(value)));
    const steps = Math.round(delta);

    if (isChromaticScaleMode(scaleModeIndex)) {
      return Math.min(127, Math.max(0, base + steps));
    }

    const snapped = snapMidiToScale(base, scaleRoot, scaleModeIndex);
    return transposeMidiByScaleDegrees(snapped, steps, scaleRoot, scaleModeIndex);
  }

  function clampPhraseNote(midi) {
    const clamped = Math.min(127, Math.max(0, Math.round(midi)));

    return snapMidiToScale(clamped, scaleRoot, scaleModeIndex);
  }

  function multiSelectLocationsForEditedStep(row, step) {
    let locations = selectedStepLocations();

    if (activeRowPianoRollEditor?.row === row) {
      locations = locations.filter((location) => location.row === row);
    }

    if (locations.length <= 1) {
      return null;
    }

    const editedStepId = stepIds[row]?.[step];

    if (!editedStepId) {
      return null;
    }

    const editedKey = stepSelectionKey(row, editedStepId);

    if (!locations.some((location) => location.key === editedKey)) {
      return null;
    }

    return locations;
  }

  function beginPhraseStepBulkGesture(row, step) {
    if (multiSelectLocationsForEditedStep(row, step)) {
      beginBulkEditGesture();
    }
  }

  function transposeDeltaForEditedNote(baselineMidi, newMidi) {
    const baseline = Math.min(127, Math.max(0, Math.round(baselineMidi)));
    const target = clampPhraseNote(newMidi);

    if (isChromaticScaleMode(scaleModeIndex)) {
      return target - baseline;
    }

    return scaleDegreeDelta(baseline, target, scaleRoot, scaleModeIndex);
  }

  function previewPhraseNoteValue(row, step, midi) {
    const locations = multiSelectLocationsForEditedStep(row, step);
    const note = clampPhraseNote(midi);

    if (!locations) {
      grid[row][step] = note;
      void pushNote(row, step);
      return;
    }

    if (!bulkEditGestureBefore) {
      beginBulkEditGesture();
    }

    if (!bulkTransposeBaselineByKey) {
      bulkTransposeBaselineByKey = new SvelteMap();

      for (const { row: editRow, step: editStep, key } of locations) {
        bulkTransposeBaselineByKey.set(key, grid[editRow][editStep]);
      }
    }

    const editedKey = stepSelectionKey(row, stepIds[row][step]);
    const editedBaseline = bulkTransposeBaselineByKey.get(editedKey) ?? grid[row][step];
    const delta = transposeDeltaForEditedNote(editedBaseline, note);
    bulkTransposeSemitones = clampTransposeSemitones(delta);

    for (const { row: editRow, step: editStep, key } of locations) {
      const baseline = bulkTransposeBaselineByKey.get(key) ?? grid[editRow][editStep];
      grid[editRow][editStep] = clampPhraseNote(stepNoteByCurrentScale(baseline, bulkTransposeSemitones));
    }

    queueBulkPreviewSync(locations, pushNote);
  }

  async function commitPhraseNoteValue(row, step, midi) {
    const locations = multiSelectLocationsForEditedStep(row, step);

    if (!locations) {
      const note = clampPhraseNote(midi);

      await commitHistory("Change note", async () => {
        grid[row][step] = note;
        await pushNote(row, step);
      });
      return;
    }

    previewPhraseNoteValue(row, step, midi);

    await commitBulkEditGesture("Bulk transpose", async () => {
      await pushRowsForSelectedLocations(locations);
    });

    bulkTransposeSemitones = 0;
  }

  function previewPhraseStepDuration(row, step, fraction) {
    const locations = multiSelectLocationsForEditedStep(row, step);
    const clampedFraction = Math.min(1, Math.max(0, fraction));

    if (!locations) {
      stepDurationFraction[row][step] = clampedFraction;
      void pushStepDurationFraction(row, step);
      return;
    }

    if (!bulkEditGestureBefore) {
      beginBulkEditGesture();
    }

    ensureBulkDurationBaseline(locations);

    const editedKey = stepSelectionKey(row, stepIds[row][step]);
    const editedBaseline =
      bulkDurationBaselineByKey?.get(editedKey) ??
      (stepDurationFraction[row][step] ?? defaultStepDurationFraction) * 100;
    const delta = clampedFraction * 100 - editedBaseline;
    bulkDurationPercent = delta;

    for (const { row: editRow, step: editStep, key } of locations) {
      const baseline = bulkDurationBaselineByKey?.get(key) ?? 100;
      stepDurationFraction[editRow][editStep] = clampStepDurationPercent(baseline + delta) / 100;
    }

    queueBulkPreviewSync(locations, pushStepDurationFraction);
  }

  async function commitPhraseStepDuration(row, step, fraction) {
    const locations = multiSelectLocationsForEditedStep(row, step);

    if (!locations) {
      await commitHistory("Change duration", async () => {
        stepDurationFraction[row][step] = Math.min(1, Math.max(0, fraction));
        await pushStepDurationFraction(row, step);
      });
      return;
    }

    previewPhraseStepDuration(row, step, fraction);

    await commitBulkEditGesture("Bulk duration", async () => {
      await pushRowsForSelectedLocations(locations);
    });

    bulkDurationPercent = 0;
  }

  function previewPhraseStepVelocity(row, step, value) {
    const locations = multiSelectLocationsForEditedStep(row, step);
    const clampedVelocity = Math.min(127, Math.max(0, value));

    if (!locations) {
      stepVelocity[row][step] = clampedVelocity;
      void pushStepVelocity(row, step);
      return;
    }

    if (!bulkEditGestureBefore) {
      beginBulkEditGesture();
    }

    ensureBulkVelocityBaseline(locations);

    const editedKey = stepSelectionKey(row, stepIds[row][step]);
    const editedBaseline =
      bulkVelocityBaselineByKey?.get(editedKey) ??
      ((stepVelocity[row][step] ?? defaultStepVelocity) / 127) * 100;
    const delta = (clampedVelocity / 127) * 100 - editedBaseline;
    bulkVelocityPercent = delta;

    for (const { row: editRow, step: editStep, key } of locations) {
      const baseline =
        bulkVelocityBaselineByKey?.get(key) ?? (defaultStepVelocity / 127) * 100;
      stepVelocity[editRow][editStep] = Math.round(
        (clampStepVelocityPercent(baseline + delta) / 100) * 127,
      );
    }

    queueBulkPreviewSync(locations, pushStepVelocity);
  }

  async function commitPhraseStepVelocity(row, step, value) {
    const locations = multiSelectLocationsForEditedStep(row, step);

    if (!locations) {
      await commitHistory("Change velocity", async () => {
        stepVelocity[row][step] = Math.min(127, Math.max(0, value));
        await pushStepVelocity(row, step);
      });
      return;
    }

    previewPhraseStepVelocity(row, step, value);

    await commitBulkEditGesture("Bulk velocity", async () => {
      await pushRowsForSelectedLocations(locations);
    });

    bulkVelocityPercent = 0;
  }

  /** @param {number} row @param {{ step: number, velocity: number }[]} updates */
  async function commitPhraseRowVelocityShape(row, updates) {
    if (updates.length === 0) return;

    await commitHistory("Draw velocity shape", async () => {
      for (const { step, velocity } of updates) {
        stepVelocity[row][step] = Math.min(127, Math.max(0, velocity));
      }

      for (const { step } of updates) {
        await pushStepVelocity(row, step);
      }
    });
  }

  async function applyPatternScale(nextRoot, nextModeIndex) {
    const root = clampScaleRoot(nextRoot);
    const mode = clampScaleModeIndex(nextModeIndex);

    scaleRoot = root;
    scaleModeIndex = mode;

    if (nativeFunctionAvailable("setPatternScale")) {
      await getNativeFunction("setPatternScale")(root, mode);
    }
  }

  async function setPatternScale(nextRoot, nextModeIndex) {
    if (seedModeActive) {
      await commitHistory("Change scale mode", async () => {
        await applyPatternScale(nextRoot, nextModeIndex);
        await applySeedModeState();
      });
      return;
    }

    await commitHistory("Change scale mode", async () => {
      await applyPatternScale(nextRoot, nextModeIndex);
    });
  }

  /** UI-only; shifts phrase rows when any row has a negative offset. */
  let phraseVisualOffsetCompensationPx = $derived(phraseGridVisualOffsetCompensationPx(
    rowTimingOffset,
  ));
  let activeScaleName = $derived(scaleName(scaleRoot, scaleModeIndex));
  let projectDateLabel = $derived(formatProjectDate(projectModifiedAt || projectCreatedAt));
  let activeKeyCenterLabel = $derived(keyCenters[scaleRoot]?.label ?? "C");
  let defaultNewStepNote = $derived(defaultStepNoteForScaleRoot(scaleRoot));
  let activeScaleModeLabel = $derived(scaleModes[scaleModeIndex]?.label ?? "Chromatic");
  let bulkPitchAriaLabel = $derived(
    isChromaticScaleMode(scaleModeIndex)
      ? "Bulk step pitch semitones"
      : "Bulk step pitch scale degrees",
  );
  let selectedStepCount = $derived(selectedStepKeysForGrid.size);
  let selectableStepCount = $derived(stepIds.reduce((count, rowStepIds) => count + rowStepIds.length, 0));
  let bulkRowOrderCycleAvailable = $derived.by(() => {
    for (const locations of bulkEditLocationsGroupedByRow().values()) {
      if (locations.length > 1) {
        return true;
      }
    }

    return false;
  });
  let bulkReverseAvailable = $derived(bulkRowOrderCycleAvailable);
  let bulkSkipActive = $derived.by(() => {
    const locations = bulkEditLocations();

    return locations.length > 0 && locations.every(({ row, step }) => stepSkipped[row][step]);
  });
  let bulkMuteActive = $derived.by(() => {
    const locations = bulkEditLocations();

    return locations.length > 0 && locations.every(({ row, step }) => stepMuted[row][step]);
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

  function bulkEditLocations() {
    const editor = activeRowPianoRollEditor;

    if (editor === null) {
      const selected = selectedStepLocations();

      if (selected.length > 0) {
        return selected;
      }

      const locations = [];

      for (let row = 0; row < stepIds.length; row += 1) {
        for (let step = 0; step < (stepIds[row]?.length ?? 0); step += 1) {
          const stepId = stepIds[row][step];
          locations.push({ row, step, key: stepSelectionKey(row, stepId) });
        }
      }

      return locations;
    }

    const row = editor.row;
    const rowStepIds = stepIds[row] ?? [];
    const locations = [];

    for (const key of selectedStepKeysForGrid) {
      if (rowFromStepSelectionKey(key) !== row) continue;

      const stepId = key.substring(key.indexOf(":") + 1);
      const step = rowStepIds.indexOf(stepId);

      if (step >= 0) {
        locations.push({ row, step, key });
      }
    }

    if (locations.length === 0) {
      for (let step = 0; step < rowStepIds.length; step += 1) {
        const stepId = rowStepIds[step];
        locations.push({ row, step, key: stepSelectionKey(row, stepId) });
      }
    }

    return locations;
  }

  function bulkEditLocationsByPosition() {
    return bulkEditLocations().sort((left, right) =>
      left.row === right.row ? left.step - right.step : left.row - right.row,
    );
  }

  function bulkEditLocationsGroupedByRow() {
    const groups = new SvelteMap();

    for (const location of bulkEditLocationsByPosition()) {
      const rowLocations = groups.get(location.row) ?? [];
      rowLocations.push(location);
      groups.set(location.row, rowLocations);
    }

    return groups;
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

  /** @param {HTMLElement} element */
  function toggleStepSelectionFromCellElement(element) {
    const row = Number.parseInt(element.dataset.stepRow ?? "-1", 10);
    const stepId = element.dataset.stepId;

    if (Number.isNaN(row) || !stepId) return false;

    const key = stepSelectionKey(row, stepId);
    if (!selectableStepKeySet.has(key)) return false;

    const next = new SvelteSet(selectedStepKeysForGrid);

    if (next.has(key)) {
      next.delete(key);
    } else {
      next.add(key);
    }

    setSelectedStepKeys(next);
    syncBulkControlsFromSelection();

    return true;
  }

  let activeStepInspector = $derived.by(() => {
    if (inspectedStep === null) return null;

    const row = inspectedStep.row;
    const step = stepIds[row]?.indexOf(inspectedStep.stepId) ?? -1;

    if (row < 0 || step < 0) return null;

    return {
      row,
      step,
      note: grid[row][step],
      velocity: stepVelocity[row][step] ?? 127,
      durationFraction: stepDurationFraction[row][step] ?? defaultStepDurationFraction,
      timingMultiplierIndex: stepTimingMultiplier[row][step] ?? defaultStepTimingMultiplierIndex,
      probability: stepProbability[row][step] ?? defaultStepProbabilityValue,
      cycle: stepCycle[row][step] ?? 1,
      cycleMask: stepCycleOffset[row][step] ?? defaultStepCycleMask,
      cycleTriggerCount: stepTriggerCountAtBeat({
        beat: playbackBeat,
        step,
        rowTimingOffset: rowTimingOffset[row],
        stepTimingMultiplier: stepTimingMultiplier[row],
        stepSkipped: stepSkipped[row],
        pulseIndex,
      }),
      muted: stepMuted[row][step] ?? false,
      skipped: stepSkipped[row][step] ?? false,
    };
  });

  let activeStepInspectorBulkEditCount = $derived.by(() => {
    if (activeStepInspector === null) return 0;

    return inspectorEditLocations(activeStepInspector.row, activeStepInspector.step).length;
  });

  let activeRowPianoRollEditor = $derived.by(() => {
    if (rowPianoRollStep === null) return null;

    const row = rowPianoRollStep.row;
    const step = stepIds[row]?.indexOf(rowPianoRollStep.stepId) ?? -1;

    if (row < 0 || step < 0) return null;

    return {
      row,
      step,
      stepId: rowPianoRollStep.stepId,
    };
  });

  let rowPianoRollCurrentStepFocusVisible = $derived(
    activeRowPianoRollEditor !== null && stretchStepsToFit,
  );

  async function removeInspectedStep() {
    if (activeStepInspector === null) return;

    const { row, step } = activeStepInspector;
    const locations = inspectorEditLocations(row, step);

    closeStepInspector();
    await removeStepsAtLocations(locations);
  }

  function closeStepInspector() {
    if (document.activeElement instanceof HTMLElement) {
      document.activeElement.blur();
    }

    inspectedStep = null;

    queueMicrotask(() => {
      if (document.activeElement instanceof HTMLElement) {
        document.activeElement.blur();
      }
    });
  }

  function closeRowPianoRollEditor() {
    if (document.activeElement instanceof HTMLElement) {
      document.activeElement.blur();
    }

    rowPianoRollStep = null;
    setSelectedStepKeys(new Set());
    syncBulkControlsFromSelection();

    queueMicrotask(() => {
      if (document.activeElement instanceof HTMLElement) {
        document.activeElement.blur();
      }
    });
  }

  /** @param {number} row @param {number} step @param {string} stepId */
  async function openStepInspector(row, step, stepId) {
    if (recordingRow !== null) {
      await finishRowRecording();
    }

    rowPianoRollStep = null;

    if (inspectedStep?.row === row && inspectedStep.stepId === stepId) {
      closeStepInspector();
      return;
    }

    const key = stepSelectionKey(row, stepId);

    if (selectedStepKeysForGrid.size <= 1 && !selectedStepKeysForGrid.has(key)) {
      setSelectedStepKeys(new Set([key]));
      syncBulkControlsFromSelection();
    }

    inspectedStep = { row, stepId };
  }

  /** @param {number} row @param {number} step @param {string} stepId */
  function prepareStepSelection(row, step, stepId) {
    const key = stepSelectionKey(row, stepId);

    if (selectedStepKeysForGrid.size <= 1 && !selectedStepKeysForGrid.has(key)) {
      setSelectedStepKeys(new Set([key]));
      syncBulkControlsFromSelection();
    }
  }

  /** @param {number} row @param {number} step @param {string} stepId @param {{ selectStep?: boolean }} [options] */
  async function openRowPianoRollEditor(row, step, stepId, { selectStep = true } = {}) {
    if (recordingRow !== null) {
      await finishRowRecording();
    }

    inspectedStep = null;
    rowPianoRollStep = { row, stepId };

    if (selectStep) {
      setSelectedStepKeys(new Set([stepSelectionKey(row, stepId)]));
    } else {
      setSelectedStepKeys(new Set());
    }

    syncBulkControlsFromSelection();
  }

  /** @param {number} row */
  async function targetRowSeedMode(row) {
    if (projectOperationBusy || row < 0 || row >= grid.length) return;

    inspectedStep = null;
    rowPianoRollStep = null;

    if (recordingRow !== null) {
      await finishRowRecording();
    }

    if (document.activeElement instanceof HTMLElement) {
      document.activeElement.blur();
    }

    setSelectedStepKeys(new Set());
    syncBulkControlsFromSelection();
    cancelMarqueeSelection();

    seedModeActive = true;
    const nextTargets = seedModeRowTargets.map((_, index) => index === row);

    void commitSeedModeState(seedModeTargetLabel(nextTargets), {
      ...currentSeedModeState(),
      rowTargets: nextTargets,
    });
  }

  /** @param {number} row */
  async function openRowPianoRollFromHeader(row) {
    const rowStepIds = stepIds[row] ?? [];
    if (rowStepIds.length === 0) return;

    if (activeRowPianoRollEditor?.row === row) {
      closeRowPianoRollEditor();
      return;
    }

    const focusedStepId =
      activeStepInspector?.row === row ? inspectedStep?.stepId : null;
    const stepId = focusedStepId && rowStepIds.includes(focusedStepId)
      ? focusedStepId
      : rowStepIds[0];
    const step = rowStepIds.indexOf(stepId);

    await openRowPianoRollEditor(row, step, stepId, { selectStep: false });
  }

  function selectAllStepsForBulkEdit() {
    setSelectedStepKeys(new Set(allSelectableStepKeys()));
    syncBulkControlsFromSelection();
  }

  /** @type {{ row: number, timeStamp: number, clientX: number, clientY: number } | null} */
  let lastRowHeaderClick = null;
  const rowHeaderDoubleClickMs = 320;
  const rowHeaderDoubleClickDistancePx = 8;

  /** @param {number} row */
  function selectAllStepsInRow(row) {
    const keys = (stepIds[row] ?? []).map((stepId) => stepSelectionKey(row, stepId));
    setSelectedStepKeys(new Set(keys));
    syncBulkControlsFromSelection();
  }

  /** @param {PointerEvent} event @param {number} row */
  function isFastRowHeaderDoubleClick(event, row) {
    if (lastRowHeaderClick === null || lastRowHeaderClick.row !== row) return false;

    const elapsed = event.timeStamp - lastRowHeaderClick.timeStamp;
    const deltaX = event.clientX - lastRowHeaderClick.clientX;
    const deltaY = event.clientY - lastRowHeaderClick.clientY;
    const distance = Math.hypot(deltaX, deltaY);

    return elapsed >= 0 &&
      elapsed <= rowHeaderDoubleClickMs &&
      distance <= rowHeaderDoubleClickDistancePx;
  }

  /** @param {PointerEvent} event */
  function shouldIgnoreRowHeaderClick(event) {
    const target = event.target;

    if (!(target instanceof Element)) return true;

    return Boolean(
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [role='slider']",
      ),
    );
  }

  /** @param {PointerEvent} event @param {number} row */
  function handleRowHeaderPointerUp(event, row) {
    if (event.button !== 0 || shouldIgnoreRowHeaderClick(event)) return;

    event.preventDefault();
    event.stopPropagation();

    if (isFastRowHeaderDoubleClick(event, row)) {
      lastRowHeaderClick = null;
      void clearPhraseRow(row);
      return;
    }

    lastRowHeaderClick = {
      row,
      timeStamp: event.timeStamp,
      clientX: event.clientX,
      clientY: event.clientY,
    };
    selectAllStepsInRow(row);
  }

  function syncBulkControlsFromSelection() {
    const [first] = selectedStepLocations();

    bulkDurationPercent = 0;
    bulkVelocityPercent = 0;
    bulkLengthDelta = 0;
    bulkTransposeSemitones = 0;

    if (!first) {
      return;
    }
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

  function dismissPhraseEditingFocus() {
    closeStepInspector();
    closeRowPianoRollEditor();

    if (selectedStepKeysForGrid.size > 0) {
      setSelectedStepKeys(new Set());
      syncBulkControlsFromSelection();
    }

    cancelMarqueeSelection();
  }

  /** @param {PointerEvent} event */
  function handleRowGapBulkSelectPointerDown(event) {
    if (event.button !== 0) return;

    const now = performance.now();
    const elapsed = now - lastRowGapPointerDownTime;
    const distance = Math.hypot(
      event.clientX - lastRowGapPointerDownX,
      event.clientY - lastRowGapPointerDownY,
    );

    if (
      lastRowGapPointerDownTime > 0 &&
      elapsed <= rowGapDoubleClickIntervalMs &&
      distance <= rowGapDoubleClickMaxDistancePx
    ) {
      lastRowGapPointerDownTime = 0;
      event.preventDefault();
      event.stopPropagation();
      selectAllStepsForBulkEdit();
      return;
    }

    dismissPhraseEditingFocus();

    lastRowGapPointerDownTime = now;
    lastRowGapPointerDownX = event.clientX;
    lastRowGapPointerDownY = event.clientY;

    beginStepMarqueeSelection(event);
  }

  const phraseStepMarqueeBlockSelector =
    "button, input, textarea, select, a, [contenteditable='true'], [role='slider'], [data-bulk-step-cell], [data-no-marquee], [data-no-long-press], [data-insert-slot], [data-remove-button], [data-multiplier-resize]";

  const compactStepMarqueeBlockSelector =
    "button, input, textarea, select, a, [contenteditable='true'], [role='slider'], [data-no-marquee], [data-no-long-press], [data-insert-slot], [data-remove-button], [data-multiplier-resize]";

  /** @param {PointerEvent} event @param {{ clientX?: number, clientY?: number, addToSelection?: boolean, toggleStep?: boolean }} [origin] */
  function beginStepMarqueeSelection(event, origin) {
    if ((!origin && event.button !== 0) || marqueeSelection) return;

    const target = event.target;

    if (!(target instanceof Element)) return;

    const stepCell = target.closest("[data-bulk-step-cell]");
    const compactStepCell =
      stepCell instanceof HTMLElement && stepCell.hasAttribute("data-compact-step-cell");

    if (((event.shiftKey && !origin) || origin?.toggleStep) && stepCell instanceof HTMLElement) {
      if (toggleStepSelectionFromCellElement(stepCell)) {
        event.preventDefault();
        event.stopPropagation();
      }
      return;
    }

    const marqueeBlockSelector = compactStepCell
      ? compactStepMarqueeBlockSelector
      : phraseStepMarqueeBlockSelector;

    if (target.closest(marqueeBlockSelector)) {
      return;
    }

    event.preventDefault();
    event.stopPropagation();

    const startX = origin?.clientX ?? event.clientX;
    const startY = origin?.clientY ?? event.clientY;
    const addToSelection = origin?.addToSelection ?? event.shiftKey;

    marqueeSelection = {
      startX,
      startY,
      currentX: event.clientX,
      currentY: event.clientY,
      addToSelection,
      baseKeys: addToSelection ? new Set(selectedStepKeysForGrid) : new Set(),
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
      combinationModeMask,
      combinationSyncDivisionIndex,
      combinationChangePhaseContinue,
      scaleRoot,
      scaleModeIndex,
      noteBandpassLowMidi,
      noteBandpassHighMidi,
      velocityTiltPivotMidi,
      velocityTiltAmount,
      globalTransposeSemitones,
      octavizerDown8vaEnabled,
      octavizerUp8vaEnabled,
      octavizerDown8vaRelativeVelocity,
      octavizerUp8vaRelativeVelocity,
      shimmerEnabled,
      shimmerDelayMultiplierIndex,
      shimmerFeedbackPercent,
      shimmerMixPercent,
      loopBraceEnabled,
      loopBraceStart,
      loopBraceEnd,
      seedModeRhythmStep,
      seedModeRowSettings: seedModeRowSettings.map((settings) => ({ ...settings })),
      seedModeRowTargets: [...seedModeRowTargets],
    };
  }

  function cloneSeedModeSnapshotState(snapshot) {
    const normalized = normalizeSeedModeState({
      rhythmStep: snapshot.seedModeRhythmStep ?? defaultSeedModeState.rhythmStep,
      rowSettings: snapshot.seedModeRowSettings ?? defaultSeedModeState.rowSettings,
      rowTargets: snapshot.seedModeRowTargets ?? defaultSeedModeState.rowTargets,
    });

    return {
      seedModeRhythmStep: normalized.rhythmStep,
      seedModeRowSettings: normalized.rowSettings.map((settings) => ({ ...settings })),
      seedModeRowTargets: [...normalized.rowTargets],
    };
  }

  function cloneSnapshot(snapshot) {
    const seedModeState = cloneSeedModeSnapshotState(snapshot);

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
      ...seedModeState,
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
    combinationModeMask = (next.combinationModeMask ?? 0) & combinationModeMaskBits;
    combinationSyncDivisionIndex =
      next.combinationSyncDivisionIndex ?? defaultCombinationSyncDivisionIndex;
    combinationChangePhaseContinue = Boolean(next.combinationChangePhaseContinue);
    scaleRoot = clampScaleRoot(next.scaleRoot ?? defaultScaleRoot);
    scaleModeIndex = clampScaleModeIndex(next.scaleModeIndex ?? defaultScaleModeIndex);
    setNoteBandpassState(
      next.noteBandpassLowMidi ?? defaultNoteBandpassLowMidi,
      next.noteBandpassHighMidi ?? defaultNoteBandpassHighMidi,
    );
    setVelocityTiltState({
      pivotMidi: next.velocityTiltPivotMidi ?? defaultVelocityTiltPivotMidi,
      amount: next.velocityTiltAmount ?? defaultVelocityTiltAmount,
    });
    setGlobalTransposeState(
      next.globalTransposeSemitones ?? defaultGlobalTransposeSemitones,
    );
    setOctavizerState({
      down8vaEnabled: next.octavizerDown8vaEnabled ?? false,
      up8vaEnabled: next.octavizerUp8vaEnabled ?? false,
      down8vaRelativeVelocity:
        next.octavizerDown8vaRelativeVelocity ?? defaultOctavizerRelativeVelocity,
      up8vaRelativeVelocity:
        next.octavizerUp8vaRelativeVelocity ?? defaultOctavizerRelativeVelocity,
    });
    setShimmerState({
      enabled: next.shimmerEnabled ?? false,
      delayMultiplierIndex:
        next.shimmerDelayMultiplierIndex ?? defaultShimmerDelayMultiplierIndex,
      feedbackPercent: next.shimmerFeedbackPercent ?? defaultShimmerFeedbackPercent,
      mixPercent: next.shimmerMixPercent ?? defaultShimmerMixPercent,
    });
    loopBraceEnabled = next.loopBraceEnabled;
    loopBraceStart = next.loopBraceStart;
    loopBraceEnd = next.loopBraceEnd;
    seedModeRhythmStep = next.seedModeRhythmStep;
    seedModeRowSettings = next.seedModeRowSettings;
    seedModeRowTargets = next.seedModeRowTargets;
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
    recordCycleLengthPulses = [...(state.recordCycleLengthPulses ?? recordCycleLengthPulses)]
      .map((value) => Math.min(64, Math.max(1, Number.parseInt(String(value), 10) || 16)));
    stepDurationFraction = cloneMatrix(state.phraseStepDurationFraction ?? stepDurationFraction);
    stepTimingMultiplier = cloneMatrix(state.phraseStepTimingMultiplier ?? stepTimingMultiplier);
    stepVelocity = cloneMatrix(state.phraseStepVelocity ?? stepVelocity);
    stepMuted = cloneMatrix(state.phraseStepMuted ?? stepMuted).map((row) => row.map(Boolean));
    stepSkipped = cloneMatrix(state.phraseStepSkipped ?? stepSkipped).map((row) => row.map(Boolean));
    stepProbability = cloneMatrix(state.phraseStepProbability ?? stepProbability);
    stepCycle = cloneMatrix(state.phraseStepCycle ?? stepCycle);
    stepCycleOffset = cloneMatrix(state.phraseStepCycleOffset ?? stepCycleOffset);
    combinationModeMask = Number.parseInt(String(state.combinationModeMask ?? 0), 10) & combinationModeMaskBits;
    scaleRoot = clampScaleRoot(state.scaleRoot ?? defaultScaleRoot);
    scaleModeIndex = clampScaleModeIndex(state.scaleModeIndex ?? defaultScaleModeIndex);
    setNoteBandpassState(
      Number.parseInt(String(state.noteBandpassLowMidi ?? defaultNoteBandpassLowMidi), 10),
      Number.parseInt(String(state.noteBandpassHighMidi ?? defaultNoteBandpassHighMidi), 10),
    );
    setVelocityTiltState({
      pivotMidi: Number.parseInt(
        String(state.velocityTiltPivotMidi ?? defaultVelocityTiltPivotMidi),
        10,
      ),
      amount: Number.parseInt(
        String(state.velocityTiltAmount ?? defaultVelocityTiltAmount),
        10,
      ),
    });
    setGlobalTransposeState(
      Number.parseInt(
        String(state.globalTransposeSemitones ?? defaultGlobalTransposeSemitones),
        10,
      ),
    );
    setOctavizerState({
      down8vaEnabled: Boolean(Number.parseInt(String(state.octavizerDown8vaEnabled ?? 0), 10)),
      up8vaEnabled: Boolean(Number.parseInt(String(state.octavizerUp8vaEnabled ?? 0), 10)),
      down8vaRelativeVelocity: Number.parseInt(
        String(state.octavizerDown8vaRelativeVelocity ?? defaultOctavizerRelativeVelocity),
        10,
      ),
      up8vaRelativeVelocity: Number.parseInt(
        String(state.octavizerUp8vaRelativeVelocity ?? defaultOctavizerRelativeVelocity),
        10,
      ),
    });
    setShimmerState({
      enabled: Boolean(Number.parseInt(String(state.shimmerEnabled ?? 0), 10)),
      delayMultiplierIndex: Number.parseInt(
        String(state.shimmerDelayMultiplierIndex ?? defaultShimmerDelayMultiplierIndex),
        10,
      ),
      feedbackPercent: Number.parseInt(
        String(state.shimmerFeedbackPercent ?? defaultShimmerFeedbackPercent),
        10,
      ),
      mixPercent: Number.parseInt(
        String(state.shimmerMixPercent ?? defaultShimmerMixPercent),
        10,
      ),
    });
    loopBraceEnabled = Boolean(Number.parseInt(String(state.loopBraceEnabled ?? 0), 10));
    loopBraceStart = Number.parseFloat(String(state.loopBraceStart ?? 0));
    loopBraceEnd = Number.parseFloat(String(state.loopBraceEnd ?? 8));
    activeGates = grid.map((row) => row.map(() => false));
    stepIds = createStepIdsForGrid(grid);
    setSelectedStepKeys(new Set());
    bulkDurationPercent = 0;
    bulkVelocityPercent = 0;
    bulkLengthDelta = 0;
    bulkTransposeSemitones = 0;
    assignSeedModeStateFromPattern(state);
    seedModeActive = false;
    seedModeGestureBefore = null;
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

    if (nativeFunctionAvailable("setCombinationSyncDivisionIndex")) {
      await getNativeFunction("setCombinationSyncDivisionIndex")(
        snapshot.combinationSyncDivisionIndex ?? defaultCombinationSyncDivisionIndex,
      );
    }

    if (nativeFunctionAvailable("setCombinationChangePhaseContinue")) {
      await getNativeFunction("setCombinationChangePhaseContinue")(
        snapshot.combinationChangePhaseContinue ? 1 : 0,
      );
    }

    if (nativeFunctionAvailable("setCombinationModeEnabled")) {
      const setCombinationModeEnabled = getNativeFunction("setCombinationModeEnabled");

      for (const mode of combinationModes) {
        await setCombinationModeEnabled(mode.index, (snapshot.combinationModeMask & mode.bit) !== 0 ? 1 : 0);
      }
    }

    if (nativeFunctionAvailable("setPatternScale")) {
      await getNativeFunction("setPatternScale")(
        snapshot.scaleRoot ?? defaultScaleRoot,
        snapshot.scaleModeIndex ?? defaultScaleModeIndex,
      );
    }

    if (nativeFunctionAvailable("setPatternNoteBandpass")) {
      await getNativeFunction("setPatternNoteBandpass")(
        snapshot.noteBandpassLowMidi ?? defaultNoteBandpassLowMidi,
        snapshot.noteBandpassHighMidi ?? defaultNoteBandpassHighMidi,
      );
    }

    setVelocityTiltState({
      pivotMidi: snapshot.velocityTiltPivotMidi ?? defaultVelocityTiltPivotMidi,
      amount: snapshot.velocityTiltAmount ?? defaultVelocityTiltAmount,
    });
    await syncVelocityTiltToNative();

    setGlobalTransposeState(
      snapshot.globalTransposeSemitones ?? defaultGlobalTransposeSemitones,
    );
    await syncGlobalTransposeToNative();

    setOctavizerState({
      down8vaEnabled: snapshot.octavizerDown8vaEnabled ?? false,
      up8vaEnabled: snapshot.octavizerUp8vaEnabled ?? false,
      down8vaRelativeVelocity:
        snapshot.octavizerDown8vaRelativeVelocity ?? defaultOctavizerRelativeVelocity,
      up8vaRelativeVelocity:
        snapshot.octavizerUp8vaRelativeVelocity ?? defaultOctavizerRelativeVelocity,
    });
    await syncOctavizerToNative();

    setShimmerState({
      enabled: snapshot.shimmerEnabled ?? false,
      delayMultiplierIndex:
        snapshot.shimmerDelayMultiplierIndex ?? defaultShimmerDelayMultiplierIndex,
      feedbackPercent: snapshot.shimmerFeedbackPercent ?? defaultShimmerFeedbackPercent,
      mixPercent: snapshot.shimmerMixPercent ?? defaultShimmerMixPercent,
    });
    await syncShimmerToNative();

    if (snapshot.loopBraceStart > previousSnapshot.loopBraceStart) {
      await pushLoopBraceEnd(snapshot.loopBraceEnd);
      await pushLoopBraceStart(snapshot.loopBraceStart);
    } else {
      await pushLoopBraceStart(snapshot.loopBraceStart);
      await pushLoopBraceEnd(snapshot.loopBraceEnd);
    }

    await pushLoopBraceEnabled(snapshot.loopBraceEnabled);
    await pushSeedModeStateToNative();
  }

  async function applyHistorySnapshot(snapshot) {
    const previousSnapshot = createHistorySnapshot();
    assignSnapshot(snapshot);
    await syncSnapshotToNative(snapshot, previousSnapshot);
  }

  async function undo() {
    seedModeGestureBefore = null;

    const entry = undoStack[undoStack.length - 1];

    if (!entry) return;

    undoStack = undoStack.slice(0, -1);
    redoStack = [...redoStack, entry];
    await applyHistorySnapshot(entry.before);
  }

  async function redo() {
    seedModeGestureBefore = null;

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

  /**
   * @param {number} targetRow
   * @param {string} stepId
   * @param {string[]} orderedTargetIds
   * @param {number} [shadowIndex]
   */
  async function moveStepToRow(targetRow, stepId, orderedTargetIds, shadowIndex = -1) {
    const sourceRow = stepIds.findIndex((ids) => ids.includes(stepId));

    if (sourceRow < 0 || sourceRow === targetRow) return;
    if (stepIds[targetRow]?.length >= maxPhraseStepsPerRow) return;

    const previewIndex = orderedTargetIds.indexOf(stepId);
    const insertionIndex = previewIndex >= 0
      ? previewIndex
      : blockCrossRowInsertionIndex(
        stepIds[targetRow],
        stepId,
        orderedTargetIds,
        shadowIndex,
      );

    if (insertionIndex < 0) return;

    const before = createHistorySnapshot();
    const selectedKeysBefore = new SvelteSet(selectedStepKeysForGrid);
    const sourceSelectionKey = stepSelectionKey(sourceRow, stepId);
    const wasSelected = selectedKeysBefore.delete(sourceSelectionKey);
    const result = moveStepBetweenRows(
      {
        grid,
        stepDurationFraction,
        stepTimingMultiplier,
        stepVelocity,
        stepMuted,
        stepSkipped,
        stepProbability,
        stepCycle,
        stepCycleOffset,
        activeGates,
      },
      stepIds,
      sourceRow,
      targetRow,
      stepId,
      orderedTargetIds,
      insertionIndex,
    );

    if (!result) return;

    ({
      grid,
      stepDurationFraction,
      stepTimingMultiplier,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      activeGates,
    } = result.matrices);
    stepIds = result.stepIds;

    if (inspectedStep?.stepId === stepId) {
      inspectedStep = { row: targetRow, stepId };
    }

    if (rowPianoRollStep?.stepId === stepId) {
      rowPianoRollStep = { row: targetRow, stepId };
    }

    const after = createHistorySnapshot();
    pushHistoryEntry("Move step to row", before, after);

    await tick();

    if (wasSelected) {
      selectedKeysBefore.add(stepSelectionKey(targetRow, stepId));
      setSelectedStepKeys(selectedKeysBefore);
      syncBulkControlsFromSelection();
    }

    await pushCurrentPhraseRow(sourceRow);
    await pushCurrentPhraseRow(targetRow);
  }

  /**
   * @param {number} targetRow
   * @param {string} movedStepId
   * @param {string[]} blockIds
   * @param {string[]} orderedTargetPreview
   * @param {number} [shadowIndex]
   */
  async function moveBulkToRow(targetRow, movedStepId, blockIds, orderedTargetPreview, shadowIndex = -1) {
    if (targetRow < 0 || targetRow >= stepIds.length) return;

    const blockLocations = stepLocationsForIds(blockIds);
    const block = blockLocations.map((location) => location.id);

    if (block.length === 0 || !block.includes(movedStepId)) return;

    const previewIndex = orderedTargetPreview.indexOf(movedStepId);
    const insertionIndex = previewIndex >= 0
      ? previewIndex
      : blockCrossRowInsertionIndex(
        stepIds[targetRow],
        movedStepId,
        orderedTargetPreview,
        shadowIndex,
      );

    if (insertionIndex < 0) return;

    const before = createHistorySnapshot();
    const selectedKeysBefore = new SvelteSet(selectedStepKeysForGrid);
    const result = moveBlockFromRowsToRow(
      {
        grid,
        stepDurationFraction,
        stepTimingMultiplier,
        stepVelocity,
        stepMuted,
        stepSkipped,
        stepProbability,
        stepCycle,
        stepCycleOffset,
        activeGates,
      },
      stepIds,
      targetRow,
      block,
      movedStepId,
      insertionIndex,
    );

    if (!result) return;

    ({
      grid,
      stepDurationFraction,
      stepTimingMultiplier,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      activeGates,
    } = result.matrices);
    stepIds = result.stepIds;

    if (inspectedStep && block.includes(inspectedStep.stepId)) {
      inspectedStep = { row: targetRow, stepId: inspectedStep.stepId };
    }

    if (rowPianoRollStep && block.includes(rowPianoRollStep.stepId)) {
      rowPianoRollStep = { row: targetRow, stepId: rowPianoRollStep.stepId };
    }

    const after = createHistorySnapshot();
    pushHistoryEntry("Move steps to row", before, after);

    await tick();

    for (const { id, row } of blockLocations) {
      selectedKeysBefore.delete(stepSelectionKey(row, id));
      selectedKeysBefore.add(stepSelectionKey(targetRow, id));
    }

    setSelectedStepKeys(selectedKeysBefore);
    syncBulkControlsFromSelection();

    const affectedRows = new SvelteSet(blockLocations.map((location) => location.row));
    affectedRows.add(targetRow);

    for (const row of affectedRows) {
      await pushCurrentPhraseRow(row);
    }
  }

  /**
   * @param {number} targetRow
   * @param {string} movedStepId
   * @param {string[]} blockIds
   * @param {string[]} previewIds
   * @param {number} shadowIndex
   */
  async function duplicateBulkToRow(targetRow, movedStepId, blockIds, previewIds, shadowIndex) {
    if (targetRow < 0 || targetRow >= stepIds.length) return;

    const block = stepLocationsForIds(blockIds).map((location) => location.id);

    if (block.length === 0 || !block.includes(movedStepId)) return;

    const insertionIndex = stepIds[targetRow].includes(movedStepId)
      ? blockDuplicateInsertionIndex(
        stepIds[targetRow],
        block.filter((id) => stepIds[targetRow].includes(id)),
        shadowIndex,
      )
      : blockCrossRowInsertionIndex(
        stepIds[targetRow],
        movedStepId,
        previewIds,
        shadowIndex,
      );

    if (insertionIndex < 0) return;

    const before = createHistorySnapshot();
    const result = duplicateBlockFromRowsToRow(
      {
        grid,
        stepDurationFraction,
        stepTimingMultiplier,
        stepVelocity,
        stepMuted,
        stepSkipped,
        stepProbability,
        stepCycle,
        stepCycleOffset,
        activeGates,
      },
      stepIds,
      targetRow,
      block,
      insertionIndex,
      createStepId,
    );

    if (!result) return;

    ({
      grid,
      stepDurationFraction,
      stepTimingMultiplier,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      activeGates,
    } = result.matrices);
    stepIds = result.stepIds;

    for (const newId of result.newIds) {
      const duplicateIndex = stepIds[targetRow].indexOf(newId);

      if (duplicateIndex >= 0) {
        activeGates[targetRow][duplicateIndex] = false;
      }
    }

    const after = createHistorySnapshot();
    pushHistoryEntry("Duplicate steps to row", before, after);

    const nextSelectedKeys = new SvelteSet(selectedStepKeysForGrid);

    for (const newId of result.newIds) {
      nextSelectedKeys.add(stepSelectionKey(targetRow, newId));
    }

    setSelectedStepKeys(nextSelectedKeys);
    syncBulkControlsFromSelection();

    await tick();
    await pushCurrentPhraseRow(targetRow);
  }

  /**
   * @param {number} targetRow
   * @param {string} stepId
   * @param {string[]} orderedTargetIds
   * @param {number} [insertionIndexOrShadow]
   */
  async function duplicateStepToDrop(targetRow, stepId, orderedTargetIds, insertionIndexOrShadow = undefined) {
    const sourceRow = stepIds.findIndex((ids) => ids.includes(stepId));

    if (sourceRow < 0 || targetRow < 0 || targetRow >= stepIds.length) return;
    if (stepIds[targetRow]?.length >= maxPhraseStepsPerRow) return;

    const previewIndex = orderedTargetIds.indexOf(stepId);
    let insertionIndex = null;

    if (previewIndex < 0) {
      insertionIndex = blockCrossRowInsertionIndex(
        stepIds[targetRow],
        stepId,
        orderedTargetIds,
        insertionIndexOrShadow ?? -1,
      );
    } else if (sourceRow === targetRow && insertionIndexOrShadow != null && insertionIndexOrShadow >= 0) {
      insertionIndex = insertionIndexOrShadow;
    }

    const before = createHistorySnapshot();
    const duplicateId = createStepId();
    const result = duplicateStepBetweenRows(
      {
        grid,
        stepDurationFraction,
        stepTimingMultiplier,
        stepVelocity,
        stepMuted,
        stepSkipped,
        stepProbability,
        stepCycle,
        stepCycleOffset,
        activeGates,
      },
      stepIds,
      sourceRow,
      targetRow,
      stepId,
      orderedTargetIds,
      duplicateId,
      insertionIndex,
    );

    if (!result) return;

    ({
      grid,
      stepDurationFraction,
      stepTimingMultiplier,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      activeGates,
    } = result.matrices);
    stepIds = result.stepIds;

    const duplicateIndex = stepIds[targetRow].indexOf(duplicateId);
    activeGates[targetRow][duplicateIndex] = false;

    const after = createHistorySnapshot();
    pushHistoryEntry("Duplicate step", before, after);

    await tick();
    await pushCurrentPhraseRow(targetRow);
  }

  /** JUCE wraps each withInitialisationData value as [payload]. */
  function unwrapJuceInit(key) {
    const raw = window.__JUCE__?.initialisationData?.[key];

    if (!Array.isArray(raw) || raw.length === 0) return null;

    if (raw.length === 1 && Array.isArray(raw[0])) return raw[0];

    return raw;
  }

  /** Native completion payloads may be wrapped as [object]. */
  function unwrapJuceNativeResult(result) {
    if (Array.isArray(result) && result.length === 1) {
      const [only] = result;

      if (only !== null && typeof only === "object") return only;
      return only;
    }

    return result;
  }

  function nativeScalar(value, fallback = "") {
    if (value === null || value === undefined) return fallback;

    if (Array.isArray(value)) {
      if (value.length === 0) return fallback;
      return nativeScalar(value[0], fallback);
    }

    return value;
  }

  function nativeFunctionAvailable(name) {
    return window.__JUCE__?.initialisationData?.__juce__functions?.includes?.(name) ?? false;
  }

  function formatProjectDate(value) {
    if (!value) return "Unsaved";

    const date = new Date(value);
    if (Number.isNaN(date.getTime())) return String(value);

    return new Intl.DateTimeFormat(undefined, {
      year: "numeric",
      month: "short",
      day: "numeric",
    }).format(date);
  }

  function assignProjectMetadata(state) {
    if (!state || typeof state !== "object" || Array.isArray(state)) return;

    projectName = String(nativeScalar(state.projectName, "Untitled Project"));
    projectDescription = String(nativeScalar(state.projectDescription, ""));
    projectCreatedAt = String(nativeScalar(state.projectCreatedAt, ""));
    projectModifiedAt = String(nativeScalar(state.projectModifiedAt, ""));
    projectFileName = String(nativeScalar(state.projectFileName, ""));
    hasPreviousProject = Boolean(
      Number.parseInt(String(nativeScalar(state.hasPreviousProject, 0)), 10),
    );
    hasNextProject = Boolean(Number.parseInt(String(nativeScalar(state.hasNextProject, 0)), 10));
  }

  async function refreshProjectState({ loadProjectContent = false } = {}) {
    if (!nativeFunctionAvailable("getProjectState")) return;

    const state = unwrapJuceNativeResult(await getNativeFunction("getProjectState")());
    if (!state || typeof state !== "object" || Array.isArray(state)) return;

    assignProjectMetadata(state);
    if (!loadProjectContent) return;

    activePatternSlot = Number.parseInt(String(state.currentPatternSlot ?? 0), 10);
    viewPatternSlot = Number.parseInt(String(state.viewPatternSlot ?? 0), 10);
    activeLoopSlot = Number.parseInt(String(state.currentLoopSlot ?? -1), 10);
    pulseIndex = Number.parseInt(String(state.pulseIndex ?? defaultPulseIndex), 10);
    combinationSyncDivisionIndex = Math.min(
      combinationSyncDivisionOptions.length - 1,
      Math.max(
        0,
        Number.parseInt(
          String(state.combinationSyncDivisionIndex ?? defaultCombinationSyncDivisionIndex),
          10,
        ),
      ),
    );
    combinationChangePhaseContinue = Boolean(
      Number.parseInt(String(state.combinationChangePhaseContinue ?? 0), 10),
    );
    swingPercent = Number.parseInt(String(state.swingPercent ?? 0), 10);
    velocityHumanizePercent = Number.parseInt(String(state.velocityHumanizePercent ?? 0), 10);
    timingHumanizePercent = Number.parseInt(String(state.timingHumanizePercent ?? 0), 10);
    swingSubdivisionIndex = Number.parseInt(String(state.swingSubdivisionIndex ?? 1), 10);
    rowColorsEnabled = Boolean(Number.parseInt(String(state.rowColorsEnabled ?? 1), 10));
    standaloneTempoBpm = Number.parseFloat(String(state.standaloneTempoBpm ?? 120));
    loopSlotAssigned = [...(state.loopSlotAssigned ?? loopSlotAssigned)].map(Boolean);
    loopSlotPattern = [...(state.loopSlotPattern ?? loopSlotPattern)].map((value) =>
      Number.parseInt(String(value), 10),
    );
    stretchStepsToFit = Boolean(
      Number.parseInt(String(state.projectStretchStepsToFit ?? 0), 10),
    );

    const nextTheme = String(state.projectThemeMode ?? defaultThemeMode);
    themeMode = applyThemeMode(nextTheme);
    setUiScalePercent(state.projectUiScalePercent ?? 100);
    await syncEditorScaleMinimumToNative();
    assignPatternState(state.patternState, activePatternSlot >= 0);
  }

  async function runProjectOperation(nativeName, args = [], { loadProjectContent = false } = {}) {
    if (projectOperationBusy || !nativeFunctionAvailable(nativeName)) return false;

    projectOperationBusy = true;
    projectOperationError = "";
    try {
      const result = unwrapJuceNativeResult(await getNativeFunction(nativeName)(...args));
      const success = Boolean(Number.parseInt(String(nativeScalar(result?.success, 0)), 10));
      const error = String(nativeScalar(result?.error, ""));

      if (error) projectOperationError = error;
      if (success) {
        assignProjectMetadata(result);
        await refreshProjectState({ loadProjectContent });
      }
      return success;
    } catch {
      projectOperationError = "The project operation could not be completed.";
      return false;
    } finally {
      projectOperationBusy = false;
    }
  }

  function saveProject() {
    void runProjectOperation("saveProject", [
      projectName,
      projectDescription,
      themeMode,
      uiScaleState.percent,
      stretchStepsToFit ? 1 : 0,
    ]);
  }

  function loadProject() {
    void runProjectOperation("loadProject", [], { loadProjectContent: true });
  }

  function createNewProject() {
    void runProjectOperation("newProject", [], { loadProjectContent: true });
  }

  function assignGeneratedPhraseRows(generated) {
    const nextNotes = cloneMatrix(generated.notes);

    grid = nextNotes;
    rowTimingOffset = [...generated.rowTimingOffset];
    stepDurationFraction = cloneMatrix(generated.stepDurationFraction);
    stepTimingMultiplier = cloneMatrix(generated.stepTimingMultiplier);
    stepVelocity = cloneMatrix(generated.stepVelocity);
    stepMuted = cloneMatrix(generated.stepMuted);
    stepSkipped = cloneMatrix(generated.stepSkipped);
    stepProbability = cloneMatrix(generated.stepProbability);
    stepCycle = cloneMatrix(generated.stepCycle);
    stepCycleOffset = cloneMatrix(generated.stepCycleOffset);
    activeGates = nextNotes.map((row) => row.map(() => false));
    stepIds = createStepIdsForGrid(nextNotes);
    rowMuted = nextNotes.map((_, index) => rowMuted[index] ?? false);
    soloRow = -1;
    rowSoloRestoreMuted = null;
    setSelectedStepKeys(new Set());
    bulkDurationPercent = 0;
    bulkVelocityPercent = 0;
    bulkLengthDelta = 0;
    bulkTransposeSemitones = 0;
  }

  async function syncGeneratedPhraseRowsToNative(applyVersion = null) {
    for (let row = 0; row < grid.length; row += 1) {
      if (applyVersion !== null && applyVersion !== seedModeApplyVersion) return;

      await pushCurrentPhraseRow(row);

      if (applyVersion !== null && applyVersion !== seedModeApplyVersion) return;

      await pushRowMutedValue(row, rowMuted[row]);

      if (applyVersion !== null && applyVersion !== seedModeApplyVersion) return;

      await pushRowTimingOffset(row);
    }
  }

  async function applyGeneratedPhraseRows(generated, applyVersion = null) {
    assignGeneratedPhraseRows(generated);
    await syncGeneratedPhraseRowsToNative(applyVersion);
  }

  function assignSeedModeStateFromPattern(state) {
    if (!state || typeof state !== "object" || Array.isArray(state)) return;

    const normalized = normalizeSeedModeState({
      rhythmStep: Number.parseInt(String(state.seedingRhythmStep ?? defaultSeedModeState.rhythmStep), 10),
      rowSettings: Array.isArray(state.seedingRowSettings)
        ? state.seedingRowSettings.map((rowSettings) => ({
            phraseLength: Number.parseInt(String(rowSettings?.phraseLength ?? defaultSeedModeState.rowSettings[0].phraseLength), 10),
            centerMidi: Number.parseInt(String(rowSettings?.centerMidi ?? defaultSeedModeState.rowSettings[0].centerMidi), 10),
            rangeSemitones: Number.parseInt(String(rowSettings?.rangeSemitones ?? defaultSeedModeState.rowSettings[0].rangeSemitones), 10),
            repetition: Number.parseInt(String(rowSettings?.repetition ?? defaultSeedModeState.rowSettings[0].repetition), 10),
            complexity: Number.parseInt(String(rowSettings?.complexity ?? defaultSeedModeState.rowSettings[0].complexity), 10),
            randomness: Number.parseInt(String(rowSettings?.randomness ?? defaultSeedModeState.rowSettings[0].randomness), 10),
            timingMeanMultiplierIndex: Number.parseInt(String(rowSettings?.timingMeanMultiplierIndex ?? defaultSeedModeState.rowSettings[0].timingMeanMultiplierIndex), 10),
            timingVariance: Number.parseInt(String(rowSettings?.timingVariance ?? defaultSeedModeState.rowSettings[0].timingVariance), 10),
            symmetry: Boolean(Number.parseInt(String(rowSettings?.symmetry ?? 0), 10)),
            seed: Number.parseInt(String(rowSettings?.seed ?? defaultSeedModeState.rowSettings[0].seed), 10),
            repetitionSeed: Number.parseInt(String(rowSettings?.repetitionSeed ?? 0), 10),
            complexitySeed: Number.parseInt(String(rowSettings?.complexitySeed ?? 0), 10),
            randomnessSeed: Number.parseInt(String(rowSettings?.randomnessSeed ?? 0), 10),
            timingVarianceSeed: Number.parseInt(String(rowSettings?.timingVarianceSeed ?? 0), 10),
          }))
        : undefined,
      rowTargets: Array.isArray(state.seedingRowTargets)
        ? state.seedingRowTargets.map((value) => Boolean(Number.parseInt(String(value ?? 0), 10)))
        : undefined,
    });

    seedModeRhythmStep = normalized.rhythmStep;
    seedModeRowSettings = normalized.rowSettings;
    seedModeRowTargets = normalized.rowTargets;
  }

  async function pushSeedModeStateToNative() {
    if (!nativeFunctionAvailable("setPatternSeedModeState")) return;

    const confirmed = unwrapJuceNativeResult(await getNativeFunction("setPatternSeedModeState")(
      seedModeRhythmStep,
      seedModeRowSettings,
      seedModeRowTargets.map((targeted) => (targeted ? 1 : 0)),
    ));

    if (confirmed && typeof confirmed === "object" && !Array.isArray(confirmed)) {
      assignSeedModeStateFromPattern(confirmed);
    }
  }

  function currentSeedModeState() {
    return normalizeSeedModeState({
      rhythmStep: seedModeRhythmStep,
      rowSettings: seedModeRowSettings,
      rowTargets: seedModeRowTargets,
    });
  }

  function seedModeTargetLabel(rowTargets = seedModeRowTargets) {
    const targetedRows = rowTargets
      .map((targeted, row) => (targeted ? row : -1))
      .filter((row) => row >= 0);

    if (targetedRows.length === 1) {
      return `Seed row ${targetedRows[0] + 1}`;
    }

    if (targetedRows.length === 0) {
      return "Change seed targets";
    }

    return "Seed rows";
  }

  function beginSeedModeGesture() {
    if (seedModeGestureBefore !== null) return;

    seedModeGestureBefore = createHistorySnapshot();
  }

  async function applySeedModeState(
    nextState = currentSeedModeState(),
    { syncNative = true, applyRhythmToAllRows = false } = {},
  ) {
    if (projectOperationBusy) return;

    const normalized = normalizeSeedModeState(nextState);
    const shouldApplyRhythmToAllRows =
      applyRhythmToAllRows || normalized.rhythmStep !== seedModeRhythmStep;

    seedModeRhythmStep = normalized.rhythmStep;
    seedModeRowSettings = normalized.rowSettings;
    seedModeRowTargets = normalized.rowTargets;

    if (!hasSeedingRowTargets(seedModeRowTargets) && !shouldApplyRhythmToAllRows) {
      if (syncNative) {
        await pushSeedModeStateToNative();
      }

      return;
    }

    const applyVersion = ++seedModeApplyVersion;
    const existing = phraseRowsFromGridState({
      grid,
      stepTimingMultiplier,
      stepDurationFraction,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      rowTimingOffset,
    });
    const generated = generateSeededPhraseRowsFromSeedModeState(
      normalized,
      scaleRoot,
      scaleModeIndex,
    );
    const merged = mergeSeededPhraseRows(existing, generated, seedModeRowTargets, {
      applyRhythmToAllRows: shouldApplyRhythmToAllRows,
    });

    try {
      assignGeneratedPhraseRows(merged);

      if (syncNative) {
        await syncGeneratedPhraseRowsToNative(applyVersion);
        await pushSeedModeStateToNative();
      }
    } catch {
      projectOperationError = "The pattern could not be seeded.";
    }
  }

  async function commitSeedModeState(
    label,
    nextState = currentSeedModeState(),
    options = {},
  ) {
    const before = seedModeGestureBefore ?? createHistorySnapshot();
    seedModeGestureBefore = null;

    await applySeedModeState(nextState, options);
    pushHistoryEntry(label, before, createHistorySnapshot());
  }

  async function enterSeedMode() {
    if (projectOperationBusy || seedModeActive) return;

    if (recordingRow !== null) {
      await finishRowRecording();
    }

    dismissPhraseEditingFocus();
    seedModeActive = true;
    await commitSeedModeState(seedModeTargetLabel());
  }

  async function exitSeedMode() {
    if (!seedModeActive) return;

    if (seedModeGestureBefore !== null) {
      const before = seedModeGestureBefore;
      seedModeGestureBefore = null;
      pushHistoryEntry(seedModeTargetLabel(), before, createHistorySnapshot());
    }

    try {
      await syncGeneratedPhraseRowsToNative();
      await pushSeedModeStateToNative();
    } catch {
      projectOperationError = "The pattern could not be seeded.";
    }

    seedModeActive = false;
  }

  async function toggleSeedMode() {
    if (seedModeActive) {
      await exitSeedMode();
      return;
    }

    await enterSeedMode();
  }

  function previewSeedModeRhythmStep(rhythmStep) {
    beginSeedModeGesture();

    void applySeedModeState({
      ...currentSeedModeState(),
      rhythmStep,
    }, { syncNative: false, applyRhythmToAllRows: true });
  }

  function commitSeedModeRhythmStep(rhythmStep) {
    void commitSeedModeState("Change seed rhythm", {
      ...currentSeedModeState(),
      rhythmStep,
    }, { syncNative: true, applyRhythmToAllRows: true });
  }

  function previewSeedModeRowSettings(rowSettings) {
    beginSeedModeGesture();

    void applySeedModeState({
      ...currentSeedModeState(),
      rowSettings,
    }, { syncNative: false });
  }

  function commitSeedModeRowSettings(rowSettings) {
    void commitSeedModeState(seedModeTargetLabel(), {
      ...currentSeedModeState(),
      rowSettings,
    }, { syncNative: true });
  }

  function nextSeedModeSeed() {
    const newSeed = randomSeedingSeed();

    void commitSeedModeState("New seed notes", {
      ...currentSeedModeState(),
      rowSettings: refreshSeedingSeedsForRows(seedModeRowSettings, seedModeRowTargets, newSeed),
    });
  }

  function shuffleSeedModeSettings() {
    const newSeed = randomSeedingSeed();

    void commitSeedModeState("Randomize seed", {
      ...currentSeedModeState(),
      rhythmStep: Math.floor(Math.random() * (seedingRhythmStepMax + 1)),
      rowSettings: refreshSeedingSeedsForRows(
        applySeedingRowSettingsUpdate(
          seedModeRowSettings,
          seedModeRowTargets,
          {
            repetition: Math.round(Math.min(100, Math.max(0, 18 + Math.random() * 70))),
            complexity: Math.round(Math.min(100, Math.max(0, 20 + Math.random() * 72))),
            randomness: Math.round(Math.min(100, Math.max(0, 24 + Math.random() * 68))),
            timingMeanMultiplierIndex: Math.round(
              seedingTimingMultiplierMinIndex
                + Math.random() * (seedingTimingMultiplierMaxIndex - seedingTimingMultiplierMinIndex),
            ),
            timingVariance: Math.round(Math.min(100, Math.max(0, 18 + Math.random() * 72))),
            symmetry: Math.random() > 0.62,
          },
        ),
        seedModeRowTargets,
        newSeed,
      ),
    });
  }

  /** @param {import("./seeding.js").SeedingReshuffleableAspect} aspect */
  function reshuffleSeedModeAspect(aspect) {
    void commitSeedModeState("Re-shuffle seed", {
      ...currentSeedModeState(),
      rowSettings: applySeedingRowSettingsUpdate(
        seedModeRowSettings,
        seedModeRowTargets,
        reshuffleSeedingAspectSeedUpdate(aspect),
      ),
    });
  }

  function toggleSeedModeRowTarget(row, { shiftKey = false } = {}) {
    if (projectOperationBusy) return;

    let next;

    if (shiftKey) {
      next = [...seedModeRowTargets];
      next[row] = !next[row];

      if (!next.some(Boolean)) {
        next[row] = true;
      }
    } else {
      const soleTarget =
        seedModeRowTargets[row] && seedModeRowTargets.filter(Boolean).length === 1;

      if (soleTarget) {
        return;
      }

      next = [false, false, false, false];
      next[row] = true;
    }

    void commitSeedModeState(seedModeTargetLabel(next), {
      ...currentSeedModeState(),
      rowTargets: next,
    }, { syncNative: true });
  }

  function toggleAllSeedModeRowTargets() {
    if (projectOperationBusy) return;

    const shouldDisableAll = seedModeRowTargets.every(Boolean);

    void commitSeedModeState(shouldDisableAll ? "Disable seed rows" : "Seed all rows", {
      ...currentSeedModeState(),
      rowTargets: shouldDisableAll ? [false, false, false, false] : [true, true, true, true],
    }, { syncNative: true });
  }

  async function toggleSeedModeRowMute(row, soloRequested = false) {
    if (projectOperationBusy) return;

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

  function cycleProject(direction) {
    void runProjectOperation("cycleProject", [direction], { loadProjectContent: true });
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

  function gateMatricesEqual(a, b) {
    if (a.length !== b.length) return false;

    for (let row = 0; row < a.length; row += 1) {
      if (a[row].length !== b[row].length) return false;

      for (let step = 0; step < a[row].length; step += 1) {
        if (a[row][step] !== b[row][step]) return false;
      }
    }

    return true;
  }

  function updateActiveGatesForBeat(beat) {
    const rawGates = gateSnapshotForBeat(beat);

    if (beat < 0) {
      previousGateSnapshot = rawGates;

      const clearedHoldUntil = inactiveGateMatrix().map((row) => row.map(() => 0));

      if (!gateMatricesEqual(activeGateHoldUntil, clearedHoldUntil)) {
        activeGateHoldUntil = clearedHoldUntil;
      }

      if (!gateMatricesEqual(activeGates, rawGates)) {
        activeGates = rawGates;
      }

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

    if (!gateMatricesEqual(activeGateHoldUntil, nextHoldUntil)) {
      activeGateHoldUntil = nextHoldUntil;
    }

    if (!gateMatricesEqual(activeGates, nextActiveGates)) {
      activeGates = nextActiveGates;
    }
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
    stepIds = createStepIdsForGrid(nextGrid);
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

    const next = [
      defaultRowTimingOffsetIndex,
      defaultRowTimingOffsetIndex,
      defaultRowTimingOffsetIndex,
      defaultRowTimingOffsetIndex,
    ];

    for (let row = 0; row < 4; row += 1) {
      const value = Number.parseInt(String(init[row]), 10);

      if (Number.isNaN(value)) continue;

      next[row] = Math.min(rowTimingOffsetCount - 1, Math.max(0, value));
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
          ? defaults[row][step] ?? defaultStepProbabilityValue
          : clampStepProbabilityPercent(value);
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
        const raw = Number.parseInt(String(rowData?.[step] ?? defaults[row][step]), 10);

        if (Number.isNaN(raw)) {
          next[row][step] = defaultStepCycleMask;
          continue;
        }

        const normalized = normalizeCyclePattern(
          cycle,
          Number.isNaN(raw) ? defaultStepCycleMask : raw,
        );

        next[row][step] = normalized.mask;
      }
    }

    stepCycleOffset = next;
  }

  /** @param {number} row @param {string[]} beforeIds @param {string[]} afterIds */
  async function commitBulkRowMove(row, beforeIds, afterIds) {
    if (beforeIds.join() === afterIds.join()) return;

    const after = createHistorySnapshot();
    const before = snapshotWithRowOrder(after, row, beforeIds, afterIds);
    pushHistoryEntry("Move steps", before, after);

    await pushCurrentPhraseRow(row);
  }

  /**
   * @param {number} row
   * @param {string[]} blockIds
   * @param {number} insertionIndex
   */
  async function duplicateBulkStepsToDrop(row, blockIds, insertionIndex) {
    if (row < 0 || row >= stepIds.length) return;
    if (stepIds[row].length + blockIds.length > maxPhraseStepsPerRow) return;

    const before = createHistorySnapshot();
    const result = duplicateBlockInRow(
      {
        grid,
        stepDurationFraction,
        stepTimingMultiplier,
        stepVelocity,
        stepMuted,
        stepSkipped,
        stepProbability,
        stepCycle,
        stepCycleOffset,
        activeGates,
      },
      stepIds,
      row,
      blockIds,
      insertionIndex,
      createStepId,
    );

    if (!result) return;

    ({
      grid,
      stepDurationFraction,
      stepTimingMultiplier,
      stepVelocity,
      stepMuted,
      stepSkipped,
      stepProbability,
      stepCycle,
      stepCycleOffset,
      activeGates,
    } = result.matrices);
    stepIds = result.stepIds;

    for (const newId of result.newIds) {
      const duplicateIndex = stepIds[row].indexOf(newId);

      if (duplicateIndex >= 0) {
        activeGates[row][duplicateIndex] = false;
      }
    }

    const after = createHistorySnapshot();
    pushHistoryEntry("Duplicate steps", before, after);

    const nextSelectedKeys = new SvelteSet(selectedStepKeysForGrid);

    for (const newId of result.newIds) {
      nextSelectedKeys.add(stepSelectionKey(row, newId));
    }

    setSelectedStepKeys(nextSelectedKeys);
    syncBulkControlsFromSelection();

    await tick();
    await pushCurrentPhraseRow(row);
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

  async function movePhraseStepFromPianoRoll(row, fromStep, toStep) {
    if (row < 0 || row >= stepIds.length) return;
    if (fromStep < 0 || fromStep >= stepIds[row].length) return;
    if (toStep < 0 || toStep >= stepIds[row].length || fromStep === toStep) return;

    const beforeIds = [...stepIds[row]];
    const movedStepId = beforeIds[fromStep];
    const afterIds = [...beforeIds];
    const [movedId] = afterIds.splice(fromStep, 1);
    afterIds.splice(toStep, 0, movedId);

    reorderRowByIds(row, afterIds);
    rowPianoRollStep = { row, stepId: movedStepId };
    await commitRowMove(row, beforeIds, afterIds);
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

  async function clearPhraseRow(row) {
    if (row < 0 || row >= grid.length || grid[row].length === 0) return;

    await commitHistory("Clear row", async () => {
      if (activeStepInspector?.row === row) {
        closeStepInspector();
      }

      if (activeRowPianoRollEditor?.row === row) {
        closeRowPianoRollEditor();
      }

      const nextSelectedKeys = new SvelteSet(selectedStepKeys);

      for (const stepId of stepIds[row] ?? []) {
        nextSelectedKeys.delete(stepSelectionKey(row, stepId));
      }

      setSelectedStepKeys(nextSelectedKeys);
      syncBulkControlsFromSelection();

      grid[row] = [];
      stepDurationFraction[row] = [];
      stepTimingMultiplier[row] = [];
      stepVelocity[row] = [];
      stepMuted[row] = [];
      stepSkipped[row] = [];
      stepProbability[row] = [];
      stepCycle[row] = [];
      stepCycleOffset[row] = [];
      activeGates[row] = [];
      stepIds[row] = [];

      await pushCurrentPhraseRow(row);
    });
  }

  async function pushRowTimingOffset(row) {
    await pushRowTimingOffsetValue(row, rowTimingOffset[row]);
  }

  async function pushRowTimingOffsetValue(row, offsetIndex) {
    if (!nativeFunctionAvailable("setPhraseRowTimingOffset")) return;

    const setPhraseRowTimingOffset = getNativeFunction("setPhraseRowTimingOffset");
    await setPhraseRowTimingOffset(row, offsetIndex);
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

  async function pushStepCyclePattern(row, step) {
    await pushStepCycle(row, step);
    await pushStepCycleOffset(row, step);
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
    const locations = bulkEditLocationsByPosition();

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
    const locations = bulkEditLocations();

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

  async function randomizeSelectedStepLengths() {
    const locations = bulkEditLocationsByPosition();

    if (locations.length === 0) return;

    const selectedIndices = locations.map(({ row, step }) =>
      stepTimingMultiplier[row][step] ?? defaultStepTimingMultiplierIndex,
    );
    const pool = timingMultiplierIndicesInRange(
      Math.min(...selectedIndices),
      Math.max(...selectedIndices),
    );

    if (pool.length === 0) return;

    await commitHistory("Randomize selected step lengths", async () => {
      const changedRows = new SvelteSet(locations.map(({ row }) => row));

      for (const { row, step } of locations) {
        stepTimingMultiplier[row][step] = pool[Math.floor(Math.random() * pool.length)];
      }

      await pushRowsForRowSet(changedRows);
    });

    syncBulkControlsFromSelection();
  }

  async function reverseSelectedStepsByRow() {
    const groups = [...bulkEditLocationsGroupedByRow().entries()]
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

  async function shiftSelectedStepsLeftByRow() {
    const groups = [...bulkEditLocationsGroupedByRow().entries()]
      .filter(([, locations]) => locations.length > 1);

    if (groups.length === 0) return;

    await commitHistory("Shift selected steps", async () => {
      const changedRows = new SvelteSet();

      for (const [row, locations] of groups) {
        const payloads = locations.map(({ step }) => stepPayloadAt(row, step));
        const shiftedPayloads = [...payloads.slice(1), payloads[0]];

        for (let index = 0; index < locations.length; index += 1) {
          writeStepPayload(row, locations[index].step, shiftedPayloads[index]);
        }

        changedRows.add(row);
      }

      await pushRowsForRowSet(changedRows);
    });

    syncBulkControlsFromSelection();
  }

  function beginRowTimingOffsetGesture() {
    if (!rowTimingOffsetGestureBefore) {
      rowTimingOffsetGestureBefore = createHistorySnapshot();
    }

    rowTimingOffsetGestureBaseline = [...rowTimingOffset];
  }

  function resetRowTimingOffsetGesture() {
    rowTimingOffsetGestureBefore = null;
    rowTimingOffsetGestureBaseline = null;
  }

  /**
   * @param {number[]} previousOffsets
   * @param {number[]} nextOffsets
   */
  function changedRowTimingOffsetRows(previousOffsets, nextOffsets) {
    const rows = [];
    const count = Math.max(previousOffsets.length, nextOffsets.length);

    for (let row = 0; row < count; row += 1) {
      if (previousOffsets[row] !== nextOffsets[row]) rows.push(row);
    }

    return rows;
  }

  /** @param {Map<number, number>} rowValues */
  function queueRowTimingOffsetPreviewSync(rowValues) {
    if (rowValues.size === 0) return;

    const snapshot = [...rowValues.entries()];

    rowTimingOffsetPreviewSyncPromise = rowTimingOffsetPreviewSyncPromise
      .catch(() => {})
      .then(async () => {
        for (const [row, offsetIndex] of snapshot) {
          await pushRowTimingOffsetValue(row, offsetIndex);
        }
      });
  }

  /** @param {number[]} rows */
  async function pushRowTimingOffsetsForRows(rows) {
    for (const row of rows) {
      await pushRowTimingOffsetValue(row, rowTimingOffset[row]);
    }
  }

  /**
   * @param {number} row
   * @param {number} offsetIndex
   * @param {{ shiftKey?: boolean, startValue?: number, positionDelta?: number } | undefined} detail
   */
  function rowTimingOffsetsForGesture(row, offsetIndex, detail) {
    const baseline = rowTimingOffsetGestureBaseline ?? rowTimingOffset;

    if (detail?.shiftKey) {
      const delta = detail.positionDelta ?? offsetIndex - (detail.startValue ?? offsetIndex);
      return rowTimingOffsetIndicesWithDelta(baseline, delta);
    }

    return rowTimingOffsetIndicesWithSingleValue(baseline, row, offsetIndex);
  }

  /**
   * @param {number} row
   * @param {number} offsetIndex
   * @param {{ shiftKey?: boolean, startValue?: number, positionDelta?: number } | undefined} detail
   */
  function previewRowTimingOffsetGesture(row, offsetIndex, detail) {
    if (!rowTimingOffsetGestureBefore) beginRowTimingOffsetGesture();

    const previous = [...rowTimingOffset];
    const next = rowTimingOffsetsForGesture(row, offsetIndex, detail);
    const changedRows = changedRowTimingOffsetRows(previous, next);

    if (changedRows.length === 0) return;

    rowTimingOffset = next;
    queueRowTimingOffsetPreviewSync(new Map(changedRows.map((changedRow) => [
      changedRow,
      next[changedRow],
    ])));
  }

  /**
   * @param {number} row
   * @param {number} offsetIndex
   * @param {{ shiftKey?: boolean, startValue?: number, positionDelta?: number } | undefined} detail
   */
  async function commitRowTimingOffsetGesture(row, offsetIndex, detail) {
    if (!rowTimingOffsetGestureBefore) beginRowTimingOffsetGesture();

    const before = rowTimingOffsetGestureBefore;
    previewRowTimingOffsetGesture(row, offsetIndex, detail);
    resetRowTimingOffsetGesture();

    if (!before) return;

    await rowTimingOffsetPreviewSyncPromise.catch(() => {});
    await pushRowTimingOffsetsForRows(changedRowTimingOffsetRows(before.rowTimingOffset, rowTimingOffset));

    const after = createHistorySnapshot();
    if (!snapshotsEqual(before, after)) {
      pushHistoryEntry(detail?.shiftKey ? "Shift row timing" : "Change row timing", before, after);
    }
  }

  async function selectRowTimingOffset(row, offsetIndex) {
    await commitHistory("Change row timing", async () => {
      rowTimingOffset[row] = clampRowTimingOffsetIndex(offsetIndex);
      await pushRowTimingOffset(row);
    });
  }

  async function selectStepTimingMultiplier(row, step, multiplierIndex) {
    await commitHistory("Change step timing", async () => {
      const nextRow = stepTimingMultiplier[row].map((value, stepIndex) =>
        stepIndex === step ? multiplierIndex : value,
      );
      stepTimingMultiplier = stepTimingMultiplier.map((rowSteps, rowIndex) =>
        rowIndex === row ? nextRow : rowSteps,
      );
      await pushStepTimingMultiplier(row, step);
    });
  }

  async function selectStepDurationFraction(row, step, fraction) {
    await commitPhraseStepDuration(row, step, fraction);
  }

  async function setStepVelocity(row, step, value) {
    await commitPhraseStepVelocity(row, step, value);
  }

  function clampBulkRelativePercent(value) {
    return clampSignedRelativePercent(value);
  }

  function clampTransposeSemitones(value) {
    const parsed = Number.parseInt(String(value), 10);

    return Number.isNaN(parsed) ? 0 : Math.min(48, Math.max(-48, parsed));
  }

  function clampStepDurationPercent(value) {
    return clampHundredScalePercent(value);
  }

  function clampStepVelocityPercent(value) {
    return clampHundredScalePercent(value);
  }

  async function pushRowsForSelectedLocations(locations) {
    const rows = [...new Set(locations.map((location) => location.row))];

    for (const row of rows) {
      await pushCurrentPhraseRow(row);
    }
  }

  function flushBulkPreviewSyncNow() {
    if (bulkPreviewSyncTimerId) {
      clearTimeout(bulkPreviewSyncTimerId);
      bulkPreviewSyncTimerId = 0;
    }

    const pending = [...bulkPreviewPendingSyncs.values()];

    if (pending.length === 0) return;

    bulkPreviewPendingSyncs.clear();
    bulkPreviewLastSyncAt = Date.now();

    bulkPreviewSyncPromise = bulkPreviewSyncPromise
      .catch(() => {})
      .then(async () => {
        for (const { row, step, pushStep } of pending) {
          await pushStep(row, step);
        }
      });
  }

  /**
   * @param {{ row: number, step: number }[]} locations
   * @param {(row: number, step: number) => Promise<void>} pushStep
   */
  function queueBulkPreviewSync(locations, pushStep) {
    for (const location of locations) {
      bulkPreviewPendingSyncs.set(`${location.row}:${location.step}`, {
        row: location.row,
        step: location.step,
        pushStep,
      });
    }

    if (bulkPreviewSyncTimerId) return;

    const elapsed = Date.now() - bulkPreviewLastSyncAt;
    const delay =
      elapsed >= bulkPreviewSyncThrottleMs ? 0 : bulkPreviewSyncThrottleMs - elapsed;

    bulkPreviewSyncTimerId = window.setTimeout(() => {
      bulkPreviewSyncTimerId = 0;
      flushBulkPreviewSyncNow();
    }, delay);
  }

  function beginBulkEditGesture() {
    if (!bulkEditGestureBefore) {
      bulkEditGestureBefore = createHistorySnapshot();
    }
  }

  function resetBulkEditGesture() {
    bulkEditGestureBefore = null;
    bulkDurationBaselineByKey = null;
    bulkVelocityBaselineByKey = null;
    bulkLengthBaselineByKey = null;
    bulkTransposeBaselineByKey = null;
  }

  async function commitBulkEditGesture(label, syncFn) {
    const before = bulkEditGestureBefore;
    resetBulkEditGesture();
    flushBulkPreviewSyncNow();

    if (!before) return;

    await bulkPreviewSyncPromise.catch(() => {});
    await syncFn();
    const after = createHistorySnapshot();
    pushHistoryEntry(label, before, after);
  }

  function ensureBulkDurationBaseline(locations) {
    if (bulkDurationBaselineByKey) return;

    bulkDurationBaselineByKey = new SvelteMap();

    for (const { row, step, key } of locations) {
      bulkDurationBaselineByKey.set(key, (stepDurationFraction[row][step] ?? defaultStepDurationFraction) * 100);
    }
  }

  function ensureBulkVelocityBaseline(locations) {
    if (bulkVelocityBaselineByKey) return;

    bulkVelocityBaselineByKey = new SvelteMap();

    for (const { row, step, key } of locations) {
      bulkVelocityBaselineByKey.set(
        key,
        ((stepVelocity[row][step] ?? defaultStepVelocity) / 127) * 100,
      );
    }
  }

  function ensureBulkLengthBaseline(locations) {
    if (bulkLengthBaselineByKey) return;

    bulkLengthBaselineByKey = new SvelteMap();

    for (const { row, step, key } of locations) {
      bulkLengthBaselineByKey.set(
        key,
        timingMultiplierAtIndex(stepTimingMultiplier[row][step] ?? defaultStepTimingMultiplierIndex),
      );
    }
  }

  function previewBulkDurationPercent(value) {
    const clamped = clampBulkRelativePercent(value);
    const locations = bulkEditLocations();

    if (locations.length === 0) return;

    ensureBulkDurationBaseline(locations);
    bulkDurationPercent = clamped;

    for (const { row, step, key } of locations) {
      const baseline = bulkDurationBaselineByKey?.get(key) ?? 100;
      stepDurationFraction[row][step] = clampStepDurationPercent(baseline + clamped) / 100;
    }

    queueBulkPreviewSync(locations, pushStepDurationFraction);
  }

  async function commitBulkDurationPercent(value) {
    const clamped = clampBulkRelativePercent(value);
    const locations = bulkEditLocations();

    if (locations.length === 0) {
      resetBulkEditGesture();
      return;
    }

    previewBulkDurationPercent(clamped);

    await commitBulkEditGesture("Bulk duration", async () => {
      await pushRowsForSelectedLocations(locations);
    });

    bulkDurationPercent = 0;
  }

  function previewBulkVelocityPercent(value) {
    const clamped = clampBulkRelativePercent(value);
    const locations = bulkEditLocations();

    if (locations.length === 0) return;

    ensureBulkVelocityBaseline(locations);
    bulkVelocityPercent = clamped;

    for (const { row, step, key } of locations) {
      const baseline = bulkVelocityBaselineByKey?.get(key) ?? (defaultStepVelocity / 127) * 100;
      stepVelocity[row][step] = Math.round((clampStepVelocityPercent(baseline + clamped) / 100) * 127);
    }

    queueBulkPreviewSync(locations, pushStepVelocity);
  }

  async function commitBulkVelocityPercent(value) {
    const clamped = clampBulkRelativePercent(value);
    const locations = bulkEditLocations();

    if (locations.length === 0) {
      resetBulkEditGesture();
      return;
    }

    previewBulkVelocityPercent(clamped);

    await commitBulkEditGesture("Bulk velocity", async () => {
      await pushRowsForSelectedLocations(locations);
    });

    bulkVelocityPercent = 0;
  }

  function previewBulkLengthDelta(value) {
    const locations = bulkEditLocations();

    if (locations.length === 0) return;

    ensureBulkLengthBaseline(locations);

    const baselines = locations.map(({ key }) =>
      bulkLengthBaselineByKey?.get(key) ??
      timingMultiplierAtIndex(defaultStepTimingMultiplierIndex),
    );
    const clamped = clampTimingMultiplierDelta(value, baselines);
    bulkLengthDelta = clamped;

    for (const { row, step, key } of locations) {
      const baseline =
        bulkLengthBaselineByKey?.get(key) ??
        timingMultiplierAtIndex(stepTimingMultiplier[row][step] ?? defaultStepTimingMultiplierIndex);
      const nextValue = clampTimingMultiplierValue(baseline + clamped);
      stepTimingMultiplier[row][step] = timingMultiplierIndexForValue(nextValue);
    }

    queueBulkPreviewSync(locations, pushStepTimingMultiplier);
  }

  async function commitBulkLengthDelta(value) {
    const locations = bulkEditLocations();

    if (locations.length === 0) {
      resetBulkEditGesture();
      return;
    }

    previewBulkLengthDelta(value);

    await commitBulkEditGesture("Bulk length", async () => {
      await pushRowsForSelectedLocations(locations);
    });

    bulkLengthDelta = 0;
  }

  function previewBulkTransposeSemitones(value) {
    const clamped = clampTransposeSemitones(value);
    const locations = bulkEditLocations();

    if (locations.length === 0) return;

    if (!bulkTransposeBaselineByKey) {
      bulkTransposeBaselineByKey = new SvelteMap();

      for (const { row, step, key } of locations) {
        bulkTransposeBaselineByKey.set(key, grid[row][step]);
      }
    }

    bulkTransposeSemitones = clamped;

    for (const { row, step, key } of locations) {
      const baseline = bulkTransposeBaselineByKey.get(key) ?? grid[row][step];
      grid[row][step] = clampPhraseNote(stepNoteByCurrentScale(baseline, clamped));
    }

    queueBulkPreviewSync(locations, pushNote);
  }

  async function commitBulkTransposeSemitones(value) {
    const clamped = clampTransposeSemitones(value);
    const locations = bulkEditLocations();

    if (locations.length === 0) {
      resetBulkEditGesture();
      return;
    }

    previewBulkTransposeSemitones(clamped);

    await commitBulkEditGesture("Bulk transpose", async () => {
      await pushRowsForSelectedLocations(locations);
    });

    bulkTransposeSemitones = 0;
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

  async function toggleBulkStepSkip() {
    const locations = bulkEditLocations();

    if (locations.length === 0) return;

    const nextSkipped = !locations.every(({ row, step }) => stepSkipped[row][step]);

    await commitHistory(nextSkipped ? "Skip selected steps" : "Unskip selected steps", async () => {
      for (const { row, step } of locations) {
        stepSkipped[row][step] = nextSkipped;
        let clearedMute = false;

        if (nextSkipped && stepMuted[row][step]) {
          stepMuted[row][step] = false;
          clearedMute = true;
        }

        await pushStepSkipped(row, step);

        if (clearedMute) {
          await pushStepMuted(row, step);
        }
      }
    });
  }

  async function toggleBulkStepMute() {
    const locations = bulkEditLocations();

    if (locations.length === 0) return;

    const nextMuted = !locations.every(({ row, step }) => stepMuted[row][step]);

    await commitHistory(nextMuted ? "Mute selected steps" : "Unmute selected steps", async () => {
      for (const { row, step } of locations) {
        stepMuted[row][step] = nextMuted;
        let clearedSkip = false;

        if (nextMuted && stepSkipped[row][step]) {
          stepSkipped[row][step] = false;
          clearedSkip = true;
        }

        await pushStepMuted(row, step);

        if (clearedSkip) {
          await pushStepSkipped(row, step);
        }
      }
    });
  }

  function inspectorEditLocations(row, step) {
    const locations = selectedStepLocations();
    const stepId = stepIds[row]?.[step];

    if (!stepId) return [];

    const key = stepSelectionKey(row, stepId);

    if (locations.length <= 1) {
      return [{ row, step, key }];
    }

    if (!locations.some((location) => location.key === key)) {
      locations.push({ row, step, key });
    }

    return locations;
  }

  function previewInspectorStepProbability(row, step, probability) {
    const locations = inspectorEditLocations(row, step);

    if (locations.length === 0) return;
    if (!bulkEditGestureBefore) beginBulkEditGesture();

    const nextProbability = clampStepProbabilityPercent(probability);

    for (const { row: editRow, step: editStep } of locations) {
      stepProbability[editRow][editStep] = nextProbability;
    }

    queueBulkPreviewSync(locations, pushStepProbability);
  }

  async function commitInspectorStepProbability(row, step, probability) {
    const locations = inspectorEditLocations(row, step);

    previewInspectorStepProbability(row, step, probability);
    await commitBulkEditGesture("Change probability", async () => {
      await pushRowsForSelectedLocations(locations);
    });
  }

  function previewInspectorStepCyclePattern(row, step, cycle, cycleMask) {
    const locations = inspectorEditLocations(row, step);

    if (locations.length === 0) return;
    if (!bulkEditGestureBefore) beginBulkEditGesture();

    const normalized = normalizeCyclePattern(cycle, cycleMask);
    const editorNormalized = normalizeEditorCyclePattern(normalized.cycle, normalized.mask);

    for (const { row: editRow, step: editStep } of locations) {
      stepCycle[editRow][editStep] = editorNormalized.cycle;
      stepCycleOffset[editRow][editStep] = editorNormalized.mask;
    }

    queueBulkPreviewSync(locations, pushStepCyclePattern);
  }

  async function commitInspectorStepCyclePattern(row, step, cycle, cycleMask) {
    const locations = inspectorEditLocations(row, step);

    previewInspectorStepCyclePattern(row, step, cycle, cycleMask);
    await commitBulkEditGesture("Change cycle", async () => {
      await pushRowsForSelectedLocations(locations);
    });
  }

  async function removeStepsAtLocations(locations) {
    const uniqueLocations = [];
    const seenKeys = new Set();

    for (const location of locations) {
      if (seenKeys.has(location.key)) continue;
      seenKeys.add(location.key);
      uniqueLocations.push(location);
    }

    if (uniqueLocations.length === 0) return;

    const before = createHistorySnapshot();

    for (const { row, step } of uniqueLocations) {
      const stepId = stepIds[row]?.[step];
      if (!stepId) continue;

      if (inspectedStep?.row === row && inspectedStep.stepId === stepId) {
        closeStepInspector();
      }

      if (rowPianoRollStep?.row === row && rowPianoRollStep.stepId === stepId) {
        closeRowPianoRollEditor();
      }
    }

    const deletedKeys = uniqueLocations
      .map(({ key }) => key)
      .filter((key) => selectedStepKeys.has(key));

    if (deletedKeys.length > 0) {
      const nextSelectedKeys = new SvelteSet(selectedStepKeys);

      for (const key of deletedKeys) {
        nextSelectedKeys.delete(key);
      }

      setSelectedStepKeys(nextSelectedKeys);
      syncBulkControlsFromSelection();
    }

    const stepsByRow = new SvelteMap();

    for (const { row, step } of uniqueLocations) {
      if (row < 0 || row >= grid.length || step < 0 || step >= grid[row].length) continue;
      if (!stepIds[row]?.[step]) continue;

      const steps = stepsByRow.get(row) ?? [];

      if (!steps.includes(step)) {
        steps.push(step);
      }

      stepsByRow.set(row, steps);
    }

    const nativeRemovals = [];

    for (const [row, steps] of stepsByRow) {
      steps.sort((left, right) => right - left);

      for (const step of steps) {
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
        nativeRemovals.push({ row, step });
      }
    }

    await tick();

    if (nativeFunctionAvailable("removePhraseStep")) {
      const removePhraseStep = getNativeFunction("removePhraseStep");

      for (const { row, step } of nativeRemovals) {
        await removePhraseStep(row, step);
      }
    }

    const after = createHistorySnapshot();
    pushHistoryEntry(uniqueLocations.length > 1 ? "Remove steps" : "Remove step", before, after);
  }

  async function removeStep(row, step) {
    if (row < 0 || row >= grid.length || step < 0 || step >= grid[row].length) return;

    const deletedStepId = stepIds[row]?.[step];
    if (!deletedStepId) return;

    const deletedKey = stepSelectionKey(row, deletedStepId);
    const locations =
      selectedStepKeysForGrid.size > 1 && selectedStepKeysForGrid.has(deletedKey)
        ? selectedStepLocations()
        : [{ row, step, key: deletedKey }];

    await removeStepsAtLocations(locations);
  }

  async function insertStep(row, step, multiplierIndex = defaultStepTimingMultiplierIndex) {
    await commitHistory("Insert step", async () => {
      const defaultDurations = defaultStepDurationGrid();
      const defaultMultipliers = defaultStepTimingMultiplierGrid();
      const defaultVelocities = defaultStepVelocityGrid();
      const defaultMuted = defaultStepMutedGrid();
      const defaultSkipped = defaultStepSkippedGrid();
      const defaultProbability = defaultStepProbabilityGrid();
      const defaultCycle = defaultStepCycleGrid();
      const defaultCycleOffset = defaultStepCycleOffsetGrid();

      grid[row].splice(step, 0, defaultNewStepNote);
      stepDurationFraction[row].splice(step, 0, defaultDurations[row]?.[0] ?? 1);
      stepTimingMultiplier[row].splice(step, 0, multiplierIndex);
      stepVelocity[row].splice(step, 0, defaultVelocities[row]?.[0] ?? 100);
      stepMuted[row].splice(step, 0, defaultMuted[row]?.[0] ?? false);
      stepSkipped[row].splice(step, 0, defaultSkipped[row]?.[0] ?? false);
      stepProbability[row].splice(step, 0, defaultProbability[row]?.[0] ?? defaultStepProbabilityValue);
      stepCycle[row].splice(step, 0, defaultCycle[row]?.[0] ?? 1);
      stepCycleOffset[row].splice(step, 0, defaultCycleOffset[row]?.[0] ?? defaultStepCycleMask);
      activeGates[row].splice(step, 0, false);
      stepIds[row].splice(step, 0, createStepId());


      if (!nativeFunctionAvailable("insertPhraseStep")) return;

      const insertPhraseStep = getNativeFunction("insertPhraseStep");
      await insertPhraseStep(row, step);
      await pushCurrentPhraseRow(row);
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
      await pushCurrentPhraseRow(row);
    });
  }

  function buildDefaultRecordedStep() {
    return {
      duration: defaultStepDurationFraction,
      multiplier: defaultStepTimingMultiplierIndex,
      velocity: defaultStepVelocity,
      muted: false,
      skipped: false,
      probability: defaultStepProbabilityValue,
      cycle: 1,
      cycleOffset: defaultStepCycleMask,
    };
  }

  async function commitRecordedNote(midi) {
    if (recordingRow === null) return;

    if (!isChromaticScaleMode(scaleModeIndex) && !isMidiInScale(midi, scaleRoot, scaleModeIndex)) {
      return;
    }

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
    const note = clampPhraseNote(midi);

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

  function applyCapturedRecordingRow(row, state) {
    const notes = Array.isArray(state?.notes) ? state.notes.map((value) => clampPhraseNote(value)) : [];

    grid[row] = notes;
    stepTimingMultiplier[row] = Array.isArray(state?.timingMultiplier)
      ? state.timingMultiplier.map((value) =>
          Math.min(
            stepTimingMultiplierCount - 1,
            Math.max(0, Number.parseInt(String(value), 10)),
          ),
        )
      : notes.map(() => defaultStepTimingMultiplierIndex);
    stepDurationFraction[row] = Array.isArray(state?.durationFraction)
      ? state.durationFraction.map((value) =>
          Math.min(1, Math.max(0, Number.parseFloat(String(value)))),
        )
      : notes.map(() => defaultStepDurationFraction);
    stepVelocity[row] = Array.isArray(state?.velocity)
      ? state.velocity.map((value) =>
          Math.min(127, Math.max(0, Number.parseInt(String(value), 10))),
        )
      : notes.map(() => defaultStepVelocity);
    stepMuted[row] = Array.isArray(state?.muted)
      ? state.muted.map((value) => Boolean(Number.parseInt(String(value), 10)))
      : notes.map(() => false);
    stepSkipped[row] = Array.isArray(state?.skipped)
      ? state.skipped.map((value) => Boolean(Number.parseInt(String(value), 10)))
      : notes.map(() => false);
    stepProbability[row] = Array.isArray(state?.probability)
      ? state.probability.map((value) =>
          Math.min(100, Math.max(0, Number.parseInt(String(value), 10))),
        )
      : notes.map(() => defaultStepProbabilityValue);
    stepCycle[row] = Array.isArray(state?.cycle)
      ? state.cycle.map((value) => Math.min(64, Math.max(1, Number.parseInt(String(value), 10))))
      : notes.map(() => 1);
    stepCycleOffset[row] = Array.isArray(state?.cycleOffset)
      ? state.cycleOffset.map((value) => Number.parseInt(String(value), 10))
      : notes.map(() => defaultStepCycleMask);
    stepIds[row] = notes.map(() => createStepId());
    activeGates[row] = notes.map(() => false);
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
    recordingCaptureNativePromise = Promise.resolve();
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

    closeStepInspector();
    closeRowPianoRollEditor();
    recordingRow = row;
    recordingHistoryBefore = createHistorySnapshot();
    recordingCapturedNotes = false;
    recordingAwaitingFirstNote = false;
    recordingCaptureNativePromise = Promise.resolve();
    recordingProgress = 0;

    if (nativeFunctionAvailable("setPhraseRowRecording")) {
      await getNativeFunction("setPhraseRowRecording")(row, recordCycleLengthPulses[row] ?? 16);
    }
  }

  function queueRecordingCaptureNative(name, ...args) {
    recordingCaptureNativePromise = recordingCaptureNativePromise
      .then(async () => {
        if (nativeFunctionAvailable(name)) {
          await getNativeFunction(name)(...args);
        }
      })
      .catch(() => {
        // Native bridge unavailable during teardown.
      });

    return recordingCaptureNativePromise;
  }

  async function finishRowRecording() {
    if (recordingRow === null) return;

    const row = recordingRow;
    const before = recordingHistoryBefore;
    let captureResult = null;
    let hadNotes = false;

    if (nativeFunctionAvailable("finishPhraseRowRecordingCapture")) {
      await recordingCaptureNativePromise;
      captureResult = await getNativeFunction("finishPhraseRowRecordingCapture")();
      hadNotes = Boolean(
        Number.parseInt(String(captureResult?.captured ?? 0), 10),
      );

      if (hadNotes) {
        const capturedRow = Number.parseInt(String(captureResult?.row ?? row), 10);
        applyCapturedRecordingRow(
          Number.isNaN(capturedRow) ? row : capturedRow,
          captureResult,
        );
      }
    } else {
      hadNotes = recordingCapturedNotes;
      await disarmRowRecordingNative();
    }

    recordingRow = null;
    recordingAwaitingFirstNote = false;
    recordingCapturedNotes = false;
    recordingCaptureNativePromise = Promise.resolve();
    recordingHistoryBefore = null;
    recordingKeysHeld = new Set();

    if (hadNotes && before !== null) {
      const after = createHistorySnapshot();
      pushHistoryEntry("Record row", before, after);
    } else if (before !== null) {
      const after = createHistorySnapshot();
      pushHistoryEntry("Clear row", before, after);
    }
  }

  async function toggleRowRecording(row) {
    if (recordingRow === row) {
      await finishRowRecording();
    } else {
      await startRowRecording(row);
    }
  }

  async function setRecordingCycleLengthPulses(value) {
    if (recordingRow === null) return;

    const next = Math.min(64, Math.max(1, Number.parseInt(String(value), 10) || 16));
    recordCycleLengthPulses[recordingRow] = next;

    if (nativeFunctionAvailable("setPhraseRowRecordingCycleLength")) {
      await getNativeFunction("setPhraseRowRecordingCycleLength")(next);
    }
  }

  async function pollRowRecordingNotes() {
    if (recordingRow === null) {
      return;
    }

    if (nativeFunctionAvailable("drainPhraseRowRecordingUpdates")) {
      try {
        const update = await getNativeFunction("drainPhraseRowRecordingUpdates")();
        const updated = Boolean(Number.parseInt(String(update?.captured ?? 0), 10));

        if (updated) {
          const updatedRow = Number.parseInt(String(update?.row ?? recordingRow), 10);
          const row = Number.isNaN(updatedRow) ? recordingRow : updatedRow;
          applyCapturedRecordingRow(row, update);
          recordingCapturedNotes = true;

          const cycleLength = Number.parseInt(
            String(update?.recordCycleLengthPulses ?? recordCycleLengthPulses[row] ?? 16),
            10,
          );

          if (!Number.isNaN(cycleLength)) {
            recordCycleLengthPulses[row] = Math.min(64, Math.max(1, cycleLength));
          }
        }
      } catch {
        // Native bridge unavailable during teardown.
      }
    }

    if (nativeFunctionAvailable("getPhraseRowRecordingKeysHeld")) {
      try {
        const held = await getNativeFunction("getPhraseRowRecordingKeysHeld")();
        const keys = Array.isArray(held) ? held : [];
        recordingKeysHeld = new Set(
          keys
            .map((value) => Number.parseInt(String(value), 10))
            .filter(
              (midi) =>
                !Number.isNaN(midi) &&
                (isChromaticScaleMode(scaleModeIndex) ||
                  isMidiInScale(midi, scaleRoot, scaleModeIndex)),
            ),
        );

        if (recordingKeysHeld.size > 0) {
          recordingAwaitingFirstNote = false;
          recordingCapturedNotes = true;
        }
      } catch {
        // Native bridge unavailable during teardown.
      }
    }

  }

  async function pollRowRecordingProgress() {
    if (recordingRow === null || !nativeFunctionAvailable("getPhraseRowRecordingProgress")) {
      recordingProgress = 0;
      return;
    }

    try {
      const result = await getNativeFunction("getPhraseRowRecordingProgress")();
      const progress = Number.parseFloat(String(result));
      recordingProgress = Number.isNaN(progress) ? 0 : Math.min(1, Math.max(0, progress));
    } catch {
      recordingProgress = 0;
    }
  }

  /** @param {number} midi */
  function onRecordPianoNotePress(midi) {
    recordingAwaitingFirstNote = false;
    recordingCapturedNotes = true;

    if (nativeFunctionAvailable("capturePhraseRowRecordedNoteOn")) {
      void queueRecordingCaptureNative(
        "capturePhraseRowRecordedNoteOn",
        midi,
        defaultStepVelocity,
      );
    } else {
      void commitRecordedNote(midi);
    }
  }

  /** @param {number} midi */
  function onRecordPianoNoteRelease(midi) {
    if (nativeFunctionAvailable("capturePhraseRowRecordedNoteOff")) {
      void queueRecordingCaptureNative("capturePhraseRowRecordedNoteOff", midi);
    }
  }

  async function pollPlaybackActivityFrame() {
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
    await pollRowRecordingProgress();

    playbackSlowPollCounter += 1;

    if (playbackSlowPollCounter >= playbackSlowPollInterval) {
      playbackSlowPollCounter = 0;
      await pollCurrentSlotState();
      await pollRowRecordingNotes();
    }
  }

  function schedulePlaybackPoll() {
    if (playbackPollInFlight) return;

    playbackPollInFlight = true;
    void pollPlaybackActivityFrame().finally(() => {
      playbackPollInFlight = false;
    });
  }

  function startPlaybackPoll() {
    schedulePlaybackPoll();

    if (playbackPollTimerId) return;

    playbackPollTimerId = window.setInterval(schedulePlaybackPoll, 16);
  }

  function stopPlaybackPoll() {
    if (playbackPollTimerId) {
      window.clearInterval(playbackPollTimerId);
      playbackPollTimerId = 0;
    }

    playbackSlowPollCounter = 0;
  }

  function queuePlaybackUiRefresh() {
    requestAnimationFrame(() => {
      window.dispatchEvent(new Event("resize"));
      schedulePlaybackPoll();
    });
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
    return clampHundredScalePercent(value);
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

    if (init === null) return;

    const raw = Array.isArray(init) ? init[0] : init;
    rowColorsEnabled = Boolean(Number.parseInt(String(raw), 10));
  }

  function loadProjectMetadataFromInitialisation() {
    const scalar = (key, fallback = "") => {
      const value = unwrapJuceInit(key);
      if (value === null) return fallback;
      return Array.isArray(value) ? value[0] ?? fallback : value;
    };

    projectName = String(scalar("projectName", "Untitled Project"));
    projectDescription = String(scalar("projectDescription", ""));
    projectCreatedAt = String(scalar("projectCreatedAt", ""));
    projectModifiedAt = String(scalar("projectModifiedAt", ""));
    projectFileName = String(scalar("projectFileName", ""));
    hasPreviousProject = Boolean(Number.parseInt(String(scalar("hasPreviousProject", 0)), 10));
    hasNextProject = Boolean(Number.parseInt(String(scalar("hasNextProject", 0)), 10));
    themeMode = String(scalar("projectThemeMode", defaultThemeMode));
    stretchStepsToFit = Boolean(
      Number.parseInt(String(scalar("projectStretchStepsToFit", 0)), 10),
    );
  }

  function loadCombinationModesFromInitialisation() {
    const init = unwrapJuceInit("combinationModeMask");
    const raw = Array.isArray(init) ? init[0] : init;
    const value = Number.parseInt(String(raw ?? 0), 10);

    combinationModeMask = Number.isNaN(value) ? 0 : value & combinationModeMaskBits;
  }

  function loadCombinationSyncDivisionFromInitialisation() {
    const init = unwrapJuceInit("combinationSyncDivisionIndex");

    if (init === null) return;

    const raw = Array.isArray(init) ? init[0] : init;
    const value = Number.parseInt(String(raw), 10);

    if (!Number.isNaN(value)) {
      combinationSyncDivisionIndex = Math.min(
        combinationSyncDivisionOptions.length - 1,
        Math.max(0, value),
      );
    }
  }

  function loadCombinationChangePhaseContinueFromInitialisation() {
    const init = unwrapJuceInit("combinationChangePhaseContinue");

    if (init === null) return;

    const raw = Array.isArray(init) ? init[0] : init;
    const value = Number.parseInt(String(raw), 10);

    if (!Number.isNaN(value)) {
      combinationChangePhaseContinue = value !== 0;
    }
  }

  function loadPatternScaleFromInitialisation() {
    scaleRoot = clampScaleRoot(unwrapJuceInit("scaleRoot") ?? defaultScaleRoot);
    scaleModeIndex = clampScaleModeIndex(unwrapJuceInit("scaleModeIndex") ?? defaultScaleModeIndex);
  }

  function loadNoteBandpassFromInitialisation() {
    const lowInit = unwrapJuceInit("noteBandpassLowMidi");
    const highInit = unwrapJuceInit("noteBandpassHighMidi");
    setNoteBandpassState(
      Number.parseInt(String(lowInit ?? defaultNoteBandpassLowMidi), 10),
      Number.parseInt(String(highInit ?? defaultNoteBandpassHighMidi), 10),
    );
  }

  function loadVelocityTiltFromInitialisation() {
    setVelocityTiltState({
      pivotMidi: Number.parseInt(
        String(unwrapJuceInit("velocityTiltPivotMidi") ?? defaultVelocityTiltPivotMidi),
        10,
      ),
      amount: Number.parseInt(
        String(unwrapJuceInit("velocityTiltAmount") ?? defaultVelocityTiltAmount),
        10,
      ),
    });
  }

  function loadGlobalTransposeFromInitialisation() {
    setGlobalTransposeState(
      Number.parseInt(
        String(
          unwrapJuceInit("globalTransposeSemitones") ?? defaultGlobalTransposeSemitones,
        ),
        10,
      ),
    );
  }

  function loadOctavizerFromInitialisation() {
    setOctavizerState({
      down8vaEnabled: Boolean(
        Number.parseInt(String(unwrapJuceInit("octavizerDown8vaEnabled") ?? 0), 10),
      ),
      up8vaEnabled: Boolean(
        Number.parseInt(String(unwrapJuceInit("octavizerUp8vaEnabled") ?? 0), 10),
      ),
      down8vaRelativeVelocity: Number.parseInt(
        String(unwrapJuceInit("octavizerDown8vaRelativeVelocity") ?? defaultOctavizerRelativeVelocity),
        10,
      ),
      up8vaRelativeVelocity: Number.parseInt(
        String(unwrapJuceInit("octavizerUp8vaRelativeVelocity") ?? defaultOctavizerRelativeVelocity),
        10,
      ),
    });
  }

  function loadShimmerFromInitialisation() {
    setShimmerState({
      enabled: Boolean(Number.parseInt(String(unwrapJuceInit("shimmerEnabled") ?? 0), 10)),
      delayMultiplierIndex: Number.parseInt(
        String(unwrapJuceInit("shimmerDelayMultiplierIndex") ?? defaultShimmerDelayMultiplierIndex),
        10,
      ),
      feedbackPercent: Number.parseInt(
        String(unwrapJuceInit("shimmerFeedbackPercent") ?? defaultShimmerFeedbackPercent),
        10,
      ),
      mixPercent: Number.parseInt(
        String(unwrapJuceInit("shimmerMixPercent") ?? defaultShimmerMixPercent),
        10,
      ),
    });
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
    cancelPatternClearArmed();
    patternCopySource = Math.min(7, Math.max(0, slot));
  }

  function cancelPatternCopy() {
    patternCopySource = -1;
  }

  function cancelPatternClearArmed() {
    patternClearArmed = false;
  }

  async function handleClearPatternClick() {
    if (patternClearArmed) {
      cancelPatternClearArmed();
      await clearSelectedPatternSlot();
      return;
    }

    cancelPatternCopy();
    patternClearArmed = true;
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
    cancelPatternClearArmed();
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
    cancelPatternClearArmed();
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
    loadProjectMetadataFromInitialisation();
    loadCombinationModesFromInitialisation();
    loadCombinationSyncDivisionFromInitialisation();
    loadCombinationChangePhaseContinueFromInitialisation();
    loadPatternScaleFromInitialisation();
    loadNoteBandpassFromInitialisation();
    loadVelocityTiltFromInitialisation();
    loadGlobalTransposeFromInitialisation();
    loadOctavizerFromInitialisation();
    loadShimmerFromInitialisation();
    loadStandaloneTransportFromInitialisation();
    loadSlotStateFromInitialisation();
  }

  loadInitialStateFromJuce();

  $effect(() => {
    if (!patternClearArmed) return;

    const handlePointerDown = (event) => {
      const target = event.target;

      if (target instanceof Element && target.closest("[data-pattern-clear]")) {
        return;
      }

      cancelPatternClearArmed();
    };

    document.addEventListener("pointerdown", handlePointerDown, true);

    return () => {
      document.removeEventListener("pointerdown", handlePointerDown, true);
    };
  });

  onMount(() => {
    const initialTheme = unwrapJuceInit("projectThemeMode");
    themeMode = applyThemeMode(initialTheme === null ? storedThemeMode() : themeMode, {
      persist: false,
    });
    const initialProjectScale = unwrapJuceInit("projectUiScalePercent");
    const projectScaleScalar =
      initialProjectScale === null
        ? null
        : Array.isArray(initialProjectScale)
          ? initialProjectScale[0]
          : initialProjectScale;
    setUiScalePercent(resolveInitialUiScalePercent(projectScaleScalar), { persist: false });
    let scaleFrameId = 0;

    const updateUiScale = () => {
      const target = appRoot ?? document.documentElement;
      const rect = target.getBoundingClientRect();

      setUiViewportSize({
        widthPx: rect.width || window.innerWidth,
        heightPx: rect.height || window.innerHeight,
        standaloneTransportAvailable,
      });
      void syncEditorScaleMinimumToNative();
    };

    const scheduleUiScaleUpdate = () => {
      if (scaleFrameId) return;

      scaleFrameId = requestAnimationFrame(() => {
        scaleFrameId = 0;
        updateUiScale();
      });
    };

    const resizeObserver = new ResizeObserver(scheduleUiScaleUpdate);
    resizeObserver.observe(appRoot ?? document.documentElement);
    updateUiScale();
    void refreshEditorFullscreenState();
    void refreshProjectState();

    const handleKeydown = (event) => {
      if (event.key === "Escape" && patternClearArmed) {
        event.preventDefault();
        cancelPatternClearArmed();
        return;
      }

      if (event.key === "Escape" && inspectedStep !== null) {
        event.preventDefault();
        closeStepInspector();
        return;
      }

      if (event.key === "Escape" && rowPianoRollStep !== null) {
        event.preventDefault();
        closeRowPianoRollEditor();
        return;
      }

      if (event.key === "Escape" && editorFullscreen) {
        event.preventDefault();
        void setEditorFullscreen(false);
        return;
      }

      if (event.code === "Space" || event.key === " ") {
        const active = document.activeElement;

        if (active instanceof HTMLElement && active.getAttribute("role") === "slider") {
          event.preventDefault();
          active.blur();
          return;
        }

        if (
          standaloneTransportAvailable &&
          !event.repeat &&
          !scaleDialogOpen &&
          !(active instanceof HTMLInputElement) &&
          !(active instanceof HTMLTextAreaElement) &&
          !(active instanceof HTMLSelectElement) &&
          !(active instanceof HTMLButtonElement) &&
          !(active instanceof HTMLElement && active.isContentEditable)
        ) {
          event.preventDefault();
          void toggleStandaloneTransport();
          return;
        }
      }

      if (!event.metaKey || event.altKey || event.ctrlKey) return;
      if (event.key.toLowerCase() !== "z") return;

      event.preventDefault();

      if (event.shiftKey) {
        redo();
      } else {
        undo();
      }
    };

    const handleVisibilityChange = () => {
      if (document.visibilityState === "visible") {
        scheduleUiScaleUpdate();
        schedulePlaybackPoll();
      }
    };

    document.addEventListener("visibilitychange", handleVisibilityChange);
    window.addEventListener("keydown", handleKeydown);
    startPlaybackPoll();

    return () => {
      resizeObserver.disconnect();
      if (scaleFrameId) cancelAnimationFrame(scaleFrameId);
      document.removeEventListener("visibilitychange", handleVisibilityChange);
      window.removeEventListener("keydown", handleKeydown);
      stopPlaybackPoll();
      document.removeEventListener("pointermove", updateMarqueePointer);
      document.removeEventListener("pointerup", finishMarqueeSelection);
      document.removeEventListener("pointercancel", cancelMarqueeSelection);
    };
  });
</script>

<main
  {@attach appRootAttachment}
  class="flex h-full flex-col overflow-hidden px-6 transition-[filter,opacity] duration-150 {standaloneTransportAvailable
    ? 'pt-2'
    : 'mp-plugin-shell pt-0'} {scaleDialogOpen ? 'pointer-events-none blur-[3px] opacity-45' : ''}"
>
  <div class="shrink-0 -mx-6">
    {#if standaloneTransportAvailable}
      <div class="flex items-center justify-end gap-2 px-6 pb-2">
        <button
          type="button"
          aria-label={standalonePlaying ? "Stop standalone transport" : "Start standalone transport"}
          aria-keyshortcuts="Space"
          title="Toggle transport (Space)"
          aria-pressed={standalonePlaying}
          data-cursor="pointer"
          class="h-8 min-w-16 rounded-md border px-3 text-sm font-semibold transition-[border-color,color,box-shadow,filter] outline-none focus:ring-1 focus:ring-focus-ring {standalonePlaying
            ? 'border-accent-strong bg-accent-strong text-control-primary-text'
            : 'mp-control-gradient border-border text-text hover:border-border-strong'}"
          onclick={toggleStandaloneTransport}
        >
          {standalonePlaying ? "Stop" : "Play"}
        </button>
        <label class="flex items-center gap-1.5 text-xs font-medium uppercase text-text-muted">
          BPM
          <input
            type="number"
            min="20"
            max="300"
            step="1"
            value={Math.round(standaloneTempoBpm)}
            class="mp-param-box mp-control-gradient h-8 rounded-md border border-border text-sm font-semibold text-text outline-none focus:border-focus-ring focus:ring-1 focus:ring-focus-ring"
            style:--param-box-chars={3}
            onchange={setStandaloneTempoFromInput}
          />
        </label>
      </div>
    {/if}
    <div class="mp-honeycomb-rail mp-top-honeycomb-rail relative z-20">
  <header class="flex w-full items-center gap-x-2 px-6 py-3">
    <div class="relative z-30 flex shrink-0 items-center gap-1.5">
      <div class="flex flex-col items-start gap-[3px]">
        <div class="flex items-start gap-1.5">
          <p class="text-sm font-bold uppercase leading-none tracking-widest text-accent">
            ofsound
          </p>
        </div>
        <div class="flex h-8 items-center gap-3">
          <h1 class="whitespace-nowrap leading-none" aria-label={pluginName}>
            <MidiPhrasesLogo name={pluginName} />
          </h1>
          <div class="mr-1 shrink-0">
            <StepViewModeToggle
              size="logo"
              compact={stretchStepsToFit}
              accent={interfaceAccent}
              onChange={(nextCompact) => {
                stretchStepsToFit = nextCompact;
              }}
            />
          </div>
        </div>
      </div>

      <div class="flex shrink-0 flex-col items-start gap-1">
        <span class="pointer-events-none text-xs font-semibold leading-none opacity-0" aria-hidden="true"
          >Pitch</span
        >
        <div class="flex shrink-0 gap-1">
          <button
            type="button"
            aria-label={undoHistoryLabel}
            title={undoHistoryLabel}
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
            aria-label={redoHistoryLabel}
            title={redoHistoryLabel}
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
    </div>

    <div class="flex shrink-0 items-center gap-1">
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-text">Pulse</span>
              <PulseDragInput
                accent={interfaceAccent}
                value={pulseIndex}
                onValueChange={applyPulseIndex}
              />
            </div>
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-text">Swing</span>
              <StepNumberDragInput
                boxed
                compact
                accent={interfaceAccent}
                value={swingPercent}
                min={0}
                max={maxPercentValue}
                resetValue={0}
                ariaLabel="Swing"
                onValueChange={(value) =>
                  applyGlobalPercent(value, "setSwingPercent", (next) => {
                    swingPercent = next;
                  })}
              />
            </div>
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-text">Sub</span>
              <DiscreteDragSelect
                compact
                accent={interfaceAccent}
                options={swingSubdivisionOptions}
                value={swingSubdivisionIndex}
                resetValue={1}
                ariaLabel="Swing subdivision"
                onValueChange={applySwingSubdivisionIndex}
              />
            </div>
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-text">Vel %</span>
              <StepNumberDragInput
                boxed
                compact
                accent={interfaceAccent}
                value={velocityHumanizePercent}
                min={0}
                max={maxPercentValue}
                resetValue={0}
                ariaLabel="Velocity humanize percent"
                onValueChange={(value) =>
                  applyGlobalPercent(value, "setVelocityHumanizePercent", (next) => {
                    velocityHumanizePercent = next;
                  })}
              />
            </div>
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-text">Time %</span>
              <StepNumberDragInput
                boxed
                compact
                accent={interfaceAccent}
                value={timingHumanizePercent}
                min={0}
                max={maxPercentValue}
                resetValue={0}
                ariaLabel="Timing humanize percent"
                onValueChange={(value) =>
                  applyGlobalPercent(value, "setTimingHumanizePercent", (next) => {
                    timingHumanizePercent = next;
                  })}
              />
            </div>
    </div>

    <div
      class="flex shrink-0 flex-col gap-1 px-1"
      role="presentation"
      aria-hidden="true"
    >
      <span class="text-xs font-semibold leading-none opacity-0" aria-hidden="true">|</span>
      <div class="h-[2.125rem] w-px shrink-0 self-center bg-border-strong"></div>
    </div>

    <div class="flex min-w-0 flex-1 items-center gap-x-1.5">
      <div class="shrink-0">
        <BulkStepEditControls
          compact
          accent={interfaceAccent}
          requireSelection={false}
          totalStepCount={selectableStepCount}
          {selectedStepCount}
          shiftAvailable={bulkRowOrderCycleAvailable}
          reverseAvailable={bulkReverseAvailable}
          skipActive={bulkSkipActive}
          muteActive={bulkMuteActive}
          durationPercent={bulkDurationPercent}
          velocityPercent={bulkVelocityPercent}
          lengthDelta={bulkLengthDelta}
          transposeSemitones={bulkTransposeSemitones}
          pitchAriaLabel={bulkPitchAriaLabel}
          onShift={shiftSelectedStepsLeftByRow}
          onReverse={reverseSelectedStepsByRow}
          onShuffle={shuffleSelectedSteps}
          onRandomizeOctaves={randomizeSelectedStepOctaves}
          onRandomizeLengths={randomizeSelectedStepLengths}
          onToggleSkip={toggleBulkStepSkip}
          onToggleMute={toggleBulkStepMute}
          onGestureStart={beginBulkEditGesture}
          onLengthPreview={previewBulkLengthDelta}
          onLengthCommit={commitBulkLengthDelta}
          onDurationPreview={previewBulkDurationPercent}
          onDurationCommit={commitBulkDurationPercent}
          onVelocityPreview={previewBulkVelocityPercent}
          onVelocityCommit={commitBulkVelocityPercent}
          onTransposePreview={previewBulkTransposeSemitones}
          onTransposeCommit={commitBulkTransposeSemitones}
        />
      </div>

      <div class="min-w-0 flex-1" aria-hidden="true"></div>

      <div class="flex shrink-0 items-stretch gap-1.5">
        <button
          type="button"
          aria-label={`${seedModeActive ? "Leave" : "Enter"} seed mode for pattern ${activePatternSlot >= 0 ? activePatternSlot + 1 : viewPatternSlot + 1} in ${activeScaleName}`}
          aria-pressed={seedModeActive}
          title={seedModeActive ? "Leave seed mode" : "Enter seed mode"}
          disabled={projectOperationBusy}
          data-cursor="pointer"
          class="flex w-12 shrink-0 items-center justify-center self-stretch rounded-md border outline-none transition-colors focus-visible:ring-1 focus-visible:ring-focus-ring disabled:opacity-40 {seedModeActive
            ? 'border-accent bg-accent text-control-primary-text shadow-accent-selection'
            : 'border-border bg-surface/30 text-accent hover:border-border-strong'}"
          onclick={() => {
            void toggleSeedMode();
          }}
        >
          <SaplingIcon class="pointer-events-none h-8 w-8" />
        </button>
        <button
          type="button"
          aria-label={`Scale mode, ${activeScaleName}. Click to edit.`}
          aria-pressed={scaleDialogOpen}
          title={activeScaleName}
          data-cursor="pointer"
          class="flex min-w-[6.25rem] shrink-0 flex-col items-center justify-center gap-1 rounded-md border border-border bg-surface/30 px-1.5 py-1.5 text-center outline-none transition-[border-color,opacity] hover:border-border-strong hover:opacity-90 focus-visible:ring-1 focus-visible:ring-focus-ring"
          onclick={() => {
            scaleDialogOpen = true;
          }}
        >
          <span
            class="pointer-events-none text-[1.5rem] font-bold leading-[0.85] tracking-tight text-text"
            >{activeKeyCenterLabel}</span
          >
          <span
            class="pointer-events-none max-w-[6.25rem] truncate text-xs font-semibold leading-none text-accent"
            >{activeScaleModeLabel}</span
          >
        </button>
      </div>

      <div class="flex shrink-0 items-center gap-1">
      <div class="flex flex-col gap-1">
        <div class="flex items-center gap-1">
          <span class="w-[3.75rem] shrink-0 text-right text-xs font-semibold leading-none text-text"
            >Patterns:</span
          >
          <div class="flex items-center gap-0.5">
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
                  class={slotButtonClasses(
                    activePatternSlot === slot,
                    true,
                    patternCopySource === slot,
                    patternCopySource >= 0 && patternCopySource !== slot,
                  )}
                  onclick={(event) => handlePatternSlotClick(event, slot)}
                >
                  {slot + 1}
                </button>
              {/each}
            </div>
            <button
              type="button"
              data-pattern-clear
              aria-label={patternClearArmed ? "Confirm clear pattern" : "Clear selected pattern"}
              title={patternClearArmed ? "Click again to clear" : "Clear pattern shown in the grid"}
              data-cursor="pointer"
              class={clearPatternButtonClasses(true, patternClearArmed)}
              onclick={handleClearPatternClick}
            >
              <RemoveXIcon class="pointer-events-none h-3 w-3" />
            </button>
          </div>
        </div>
        <div class="flex items-center gap-1">
          <span class="w-[3.75rem] shrink-0 text-right text-xs font-semibold leading-none text-text"
            >Loops:</span
          >
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
        </div>
      </div>
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
    </div>
    </div>

  </header>
    </div>
  </div>

  <section class="flex min-h-0 flex-1 flex-col">
    <div class="bg-workspace shrink-0 -mx-6 w-[calc(100%+3rem)] px-6">
      <div class="flex min-w-0 items-stretch">
      {#if seedModeActive}
        <SeedModePanel
          rhythmStep={seedModeRhythmStep}
          rowSettings={seedModeRowSettings}
          rowTargets={seedModeRowTargets}
          root={scaleRoot}
          modeIndex={scaleModeIndex}
          rowColorsEnabled={rowColorsEnabled}
          busy={projectOperationBusy}
          onGestureStart={beginSeedModeGesture}
          onRhythmPreview={previewSeedModeRhythmStep}
          onRhythmCommit={commitSeedModeRhythmStep}
          onRowSettingsPreview={previewSeedModeRowSettings}
          onRowSettingsCommit={commitSeedModeRowSettings}
          onShuffle={shuffleSeedModeSettings}
          onNextSeed={nextSeedModeSeed}
          onReshuffleAspect={reshuffleSeedModeAspect}
          onRowTargetToggle={toggleSeedModeRowTarget}
          onToggleAllRowTargets={toggleAllSeedModeRowTargets}
        />
      {/if}
      <div data-phrase-grid-field class="relative flex min-w-0 flex-1 flex-col" {@attach phraseGridFieldAttachment}>
        <div
          data-phrase-grid-marquee-zone="top"
          class="h-6 shrink-0"
          role="presentation"
          aria-hidden="true"
          onpointerdown={handleRowGapBulkSelectPointerDown}
        ></div>
        {#each grid as _row, row (row)}
          {@const rowAccent = rowAccentFor(row, rowColorsEnabled)}
          {@const rowPianoRollFocusMuted =
            activeRowPianoRollEditor !== null && activeRowPianoRollEditor.row !== row}
          <div
            class="relative z-10 flex min-w-0 shrink-0 items-stretch gap-1 transition-opacity duration-150 {rowPianoRollFocusMuted
              ? 'opacity-50'
              : ''}"
          >
            <!-- svelte-ignore a11y_no_static_element_interactions -->
            <div
              data-row-header={row}
              onpointerup={(event) => handleRowHeaderPointerUp(event, row)}
              title="Click to select all steps in this row; double-click to clear row"
              class="relative flex shrink-0 self-stretch items-center border-r border-border-subtle pl-6 pr-6 {activeRowPianoRollEditor?.row === row
                ? 'bg-row-header-active'
                : 'bg-row-header'} {seedModeActive
              ? 'ml-0'
              : '-ml-6'} {row <
              grid.length - 1
                ? 'border-b'
                : ''}"
              style:--row-header-accent={rowAccentColorVar(rowAccent)}
            >
              <div class="pointer-events-none relative z-10 -ml-2 flex -translate-y-[12px] items-center gap-2.5 px-1">
                {#if rowMuted[row]}
                  <div class={rowMutedOverlayClasses} aria-hidden="true"></div>
                {/if}
                <button
                  type="button"
                  aria-label={rowMuted[row] ? "Turn row on" : "Turn row off"}
                  aria-pressed={!rowMuted[row]}
                  data-cursor="pointer"
                  class="row-power-toggle pointer-events-auto relative z-20 flex h-9 w-9 shrink-0 self-start items-center justify-center rounded-md border-0 bg-transparent p-0 outline-none focus-visible:ring-1 focus-visible:ring-focus-ring {rowMuted[row]
                    ? rowPowerToggleOffClasses
                    : rowAccent.textAccent}"
                  onclick={(event) => seedModeActive
                    ? toggleSeedModeRowMute(row, event.shiftKey)
                    : toggleRowMute(row, event.shiftKey)}
                  title="Shift-click to solo row"
                >
                  <RowDisableIcon class="row-power-toggle-icon h-9 w-9" />
                </button>
                <div class="pointer-events-auto">
                  <BipolarKnob
                    accent={rowAccent}
                    options={timingOffsetOptions}
                    value={rowTimingOffset[row]}
                    resetValue={defaultRowTimingOffsetIndex}
                    ariaLabel="Row timing offset"
                    sizeClass="h-7 w-7"
                    muted={rowMuted[row]}
                    deferCommit
                    onGestureStart={beginRowTimingOffsetGesture}
                    onValuePreview={(offsetIndex, detail) =>
                      previewRowTimingOffsetGesture(row, offsetIndex, detail)}
                    onValueCommit={(offsetIndex, detail) =>
                      commitRowTimingOffsetGesture(row, offsetIndex, detail)}
                    onValueChange={(offsetIndex) => selectRowTimingOffset(row, offsetIndex)}
                  />
                </div>
              </div>
              <div
                class="pointer-events-none absolute right-1.5 bottom-1.5 z-[1] flex items-end gap-2"
              >
                <button
                  type="button"
                  aria-label={recordingRow === row ? "Stop row recording" : "Record row from MIDI"}
                  aria-pressed={recordingRow === row}
                  data-cursor="pointer"
                  class="pointer-events-auto relative z-20 mr-1 flex shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors focus:outline-none focus-visible:outline-none {rowMuted[row]
                    ? 'text-text-faint hover:text-danger'
                    : recordingRow === row
                      ? 'text-danger'
                      : 'text-text-faint hover:text-danger'}"
                  style:width="{rowHeaderRecordIconSizePx}px"
                  style:height="{rowHeaderRecordIconSizePx}px"
                  onclick={() => toggleRowRecording(row)}
                  title={recordingRow === row
                    ? "Stop loop overdub recording"
                    : "Loop overdub from MIDI keyboard into this row"}
                >
                  <RowRecordIcon
                    compact
                    class="pointer-events-none"
                    style={rowHeaderRecordIconStyle}
                    recording={recordingRow === row}
                  />
                </button>
                <button
                  type="button"
                  aria-label={activeRowPianoRollEditor?.row === row
                    ? `Close row ${row + 1} piano roll editor`
                    : `Edit row ${row + 1} in piano roll`}
                  aria-pressed={activeRowPianoRollEditor?.row === row}
                  disabled={stepIds[row].length === 0}
                  data-cursor={stepIds[row].length > 0 ? "pointer" : undefined}
                  class="pointer-events-auto relative z-20 flex shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors focus:outline-none focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-focus-ring disabled:cursor-default disabled:opacity-45 {activeRowPianoRollEditor?.row === row
                    ? '[color:var(--row-header-accent)]'
                    : 'text-text-faint hover:[color:var(--row-header-accent)]'}"
                  style:width="{rowHeaderPianoRollIconSizePx}px"
                  style:height="{rowHeaderPianoRollIconSizePx}px"
                  onclick={() => openRowPianoRollFromHeader(row)}
                  title={activeRowPianoRollEditor?.row === row
                    ? "Show the full piano roll"
                    : stepIds[row].length > 0
                      ? `Edit row ${row + 1} in the monophonic piano roll`
                      : `Add a step to row ${row + 1} to edit it in the piano roll`}
                >
                  <RowEditPencilIcon
                    class="pointer-events-none"
                    style={rowHeaderPianoRollIconStyle}
                  />
                </button>
                <button
                  type="button"
                  aria-label={seedModeActive && seedModeRowTargets[row]
                    ? `Row ${row + 1} is targeted in the seed panel`
                    : `Seed row ${row + 1}`}
                  aria-pressed={seedModeActive && seedModeRowTargets[row]}
                  data-cursor="pointer"
                  class="pointer-events-auto relative z-20 flex shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors focus:outline-none focus-visible:outline-none {seedModeActive && seedModeRowTargets[row]
                    ? '[color:var(--row-header-accent)]'
                    : 'text-text-faint hover:[color:var(--row-header-accent)] transition-colors duration-150'}"
                  style:width="{rowHeaderSeedIconSizePx}px"
                  style:height="{rowHeaderSeedIconSizePx}px"
                  onclick={() => targetRowSeedMode(row)}
                  title={`Target row ${row + 1} in the seed panel`}
                >
                  <SaplingIcon
                    class="pointer-events-none"
                    style={rowHeaderSeedIconStyle}
                  />
                </button>
              </div>
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
              {stepTimingMultiplierById}
              stepVelocity={stepVelocity[row]}
              stepMuted={stepMuted[row]}
              stepSkipped={stepSkipped[row]}
              stepProbability={stepProbability[row]}
              stepCycle={stepCycle[row]}
              stepCycleOffset={stepCycleOffset[row]}
              activeGates={activeGates[row]}
              selectedStepIds={selectedStepIdsByRow[row]}
              allSelectedStepIds={selectedStepIdsForDragGhost}
              stepInspectionActive={activeStepInspector !== null || rowPianoRollCurrentStepFocusVisible}
              stepInspectorOpen={activeStepInspector !== null}
              stretchToFit={stretchStepsToFit}
              contentFitScale={phraseStepsContentFitScale}
              fitGridColumns={compactGridLayout.totalColumns}
              inspectedStepId={
                activeStepInspector !== null && inspectedStep?.row === row
                  ? inspectedStep.stepId
                  : rowPianoRollCurrentStepFocusVisible && activeRowPianoRollEditor?.row === row
                    ? activeRowPianoRollEditor.stepId
                    : null
              }
              stepInspectorHighlightedId={
                activeStepInspector !== null && inspectedStep?.row === row
                  ? inspectedStep.stepId
                  : null
              }
              stepNoteValue={stepNoteByCurrentScale}
              defaultStepNote={defaultNewStepNote}
              {timingMultiplierOptions}
              onReorder={reorderRowByIds}
              onMoveCommitted={commitRowMove}
              onBulkMoveCommitted={commitBulkRowMove}
              onBulkStepDuplicateDrop={duplicateBulkStepsToDrop}
              onCrossRowMove={moveStepToRow}
              onBulkCrossRowMove={moveBulkToRow}
              onBulkCrossRowDuplicateDrop={duplicateBulkToRow}
              onStepDuplicateDrop={duplicateStepToDrop}
              onDuplicateDragChange={(stepId) => duplicateDragStepId = stepId}
              onBulkDragSessionChange={(blockIds) => bulkDragStepIds = blockIds}
              {duplicateDragStepId}
              bulkDragStepIds={bulkDragStepIds}
              onRemoveStep={removeStep}
              onInsertStep={insertStep}
              onDuplicateStep={duplicateStep}
              onNotePreview={previewPhraseNoteValue}
              onNoteCommit={commitPhraseNoteValue}
              onStepBulkGestureStart={beginPhraseStepBulkGesture}
              onMultiplierChange={selectStepTimingMultiplier}
              onCompactTimingPreview={(multipliers) => {
                compactTimingPreview = { row, multipliers };
              }}
              onCompactTimingPreviewEnd={() => {
                if (compactTimingPreview?.row === row) {
                  compactTimingPreview = null;
                }
              }}
              onStepMove={movePhraseStepFromPianoRoll}
              onDurationPreview={previewPhraseStepDuration}
              onDurationCommit={commitPhraseStepDuration}
              onVelocityPreview={previewPhraseStepVelocity}
              onVelocityCommit={commitPhraseStepVelocity}
              onStepMuteChange={setStepMuted}
              onStepSkipChange={setStepSkipped}
              onInspectStep={openStepInspector}
              onPrepareStepSelection={prepareStepSelection}
              onBulkSelectPointerDown={beginStepMarqueeSelection}
              onBulkSelectBackgroundDoubleClick={selectAllStepsForBulkEdit}
              onDismissPhraseBackground={dismissPhraseEditingFocus}
            />
          </div>
          {#if row < grid.length - 1}
            <div
              class="h-2.5 shrink-0"
              role="presentation"
              aria-hidden="true"
              onpointerdown={handleRowGapBulkSelectPointerDown}
            ></div>
          {/if}
        {/each}
        <div
          data-phrase-grid-marquee-zone="bottom"
          class="h-6 shrink-0"
          role="presentation"
          aria-hidden="true"
          onpointerdown={handleRowGapBulkSelectPointerDown}
        ></div>
      </div>
      </div>
    </div>

    <div class="-mx-6 shrink-0">
    <CombinationModeRail
      mask={combinationModeMask}
      onToggle={toggleCombinationMode}
      {combinationSyncDivisionIndex}
      onCombinationSyncDivisionChange={applyCombinationSyncDivisionIndex}
      combinationChangePhaseContinue={combinationChangePhaseContinue}
      onCombinationChangePhaseContinueChange={applyCombinationChangePhaseContinue}
      noteBandpassLowMidi={noteBandpassLowMidi}
      noteBandpassHighMidi={noteBandpassHighMidi}
      onNoteBandpassChange={handleNoteBandpassChange}
      onNoteBandpassCommit={commitNoteBandpass}
      velocityTiltPivotMidi={velocityTiltPivotMidi}
      velocityTiltAmount={velocityTiltAmount}
      globalTransposeSemitones={globalTransposeSemitones}
      onVelocityTiltPivotPreview={previewVelocityTiltPivot}
      onVelocityTiltPivotCommit={commitVelocityTiltPivot}
      onVelocityTiltAmountPreview={previewVelocityTiltAmount}
      onVelocityTiltAmountCommit={commitVelocityTiltAmount}
      onVelocityTiltXYCommit={commitVelocityTiltXY}
      onGlobalTransposePreview={previewGlobalTranspose}
      onGlobalTransposeCommit={commitGlobalTranspose}
      octavizerDown8vaEnabled={octavizerDown8vaEnabled}
      octavizerUp8vaEnabled={octavizerUp8vaEnabled}
      octavizerDown8vaRelativeVelocity={octavizerDown8vaRelativeVelocity}
      octavizerUp8vaRelativeVelocity={octavizerUp8vaRelativeVelocity}
      onOctavizerDown8vaToggle={handleOctavizerDown8vaToggle}
      onOctavizerUp8vaToggle={handleOctavizerUp8vaToggle}
      onProcessingParamGestureStart={beginProcessingParamGesture}
      onOctavizerDown8vaRelativeVelocityPreview={previewOctavizerDown8vaRelativeVelocity}
      onOctavizerDown8vaRelativeVelocityCommit={commitOctavizerDown8vaRelativeVelocity}
      onOctavizerUp8vaRelativeVelocityPreview={previewOctavizerUp8vaRelativeVelocity}
      onOctavizerUp8vaRelativeVelocityCommit={commitOctavizerUp8vaRelativeVelocity}
      shimmerEnabled={shimmerEnabled}
      shimmerDelayMultiplierIndex={shimmerDelayMultiplierIndex}
      shimmerFeedbackPercent={shimmerFeedbackPercent}
      shimmerMixPercent={shimmerMixPercent}
      onShimmerToggle={handleShimmerToggle}
      onShimmerDelayPreview={previewShimmerDelay}
      onShimmerDelayCommit={commitShimmerDelay}
      onShimmerFeedbackPreview={previewShimmerFeedback}
      onShimmerFeedbackCommit={commitShimmerFeedback}
      onShimmerMixPreview={previewShimmerMix}
      onShimmerMixCommit={commitShimmerMix}
    />
    </div>

    <div class="bg-workspace -mx-6 flex min-h-0 min-w-0 w-[calc(100%+3rem)] flex-1 flex-col overflow-hidden px-6">
    {#if activeStepInspector !== null}
      <StepInspector
        row={activeStepInspector.row}
        step={activeStepInspector.step}
        bulkEditStepCount={activeStepInspectorBulkEditCount}
        note={activeStepInspector.note}
        velocity={activeStepInspector.velocity}
        durationFraction={activeStepInspector.durationFraction}
        timingMultiplierIndex={activeStepInspector.timingMultiplierIndex}
        probability={activeStepInspector.probability}
        cycle={activeStepInspector.cycle}
        cycleMask={activeStepInspector.cycleMask}
        cycleTriggerCount={activeStepInspector.cycleTriggerCount}
        muted={activeStepInspector.muted}
        skipped={activeStepInspector.skipped}
        {scaleRoot}
        {scaleModeIndex}
        accent={rowAccentFor(activeStepInspector.row, rowColorsEnabled)}
        defaultStepNote={defaultNewStepNote}
        stepNoteValue={stepNoteByCurrentScale}
        onNoteGestureStart={() =>
          beginPhraseStepBulkGesture(activeStepInspector.row, activeStepInspector.step)}
        onNotePreview={(midi) =>
          previewPhraseNoteValue(activeStepInspector.row, activeStepInspector.step, midi)}
        onNoteCommit={(midi) =>
          commitPhraseNoteValue(activeStepInspector.row, activeStepInspector.step, midi)}
        onNoteChange={(midi) =>
          commitPhraseNoteValue(activeStepInspector.row, activeStepInspector.step, midi)}
        onVelocityChange={(value) =>
          setStepVelocity(activeStepInspector.row, activeStepInspector.step, value)}
        onDurationGestureStart={beginBulkEditGesture}
        onDurationPreview={(percent) =>
          previewPhraseStepDuration(
            activeStepInspector.row,
            activeStepInspector.step,
            percent / 100,
          )}
        onDurationCommit={(percent) =>
          commitPhraseStepDuration(
            activeStepInspector.row,
            activeStepInspector.step,
            percent / 100,
          )}
        onTimingMultiplierChange={(index) =>
          selectStepTimingMultiplier(
            activeStepInspector.row,
            activeStepInspector.step,
            index,
          )}
        onProbabilityGestureStart={beginBulkEditGesture}
        onProbabilityPreview={(value) =>
          previewInspectorStepProbability(activeStepInspector.row, activeStepInspector.step, value)}
        onProbabilityCommit={(value) =>
          commitInspectorStepProbability(activeStepInspector.row, activeStepInspector.step, value)}
        onCycleGestureStart={beginBulkEditGesture}
        onCyclePatternPreview={(nextCycle, nextMask) =>
          previewInspectorStepCyclePattern(
            activeStepInspector.row,
            activeStepInspector.step,
            nextCycle,
            nextMask,
          )}
        onCyclePatternCommit={(nextCycle, nextMask) =>
          commitInspectorStepCyclePattern(
            activeStepInspector.row,
            activeStepInspector.step,
            nextCycle,
            nextMask,
          )}
        onMutedChange={(value) =>
          setStepMuted(activeStepInspector.row, activeStepInspector.step, value)}
        onSkippedChange={(value) =>
          setStepSkipped(activeStepInspector.row, activeStepInspector.step, value)}
        onRemove={removeInspectedStep}
      />
    {:else if activeRowPianoRollEditor !== null}
      <RowPianoRollEditor
        row={activeRowPianoRollEditor.row}
        stepIds={stepIds[activeRowPianoRollEditor.row]}
        notes={grid[activeRowPianoRollEditor.row]}
        stepDurationFraction={stepDurationFraction[activeRowPianoRollEditor.row]}
        stepTimingMultiplier={stepTimingMultiplier[activeRowPianoRollEditor.row]}
        stepVelocity={stepVelocity[activeRowPianoRollEditor.row]}
        stepMuted={stepMuted[activeRowPianoRollEditor.row]}
        stepSkipped={stepSkipped[activeRowPianoRollEditor.row]}
        activeGates={activeGates[activeRowPianoRollEditor.row]}
        rowTimingOffset={rowTimingOffset[activeRowPianoRollEditor.row]}
        {pulseIndex}
        {scaleRoot}
        {scaleModeIndex}
        inspectedStepId={rowPianoRollCurrentStepFocusVisible
          ? activeRowPianoRollEditor.stepId
          : null}
        selectedStepIds={selectedStepIdsByRow[activeRowPianoRollEditor.row]}
        accent={rowAccentFor(activeRowPianoRollEditor.row, rowColorsEnabled)}
        onInspectStep={(row, step, stepId) => {
          rowPianoRollStep = { row, stepId };
          prepareStepSelection(row, step, stepId);
        }}
        onNotePreview={previewPhraseNoteValue}
        onNoteCommit={commitPhraseNoteValue}
        onShapeVelocitiesCommit={commitPhraseRowVelocityShape}
        onStepMove={movePhraseStepFromPianoRoll}
        onStepResize={selectStepTimingMultiplier}
        onStepBulkGestureStart={beginPhraseStepBulkGesture}
        onOpenAdvancedInspector={openStepInspector}
        onInsertStep={insertStep}
        onDuplicateStep={duplicateStep}
        onBulkSelectPointerDown={beginStepMarqueeSelection}
      />
    {:else if recordingRow !== null}
      {@const recordingAccent = rowAccentFor(recordingRow, rowColorsEnabled)}
      <section class="flex min-h-0 w-full flex-1 overflow-hidden bg-workspace">
        <RecordModePanel
          row={recordingRow}
          cycleLengthPulses={recordCycleLengthPulses[recordingRow] ?? 16}
          progress={recordingProgress}
          accent={recordingAccent}
          onCycleLengthChange={setRecordingCycleLengthPulses}
        />
        <RecordPianoKeyboard
          row={recordingRow}
          {scaleRoot}
          {scaleModeIndex}
          accent={recordingAccent}
          heldKeys={recordingKeysHeld}
          onNotePress={onRecordPianoNotePress}
          onNoteRelease={onRecordPianoNoteRelease}
        />
      </section>
    {:else}
      <PianoRollPreview
        notes={grid}
        {rowColorsEnabled}
        {rowMuted}
        {rowTimingOffset}
        {stepDurationFraction}
        {stepTimingMultiplier}
        {stepVelocity}
        {rowMidiChannel}
        {stepMuted}
        {stepSkipped}
        stepProbability={stepProbability}
        stepCycle={stepCycle}
        stepCycleOffset={stepCycleOffset}
        {combinationModeMask}
        {scaleRoot}
        {scaleModeIndex}
        noteBandpassLowMidi={noteBandpassLowMidi}
        noteBandpassHighMidi={noteBandpassHighMidi}
        velocityTiltPivotMidi={velocityTiltPivotMidi}
        velocityTiltAmount={velocityTiltAmount}
        globalTransposeSemitones={globalTransposeSemitones}
        octavizerDown8vaEnabled={octavizerDown8vaEnabled}
        octavizerUp8vaEnabled={octavizerUp8vaEnabled}
        octavizerDown8vaRelativeVelocity={octavizerDown8vaRelativeVelocity}
        octavizerUp8vaRelativeVelocity={octavizerUp8vaRelativeVelocity}
        shimmerEnabled={shimmerEnabled}
        shimmerDelayMultiplierIndex={shimmerDelayMultiplierIndex}
        shimmerFeedbackPercent={shimmerFeedbackPercent}
        shimmerMixPercent={shimmerMixPercent}
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
    </div>
    <div class="-mx-6 w-[calc(100%+3rem)] shrink-0">
      <div class="mp-honeycomb-rail relative z-20 overflow-x-auto">
      <div class="grid w-full min-w-[70rem] grid-cols-[auto_minmax(28rem,1fr)_auto] items-center gap-5 px-6 py-1.5">
        <div class="flex shrink-0 items-center gap-2">
          <button
            type="button"
            class="h-8 rounded border border-border-subtle bg-surface-raised px-4 text-xs font-semibold uppercase tracking-[0.16em] text-text transition-colors hover:border-accent/60 disabled:opacity-40"
            disabled={projectOperationBusy || !nativeFunctionAvailable("newProject")}
            data-cursor="pointer"
            onclick={createNewProject}
            title="Start a new empty project"
          >New</button>
          <button
            type="button"
            class="h-8 rounded border border-border-subtle bg-surface-raised px-4 text-xs font-semibold uppercase tracking-[0.16em] text-text transition-colors hover:border-accent/60 disabled:opacity-40"
            disabled={projectOperationBusy || !nativeFunctionAvailable("loadProject")}
            data-cursor="pointer"
            onclick={loadProject}
          >Load</button>
          <button
            type="button"
            class="h-8 rounded border border-border-subtle bg-surface-raised px-4 text-xs font-semibold uppercase tracking-[0.16em] text-text transition-colors hover:border-accent/60 disabled:opacity-40"
            disabled={projectOperationBusy || !nativeFunctionAvailable("saveProject")}
            data-cursor="pointer"
            onclick={saveProject}
          >Save</button>
        </div>

        <div class="flex min-w-0 items-stretch gap-2">
          <button
            type="button"
            aria-label="Load previous project"
            title="Previous project"
            class="h-8 w-10 rounded-l border border-border-subtle bg-surface-raised text-xl text-text-muted transition-colors hover:border-accent/60 hover:text-accent disabled:opacity-25"
            disabled={projectOperationBusy || !hasPreviousProject}
            data-cursor="pointer"
            onclick={() => cycleProject(-1)}
          >&#9664;</button>
          <div
            class="flex h-8 min-w-0 flex-1 items-center gap-3 rounded border border-border-subtle bg-surface/80 px-4 shadow-inner"
            title={projectFileName || "Unsaved project"}
          >
            <input
              aria-label="Project name"
              class="w-36 shrink-0 truncate border-0 bg-transparent p-0 text-sm font-semibold tracking-wide text-text outline-none placeholder:text-text-faint focus:text-accent"
              maxlength="96"
              bind:value={projectName}
              placeholder="Untitled Project"
            />
            <input
              aria-label="Project description"
              class="min-w-0 flex-1 truncate border-0 bg-transparent p-0 text-xs text-text outline-none placeholder:text-text-faint focus:text-text"
              maxlength="240"
              bind:value={projectDescription}
              placeholder="Add a project description…"
            />
            <span class="shrink-0 text-[0.65rem] font-semibold uppercase tracking-[0.14em] text-text">
              {projectDateLabel}
            </span>
          </div>
          <button
            type="button"
            aria-label="Load next project"
            title="Next project"
            class="h-8 w-10 rounded-r border border-border-subtle bg-surface-raised text-xl text-text-muted transition-colors hover:border-accent/60 hover:text-accent disabled:opacity-25"
            disabled={projectOperationBusy || !hasNextProject}
            data-cursor="pointer"
            onclick={() => cycleProject(1)}
          >&#9654;</button>
        </div>

        <div class="flex shrink-0 items-center justify-end gap-1.5">
          {#if showRowColorsToggle}
            <ColorsToggle
              accent={interfaceAccent}
              enabled={rowColorsEnabled}
              onChange={async (next) => {
                rowColorsEnabled = next;
                await pushRowColorsEnabled();
              }}
            />
          {/if}
          <ThemeModeToggle value={themeMode} onValueChange={setThemeMode} />
          <UiScaleDragInput
            value={uiScaleState.percent}
            onValueChange={setExplicitUiScalePercent}
          />
          <button
            type="button"
            aria-label={editorFullscreen ? "Exit fullscreen" : "Enter fullscreen"}
            aria-pressed={editorFullscreen}
            title={editorFullscreen ? "Exit fullscreen" : "Enter fullscreen"}
            disabled={editorFullscreenBusy || !nativeFunctionAvailable("setEditorFullscreen")}
            data-cursor="pointer"
            class={editorFullscreenButtonClasses()}
            onclick={toggleEditorFullscreen}
          >
            <FullscreenIcon class="pointer-events-none h-4 w-4" />
          </button>
        </div>
        {#if projectOperationError}
          <div class="col-start-2 -mt-1 truncate text-[0.65rem] text-danger" role="status">
            {projectOperationError}
          </div>
        {/if}
      </div>
      </div>
    </div>
  </section>
  {#if marqueeSelection}
    <div
      class="pointer-events-none fixed z-[9999] rounded-sm border border-accent/55 bg-link-hover/10 shadow-accent-selection"
      style={marqueeRectStyle}
      aria-hidden="true"
    ></div>
  {/if}
</main>

{#if scaleDialogOpen}
  <ScaleModeDialog
    root={scaleRoot}
    modeIndex={scaleModeIndex}
    patternSlot={activePatternSlot >= 0 ? activePatternSlot : viewPatternSlot}
    onClose={() => {
      scaleDialogOpen = false;
    }}
    onChange={setPatternScale}
  />
{/if}
