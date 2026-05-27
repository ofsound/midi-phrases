<script>
  import { onMount } from "svelte";
  import { getNativeFunction } from "@juce/index.js";
  import {
    defaultPhraseGrid,
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

  const timingOffsetOptions = [
    { index: 0, label: "-.75" },
    { index: 1, label: "-.5" },
    { index: 2, label: "-.25" },
    { index: 3, label: "0" },
    { index: 4, label: ".25" },
    { index: 5, label: ".5" },
    { index: 6, label: ".75" },
  ];

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

  function loadRowMutedFromInitialisation() {
    const init = window.__JUCE__?.initialisationData?.phraseRowMuted;

    if (!Array.isArray(init)) return;

    const next = [false, false, false, false];

    for (let row = 0; row < 4; row += 1) {
      next[row] = Boolean(init[row]);
    }

    rowMuted = next;
  }

  function loadRowTimingOffsetFromInitialisation() {
    const init = window.__JUCE__?.initialisationData?.phraseRowTimingOffset;

    if (!Array.isArray(init)) return;

    const next = [3, 3, 3, 3];

    for (let row = 0; row < 4; row += 1) {
      const value = Number.parseInt(String(init[row]), 10);

      if (Number.isNaN(value)) continue;

      next[row] = Math.min(6, Math.max(0, value));
    }

    rowTimingOffset = next;
  }

  async function pushNote(row, step) {
    const setPhraseNote = getNativeFunction("setPhraseNote");

    if (!setPhraseNote) return;

    await setPhraseNote(row, step, grid[row][step]);
  }

  async function pushRowMuted(row) {
    const setPhraseRowMuted = getNativeFunction("setPhraseRowMuted");

    if (!setPhraseRowMuted) return;

    await setPhraseRowMuted(row, rowMuted[row] ? 1 : 0);
  }

  async function pushRowTimingOffset(row) {
    const setPhraseRowTimingOffset = getNativeFunction("setPhraseRowTimingOffset");

    if (!setPhraseRowTimingOffset) return;

    await setPhraseRowTimingOffset(row, rowTimingOffset[row]);
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

  onMount(async () => {
    loadGridFromInitialisation();
    loadRowMutedFromInitialisation();
    loadRowTimingOffsetFromInitialisation();

    for (let row = 0; row < grid.length; row += 1) {
      await pushRowMuted(row);
      await pushRowTimingOffset(row);

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
    <div class="w-full max-w-5xl">
      <div class="flex flex-col gap-2">
        {#each grid as _row, row}
          <div class="grid grid-cols-[auto_2.5rem_repeat(4,minmax(0,1fr))] items-center gap-2">
            <div class="flex items-center gap-0.5" role="radiogroup" aria-label="Row timing offset">
              {#each timingOffsetOptions as option}
                <button
                  type="button"
                  role="radio"
                  aria-checked={rowTimingOffset[row] === option.index}
                  aria-label="Timing offset {option.label}"
                  class="min-w-[2.1rem] rounded border px-1 py-1 font-mono text-[10px] leading-none transition-colors outline-none {rowTimingOffset[row] ===
                  option.index
                    ? 'border-emerald-500 bg-emerald-950 text-emerald-300'
                    : 'border-zinc-700 bg-zinc-900 text-zinc-400 hover:border-zinc-600 hover:text-zinc-200'}"
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
          </div>
        {/each}
      </div>
    </div>
  </section>
</main>
