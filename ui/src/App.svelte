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
  } from "./midiNoteNames.js";
  import RowDisableIcon from "./RowDisableIcon.svelte";
  import RowReverseOrderIcon from "./RowReverseOrderIcon.svelte";
  import BipolarKnob from "./BipolarKnob.svelte";
  import MidiChannelStepper from "./MidiChannelStepper.svelte";
  import PhraseRow from "./PhraseRow.svelte";
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import PianoRollPreview from "./PianoRollPreview.svelte";
  import {
    defaultStepTimingMultiplierIndex,
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
  import { defaultPulseIndex, pulseOptions } from "./pulseLayout.js";
  import {
    phraseBeatGuideGlobalLeftPx,
    phraseGridVisualOffsetCompensationPx,
    phraseRowLeadingControlsAfterMuteWidthPx,
  } from "./phraseRowLayout.js";
  import {
    emeraldRowAccent,
    rowAccentFor,
    rowMutedOverlayClasses,
    rowMuteControlClasses,
    rowPowerToggleOffClasses,
    rowReverseControlClasses,
    toggleIconRestClasses,
  } from "./rowAccentTheme.js";

  let pluginName = "MIDI Phrases";
  let version = "0.0.1";
  let grid = defaultPhraseGrid();
  /** @type {boolean[]} */
  let rowMuted = [false, false, false, false];
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
  let pulseIndex = defaultPulseIndex;
  let swingPercent = 0;
  let velocityHumanizePercent = 0;
  let timingHumanizePercent = 0;
  let swingSubdivisionIndex = 1;
  let rowColorsEnabled = false;

  /** UI-only; shifts phrase rows and beat-one guide when any row has a negative offset. */
  $: phraseVisualOffsetCompensationPx = phraseGridVisualOffsetCompensationPx(
    rowTimingOffset,
    pulseIndex,
  );

  function createStepId() {
    const id = `step-${nextStepId}`;
    nextStepId += 1;
    return id;
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

    await pushMovePhraseStep(row, move.from, move.to);
  }

  async function pushNote(row, step) {
    if (!nativeFunctionAvailable("setPhraseNote")) return;

    const setPhraseNote = getNativeFunction("setPhraseNote");
    await setPhraseNote(row, step, grid[row][step]);
  }

  async function pushRowMuted(row) {
    if (!nativeFunctionAvailable("setPhraseRowMuted")) return;

    const setPhraseRowMuted = getNativeFunction("setPhraseRowMuted");
    await setPhraseRowMuted(row, rowMuted[row] ? 1 : 0);
  }

  async function pushReversePhraseRowSteps(row) {
    if (!nativeFunctionAvailable("reversePhraseRowSteps")) return;

    const reversePhraseRowSteps = getNativeFunction("reversePhraseRowSteps");
    await reversePhraseRowSteps(row);
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
    grid[row][step] = Math.min(127, Math.max(0, midi));
    grid = grid;
    await pushNote(row, step);
  }

  async function toggleRowMute(row) {
    rowMuted[row] = !rowMuted[row];
    rowMuted = rowMuted;
    await pushRowMuted(row);
  }

  async function reverseRowStepOrder(row) {
    if (!grid[row] || grid[row].length <= 1) return;

    grid[row].reverse();
    stepDurationFraction[row].reverse();
    stepTimingMultiplier[row].reverse();
    stepVelocity[row].reverse();
    stepMuted[row].reverse();
    stepSkipped[row].reverse();
    stepProbability[row].reverse();
    stepCycle[row].reverse();
    stepCycleOffset[row].reverse();
    activeGates[row].reverse();
    stepIds[row].reverse();

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

    await pushReversePhraseRowSteps(row);
  }

  async function selectRowTimingOffset(row, offsetIndex) {
    rowTimingOffset[row] = offsetIndex;
    rowTimingOffset = rowTimingOffset;
    await pushRowTimingOffset(row);
  }

  async function selectRowMidiChannel(row, channel) {
    rowMidiChannel[row] = Math.min(16, Math.max(1, channel));
    rowMidiChannel = rowMidiChannel;
    await pushRowMidiChannel(row);
  }

  async function selectStepTimingMultiplier(row, step, multiplierIndex) {
    stepTimingMultiplier[row][step] = multiplierIndex;
    stepTimingMultiplier = stepTimingMultiplier;
    await pushStepTimingMultiplier(row, step);
  }

  async function selectStepDurationFraction(row, step, fraction) {
    stepDurationFraction[row][step] = Math.min(1, Math.max(0, fraction));
    stepDurationFraction = stepDurationFraction;
    await pushStepDurationFraction(row, step);
  }

  async function setStepVelocity(row, step, value) {
    stepVelocity[row][step] = Math.min(127, Math.max(0, value));
    stepVelocity = stepVelocity;
    await pushStepVelocity(row, step);
  }

  async function setStepMuted(row, step, muted) {
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
  }

  async function setStepSkipped(row, step, skipped) {
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
  }

  async function setStepProbability(row, step, probability) {
    stepProbability[row][step] = Math.min(100, Math.max(0, probability));
    stepProbability = stepProbability;
    await pushStepProbability(row, step);
  }

  async function setStepCycle(row, step, cycle) {
    const nextCycle = Math.min(64, Math.max(1, cycle));
    stepCycle[row][step] = nextCycle;
    stepCycleOffset[row][step] = Math.min(stepCycleOffset[row][step], nextCycle - 1);
    stepCycle = stepCycle;
    stepCycleOffset = stepCycleOffset;
    await pushStepCycle(row, step);
    await pushStepCycleOffset(row, step);
  }

  async function setStepCycleOffset(row, step, cycleOffset) {
    const maxOffset = Math.max(0, (stepCycle[row][step] ?? 1) - 1);
    stepCycleOffset[row][step] = Math.min(maxOffset, Math.max(0, cycleOffset));
    stepCycleOffset = stepCycleOffset;
    await pushStepCycleOffset(row, step);
  }

  async function removeStep(row, step) {
    if (step < 0 || step >= grid[row].length) return;

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
  }

  async function insertStep(row, step) {
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
  }

  async function duplicateStep(row, step) {
    if (step <= 0) return;

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

    playbackPollFrameId = requestAnimationFrame(pollPlaybackActivity);
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
    pulseIndex = Math.min(pulseOptions.length - 1, Math.max(0, nextIndex));

    if (!nativeFunctionAvailable("setPulseIndex")) return;

    const result = await getNativeFunction("setPulseIndex")(pulseIndex);
    const confirmed = Number.parseInt(String(result), 10);

    if (!Number.isNaN(confirmed)) {
      pulseIndex = Math.min(pulseOptions.length - 1, Math.max(0, confirmed));
    }
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
    const clamped = clampPercent(next);
    assign(clamped);

    if (!nativeFunctionAvailable(nativeName)) return;

    const result = await getNativeFunction(nativeName)(clamped);
    const confirmed = clampPercent(result);
    assign(confirmed);
  }

  async function applySwingSubdivisionIndex(nextIndex) {
    swingSubdivisionIndex = Math.min(swingSubdivisionValues.length - 1, Math.max(0, nextIndex));

    if (!nativeFunctionAvailable("setSwingSubdivisionIndex")) return;

    const result = await getNativeFunction("setSwingSubdivisionIndex")(swingSubdivisionIndex);
    const confirmed = Number.parseInt(String(result), 10);

    if (!Number.isNaN(confirmed)) {
      swingSubdivisionIndex = Math.min(swingSubdivisionValues.length - 1, Math.max(0, confirmed));
    }
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
      const value = Number.parseInt(String(raw), 10);

      if (!Number.isNaN(value)) loopBraceStart = Math.max(0, value);
    }

    const endInit = unwrapJuceInit("loopBraceEnd");

    if (endInit !== null) {
      const raw = Array.isArray(endInit) ? endInit[0] : endInit;
      const value = Number.parseInt(String(raw), 10);

      if (!Number.isNaN(value)) loopBraceEnd = Math.max(loopBraceStart + 1, value);
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
    if (next.enabled !== undefined) {
      loopBraceEnabled = next.enabled;
      await pushLoopBraceEnabled(next.enabled);
    }

    if (next.start !== undefined) {
      loopBraceStart = next.start;
      await pushLoopBraceStart(next.start);
    }

    if (next.end !== undefined) {
      loopBraceEnd = next.end;
      await pushLoopBraceEnd(next.end);
    }
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
  }

  loadInitialStateFromJuce();

  onMount(() => {
    playbackPollFrameId = requestAnimationFrame(pollPlaybackActivity);

    return () => {
      cancelAnimationFrame(playbackPollFrameId);
    };
  });
</script>

<main class="flex h-full flex-col overflow-hidden p-6">
  <div class="shrink-0 -mx-6">
  <header class="flex items-end justify-between gap-4 px-6 pb-6">
    <div class="flex min-w-0 flex-1 items-end gap-1">
      <div class="flex w-10 shrink-0 flex-col items-start self-start">
        <p class="text-xs font-medium uppercase tracking-widest text-emerald-400">ofsound</p>
        <h1 class="whitespace-nowrap text-xl font-semibold tracking-tight text-zinc-100">
          {pluginName}
        </h1>
      </div>

      <!-- Mute column + gap + rest of row leading controls | PhraseRow (pl-2 + insert + steps) -->
      <div class="flex min-w-0 flex-1 items-end gap-1">
        <div
          class="shrink-0"
          style:width="{phraseRowLeadingControlsAfterMuteWidthPx}px"
          aria-hidden="true"
        ></div>
        <div class="flex min-w-0 items-end gap-6 pl-2">
        <div class="w-4 shrink-0 self-stretch" aria-hidden="true"></div>
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
        </div>
      </div>
    </div>

    <div class="flex shrink-0 items-center gap-3">
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
      <p class="pt-0.5 text-sm text-zinc-500">v{version}</p>
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
                onclick={() => toggleRowMute(row)}
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
                aria-label="Reverse row step order"
                class="{rowReverseControlClasses} {rowAccent.controlFocus} {rowMuted[row]
                  ? 'border-zinc-800/90 text-zinc-600'
                  : `border-zinc-700 ${toggleIconRestClasses}`}"
                onclick={() => reverseRowStepOrder(row)}
                title="Reverse row step order"
              >
                <RowReverseOrderIcon class="pointer-events-none h-5 w-5" />
              </button>
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
  </section>
</main>
