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
  import DiscreteSlider from "./DiscreteSlider.svelte";
  import ContinuousSlider from "./ContinuousSlider.svelte";
  import NoteDragInput from "./NoteDragInput.svelte";
  import StepInsertZone from "./StepInsertZone.svelte";

  const maxPhraseSteps = 4;

  let pluginName = "MIDI Phrases";
  let version = "0.0.1";
  let grid = defaultPhraseGrid();
  /** @type {boolean[]} */
  let rowMuted = [false, false, false, false];
  /** @type {number[]} */
  let rowTimingOffset = [3, 3, 3, 3];
  /** @type {number[][]} */
  let stepDurationFraction = defaultStepDurationGrid();
  /** @type {number[][]} */
  let stepTimingMultiplier = defaultStepTimingMultiplierGrid();
  /** @type {number[][]} */
  let stepVelocity = defaultStepVelocityGrid();
  /** @type {boolean[][]} */
  let activeGates = defaultPhraseGrid().map((row) => row.map(() => false));

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

  const durationFractionOptions = [
    { index: 0, label: ".25" },
    { index: 1, label: ".5" },
    { index: 2, label: ".75" },
    { index: 3, label: "1" },
  ];

  const timingMultiplierOptions = [
    { index: 0, label: ".25" },
    { index: 1, label: ".5" },
    { index: 2, label: "1" },
    { index: 3, label: "2" },
    { index: 4, label: "4" },
  ];

  const timingMultiplierValues = [0.25, 0.5, 1, 2, 4];

  /** @param {number} index */
  function stepTimingMultiplierValue(index) {
    return timingMultiplierValues[index] ?? 1;
  }

  const stepCellPlaybackClass = (active) =>
    active
      ? "border-emerald-300 ring-2 ring-emerald-400/90"
      : "border-zinc-700";

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
        const value = Number.parseInt(String(rowData?.[step] ?? defaults[row][step]), 10);

        next[row][step] = Number.isNaN(value)
          ? defaults[row][step] ?? defaults[row][0]
          : Math.min(3, Math.max(0, value));
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
          : Math.min(4, Math.max(0, value));
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

  async function selectStepTimingMultiplier(row, step, multiplierIndex) {
    stepTimingMultiplier[row][step] = multiplierIndex;
    stepTimingMultiplier = stepTimingMultiplier;
    await pushStepTimingMultiplier(row, step);
  }

  async function selectStepDurationFraction(row, step, fractionIndex) {
    stepDurationFraction[row][step] = fractionIndex;
    stepDurationFraction = stepDurationFraction;
    await pushStepDurationFraction(row, step);
  }

  async function setStepVelocity(row, step, value) {
    stepVelocity[row][step] = Math.min(127, Math.max(0, value));
    stepVelocity = stepVelocity;
    await pushStepVelocity(row, step);
  }

  async function removeStep(row, step) {
    grid[row].splice(step, 1);
    stepDurationFraction[row].splice(step, 1);
    stepTimingMultiplier[row].splice(step, 1);
    stepVelocity[row].splice(step, 1);
    activeGates[row].splice(step, 1);

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
    activeGates = activeGates;

    if (!nativeFunctionAvailable("removePhraseStep")) return;

    const removePhraseStep = getNativeFunction("removePhraseStep");
    await removePhraseStep(row, step);
  }

  async function insertStep(row, step) {
    if (grid[row].length >= maxPhraseSteps) return;

    const defaults = defaultPhraseGrid();
    const defaultDurations = defaultStepDurationGrid();
    const defaultMultipliers = defaultStepTimingMultiplierGrid();
    const defaultVelocities = defaultStepVelocityGrid();

    grid[row].splice(step, 0, defaults[row][0]);
    stepDurationFraction[row].splice(step, 0, defaultDurations[row][0]);
    stepTimingMultiplier[row].splice(step, 0, defaultMultipliers[row][0]);
    stepVelocity[row].splice(step, 0, defaultVelocities[row][0]);
    activeGates[row].splice(step, 0, false);

    grid = grid;
    stepDurationFraction = stepDurationFraction;
    stepTimingMultiplier = stepTimingMultiplier;
    stepVelocity = stepVelocity;
    activeGates = activeGates;

    if (!nativeFunctionAvailable("insertPhraseStep")) return;

    const insertPhraseStep = getNativeFunction("insertPhraseStep");
    await insertPhraseStep(row, step);
  }

  function applyPlaybackActivity(result) {
    if (!Array.isArray(result)) return;

    let changed = false;

    for (let row = 0; row < 4; row += 1) {
      const rowData = result[row];

      if (!Array.isArray(rowData)) continue;

      for (let step = 0; step < activeGates[row].length; step += 1) {
        const active = Boolean(rowData?.[step]);

        if (activeGates[row][step] !== active) {
          activeGates[row][step] = active;
          changed = true;
        }
      }
    }

    if (changed) activeGates = activeGates;
  }

  async function pollPlaybackActivity() {
    if (nativeFunctionAvailable("getPhraseStepPlaybackActivity")) {
      const getPhraseStepPlaybackActivity = getNativeFunction("getPhraseStepPlaybackActivity");

      try {
        const result = await getPhraseStepPlaybackActivity();
        applyPlaybackActivity(result);
      } catch {
        // WebView bridge may be unavailable during teardown.
      }
    }

    playbackPollFrameId = requestAnimationFrame(pollPlaybackActivity);
  }

  onMount(() => {
    loadGridFromInitialisation();
    loadRowMutedFromInitialisation();
    loadRowTimingOffsetFromInitialisation();
    loadStepDurationFromInitialisation();
    loadStepTimingMultiplierFromInitialisation();
    loadStepVelocityFromInitialisation();
    playbackPollFrameId = requestAnimationFrame(pollPlaybackActivity);

    return () => {
      cancelAnimationFrame(playbackPollFrameId);
    };
  });
</script>

<main class="flex min-h-screen flex-col p-6">
  <header class="flex items-start justify-between gap-4">
    <div>
      <p class="text-xs font-medium uppercase tracking-widest text-emerald-400">ofsound</p>
      <h1 class="mt-1 text-xl font-semibold tracking-tight text-zinc-100">{pluginName}</h1>
    </div>
    <p class="shrink-0 pt-0.5 text-sm text-zinc-500">v{version}</p>
  </header>

  <section class="mt-16 flex flex-1 flex-col items-start justify-center">
    <div class="w-full">
      <div class="flex flex-col gap-10">
        {#each grid as _row, row}
          <div class="flex items-center gap-2">
            <DiscreteSlider
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
            <div class="flex min-w-0 flex-1 pt-2 pl-2">
              {#if grid[row].length < maxPhraseSteps}
                <StepInsertZone onInsert={() => insertStep(row, 0)} />
              {/if}
              {#each grid[row] as _note, step}
                <div
                  style:flex="{stepTimingMultiplierValue(stepTimingMultiplier[row][step])} 1 0"
                  class="relative min-w-0"
                >
                  <button
                    type="button"
                    aria-label="Remove step"
                    class="absolute top-0 left-0 z-10 flex h-4 w-4 -translate-x-1/2 -translate-y-1/2 items-center justify-center rounded-full border border-white/90 bg-zinc-700 text-white shadow-md transition-colors outline-none hover:bg-zinc-600 focus:border-emerald-400 focus:ring-1 focus:ring-emerald-400"
                    onclick={() => removeStep(row, step)}
                  >
                    <svg viewBox="0 0 10 10" class="h-2 w-2" aria-hidden="true">
                      <path
                        d="M2 2 L8 8 M8 2 L2 8"
                        fill="none"
                        stroke="currentColor"
                        stroke-width="1.75"
                        stroke-linecap="square"
                      />
                    </svg>
                  </button>
                  <div
                    class="flex min-w-0 overflow-hidden rounded-lg border bg-zinc-900 outline-none transition-[border-color,box-shadow,flex-grow] duration-200 {stepCellPlaybackClass(
                      activeGates[row][step],
                    )} focus-within:border-emerald-500 focus-within:ring-1 focus-within:ring-emerald-500"
                  >
                    <div class="flex shrink-0 items-stretch border-r border-zinc-800">
                      <NoteDragInput
                        value={grid[row][step]}
                        ariaLabel="Step note"
                        onValueChange={(midi) => setPhraseNoteValue(row, step, midi)}
                      />
                    </div>
                    <div class="flex min-w-0 flex-1 flex-col gap-2 px-2 py-1.5">
                      <DiscreteSlider
                        label="Multiplier"
                        fullWidth
                        options={timingMultiplierOptions}
                        value={stepTimingMultiplier[row][step]}
                        ariaLabel="Step timing multiplier"
                        onValueChange={(multiplierIndex) =>
                          selectStepTimingMultiplier(row, step, multiplierIndex)}
                      />
                      <DiscreteSlider
                        label="Duration"
                        fullWidth
                        options={durationFractionOptions}
                        value={stepDurationFraction[row][step]}
                        ariaLabel="Step duration fraction"
                        onValueChange={(fractionIndex) =>
                          selectStepDurationFraction(row, step, fractionIndex)}
                      />
                      <ContinuousSlider
                        label="Velocity"
                        fullWidth
                        min={0}
                        max={127}
                        value={stepVelocity[row][step]}
                        ariaLabel="Step velocity"
                        onValueChange={(value) => setStepVelocity(row, step, value)}
                      />
                    </div>
                  </div>
                </div>
                {#if grid[row].length < maxPhraseSteps}
                  <StepInsertZone onInsert={() => insertStep(row, step + 1)} />
                {/if}
              {/each}
            </div>
          </div>
        {/each}
      </div>
    </div>
  </section>
</main>
