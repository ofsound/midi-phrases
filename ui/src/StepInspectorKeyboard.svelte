<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import {
    buildRecordPianoKeys,
    clampStepInspectorOctaveOffset,
    scalePreviewBlackKeyWidthRatio,
    stepInspectorMidiRange,
    stepInspectorOctaveOffsetForNote,
  } from "./pianoKeyboardLayout.js";

  /**
   * @typedef {Object} Props
   * @property {number} [note]
   * @property {string} [stepKey]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(midi: number) => void | Promise<void>} [onNoteChange]
   */

  /** @type {Props} */
  let {
    note = 60,
    stepKey = "",
    accent = emeraldRowAccent,
    onNoteChange = () => {},
  } = $props();

  const selectedNoteLabelClass =
    "pointer-events-none max-w-full truncate text-[10px] font-bold leading-none tabular-nums text-text-inverse";

  let octaveOffset = $state(0);
  let appliedStepKey = "";

  $effect(() => {
    if (stepKey !== appliedStepKey) {
      octaveOffset = stepInspectorOctaveOffsetForNote(note);
      appliedStepKey = stepKey;
    }
  });

  let range = $derived(stepInspectorMidiRange(octaveOffset));
  let layout = $derived(
    buildRecordPianoKeys(range.lowest, range.highest, {
      blackKeyWidthRatio: scalePreviewBlackKeyWidthRatio,
    }),
  );

  /** @param {number} midi */
  function isSelectedNote(midi) {
    return midi === note;
  }

  /** @param {number} midi */
  function selectNote(midi) {
    if (midi === note) return;

    onNoteChange(midi);
  }

  function shiftOctaveDown() {
    octaveOffset = clampStepInspectorOctaveOffset(octaveOffset - 1);
  }

  function shiftOctaveUp() {
    octaveOffset = clampStepInspectorOctaveOffset(octaveOffset + 1);
  }

  /** @param {PointerEvent} event @param {number} midi */
  function onKeyPointerDown(event, midi) {
    event.preventDefault();
    event.currentTarget.setPointerCapture(event.pointerId);
    selectNote(midi);
  }

  /** @param {PointerEvent} event */
  function onKeyPointerUp(event) {
    event.currentTarget.releasePointerCapture(event.pointerId);
  }
</script>

<div class="flex h-full min-h-0 min-w-0 flex-1 items-center gap-2">
  <button
    type="button"
    data-cursor="pointer"
    class="flex h-7 w-7 shrink-0 items-center justify-center rounded-md border border-border bg-surface text-base font-semibold text-text transition-colors hover:border-border-strong hover:bg-surface-muted outline-none focus-visible:ring-1 {accent.ringFocus}"
    aria-label="Shift keyboard down one octave"
    title="Octave down"
    onclick={shiftOctaveDown}
  >
    −
  </button>

  <div
    class="relative flex h-full min-h-12 min-w-0 flex-1 touch-none select-none overflow-hidden rounded-lg border border-border-subtle bg-app/80"
    role="group"
    aria-label="Step note keyboard"
  >
    <div class="relative z-0 flex h-full w-full">
      {#each layout.whites as { midi } (midi)}
        <button
          type="button"
          data-cursor="pointer"
          class="relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end border-r border-b border-border-strong/70 pb-1 transition-[filter,background-color] duration-75 last:border-r-0 hover:brightness-105 active:brightness-95 {isSelectedNote(
            midi,
          )
            ? accent.pianoNoteActive
            : 'bg-gradient-to-b from-input to-surface-muted hover:from-surface hover:to-surface-subtle'}"
          aria-label={midiToNoteName(midi)}
          aria-pressed={isSelectedNote(midi)}
          onpointerdown={(event) => onKeyPointerDown(event, midi)}
          onpointerup={onKeyPointerUp}
          onpointercancel={onKeyPointerUp}
        >
          {#if isSelectedNote(midi)}
            <span class={selectedNoteLabelClass}>{midiToNoteName(midi)}</span>
          {/if}
        </button>
      {/each}
    </div>

    <div class="pointer-events-none absolute inset-0 z-10">
      {#each layout.blacks as { midi, centerPercent, widthPercent } (midi)}
        <button
          type="button"
          data-cursor="pointer"
          class="pointer-events-auto absolute top-0 z-10 flex h-[58%] max-w-[2rem] min-w-[0.5rem] -translate-x-1/2 flex-col items-center justify-end rounded-b-md border border-border-subtle/80 pb-1 shadow-md transition-[filter,background-color] duration-75 active:brightness-110 {isSelectedNote(
            midi,
          )
            ? accent.pianoNoteActive
            : 'bg-gradient-to-b from-surface-subtle to-app hover:from-surface-subtle hover:to-surface'}"
          style:left="{centerPercent}%"
          style:width="{widthPercent}%"
          aria-label={midiToNoteName(midi)}
          aria-pressed={isSelectedNote(midi)}
          onpointerdown={(event) => onKeyPointerDown(event, midi)}
          onpointerup={onKeyPointerUp}
          onpointercancel={onKeyPointerUp}
        >
          {#if isSelectedNote(midi)}
            <span class={selectedNoteLabelClass}>{midiToNoteName(midi)}</span>
          {/if}
        </button>
      {/each}
    </div>
  </div>

  <button
    type="button"
    data-cursor="pointer"
    class="flex h-7 w-7 shrink-0 items-center justify-center rounded-md border border-border bg-surface text-base font-semibold text-text transition-colors hover:border-border-strong hover:bg-surface-muted outline-none focus-visible:ring-1 {accent.ringFocus}"
    aria-label="Shift keyboard up one octave"
    title="Octave up"
    onclick={shiftOctaveUp}
  >
    +
  </button>
</div>
