<script>
  import { onMount } from "svelte";
  import { getNativeFunction } from "@juce/index.js";
  import {
    defaultPhraseGrid,
    midiToNoteName,
    noteNameToMidi,
  } from "./midiNoteNames.js";

  let pluginName = "MIDI Phrases";
  let version = "0.0.1";
  let grid = defaultPhraseGrid();
  /** @type {string[][]} */
  let displayNames = grid.map((row) => row.map((note) => midiToNoteName(note)));

  if (window.__JUCE__?.initialisationData?.pluginName?.[0]) {
    pluginName = window.__JUCE__.initialisationData.pluginName[0];
  }

  if (window.__JUCE__?.initialisationData?.version?.[0]) {
    version = window.__JUCE__.initialisationData.version[0];
  }

  function loadGridFromInitialisation() {
    const init = window.__JUCE__?.initialisationData?.phraseNotes;

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

  async function pushNote(row, step) {
    const setPhraseNote = getNativeFunction("setPhraseNote");

    if (!setPhraseNote) return;

    await setPhraseNote(row, step, grid[row][step]);
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

  onMount(async () => {
    loadGridFromInitialisation();

    for (let row = 0; row < grid.length; row += 1) {
      for (let step = 0; step < grid[row].length; step += 1) {
        await pushNote(row, step);
      }
    }
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

  <section class="mt-8 flex flex-1 flex-col items-center justify-center">
    <div class="w-full max-w-md">
      <div class="grid grid-cols-4 gap-2">
        {#each grid as _row, row}
          {#each grid[row] as _note, step}
            <label>
              <input
                type="text"
                spellcheck="false"
                autocomplete="off"
                class="w-full rounded-lg border border-zinc-700 bg-zinc-900 px-2 py-2 text-center font-mono text-sm text-zinc-100 outline-none focus:border-emerald-500 focus:ring-1 focus:ring-emerald-500"
                value={displayNames[row][step]}
                oninput={(e) => onNoteInput(row, step, e)}
                onchange={() => commitNote(row, step)}
                onblur={() => commitNote(row, step)}
              />
            </label>
          {/each}
        {/each}
      </div>
    </div>
  </section>
</main>
