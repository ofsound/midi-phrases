<script>
  import { onMount } from "svelte";
  import { getNativeFunction } from "@juce/index.js";
  import {
    defaultPhraseGrid,
    defaultStepDurationGrid,
    defaultStepTimingMultiplierGrid,
    defaultStepVelocityGrid,
  } from "./midiNoteNames.js";
  import SpeakerIcon from "./SpeakerIcon.svelte";
  import BipolarKnob from "./BipolarKnob.svelte";
  import MidiChannelStepper from "./MidiChannelStepper.svelte";
  import PhraseRow from "./PhraseRow.svelte";
  import PianoRollPreview from "./PianoRollPreview.svelte";
  import {
    defaultStepTimingMultiplierIndex,
    findSingleMove,
    stepTimingMultiplierCount,
    timingMultiplierOptions,
  } from "./stepCellLayout.js";
  import { sanitizeOrderedIds } from "./dndUtils.js";
  import { isStepActiveAtBeat } from "./phraseSchedule.js";
  import { defaultPulseIndex, pulseOptions } from "./pulseLayout.js";

  let pluginName = "MIDI Phrases";
  let version = "0.0.1";
  let grid = defaultPhraseGrid();
  /** @type {boolean[]} */
  let rowMuted = [false, false, false, false];
  /** @type {number[]} */
  let rowTimingOffset = [3, 3, 3, 3];
  /** @type {number[]} */
  let rowMidiChannel = [1, 1, 1, 1];
  /** @type {number[][]} */
  let stepDurationFraction = defaultStepDurationGrid();
  /** @type {number[][]} */
  let stepTimingMultiplier = defaultStepTimingMultiplierGrid();
  /** @type {number[][]} */
  let stepVelocity = defaultStepVelocityGrid();
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
    activeGates[row] = reorder(activeGates[row]);
    stepIds[row] = validIds;

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
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
            pulseIndex,
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

  async function removeStep(row, step) {
    if (step < 0 || step >= grid[row].length) return;

    grid[row].splice(step, 1);
    stepDurationFraction[row].splice(step, 1);
    stepTimingMultiplier[row].splice(step, 1);
    stepVelocity[row].splice(step, 1);
    activeGates[row].splice(step, 1);
    stepIds[row].splice(step, 1);

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
    activeGates = activeGates;
    stepIds = stepIds;

    if (!nativeFunctionAvailable("removePhraseStep")) return;

    const removePhraseStep = getNativeFunction("removePhraseStep");
    await removePhraseStep(row, step);
  }

  async function insertStep(row, step) {
    const defaults = defaultPhraseGrid();
    const defaultDurations = defaultStepDurationGrid();
    const defaultMultipliers = defaultStepTimingMultiplierGrid();
    const defaultVelocities = defaultStepVelocityGrid();
    const defaultNote = defaults[row]?.[0] ?? 60;

    grid[row].splice(step, 0, defaultNote);
    stepDurationFraction[row].splice(step, 0, defaultDurations[row]?.[0] ?? 1);
    stepTimingMultiplier[row].splice(
      step,
      0,
      defaultMultipliers[row]?.[0] ?? defaultStepTimingMultiplierIndex,
    );
    stepVelocity[row].splice(step, 0, defaultVelocities[row]?.[0] ?? 100);
    activeGates[row].splice(step, 0, false);
    stepIds[row].splice(step, 0, createStepId());

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
    activeGates = activeGates;
    stepIds = stepIds;

    if (!nativeFunctionAvailable("insertPhraseStep")) return;

    const insertPhraseStep = getNativeFunction("insertPhraseStep");
    await insertPhraseStep(row, step);
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

  async function setPulseFromSelect(event) {
    const nextIndex = Number.parseInt(event.currentTarget.value, 10);

    if (Number.isNaN(nextIndex)) return;

    pulseIndex = Math.min(pulseOptions.length - 1, Math.max(0, nextIndex));

    if (!nativeFunctionAvailable("setPulseIndex")) return;

    const result = await getNativeFunction("setPulseIndex")(pulseIndex);
    const confirmed = Number.parseInt(String(result), 10);

    if (!Number.isNaN(confirmed)) {
      pulseIndex = Math.min(pulseOptions.length - 1, Math.max(0, confirmed));
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
    loadPulseFromInitialisation();
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

<main class="flex min-h-screen flex-col p-6">
  <header class="flex items-start justify-between gap-4">
    <div class="flex min-w-0 flex-1 items-start gap-6">
      <div>
        <p class="text-xs font-medium uppercase tracking-widest text-emerald-400">ofsound</p>
        <h1 class="mt-1 text-xl font-semibold tracking-tight text-zinc-100">{pluginName}</h1>
      </div>
      <label class="flex items-center gap-2 pt-1 text-xs font-medium uppercase text-zinc-500">
        Pulse
        <select
          class="h-8 min-w-[6.5rem] rounded-md border border-zinc-700 bg-zinc-950 px-2 text-sm font-semibold normal-case text-zinc-100 outline-none focus:border-emerald-500 focus:ring-1 focus:ring-emerald-500"
          value={pulseIndex}
          onchange={setPulseFromSelect}
        >
          {#each pulseOptions as option (option.index)}
            <option value={option.index}>{option.label}</option>
          {/each}
        </select>
      </label>
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

  <section class="mt-4 flex flex-1 flex-col items-start justify-start">
    <div class="w-full">
      <div class="flex flex-col gap-5">
        {#each grid as _row, row}
          <div class="flex items-center gap-2">
            <MidiChannelStepper
              value={rowMidiChannel[row]}
              ariaLabel="Row {row + 1} MIDI channel"
              onValueChange={(channel) => selectRowMidiChannel(row, channel)}
            />
            <BipolarKnob
              label="Offset"
              options={timingOffsetOptions}
              value={rowTimingOffset[row]}
              ariaLabel="Row timing offset"
              onValueChange={(offsetIndex) => selectRowTimingOffset(row, offsetIndex)}
            />
            <div class="flex shrink-0 items-center justify-center">
              <button
                type="button"
                aria-label={rowMuted[row] ? "Unmute voice" : "Mute voice"}
                aria-pressed={!rowMuted[row]}
                class="flex h-9 w-9 items-center justify-center rounded-lg border border-zinc-700 bg-zinc-900 transition-colors outline-none hover:border-zinc-600 focus:border-emerald-500 focus:ring-1 focus:ring-emerald-500 {rowMuted[row]
                  ? 'text-zinc-500'
                  : 'text-emerald-400'}"
                onclick={() => toggleRowMute(row)}
              >
                <SpeakerIcon class="h-4 w-4" />
              </button>
            </div>
            <PhraseRow
              {row}
              timingOffsetIndex={rowTimingOffset[row]}
              {pulseIndex}
              stepIds={stepIds[row]}
              notes={grid[row]}
              stepDurationFraction={stepDurationFraction[row]}
              stepTimingMultiplier={stepTimingMultiplier[row]}
              stepVelocity={stepVelocity[row]}
              activeGates={activeGates[row]}
              {timingMultiplierOptions}
              onReorder={reorderRowByIds}
              onMoveCommitted={commitRowMove}
              onRemoveStep={removeStep}
              onInsertStep={insertStep}
              onNoteChange={setPhraseNoteValue}
              onMultiplierChange={selectStepTimingMultiplier}
              onDurationChange={selectStepDurationFraction}
              onVelocityChange={setStepVelocity}
            />
          </div>
        {/each}
      </div>

      <PianoRollPreview
        notes={grid}
        {rowMuted}
        {rowTimingOffset}
        {stepDurationFraction}
        {stepTimingMultiplier}
        {stepVelocity}
        {pulseIndex}
        loopEnabled={loopBraceEnabled}
        loopStart={loopBraceStart}
        loopEnd={loopBraceEnd}
        {playbackBeat}
        onLoopBraceChange={updateLoopBrace}
      />
    </div>
  </section>
</main>
