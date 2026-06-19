<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { pianoBlackKeyClass, pianoWhiteKeyClass } from "./pianoKeyboardTheme.js";
  import {
    buildRecordPianoKeys,
    clampStepInspectorOctaveOffset,
    scalePreviewBlackKeyWidthRatio,
    stepInspectorMidiRange,
    stepInspectorOctaveOffsetForNote,
  } from "./pianoKeyboardLayout.js";
  import { isChromaticScaleMode, isMidiInScale, scaleName } from "./scaleUtils.js";

  /**
   * @typedef {Object} Props
   * @property {number} [note]
   * @property {string} [stepKey]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [embedded]
   * @property {(midi: number) => void | Promise<void>} [onNoteChange]
   */

  /** @type {Props} */
  let {
    note = 60,
    stepKey = "",
    scaleRoot = 0,
    scaleModeIndex = 0,
    accent = emeraldRowAccent,
    embedded = false,
    onNoteChange = () => {},
  } = $props();

  const whiteKeyClass =
    "relative z-0 flex h-full min-w-0 flex-1 flex-col items-center justify-end last:border-r-0";

  const blackKeyClass =
    "pointer-events-auto absolute top-0 z-10 flex h-[58%] max-w-[2rem] min-w-[0.5rem] -translate-x-1/2 flex-col items-center justify-end rounded-b-md";

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
  let chromatic = $derived(isChromaticScaleMode(scaleModeIndex));
  let currentNoteName = $derived(midiToNoteName(note));
  let currentScaleName = $derived(scaleName(scaleRoot, scaleModeIndex));

  /** @param {number} midi */
  function isKeyInteractive(midi) {
    return chromatic || isMidiInScale(midi, scaleRoot, scaleModeIndex);
  }

  /** @param {number} midi */
  function isSelectedNote(midi) {
    return midi === note;
  }

  /** @param {number} midi */
  function selectNote(midi) {
    if (!isKeyInteractive(midi) || midi === note) return;

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

  let keyboardSurfaceClass = $derived(
    embedded
      ? "relative flex h-full min-h-12 min-w-0 flex-1 touch-none select-none overflow-hidden"
      : "relative flex h-full min-h-12 min-w-0 flex-1 touch-none select-none overflow-hidden rounded-lg border border-border-subtle bg-app/80",
  );
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

  <div class="flex h-full min-h-0 min-w-0 flex-1 flex-col">
    <div class="flex h-7 shrink-0 items-center justify-between gap-4 px-1 text-[9px] font-medium uppercase tracking-wider text-text-muted">
      <span>
        Note
        <strong class="ml-1 font-mono text-xs font-bold tracking-normal {accent.textAccentStrong}">{currentNoteName}</strong>
      </span>
      <span class="min-w-0 truncate text-right">
        Scale
        <strong class="ml-1 text-xs font-semibold normal-case tracking-normal {accent.textAccentStrong}">{currentScaleName}</strong>
      </span>
    </div>

    <div
      class={keyboardSurfaceClass}
      role="group"
      aria-label={`Step note keyboard, ${currentNoteName}, ${currentScaleName}`}
    >
      <div class="relative z-0 flex h-full w-full">
        {#each layout.whites as { midi } (midi)}
          {@const interactive = isKeyInteractive(midi)}
          {@const selected = isSelectedNote(midi)}
          <button
            type="button"
            data-cursor={interactive ? "pointer" : "default"}
            class="{whiteKeyClass} {selected ? accent.pianoNoteActive : pianoWhiteKeyClass} {interactive || selected ? '' : 'pointer-events-none opacity-35'}"
            aria-label={midiToNoteName(midi)}
            aria-pressed={selected}
            aria-disabled={!interactive}
            disabled={!interactive}
            onpointerdown={interactive ? (event) => onKeyPointerDown(event, midi) : undefined}
            onpointerup={interactive ? onKeyPointerUp : undefined}
            onpointercancel={interactive ? onKeyPointerUp : undefined}
          ></button>
        {/each}
      </div>

      <div class="pointer-events-none absolute inset-0 z-10">
        {#each layout.blacks as { midi, centerPercent, widthPercent } (midi)}
          {@const interactive = isKeyInteractive(midi)}
          {@const selected = isSelectedNote(midi)}
          <button
            type="button"
            data-cursor={interactive ? "pointer" : "default"}
            class="{blackKeyClass} {selected ? accent.pianoNoteActive : pianoBlackKeyClass} {interactive || selected ? '' : 'pointer-events-none opacity-35'}"
            style:left="{centerPercent}%"
            style:width="{widthPercent}%"
            aria-label={midiToNoteName(midi)}
            aria-pressed={selected}
            aria-disabled={!interactive}
            disabled={!interactive}
            onpointerdown={interactive ? (event) => onKeyPointerDown(event, midi) : undefined}
            onpointerup={interactive ? onKeyPointerUp : undefined}
            onpointercancel={interactive ? onKeyPointerUp : undefined}
          ></button>
        {/each}
      </div>
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
