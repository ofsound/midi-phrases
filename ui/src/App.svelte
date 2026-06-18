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
  import RowRecordIcon from "./RowRecordIcon.svelte";
  import BulkStepEditControls from "./BulkStepEditControls.svelte";
  import ScaleModeDialog from "./ScaleModeDialog.svelte";
  import BipolarKnob from "./BipolarKnob.svelte";
  import PhraseRow from "./PhraseRow.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import PianoRollPreview from "./PianoRollPreview.svelte";
  import RowPianoRollEditor from "./RowPianoRollEditor.svelte";
  import CombinationModeRail from "./CombinationModeRail.svelte";
  import RecordPianoKeyboard from "./RecordPianoKeyboard.svelte";
  import StepInspector from "./StepInspector.svelte";
  import {
    defaultStepTimingMultiplierIndex,
    maxPhraseStepsPerRow,
    findSingleMove,
    insertStepTimingMultiplierOptions,
    compactPhraseGridLayout,
    stepTimingMultiplierCount,
    timingMultiplierOptions,
  } from "./stepCellLayout.js";
  import { sanitizeOrderedIds } from "./dndUtils.js";
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
    combinationModeMaskBits,
    combinationModes,
    isStepActiveAtBeat,
    swingSubdivisionOptions,
    swingSubdivisionValues,
  } from "./phraseSchedule.js";
  import DiscreteDragSelect from "./DiscreteDragSelect.svelte";
  import PulseNoteButtonGroup from "./PulseNoteButtonGroup.svelte";
  import ColorsToggle from "./ColorsToggle.svelte";
  import ThemeModeToggle from "./ThemeModeToggle.svelte";
  import StretchToFitToggle from "./StretchToFitToggle.svelte";
  import FullscreenIcon from "./FullscreenIcon.svelte";
  import MidiPhrasesLogo from "./MidiPhrasesLogo.svelte";
  import UiScaleDragInput from "./UiScaleDragInput.svelte";
  import RemoveXIcon from "./RemoveXIcon.svelte";
  import { defaultPulseIndex, pulseOptions } from "./pulseLayout.js";
  import {
    phraseGridVisualOffsetCompensationPx,
  } from "./phraseRowLayout.js";
  import {
    interfaceAccent,
    rowAccentFor,
    rowMutedOverlayClasses,
    rowMuteControlClasses,
    rowPowerToggleOffClasses,
    rowActionIconControlClasses,
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
    setUiScalePercent,
    setUiViewportSize,
    storedUiScalePercent,
    uiScaleState,
  } from "./uiScale.svelte.js";

  const randomStepTimingMultiplierIndices = insertStepTimingMultiplierOptions(timingMultiplierOptions)
    .map((option) => option.index);

  /** @type {HTMLElement | null} */
  let appRoot = $state(null);
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
  let stretchStepsToFit = $state(false);
  let compactGridLayout = $derived(
    compactPhraseGridLayout(stepTimingMultiplier, rowTimingOffset),
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

  let playbackPollFrameId = 0;
  let slotSelectionInFlight = 0;
  let previousGateSnapshot = defaultPhraseGrid().map((row) => row.map(() => false));
  let activeGateHoldUntil = defaultPhraseGrid().map((row) => row.map(() => 0));
  let bulkPreviewSyncPromise = Promise.resolve();

  /** Row index armed for MIDI capture, or null. */
  let recordingRow = $state(null);
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
  let scaleRoot = $state(defaultScaleRoot);
  let scaleModeIndex = $state(defaultScaleModeIndex);
  let noteBandpassLowMidi = $state(defaultNoteBandpassLowMidi);
  let noteBandpassHighMidi = $state(defaultNoteBandpassHighMidi);
  let velocityTiltPivotMidi = $state(defaultVelocityTiltPivotMidi);
  let velocityTiltAmount = $state(defaultVelocityTiltAmount);
  let octavizerDown8vaEnabled = $state(false);
  let octavizerUp8vaEnabled = $state(false);
  let octavizerDown8vaRelativeVelocity = $state(defaultOctavizerRelativeVelocity);
  let octavizerUp8vaRelativeVelocity = $state(defaultOctavizerRelativeVelocity);
  let shimmerEnabled = $state(false);
  let shimmerDelayMultiplierIndex = $state(defaultShimmerDelayMultiplierIndex);
  let shimmerFeedbackPercent = $state(defaultShimmerFeedbackPercent);
  let shimmerMixPercent = $state(defaultShimmerMixPercent);
  let scaleDialogOpen = $state(false);
  let pulseIndex = $state(defaultPulseIndex);
  let swingPercent = $state(0);
  let velocityHumanizePercent = $state(0);
  let timingHumanizePercent = $state(0);
  let swingSubdivisionIndex = $state(1);
  let rowColorsEnabled = $state(true);
  let themeMode = $state(defaultThemeMode);
  let undoStack = $state([]);
  let redoStack = $state([]);
  const selectedStepKeys = new SvelteSet();
  /** @type {string[][]} */
  let selectableStepKeySet = $derived(new Set(allSelectableStepKeys()));
  let selectedStepKeysForGrid = $derived(
    new Set([...selectedStepKeys].filter((key) => selectableStepKeySet.has(key))),
  );
  let selectedStepIdsByRow = $derived(selectedStepIdsByRowForKeys(selectedStepKeysForGrid));
  let bulkDurationPercent = $state(0);
  let bulkVelocityPercent = $state(0);
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
  const rowGapDoubleClickIntervalMs = 400;
  const rowGapDoubleClickMaxDistancePx = 16;
  const stepTriggerFlashMs = 110;
  const historyButtonBaseClasses =
    "flex h-8 w-8 items-center justify-center rounded-md border transition-[border-color,color,box-shadow,filter] outline-none focus:ring-1 focus:ring-focus-ring disabled:border-border-subtle disabled:text-text-faint";

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

  function setExplicitUiScalePercent(next) {
    setUiScalePercent(next);
    void syncEditorScaleMinimumToNative();
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

  function slotButtonClasses(active, assigned = true, copySource = false, copyTarget = false) {
    if (copySource) {
      return "flex h-[2.1rem] w-[2.1rem] items-center justify-center rounded-sm border border-accent text-sm font-semibold leading-none text-control-primary-text outline-none focus:ring-1 focus:ring-focus-ring mp-slot-copy-source";
    }

    const base = `flex h-[2.1rem] w-[2.1rem] items-center justify-center rounded-sm border text-sm font-semibold leading-none transition-[border-color,color,box-shadow,filter] outline-none focus:ring-1 focus:ring-focus-ring ${
      active
        ? "border-accent bg-accent text-control-primary-text"
        : assigned
          ? "mp-control-gradient border-border text-text hover:border-border-strong"
          : "mp-control-gradient-muted border-border-subtle text-text-faint hover:border-border hover:text-text-secondary"
    }`;

    return copyTarget ? `${base} mp-slot-copy-target` : base;
  }

  function clearPatternButtonClasses(enabled, armed = false) {
    return `flex h-[2.1rem] w-[2.1rem] items-center justify-center border-0 bg-transparent transition-colors outline-none focus:ring-1 focus:ring-focus-ring ${
      armed
        ? "mp-pattern-clear-armed text-danger"
        : enabled
          ? "text-danger hover:text-danger"
          : "text-text-faint"
    }`;
  }

  function outputMuteButtonClasses(active) {
    return `flex h-[calc(2.1rem*1.33)] w-[calc(2.1rem*1.33)] items-center justify-center rounded-sm border text-sm font-semibold leading-none transition-[border-color,color,box-shadow,filter] outline-none focus:ring-1 focus:ring-focus-ring ${
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

  /** @param {number} row @param {{ step: number, midi: number }[]} updates */
  async function commitPhraseRowNoteShape(row, updates) {
    if (updates.length === 0) return;

    await commitHistory("Draw note shape", async () => {
      for (const { step, midi } of updates) {
        grid[row][step] = clampPhraseNote(midi);
      }

      await pushCurrentPhraseRow(row);
    });
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

  async function setPatternScale(nextRoot, nextModeIndex) {
    const root = clampScaleRoot(nextRoot);
    const mode = clampScaleModeIndex(nextModeIndex);

    await commitHistory("Change scale mode", async () => {
      scaleRoot = root;
      scaleModeIndex = mode;

      if (nativeFunctionAvailable("setPatternScale")) {
        await getNativeFunction("setPatternScale")(root, mode);
      }
    });
  }

  /** UI-only; shifts phrase rows when any row has a negative offset. */
  let phraseVisualOffsetCompensationPx = $derived(phraseGridVisualOffsetCompensationPx(
    rowTimingOffset,
  ));
  let activeScaleName = $derived(scaleName(scaleRoot, scaleModeIndex));
  let activeKeyCenterLabel = $derived(keyCenters[scaleRoot]?.label ?? "C");
  let defaultNewStepNote = $derived(defaultStepNoteForScaleRoot(scaleRoot));
  let activeScaleModeLabel = $derived(scaleModes[scaleModeIndex]?.shortLabel ?? "Chrom");
  let bulkPitchAriaLabel = $derived(
    isChromaticScaleMode(scaleModeIndex)
      ? "Bulk step pitch semitones"
      : "Bulk step pitch scale degrees",
  );
  let selectedStepCount = $derived(selectedStepKeysForGrid.size);
  let selectableStepCount = $derived(stepIds.reduce((count, rowStepIds) => count + rowStepIds.length, 0));
  let bulkReverseAvailable = $derived.by(() => {
    for (const locations of bulkEditLocationsGroupedByRow().values()) {
      if (locations.length > 1) {
        return true;
      }
    }

    return false;
  });
  let bulkSkipActive = $derived.by(() => {
    const locations = bulkEditLocations();

    return locations.length > 0 && locations.every(({ row, step }) => stepSkipped[row][step]);
  });
  let bulkMuteActive = $derived.by(() => {
    const locations = bulkEditLocations();

    return locations.length > 0 && locations.every(({ row, step }) => stepMuted[row][step]);
  });
  let rowPianoRollBulkStepCount = $derived.by(() => {
    const editor = activeRowPianoRollEditor;
    if (editor === null) return 0;

    const row = editor.row;
    const selected = selectedStepIdsByRow[row]?.length ?? 0;
    const total = stepIds[row]?.length ?? 0;

    return selected > 0 ? selected : total;
  });
  let rowPianoRollBulkReverseAvailable = $derived(rowPianoRollBulkStepCount > 1);
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
      probability: stepProbability[row][step] ?? 100,
      cycle: stepCycle[row][step] ?? 1,
      cycleMask: stepCycleOffset[row][step] ?? defaultStepCycleMask,
    };
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

    inspectedStep = { row, stepId };
  }

  /** @param {number} row @param {number} step @param {string} stepId */
  function prepareStepSelection(row, step, stepId) {
    setSelectedStepKeys(new Set([stepSelectionKey(row, stepId)]));
    syncBulkControlsFromSelection();
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

  function syncBulkControlsFromSelection() {
    const [first] = selectedStepLocations();

    bulkDurationPercent = 0;
    bulkVelocityPercent = 0;
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

  /** @param {PointerEvent} event */
  function beginStepMarqueeSelection(event) {
    if (event.button !== 0 || marqueeSelection) return;

    const target = event.target;

    if (!(target instanceof Element)) return;

    const stepCell = target.closest("[data-bulk-step-cell]");
    const compactStepCell =
      stepCell instanceof HTMLElement && stepCell.hasAttribute("data-compact-step-cell");

    if (event.shiftKey && stepCell instanceof HTMLElement) {
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
      combinationModeMask,
      scaleRoot,
      scaleModeIndex,
      noteBandpassLowMidi,
      noteBandpassHighMidi,
      velocityTiltPivotMidi,
      velocityTiltAmount,
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
    combinationModeMask = (next.combinationModeMask ?? 0) & combinationModeMaskBits;
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

    if (locations.length === 0 || randomStepTimingMultiplierIndices.length === 0) return;

    await commitHistory("Randomize selected step lengths", async () => {
      const changedRows = new SvelteSet(locations.map(({ row }) => row));

      for (const { row, step } of locations) {
        const randomIndex = Math.floor(Math.random() * randomStepTimingMultiplierIndices.length);
        stepTimingMultiplier[row][step] = randomStepTimingMultiplierIndices[randomIndex];
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

  async function selectRowTimingOffset(row, offsetIndex) {
    await commitHistory("Change row timing", async () => {
      rowTimingOffset[row] = offsetIndex;
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
    const parsed = Number.parseInt(String(value), 10);

    return Number.isNaN(parsed) ? 0 : Math.min(100, Math.max(-100, parsed));
  }

  function clampTransposeSemitones(value) {
    const parsed = Number.parseInt(String(value), 10);

    return Number.isNaN(parsed) ? 0 : Math.min(48, Math.max(-48, parsed));
  }

  function clampStepDurationPercent(value) {
    return Math.min(100, Math.max(0, value));
  }

  function clampStepVelocityPercent(value) {
    return Math.min(100, Math.max(0, value));
  }

  async function pushRowsForSelectedLocations(locations) {
    const rows = [...new Set(locations.map((location) => location.row))];

    for (const row of rows) {
      await pushCurrentPhraseRow(row);
    }
  }

  /**
   * @param {{ row: number, step: number }[]} locations
   * @param {(row: number, step: number) => Promise<void>} pushStep
   */
  function queueBulkPreviewSync(locations, pushStep) {
    const snapshot = locations.map(({ row, step }) => ({ row, step }));

    bulkPreviewSyncPromise = bulkPreviewSyncPromise
      .catch(() => {})
      .then(async () => {
        for (const { row, step } of snapshot) {
          await pushStep(row, step);
        }
      });
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

  async function setStepProbability(row, step, probability) {
    await commitHistory("Change probability", async () => {
      stepProbability[row][step] = Math.min(100, Math.max(0, probability));
      await pushStepProbability(row, step);
    });
  }

  async function setStepCyclePattern(row, step, cycle, cycleMask) {
    const normalized = normalizeCyclePattern(cycle, cycleMask);
    const editorNormalized = normalizeEditorCyclePattern(normalized.cycle, normalized.mask);

    await commitHistory("Change cycle", async () => {
      stepCycle[row][step] = editorNormalized.cycle;
      stepCycleOffset[row][step] = editorNormalized.mask;
      await pushStepCycle(row, step);
      await pushStepCycleOffset(row, step);
    });
  }

  async function setStepCycle(row, step, cycle) {
    await setStepCyclePattern(row, step, cycle, stepCycleOffset[row][step] ?? defaultStepCycleMask);
  }

  async function setStepCycleMask(row, step, cycleMask) {
    await setStepCyclePattern(row, step, stepCycle[row][step] ?? 1, cycleMask);
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
      stepProbability[row].splice(step, 0, defaultProbability[row]?.[0] ?? 100);
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
      probability: 100,
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

    closeStepInspector();
    closeRowPianoRollEditor();
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
            .filter(
              (midi) =>
                !Number.isNaN(midi) &&
                (isChromaticScaleMode(scaleModeIndex) ||
                  isMidiInScale(midi, scaleRoot, scaleModeIndex)),
            ),
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
        if (!isChromaticScaleMode(scaleModeIndex) && !isMidiInScale(midi, scaleRoot, scaleModeIndex)) {
          continue;
        }

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

    if (init === null) return;

    const raw = Array.isArray(init) ? init[0] : init;
    rowColorsEnabled = Boolean(Number.parseInt(String(raw), 10));
  }

  function loadCombinationModesFromInitialisation() {
    const init = unwrapJuceInit("combinationModeMask");
    const raw = Array.isArray(init) ? init[0] : init;
    const value = Number.parseInt(String(raw ?? 0), 10);

    combinationModeMask = Number.isNaN(value) ? 0 : value & combinationModeMaskBits;
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
    loadCombinationModesFromInitialisation();
    loadPatternScaleFromInitialisation();
    loadNoteBandpassFromInitialisation();
    loadVelocityTiltFromInitialisation();
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
    themeMode = applyThemeMode(storedThemeMode(), { persist: false });
    setUiScalePercent(storedUiScalePercent(), { persist: false });
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

    window.addEventListener("keydown", handleKeydown);
    playbackPollFrameId = requestAnimationFrame(pollPlaybackActivity);

    return () => {
      resizeObserver.disconnect();
      if (scaleFrameId) cancelAnimationFrame(scaleFrameId);
      window.removeEventListener("keydown", handleKeydown);
      cancelAnimationFrame(playbackPollFrameId);
      document.removeEventListener("pointermove", updateMarqueePointer);
      document.removeEventListener("pointerup", finishMarqueeSelection);
      document.removeEventListener("pointercancel", cancelMarqueeSelection);
    };
  });
</script>

<main {@attach appRootAttachment} class="flex h-full flex-col overflow-hidden px-6 pt-3 transition-[filter,opacity] duration-150 {scaleDialogOpen ? 'pointer-events-none blur-[3px] opacity-45' : ''}">
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
            class="mp-control-gradient h-8 w-[4.5rem] rounded-md border border-border px-2 text-sm font-semibold text-text outline-none focus:border-focus-ring focus:ring-1 focus:ring-focus-ring"
            onchange={setStandaloneTempoFromInput}
          />
        </label>
      </div>
    {/if}
    <div class="mp-honeycomb-rail relative z-20">
  <header class="flex items-end gap-3 px-6 pb-3 pt-3">
    <div class="relative z-30 flex shrink-0 -translate-y-3 items-end gap-5">
      <div class="flex flex-col items-start gap-[3px]">
        <div class="flex items-start gap-1.5">
          <p class="text-sm font-bold uppercase leading-none tracking-widest text-accent">
            ofsound
          </p>
        </div>
        <div class="flex h-8 items-end">
          <h1 class="translate-y-0.5 whitespace-nowrap leading-none" aria-label={pluginName}>
            <MidiPhrasesLogo name={pluginName} />
          </h1>
        </div>
      </div>

      <button
        type="button"
        aria-label={`Scale mode, ${activeScaleName}. Click to edit.`}
        aria-pressed={scaleDialogOpen}
        title={activeScaleName}
        data-cursor="pointer"
        class="flex flex-col items-start gap-1 border-0 bg-transparent p-0 text-left outline-none transition-opacity hover:opacity-90 focus-visible:ring-1 focus-visible:ring-focus-ring"
        onclick={() => {
          scaleDialogOpen = true;
        }}
      >
        <div class="h-5" aria-hidden="true"></div>
        <div class="-translate-y-0.5 flex h-8 flex-col items-start justify-end gap-1">
          <p class="text-base font-semibold leading-none text-text">{activeKeyCenterLabel}</p>
          <p class="text-sm font-semibold uppercase leading-none text-accent">{activeScaleModeLabel}</p>
        </div>
      </button>
    </div>

    <div class="relative flex min-w-0 flex-1 -translate-y-3 flex-nowrap items-end justify-start gap-x-3">
        <div class="flex flex-col items-start gap-1">
          <span class="text-xs font-semibold leading-none text-text-muted">Pulse</span>
          <PulseNoteButtonGroup
            accent={interfaceAccent}
            value={pulseIndex}
            onValueChange={applyPulseIndex}
          />
        </div>
        <div class="flex items-end">
          <div class="flex items-end gap-2">
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-text-muted">Swing</span>
              <StepNumberDragInput
                boxed
                compact
                accent={interfaceAccent}
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
              <span class="text-xs font-semibold leading-none text-text-muted">Sub</span>
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
          </div>
          <div class="w-2 shrink-0" aria-hidden="true"></div>
          <div class="flex items-end gap-2">
            <div class="flex flex-col items-start gap-1">
              <span class="text-xs font-semibold leading-none text-text-muted">Vel %</span>
            <StepNumberDragInput
              boxed
              compact
              accent={interfaceAccent}
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
            <span class="text-xs font-semibold leading-none text-text-muted">Time %</span>
            <StepNumberDragInput
              boxed
              compact
              accent={interfaceAccent}
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
        <BulkStepEditControls
          accent={interfaceAccent}
          requireSelection={false}
          totalStepCount={selectableStepCount}
          {selectedStepCount}
          reverseAvailable={bulkReverseAvailable}
          skipActive={bulkSkipActive}
          muteActive={bulkMuteActive}
          durationPercent={bulkDurationPercent}
          velocityPercent={bulkVelocityPercent}
          transposeSemitones={bulkTransposeSemitones}
          pitchAriaLabel={bulkPitchAriaLabel}
          onReverse={reverseSelectedStepsByRow}
          onShuffle={shuffleSelectedSteps}
          onRandomizeOctaves={randomizeSelectedStepOctaves}
          onRandomizeLengths={randomizeSelectedStepLengths}
          onToggleSkip={toggleBulkStepSkip}
          onToggleMute={toggleBulkStepMute}
          onGestureStart={beginBulkEditGesture}
          onDurationPreview={previewBulkDurationPercent}
          onDurationCommit={commitBulkDurationPercent}
          onVelocityPreview={previewBulkVelocityPercent}
          onVelocityCommit={commitBulkVelocityPercent}
          onTransposePreview={previewBulkTransposeSemitones}
          onTransposeCommit={commitBulkTransposeSemitones}
        />
        <div class="flex shrink-0 items-end gap-1">
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
        <div class="ml-auto flex shrink-0 items-end gap-1.5 pl-3">
          <StretchToFitToggle
            enabled={stretchStepsToFit}
            onChange={(enabled) => {
              stretchStepsToFit = enabled;
            }}
          />
          <ColorsToggle
            accent={interfaceAccent}
            enabled={rowColorsEnabled}
            onChange={async (next) => {
              rowColorsEnabled = next;
              await pushRowColorsEnabled();
            }}
          />
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
    </div>

  </header>
    </div>
  </div>

  <section class="flex min-h-0 flex-1 flex-col">
    <div class="w-full shrink-0">
      <div data-phrase-grid-field class="relative flex flex-col">
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
            class="relative z-10 flex min-w-0 shrink-0 items-center gap-1 transition-opacity duration-150 {rowPianoRollFocusMuted
              ? 'opacity-50'
              : ''}"
          >
            <div
              data-row-header={row}
              class="relative -ml-6 flex shrink-0 self-stretch items-center border-r border-border-subtle pl-6 pr-6 {row <
              grid.length - 1
                ? 'border-b'
                : ''}"
            >
              <button
                type="button"
                aria-label={activeRowPianoRollEditor?.row === row
                  ? `Close row ${row + 1} piano roll editor`
                  : `Edit row ${row + 1} in piano roll`}
                aria-pressed={activeRowPianoRollEditor?.row === row}
                disabled={stepIds[row].length === 0}
                data-cursor={stepIds[row].length > 0 ? "pointer" : undefined}
                class="absolute inset-0 z-0 border-0 bg-surface/25 p-0 outline-none transition-colors hover:bg-surface/55 focus-visible:ring-1 focus-visible:ring-inset focus-visible:ring-focus-ring disabled:cursor-default disabled:opacity-60"
                onclick={() => openRowPianoRollFromHeader(row)}
                title={activeRowPianoRollEditor?.row === row
                  ? "Show the full piano roll"
                  : stepIds[row].length > 0
                    ? `Edit row ${row + 1} in the monophonic piano roll`
                    : `Add a step to row ${row + 1} to edit it in the piano roll`}
              ></button>
              <div class="pointer-events-none relative z-10 flex items-center gap-1 px-1">
                {#if rowMuted[row]}
                  <div class={rowMutedOverlayClasses} aria-hidden="true"></div>
                {/if}
                <button
                  type="button"
                  aria-label={rowMuted[row] ? "Turn row on" : "Turn row off"}
                  aria-pressed={!rowMuted[row]}
                  data-cursor="pointer"
                  class="pointer-events-auto {rowMuteControlClasses} {rowMuted[row]
                    ? rowPowerToggleOffClasses
                    : rowAccent.textAccent}"
                  onclick={(event) => toggleRowMute(row, event.shiftKey)}
                  title="Shift-click to solo row"
                >
                  <RowDisableIcon class="h-6 w-6" />
                </button>
                <button
                  type="button"
                  aria-label={recordingRow === row ? "Stop row recording" : "Record row from MIDI"}
                  aria-pressed={recordingRow === row}
                  data-cursor="pointer"
                  class="pointer-events-auto {rowActionIconControlClasses} transition-colors {rowMuted[row]
                    ? 'text-text-faint hover:text-danger'
                    : recordingRow === row
                      ? 'text-danger'
                      : 'text-text-faint hover:text-danger'}"
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
                <div class="pointer-events-auto">
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
              stepVelocity={stepVelocity[row]}
              stepMuted={stepMuted[row]}
              stepSkipped={stepSkipped[row]}
              stepProbability={stepProbability[row]}
              stepCycle={stepCycle[row]}
              stepCycleOffset={stepCycleOffset[row]}
              activeGates={activeGates[row]}
              selectedStepIds={selectedStepIdsByRow[row]}
              stepInspectionActive={activeStepInspector !== null || rowPianoRollCurrentStepFocusVisible}
              stepInspectorOpen={activeStepInspector !== null}
              stretchToFit={stretchStepsToFit}
              fitGridColumns={compactGridLayout.totalColumns}
              fitGridStartColumn={compactGridLayout.rowStartColumns[row]}
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
              onRemoveStep={removeStep}
              onInsertStep={insertStep}
              onDuplicateStep={duplicateStep}
              onNotePreview={previewPhraseNoteValue}
              onNoteCommit={commitPhraseNoteValue}
              onStepBulkGestureStart={beginPhraseStepBulkGesture}
              onMultiplierChange={selectStepTimingMultiplier}
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
              class="h-2 shrink-0"
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

    <div class="-mx-6 shrink-0">
    <CombinationModeRail
      mask={combinationModeMask}
      onToggle={toggleCombinationMode}
      noteBandpassLowMidi={noteBandpassLowMidi}
      noteBandpassHighMidi={noteBandpassHighMidi}
      onNoteBandpassChange={handleNoteBandpassChange}
      onNoteBandpassCommit={commitNoteBandpass}
      velocityTiltPivotMidi={velocityTiltPivotMidi}
      velocityTiltAmount={velocityTiltAmount}
      onVelocityTiltPivotPreview={previewVelocityTiltPivot}
      onVelocityTiltPivotCommit={commitVelocityTiltPivot}
      onVelocityTiltAmountPreview={previewVelocityTiltAmount}
      onVelocityTiltAmountCommit={commitVelocityTiltAmount}
      onVelocityTiltXYCommit={commitVelocityTiltXY}
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

    <div class="flex min-h-0 min-w-0 flex-1 flex-col overflow-hidden">
    {#if activeStepInspector !== null}
      <StepInspector
        row={activeStepInspector.row}
        step={activeStepInspector.step}
        note={activeStepInspector.note}
        velocity={activeStepInspector.velocity}
        durationFraction={activeStepInspector.durationFraction}
        timingMultiplierIndex={activeStepInspector.timingMultiplierIndex}
        probability={activeStepInspector.probability}
        cycle={activeStepInspector.cycle}
        cycleMask={activeStepInspector.cycleMask}
        {scaleRoot}
        {scaleModeIndex}
        accent={rowAccentFor(activeStepInspector.row, rowColorsEnabled)}
        onNoteChange={(midi) =>
          commitPhraseNoteValue(activeStepInspector.row, activeStepInspector.step, midi)}
        onVelocityChange={(value) =>
          setStepVelocity(activeStepInspector.row, activeStepInspector.step, value)}
        onDurationChange={(percent) =>
          selectStepDurationFraction(
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
        onProbabilityChange={(value) =>
          setStepProbability(activeStepInspector.row, activeStepInspector.step, value)}
        onCyclePatternCommit={(nextCycle, nextMask) =>
          setStepCyclePattern(activeStepInspector.row, activeStepInspector.step, nextCycle, nextMask)}
        onClose={closeStepInspector}
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
        inspectedStepId={rowPianoRollCurrentStepFocusVisible
          ? activeRowPianoRollEditor.stepId
          : null}
        selectedStepIds={selectedStepIdsByRow[activeRowPianoRollEditor.row]}
        accent={rowAccentFor(activeRowPianoRollEditor.row, rowColorsEnabled)}
        onInspectStep={(row, step, stepId) =>
          openRowPianoRollEditor(row, step, stepId, {
            selectStep: stretchStepsToFit,
          })}
        onNotePreview={previewPhraseNoteValue}
        onNoteCommit={commitPhraseNoteValue}
        onShapeNotesCommit={commitPhraseRowNoteShape}
        onShapeVelocitiesCommit={commitPhraseRowVelocityShape}
        onStepMove={movePhraseStepFromPianoRoll}
        onStepResize={selectStepTimingMultiplier}
        onDurationPreview={previewPhraseStepDuration}
        onDurationCommit={commitPhraseStepDuration}
        onStepBulkGestureStart={beginPhraseStepBulkGesture}
        onOpenAdvancedInspector={openStepInspector}
        onInsertStep={insertStep}
        onBulkSelectPointerDown={beginStepMarqueeSelection}
        bulkDurationPercent={bulkDurationPercent}
        bulkVelocityPercent={bulkVelocityPercent}
        bulkTransposeSemitones={bulkTransposeSemitones}
        bulkPitchAriaLabel={bulkPitchAriaLabel}
        bulkReverseAvailable={rowPianoRollBulkReverseAvailable}
        bulkSkipActive={bulkSkipActive}
        bulkMuteActive={bulkMuteActive}
        onBulkReverse={reverseSelectedStepsByRow}
        onBulkShuffle={shuffleSelectedSteps}
        onBulkRandomizeOctaves={randomizeSelectedStepOctaves}
        onBulkRandomizeLengths={randomizeSelectedStepLengths}
        onBulkToggleSkip={toggleBulkStepSkip}
        onBulkToggleMute={toggleBulkStepMute}
        onBulkGestureStart={beginBulkEditGesture}
        onBulkDurationPreview={previewBulkDurationPercent}
        onBulkDurationCommit={commitBulkDurationPercent}
        onBulkVelocityPreview={previewBulkVelocityPercent}
        onBulkVelocityCommit={commitBulkVelocityPercent}
        onBulkTransposePreview={previewBulkTransposeSemitones}
        onBulkTransposeCommit={commitBulkTransposeSemitones}
        onClose={closeRowPianoRollEditor}
      />
    {:else if recordingRow !== null}
      <RecordPianoKeyboard
        row={recordingRow}
        {scaleRoot}
        {scaleModeIndex}
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
        {combinationModeMask}
        {scaleRoot}
        {scaleModeIndex}
        noteBandpassLowMidi={noteBandpassLowMidi}
        noteBandpassHighMidi={noteBandpassHighMidi}
        velocityTiltPivotMidi={velocityTiltPivotMidi}
        velocityTiltAmount={velocityTiltAmount}
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
      <div class="h-[calc(2.5rem*0.33)] shrink-0 bg-app" role="presentation" aria-hidden="true"></div>
      <div class="mp-honeycomb-rail relative z-20 flex h-20 items-center justify-center overflow-x-auto px-6">
      <div class="flex min-w-max items-center gap-8">
        <div class="flex items-center gap-2">
          <span class="text-sm font-semibold leading-none text-text-muted">Patterns:</span>
          <div class="flex items-center gap-1">
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
              <RemoveXIcon class="pointer-events-none h-4 w-4" />
            </button>
          </div>
        </div>
        <div class="flex items-center gap-2">
          <span class="text-sm font-semibold leading-none text-text-muted">Loops:</span>
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
