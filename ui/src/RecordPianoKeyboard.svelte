<script>
  import { SvelteSet } from "svelte/reactivity";
  import { midiToNoteName } from "./midiNoteNames.js";
  import PianoKeyboardScrollViewport from "./PianoKeyboardScrollViewport.svelte";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { pianoBlackKeyClass, pianoBlackKeySeamMaskClass, pianoWhiteKeyClass } from "./pianoKeyboardTheme.js";
  import {
    buildRecordPianoKeys,
    recordPianoMidiRange,
    recordPianoRangeLabel,
  } from "./pianoKeyboardLayout.js";
  import {
    highestVisibleMidiFromScrollLeft,
    lowestVisibleMidiFromScrollLeft,
    pianoKeyboardFullMidiRange,
    scrollLeftForLowestMidi,
  } from "./pianoKeyboardScroll.js";
  import { isChromaticScaleMode, isKeyCenterPitchClass, isMidiInScale } from "./scaleUtils.js";

  /**
   * @typedef {import('./pianoKeyboardScroll.js').PianoKeyboardScrollViewportHandle} PianoKeyboardScrollViewportHandle
   */

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {number} [scaleRoot]
   * @property {number} [scaleModeIndex]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {Set<number>} [heldKeys]
   * @property {boolean} [standaloneTransportAvailable]
   * @property {(midi: number) => void} [onNotePress]
   * @property {(midi: number) => void} [onNoteRelease]
   */

  /** @type {Props} */
  let {
    row = 0,
    scaleRoot = 0,
    scaleModeIndex = 0,
    accent = emeraldRowAccent,
    heldKeys = new Set(),
    standaloneTransportAvailable = false,
    onNotePress = () => {},
    onNoteRelease = () => {}
  } = $props();

  const scaleToneMarkerClass =
    "pointer-events-none mb-1 h-2 w-2 shrink-0 rounded-full bg-accent shadow-[0_0_6px_color-mix(in_srgb,var(--color-accent)_80%,transparent)]";

  /** Default view: three octave-up steps from the bottom (starts ~C1, not C-2). */
  const defaultRecordOctaveOffset = 3;

  /** @type {PianoKeyboardScrollViewportHandle | null} */
  let viewportHandle = null;

  let didInitialScroll = false;
  let scrollState = $state({ scrollLeftPx: 0, viewportWidthPx: 0, keyWidthPx: 0 });
  let initialScrollLeftPx = $state(0);

  /** @type {Set<number>} */
  const pointerHeldKeys = new SvelteSet();

  let fullRange = pianoKeyboardFullMidiRange();
  let layout = $derived(buildRecordPianoKeys(fullRange.lowest, fullRange.highest));
  let rangeLabel = $derived.by(() => {
    if (scrollState.keyWidthPx <= 0) {
      const range = recordPianoMidiRange(defaultRecordOctaveOffset);

      return recordPianoRangeLabel(range.lowest, range.highest);
    }

    const lowest = lowestVisibleMidiFromScrollLeft(
      scrollState.scrollLeftPx,
      layout.whites,
      scrollState.keyWidthPx,
    );
    const highest = highestVisibleMidiFromScrollLeft(
      scrollState.scrollLeftPx,
      scrollState.viewportWidthPx,
      layout.whites,
      scrollState.keyWidthPx,
    );

    return recordPianoRangeLabel(lowest, highest);
  });
  let chromatic = $derived(isChromaticScaleMode(scaleModeIndex));

  /** @param {PianoKeyboardScrollViewportHandle} handle */
  function registerViewport(handle) {
    viewportHandle = handle;

    if (!didInitialScroll) {
      didInitialScroll = true;
      scrollToDefaultRange(handle);
    }
  }

  /** @param {PianoKeyboardScrollViewportHandle} handle */
  function scrollToDefaultRange(handle) {
    const range = recordPianoMidiRange(defaultRecordOctaveOffset);
    const keyWidthPx = handle.getKeyWidthPx();

    if (keyWidthPx <= 0) return;

    handle.scrollToLeft(scrollLeftForLowestMidi(range.lowest, layout.whites, keyWidthPx));
  }

  /** @param {number} scrollLeftPx @param {number} viewportWidthPx @param {number} keyWidthPx */
  function handleScrollChange(scrollLeftPx, viewportWidthPx, keyWidthPx) {
    scrollState = { scrollLeftPx, viewportWidthPx, keyWidthPx };
  }

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
    onNoteRelease(midi);
  }

  function shiftOctaveDown() {
    viewportHandle?.scrollByOctave(-1);
  }

  function shiftOctaveUp() {
    viewportHandle?.scrollByOctave(1);
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

  $effect(() => {
    if (didInitialScroll) return;

    const range = recordPianoMidiRange(defaultRecordOctaveOffset);
    initialScrollLeftPx = scrollLeftForLowestMidi(range.lowest, layout.whites, 23);
  });
</script>

<section class="flex min-h-0 w-full flex-1 flex-col">
  <div class="mb-2 flex shrink-0 items-center justify-end gap-2">
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

  <div
    class="flex h-0 min-h-[12rem] flex-1 flex-col overflow-hidden rounded-xl border border-border-subtle bg-workspace"
  >
    <PianoKeyboardScrollViewport
      whiteCount={layout.whiteCount}
      {initialScrollLeftPx}
      onScrollChange={handleScrollChange}
      onViewportReady={registerViewport}
      class="h-full"
    >
      {#snippet children({ keyWidthPx })}
        <div class="relative h-full w-full touch-none select-none">
          <div class="relative z-0 flex h-full w-full">
            {#each layout.whites as { midi } (midi)}
              {@const usable = isKeyUsable(midi)}
              <button
                type="button"
                data-cursor={usable ? "pointer" : "default"}
                class="relative z-0 flex h-full shrink-0 flex-col items-center justify-end transition-[filter,background-color] duration-75 {usable
                  ? 'hover:brightness-[0.98] active:brightness-95'
                  : 'pointer-events-none'} {isKeyHeld(midi)
                  ? accent.pianoNoteActive
                  : pianoWhiteKeyClass}"
                style:width="{keyWidthPx}px"
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
      {/snippet}
    </PianoKeyboardScrollViewport>
  </div>

  <p class="mt-2 shrink-0 text-center text-[11px] text-text-faint">
    {#if standaloneTransportAvailable}
      QWERTY keys play notes · Z/X shift octave · Space toggles transport
    {:else if chromatic}
      Click keys or play MIDI · starts on first note · stop quantizes the loop to 0.25×
    {:else}
      Click scale keys or play MIDI · off-scale notes are ignored · stop quantizes the loop to 0.25×
    {/if}
  </p>
</section>
