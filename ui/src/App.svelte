<script>
  import { onMount } from "svelte";
  import { getNativeFunction } from "@juce/index.js";
  import {
    defaultPhraseGrid,
    defaultStepDurationGrid,
    defaultStepVelocityGrid,
    midiToNoteName,
    noteNameToMidi,
  } from "./midiNoteNames.js";
  import SpeakerIcon from "./SpeakerIcon.svelte";

  let pluginName = "MIDI Phrases";
  let version = "0.0.1";
  let grid = defaultPhraseGrid();
  /** @type {string[][]} */
  let displayNames = grid.map((row) => row.map((note) => midiToNoteName(note)));
  /** @type {boolean[]} */
  let rowMuted = [false, false, false, false];
  /** @type {number[]} */
  let rowTimingOffset = [3, 3, 3, 3];
  /** @type {number[][]} */
  let stepDurationFraction = defaultStepDurationGrid();
  /** @type {number[][]} */
  let stepVelocity = defaultStepVelocityGrid();

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

  const toggleButtonClass = (active) =>
    active
      ? "border-emerald-500 bg-emerald-950 text-emerald-300"
      : "border-zinc-700 bg-zinc-900 text-zinc-400 hover:border-zinc-600 hover:text-zinc-200";

  const stepCellToggleClass = (active) =>
    active
      ? "bg-emerald-950 text-emerald-300"
      : "bg-transparent text-zinc-400 hover:bg-zinc-800/80 hover:text-zinc-200";

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

    const nextGrid = defaultPhraseGrid();
    const nextNames = defaultPhraseGrid().map((row) =>
      row.map((note) => midiToNoteName(note)),
    );

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];

      if (!Array.isArray(rowData)) continue;

      for (let step = 0; step < 4; step += 1) {
        const midi = Number.parseInt(String(rowData[step]), 10);

        if (Number.isNaN(midi)) continue;

        const clamped = Math.min(127, Math.max(0, midi));
        nextGrid[row][step] = clamped;
        nextNames[row][step] = midiToNoteName(clamped);
      }
    }

    grid = nextGrid;
    displayNames = nextNames;
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

    const next = defaultStepDurationGrid();

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];

      if (!Array.isArray(rowData)) continue;

      for (let step = 0; step < 4; step += 1) {
        const value = Number.parseInt(String(rowData[step]), 10);

        if (Number.isNaN(value)) continue;

        next[row][step] = Math.min(3, Math.max(0, value));
      }
    }

    stepDurationFraction = next;
  }

  function loadStepVelocityFromInitialisation() {
    const init = unwrapJuceInit("phraseStepVelocity");

    if (!Array.isArray(init)) return;

    const next = defaultStepVelocityGrid();

    for (let row = 0; row < 4; row += 1) {
      const rowData = init[row];

      if (!Array.isArray(rowData)) continue;

      for (let step = 0; step < 4; step += 1) {
        const value = Number.parseInt(String(rowData[step]), 10);

        if (Number.isNaN(value)) continue;

        next[row][step] = Math.min(127, Math.max(0, value));
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

  function onNoteInput(row, step, event) {
    displayNames[row][step] = event.currentTarget.value;
    displayNames = displayNames;
  }

  async function commitNote(row, step) {
    const parsed = noteNameToMidi(displayNames[row][step]);

    if (parsed === null) {
      displayNames[row][step] = midiToNoteName(grid[row][step]);
      displayNames = displayNames;
      return;
    }

    grid[row][step] = parsed;
    displayNames[row][step] = midiToNoteName(parsed);
    grid = grid;
    displayNames = displayNames;
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

  async function selectStepDurationFraction(row, step, fractionIndex) {
    stepDurationFraction[row][step] = fractionIndex;
    stepDurationFraction = stepDurationFraction;
    await pushStepDurationFraction(row, step);
  }

  async function onStepVelocityInput(row, step, event) {
    const value = Number(event.currentTarget.value);

    if (Number.isNaN(value)) return;

    stepVelocity[row][step] = Math.min(127, Math.max(0, value));
    stepVelocity = stepVelocity;
    await pushStepVelocity(row, step);
  }

  onMount(() => {
    loadGridFromInitialisation();
    loadRowMutedFromInitialisation();
    loadRowTimingOffsetFromInitialisation();
    loadStepDurationFromInitialisation();
    loadStepVelocityFromInitialisation();
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

  <section class="mt-16 flex flex-1 flex-col items-center justify-center">
    <div class="w-full max-w-6xl">
      <div class="flex flex-col gap-10">
        {#each grid as _row, row}
          <div class="grid grid-cols-[auto_2.5rem_repeat(4,minmax(0,1fr))] items-center gap-2">
            <div class="flex items-center gap-0.5" role="radiogroup" aria-label="Row timing offset">
              {#each timingOffsetOptions as option}
                <button
                  type="button"
                  role="radio"
                  aria-checked={rowTimingOffset[row] === option.index}
                  aria-label="Timing offset {option.label}"
                  class="min-w-[2.1rem] rounded border px-1 py-1 font-mono text-[10px] leading-none transition-colors outline-none {toggleButtonClass(
                    rowTimingOffset[row] === option.index,
                  )}"
                  onclick={() => selectRowTimingOffset(row, option.index)}
                >
                  {option.label}
                </button>
              {/each}
            </div>
            <div class="flex items-center justify-center">
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
            {#each grid[row] as _note, step}
              <div
                class="flex min-w-0 flex-col overflow-hidden rounded-lg border border-zinc-700 bg-zinc-900 outline-none focus-within:border-emerald-500 focus-within:ring-1 focus-within:ring-emerald-500"
              >
                <div class="flex min-w-0 items-stretch">
                  <label class="shrink-0 border-r border-zinc-800">
                    <input
                      type="text"
                      spellcheck="false"
                      autocomplete="off"
                      class="w-12 border-0 bg-transparent px-2 py-2 text-left font-mono text-sm text-zinc-100 outline-none"
                      value={displayNames[row][step]}
                      oninput={(e) => onNoteInput(row, step, e)}
                      onchange={() => commitNote(row, step)}
                      onblur={() => commitNote(row, step)}
                    />
                  </label>
                  <div
                    class="flex min-w-0 flex-1 items-stretch"
                    role="radiogroup"
                    aria-label="Step duration fraction"
                  >
                    {#each durationFractionOptions as option, optionIndex}
                      <button
                        type="button"
                        role="radio"
                        aria-checked={stepDurationFraction[row][step] === option.index}
                        aria-label="Duration {option.label}"
                        class="min-w-[2rem] flex-1 border-0 px-1 py-2 font-mono text-[10px] leading-none transition-colors outline-none {optionIndex >
                        0
                          ? 'border-l border-zinc-800'
                          : ''} {stepCellToggleClass(
                          stepDurationFraction[row][step] === option.index,
                        )}"
                        onclick={() => selectStepDurationFraction(row, step, option.index)}
                      >
                        {option.label}
                      </button>
                    {/each}
                  </div>
                </div>
                <div class="flex items-center gap-2 border-t border-zinc-800 px-2 py-1">
                  <input
                    type="range"
                    min="0"
                    max="127"
                    aria-label="Step velocity"
                    class="min-w-0 flex-1 accent-emerald-500"
                    value={stepVelocity[row][step]}
                    oninput={(e) => onStepVelocityInput(row, step, e)}
                    onchange={(e) => onStepVelocityInput(row, step, e)}
                  />
                  <span class="w-8 shrink-0 text-right font-mono text-xs tabular-nums text-zinc-400">
                    {stepVelocity[row][step]}
                  </span>
                </div>
              </div>
            {/each}
          </div>
        {/each}
      </div>
    </div>
  </section>
</main>
