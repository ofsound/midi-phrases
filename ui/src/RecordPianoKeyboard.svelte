<script>
  import { SvelteSet } from "svelte/reactivity";
  import { midiToNoteName } from "./midiNoteNames.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    buildRecordPianoKeys,
    clampRecordPianoOctaveOffset,
    recordPianoMidiRange,
    recordPianoRangeLabel,
  } from "./pianoKeyboardLayout.js";

  
  
  
  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {Set<number>} [heldKeys]
   * @property {(midi: number) => void} [onNotePress]
   */

  /** @type {Props} */
  let {
    row = 0,
    accent = emeraldRowAccent,
    heldKeys = new Set(),
    onNotePress = () => {}
  } = $props();

  /** Default view: three octave-up steps from the bottom (starts ~C1, not C-2). */
  let octaveOffset = $state(3);

  /** @type {Set<number>} */
  const pointerHeldKeys = new SvelteSet();

  let range = $derived(recordPianoMidiRange(octaveOffset));
  let layout = $derived(buildRecordPianoKeys(range.lowest, range.highest));
  let rangeLabel = $derived(recordPianoRangeLabel(range.lowest, range.highest));

  function isKeyHeld(midi) {
    return heldKeys.has(midi) || pointerHeldKeys.has(midi);
  }

  /** @param {number} midi */
  function pressKey(midi) {
    if (pointerHeldKeys.has(midi)) return;

    pointerHeldKeys.add(midi);
    onNotePress(midi);
  }

  /** @param {number} midi */
  function releaseKey(midi) {
    if (!pointerHeldKeys.has(midi)) return;

    pointerHeldKeys.delete(midi);
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

<section class="flex min-h-0 w-full flex-1 flex-col">
  <div class="mb-2 flex shrink-0 items-center justify-between gap-3">
    <p class="text-xs font-medium uppercase tracking-widest text-text-muted">
      Record keyboard · row {row + 1}
    </p>
    <div class="flex items-center gap-2">
      <button
        type="button"
        data-cursor="pointer"
        class="flex h-8 w-8 items-center justify-center rounded-md border border-border bg-surface text-lg font-semibold text-text transition-colors hover:border-border-strong hover:bg-surface-muted outline-none focus-visible:ring-1 {accent.ringFocus}"
        aria-label="Shift keyboard down one octave"
        title="Octave down"
        onclick={shiftOctaveDown}
      >
        −
      </button>
      <span class="min-w-[7.5rem] text-center text-xs font-medium tabular-nums text-text-secondary">
        {rangeLabel}
      </span>
      <button
        type="button"
        data-cursor="pointer"
        class="flex h-8 w-8 items-center justify-center rounded-md border border-border bg-surface text-lg font-semibold text-text transition-colors hover:border-border-strong hover:bg-surface-muted outline-none focus-visible:ring-1 {accent.ringFocus}"
        aria-label="Shift keyboard up one octave"
        title="Octave up"
        onclick={shiftOctaveUp}
      >
        +
      </button>
    </div>
  </div>

  <div
    class="flex h-0 min-h-[12rem] flex-1 flex-col overflow-hidden rounded-xl border border-border-subtle bg-app/80"
  >
    <div class="relative flex min-h-0 flex-1 touch-none select-none">
      <div class="relative z-0 flex h-full min-h-[10rem] w-full">
        {#each layout.whites as { midi } (midi)}
          <button
            type="button"
            data-cursor="pointer"
            class="relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end border-r border-b border-border/80 transition-[filter,background-color] duration-75 last:border-r-0 hover:brightness-105 active:brightness-95 {isKeyHeld(
              midi,
            )
              ? accent.pianoNoteActive
              : 'bg-gradient-to-b from-input to-surface-muted hover:from-surface hover:to-surface-subtle'}"
            aria-label={midiToNoteName(midi)}
            onpointerdown={(event) => onWhitePointerDown(event, midi)}
            onpointerup={(event) => onWhitePointerUp(event, midi)}
            onpointercancel={(event) => onWhitePointerUp(event, midi)}
          >
            {#if midi % 12 === 0}
              <span
                class="pointer-events-none mb-1 text-[11px] font-bold leading-none text-text-inverse tabular-nums"
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
            data-cursor="pointer"
            class="pointer-events-auto absolute top-0 z-10 h-[58%] max-w-[2.75rem] min-w-[0.75rem] -translate-x-1/2 rounded-b-md border border-border-subtle/80 shadow-md transition-[filter,background-color] duration-75 active:brightness-110 {isKeyHeld(
              midi,
            )
              ? accent.pianoNoteActive
              : 'bg-gradient-to-b from-surface-subtle to-app hover:from-surface-subtle hover:to-surface'}"
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

  <p class="mt-2 shrink-0 text-center text-[11px] text-text-faint">
    Click keys or play MIDI · each note adds a 1× step · chords at the same instant are ignored
  </p>
</section>
