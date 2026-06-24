<script>
  import { SvelteSet } from "svelte/reactivity";
  import { midiToNoteName } from "./midiNoteNames.js";
  import PianoKeyboardAspectFrame from "./PianoKeyboardAspectFrame.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { pianoBlackKeyClass, pianoBlackKeySeamMaskClass, pianoWhiteKeyClass } from "./pianoKeyboardTheme.js";
  import {
    buildRecordPianoKeys,
    clampRecordPianoOctaveOffset,
    recordPianoMidiRange,
    recordPianoRangeLabel,
  } from "./pianoKeyboardLayout.js";
  import { isChromaticScaleMode, isKeyCenterPitchClass, isMidiInScale } from "./scaleUtils.js";

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {Set<number>} [heldKeys]
   * @property {(midi: number) => void} [onNotePress]
   */

  /** @type {Props} */
  let {
    row = 0,
    scaleRoot = 0,
    scaleModeIndex = 0,
    accent = emeraldRowAccent,
    heldKeys = new Set(),
    onNotePress = () => {}
  } = $props();

  const scaleToneMarkerClass =
    "pointer-events-none mb-1 h-2 w-2 shrink-0 rounded-full bg-accent shadow-[0_0_6px_rgba(52,211,153,0.8)]";

  /** Default view: three octave-up steps from the bottom (starts ~C1, not C-2). */
  let octaveOffset = $state(3);

  /** @type {Set<number>} */
  const pointerHeldKeys = new SvelteSet();

  let range = $derived(recordPianoMidiRange(octaveOffset));
  let layout = $derived(buildRecordPianoKeys(range.lowest, range.highest));
  let rangeLabel = $derived(recordPianoRangeLabel(range.lowest, range.highest));
  let chromatic = $derived(isChromaticScaleMode(scaleModeIndex));

  /** @param {number} midi */
  function isKeyUsable(midi) {
    return chromatic || isMidiInScale(midi, scaleRoot, scaleModeIndex);
  }

  function isKeyHeld(midi) {
    return isKeyUsable(midi) && (heldKeys.has(midi) || pointerHeldKeys.has(midi));
  }

  /** @param {number} midi */
  function pressKey(midi) {
    if (!isKeyUsable(midi) || pointerHeldKeys.has(midi)) return;

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
    <PianoKeyboardAspectFrame whiteCount={layout.whiteCount} class="min-h-0 flex-1">
      <div class="relative h-full w-full">
        <div class="relative z-0 flex h-full w-full">
        {#each layout.whites as { midi } (midi)}
          {@const usable = isKeyUsable(midi)}
          <button
            type="button"
            data-cursor={usable ? "pointer" : "default"}
            class="relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end transition-[filter,background-color] duration-75 last:border-r-0 {usable
              ? 'hover:brightness-[0.98] active:brightness-95'
              : 'pointer-events-none'} {isKeyHeld(midi)
              ? accent.pianoNoteActive
              : pianoWhiteKeyClass}"
            aria-label={midiToNoteName(midi)}
            aria-disabled={!usable}
            onpointerdown={usable ? (event) => onWhitePointerDown(event, midi) : undefined}
            onpointerup={usable ? (event) => onWhitePointerUp(event, midi) : undefined}
            onpointercancel={usable ? (event) => onWhitePointerUp(event, midi) : undefined}
          >
            {#if usable && !isKeyHeld(midi)}
              <span class={scaleToneMarkerClass}></span>
            {/if}
            {#if isKeyCenterPitchClass(midi, scaleRoot)}
              <span
                class="pointer-events-none mb-1 text-[11px] font-bold leading-none text-text-inverse tabular-nums"
              >
                {midiToNoteName(midi)}
              </span>
            {/if}
          </button>
        {/each}
      </div>

      <div class="pointer-events-none absolute inset-0">
        {#each layout.blacks as { midi, centerPercent } (`seam-${midi}`)}
          <div
            class="{pianoBlackKeySeamMaskClass}"
            style:left="calc({centerPercent}% - 1px)"
            style:width="2px"
            style:height="100%"
            aria-hidden="true"
          ></div>
        {/each}
      </div>

      <div class="pointer-events-none absolute inset-0 z-10">
        {#each layout.blacks as { midi, centerPercent, widthPercent } (midi)}
          {@const usable = isKeyUsable(midi)}
          <button
            type="button"
            data-cursor={usable ? "pointer" : "default"}
            class="{usable
              ? 'pointer-events-auto hover:brightness-110 active:brightness-125'
              : 'pointer-events-none'} absolute top-0 z-10 flex h-[58%] -translate-x-1/2 flex-col items-center justify-end rounded-b-md pb-1 {isKeyHeld(midi)
              ? accent.pianoNoteActive
              : pianoBlackKeyClass}"
            style:left="{centerPercent}%"
            style:width="{widthPercent}%"
            aria-label={midiToNoteName(midi)}
            aria-disabled={!usable}
            onpointerdown={usable
              ? (event) => {
                  event.preventDefault();
                  event.currentTarget.setPointerCapture(event.pointerId);
                  pressKey(midi);
                }
              : undefined}
            onpointerup={usable
              ? (event) => {
                  event.currentTarget.releasePointerCapture(event.pointerId);
                  releaseKey(midi);
                }
              : undefined}
            onpointercancel={usable
              ? (event) => {
                  event.currentTarget.releasePointerCapture(event.pointerId);
                  releaseKey(midi);
                }
              : undefined}
          >
            {#if usable && !isKeyHeld(midi)}
              <span class={scaleToneMarkerClass}></span>
            {/if}
            {#if isKeyCenterPitchClass(midi, scaleRoot)}
              <span
                class="pointer-events-none mb-1 text-[10px] font-bold leading-none text-text-secondary tabular-nums"
              >
                {midiToNoteName(midi)}
              </span>
            {/if}
          </button>
        {/each}
      </div>
      </div>
    </PianoKeyboardAspectFrame>
  </div>

  <p class="mt-2 shrink-0 text-center text-[11px] text-text-faint">
    {#if chromatic}
      Click keys or play MIDI · each note adds a 1× step · chords at the same instant are ignored
    {:else}
      Click scale keys or play MIDI · off-scale notes are ignored · chords at the same instant are ignored
    {/if}
  </p>
</section>
