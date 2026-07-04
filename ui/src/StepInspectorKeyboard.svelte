<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import PianoKeyboardScrollViewport from "./PianoKeyboardScrollViewport.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { pianoBlackKeyClass, pianoBlackKeySeamMaskClass, pianoWhiteKeyClass } from "./pianoKeyboardTheme.js";
  import {
    buildRecordPianoKeys,
    scalePreviewBlackKeyWidthRatio,
    stepInspectorMidiRange,
    stepInspectorOctaveOffsetForNote,
  } from "./pianoKeyboardLayout.js";
  import {
    pianoKeyboardFullMidiRange,
    scrollLeftForLowestMidi,
  } from "./pianoKeyboardScroll.js";
  import { isChromaticScaleMode, isMidiInScale, scaleName } from "./scaleUtils.js";
  import { inspectorOctaveButtonClasses } from "./inspectorSidebar.js";

  /**
   * @typedef {import('./pianoKeyboardScroll.js').PianoKeyboardScrollViewportHandle} PianoKeyboardScrollViewportHandle
   */

  /**
   * @typedef {Object} Props
   * @property {number} [note]
   * @property {string} [stepKey]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(midi: number) => void | Promise<void>} [onNoteChange]
   */

  /** @type {Props} */
  let {
    note = 60,
    stepKey = "",
    scaleRoot = 0,
    scaleModeIndex = 0,
    accent = emeraldRowAccent,
    onNoteChange = () => {},
  } = $props();

  const whiteKeyClass =
    "relative z-0 flex h-full shrink-0 flex-col items-center justify-end";

  const blackKeyClass =
    "absolute top-0 z-10 flex h-[58%] -translate-x-1/2 flex-col items-center justify-end rounded-b-md";

  /** @type {PianoKeyboardScrollViewportHandle | null} */
  let viewportHandle = null;

  let appliedStepKey = "";
  let initialScrollLeftPx = $state(0);

  let fullRange = pianoKeyboardFullMidiRange();
  let layout = $derived(
    buildRecordPianoKeys(fullRange.lowest, fullRange.highest, {
      blackKeyWidthRatio: scalePreviewBlackKeyWidthRatio,
    }),
  );
  let chromatic = $derived(isChromaticScaleMode(scaleModeIndex));
  let currentNoteName = $derived(midiToNoteName(note));
  let currentScaleName = $derived(scaleName(scaleRoot, scaleModeIndex));
  let octaveButtonClass = $derived(inspectorOctaveButtonClasses(accent));

  /** @param {PianoKeyboardScrollViewportHandle} handle */
  function registerViewport(handle) {
    viewportHandle = handle;

    if (appliedStepKey === "") {
      centerOnNote(note);
      appliedStepKey = stepKey;
    }
  }

  /** @param {number} noteMidi */
  function centerOnNote(noteMidi) {
    if (!viewportHandle) return;

    const range = stepInspectorMidiRange(stepInspectorOctaveOffsetForNote(noteMidi));
    const keyWidthPx = viewportHandle.getKeyWidthPx();

    if (keyWidthPx <= 0) return;

    viewportHandle.scrollToLeft(scrollLeftForLowestMidi(range.lowest, layout.whites, keyWidthPx));
  }

  $effect(() => {
    if (stepKey === appliedStepKey) return;

    appliedStepKey = stepKey;

    if (viewportHandle) {
      centerOnNote(note);
      return;
    }

    const range = stepInspectorMidiRange(stepInspectorOctaveOffsetForNote(note));
    initialScrollLeftPx = scrollLeftForLowestMidi(range.lowest, layout.whites, 23);
  });

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
    viewportHandle?.scrollByOctave(-1);
  }

  function shiftOctaveUp() {
    viewportHandle?.scrollByOctave(1);
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

<div class="flex h-full min-h-0 min-w-0 flex-1">
  <div class="flex h-full min-h-0 min-w-0 flex-1 flex-col gap-2">
    <div class="flex h-11 shrink-0 items-center justify-between gap-2">
      <button
        type="button"
        data-cursor="pointer"
        class={octaveButtonClass}
        aria-label="Shift keyboard down one octave"
        title="Octave down"
        onclick={shiftOctaveDown}
      >
        −
      </button>
      <button
        type="button"
        data-cursor="pointer"
        class={octaveButtonClass}
        aria-label="Shift keyboard up one octave"
        title="Octave up"
        onclick={shiftOctaveUp}
      >
        +
      </button>
    </div>

    <div class="flex min-h-0 flex-1 flex-col overflow-hidden">
      <PianoKeyboardScrollViewport
        whiteCount={layout.whiteCount}
        {initialScrollLeftPx}
        onViewportReady={registerViewport}
        class="h-full"
      >
        {#snippet children({ keyWidthPx })}
          <div
            class="relative h-full w-full touch-none select-none"
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
                  class="{whiteKeyClass} {selected
                    ? `border-r border-b ${accent.pianoNoteActive}`
                    : pianoWhiteKeyClass} {interactive
                    ? ''
                    : 'pointer-events-none'}"
                  style:width="{keyWidthPx}px"
                  aria-label={midiToNoteName(midi)}
                  aria-pressed={selected}
                  aria-disabled={!interactive}
                  onpointerdown={interactive ? (event) => onKeyPointerDown(event, midi) : undefined}
                  onpointerup={interactive ? onKeyPointerUp : undefined}
                  onpointercancel={interactive ? onKeyPointerUp : undefined}
                >
                  {#if midi % 12 === 0}
                    <span
                      class="pointer-events-none mb-1.5 text-[11px] font-bold leading-none tabular-nums text-text-inverse"
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
                {@const interactive = isKeyInteractive(midi)}
                {@const selected = isSelectedNote(midi)}
                <button
                  type="button"
                  data-cursor={interactive ? "pointer" : "default"}
                  class="{blackKeyClass} {selected
                    ? `border ${accent.pianoNoteActive} hover:brightness-110 active:brightness-125`
                    : pianoBlackKeyClass} {interactive
                    ? 'pointer-events-auto'
                    : 'pointer-events-none'}"
                  style:left="{centerPercent}%"
                  style:width="{widthPercent}%"
                  aria-label={midiToNoteName(midi)}
                  aria-pressed={selected}
                  aria-disabled={!interactive}
                  onpointerdown={interactive ? (event) => onKeyPointerDown(event, midi) : undefined}
                  onpointerup={interactive ? onKeyPointerUp : undefined}
                  onpointercancel={interactive ? onKeyPointerUp : undefined}
                ></button>
              {/each}
            </div>
          </div>
        {/snippet}
      </PianoKeyboardScrollViewport>
    </div>
  </div>
</div>
