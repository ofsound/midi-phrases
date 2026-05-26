<script>
  import { onMount } from "svelte";
  import { getNativeFunction } from "@juce/index.js";

  let pluginName = "MIDI Phrases";
  let version = "0.0.1";
  let notes = [60, 60, 60, 60];
  let bridgeReady = false;

  if (window.__JUCE__?.initialisationData?.pluginName?.[0]) {
    pluginName = window.__JUCE__.initialisationData.pluginName[0];
  }

  if (window.__JUCE__?.initialisationData?.version?.[0]) {
    version = window.__JUCE__.initialisationData.version[0];
  }

  function clampMidi(value) {
    const n = Number.parseInt(String(value), 10);
    if (Number.isNaN(n)) return 0;
    return Math.min(127, Math.max(0, n));
  }

  async function pushNote(index) {
    const setPhraseNote = getNativeFunction("setPhraseNote");
    if (!setPhraseNote) return;
    await setPhraseNote(index, notes[index]);
  }

  async function onNoteInput(index, event) {
    notes[index] = clampMidi(event.currentTarget.value);
    await pushNote(index);
  }

  onMount(async () => {
    bridgeReady = Boolean(getNativeFunction("setPhraseNote"));
    for (let i = 0; i < notes.length; i += 1) {
      await pushNote(i);
    }
  });
</script>

<main class="flex min-h-screen flex-col items-center justify-center gap-8 p-8">
  <div class="text-center">
    <p class="text-xs font-medium uppercase tracking-widest text-emerald-400">ofsound</p>
    <h1 class="mt-2 text-2xl font-semibold tracking-tight">{pluginName}</h1>
    <p class="mt-1 text-sm text-zinc-400">v{version}</p>
  </div>

  <section class="w-full max-w-lg">
    <p class="mb-3 text-center text-sm text-zinc-400">
      Four notes, one per quarter when transport plays (defaults: 60).
    </p>

    <div class="grid grid-cols-4 gap-3">
      {#each notes as note, index}
        <label class="flex flex-col gap-1 text-center">
          <span class="text-xs font-medium text-zinc-500">Note {index + 1}</span>
          <input
            type="number"
            min="0"
            max="127"
            step="1"
            class="w-full rounded-lg border border-zinc-700 bg-zinc-900 px-2 py-2 text-center text-lg font-mono text-zinc-100 outline-none focus:border-emerald-500 focus:ring-1 focus:ring-emerald-500"
            value={note}
            oninput={(e) => onNoteInput(index, e)}
          />
        </label>
      {/each}
    </div>

    <p class="mt-4 text-center text-xs text-zinc-500">
      {#if bridgeReady}
        Connected to plugin — edit values, press play in your DAW.
      {:else}
        Dev preview only (no JUCE bridge). Load the plugin to hear MIDI.
      {/if}
    </p>
  </section>
</main>
