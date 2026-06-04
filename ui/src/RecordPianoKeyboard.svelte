<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    buildRecordPianoKeys,
    clampRecordPianoOctaveOffset,
    recordPianoMidiRange,
    recordPianoRangeLabel,
  } from "./pianoKeyboardLayout.js";

  export let row = 0;
  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  /** @type {Set<number>} */
  export let heldKeys = new Set();
  /** @type {(midi: number) => void} */
  export let onNotePress = () => {};

  /** Default view: three octave-up steps from the bottom (starts ~C1, not C-2). */
  let octaveOffset = 3;

  /** @type {Set<number>} */
  let pointerHeldKeys = new Set();

  $: octaveOffset = clampRecordPianoOctaveOffset(octaveOffset);
  $: range = recordPianoMidiRange(octaveOffset);
  $: layout = buildRecordPianoKeys(range.lowest, range.highest);
  $: rangeLabel = recordPianoRangeLabel(range.lowest, range.highest);

  function isKeyHeld(midi) {
    return heldKeys.has(midi) || pointerHeldKeys.has(midi);
  }

  /** @param {number} midi */
  function pressKey(midi) {
    if (pointerHeldKeys.has(midi)) return;

    pointerHeldKeys = new Set([...pointerHeldKeys, midi]);
    onNotePress(midi);
  }

  /** @param {number} midi */
  function releaseKey(midi) {
    if (!pointerHeldKeys.has(midi)) return;

    const next = new Set(pointerHeldKeys);
    next.delete(midi);
    pointerHeldKeys = next;
  }

  function shiftOctaveDown() {
    octaveOffset = clampRecordPianoOctaveOffset(octaveOffset - 1);
  }

  function shiftOctaveUp() {
    octaveOffset = clampRecordPianoOctaveOffset(octaveOffset + 1);
  }

  /** @param {PointerEvent} event @param {number} midi */
  function onWhitePointerDown(event, midi) {
    event.preventDefault();
    event.currentTarget.setPointerCapture(event.pointerId);
    pressKey(midi);
  }

  /** @param {PointerEvent} event @param {number} midi */
  function onWhitePointerUp(event, midi) {
    event.currentTarget.releasePointerCapture(event.pointerId);
    releaseKey(midi);
  }
</script>

<section class="mt-6 flex min-h-0 w-full flex-1 flex-col">
  <div class="mb-2 flex shrink-0 items-center justify-between gap-3">
    <p class="text-xs font-medium uppercase tracking-widest text-zinc-500">
      Record keyboard · row {row + 1}
    </p>
    <div class="flex items-center gap-2">
      <button
        type="button"
        class="flex h-8 w-8 items-center justify-center rounded-md border border-zinc-700 bg-zinc-900 text-lg font-semibold text-zinc-200 transition-colors hover:border-zinc-500 hover:bg-zinc-800 outline-none focus-visible:ring-1 {accent.ringFocus}"
        aria-label="Shift keyboard down one octave"
        title="Octave down"
        onclick={shiftOctaveDown}
      >
        −
      </button>
      <span class="min-w-[7.5rem] text-center text-xs font-medium tabular-nums text-zinc-400">
        {rangeLabel}
      </span>
      <button
        type="button"
        class="flex h-8 w-8 items-center justify-center rounded-md border border-zinc-700 bg-zinc-900 text-lg font-semibold text-zinc-200 transition-colors hover:border-zinc-500 hover:bg-zinc-800 outline-none focus-visible:ring-1 {accent.ringFocus}"
        aria-label="Shift keyboard up one octave"
        title="Octave up"
        onclick={shiftOctaveUp}
      >
        +
      </button>
    </div>
  </div>

  <div
    class="flex h-0 min-h-[12rem] flex-1 flex-col overflow-hidden rounded-xl border border-zinc-800 bg-zinc-950/80"
  >
    <div class="relative flex min-h-0 flex-1 touch-none select-none">
      <div class="relative z-0 flex h-full min-h-[10rem] w-full">
        {#each layout.whites as { midi } (midi)}
          <button
            type="button"
            class="relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end border-r border-b border-zinc-700/80 transition-[filter,background-color] duration-75 last:border-r-0 hover:brightness-105 active:brightness-95 {isKeyHeld(
              midi,
            )
              ? accent.pianoNoteActive
              : 'bg-gradient-to-b from-zinc-200 to-zinc-400 hover:from-zinc-100 hover:to-zinc-300'}"
            aria-label={midiToNoteName(midi)}
            onpointerdown={(event) => onWhitePointerDown(event, midi)}
            onpointerup={(event) => onWhitePointerUp(event, midi)}
            onpointercancel={(event) => onWhitePointerUp(event, midi)}
          >
            {#if midi % 12 === 0}
              <span
                class="pointer-events-none mb-1 text-[11px] font-bold leading-none text-black tabular-nums"
              >
                {midiToNoteName(midi)}
              </span>
            {/if}
          </button>
        {/each}
      </div>

      <div class="pointer-events-none absolute inset-0 z-10">
        {#each layout.blacks as { midi, centerPercent, widthPercent } (midi)}
          <button
            type="button"
            class="pointer-events-auto absolute top-0 z-10 h-[58%] max-w-[2.75rem] min-w-[0.75rem] -translate-x-1/2 rounded-b-md border border-zinc-900/80 shadow-md transition-[filter,background-color] duration-75 active:brightness-110 {isKeyHeld(
              midi,
            )
              ? accent.pianoNoteActive
              : 'bg-gradient-to-b from-zinc-700 to-zinc-950 hover:from-zinc-600 hover:to-zinc-900'}"
            style:left="{centerPercent}%"
            style:width="{widthPercent}%"
            aria-label={midiToNoteName(midi)}
            onpointerdown={(event) => {
              event.preventDefault();
              event.currentTarget.setPointerCapture(event.pointerId);
              pressKey(midi);
            }}
            onpointerup={(event) => {
              event.currentTarget.releasePointerCapture(event.pointerId);
              releaseKey(midi);
            }}
            onpointercancel={(event) => {
              event.currentTarget.releasePointerCapture(event.pointerId);
              releaseKey(midi);
            }}
          ></button>
        {/each}
      </div>
    </div>
  </div>

  <p class="mt-2 shrink-0 text-center text-[11px] text-zinc-600">
    Click keys or play MIDI · each note adds a 1× step · chords at the same instant are ignored
  </p>
</section>
