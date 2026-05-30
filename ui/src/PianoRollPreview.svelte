<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { beatFromClientX, clampLoopBrace } from "./loopBraceLayout.js";
  import { defaultPulseIndex } from "./pulseLayout.js";
  import { rowAccentFor } from "./rowAccentTheme.js";
  import {
    buildPhraseSchedule,
    DEFAULT_PREVIEW_LENGTH_QUARTERS,
    isBlackKey,
    isScheduledNoteActiveAtBeat,
    pitchRangeForSchedule,
  } from "./phraseSchedule.js";

  export let notes = [];
  export let rowMuted = [];
  export let rowReversed = [];
  export let rowTimingOffset = [];
  export let stepDurationFraction = [];
  export let stepTimingMultiplier = [];
  export let stepVelocity = [];
  export let stepMuted = [];
  export let stepSkipped = [];
  export let stepProbability = [];
  export let stepCycle = [];
  export let stepCycleOffset = [];
  export let pulseIndex = defaultPulseIndex;
  export let rowColorsEnabled = false;
  export let lengthQuarters = DEFAULT_PREVIEW_LENGTH_QUARTERS;
  export let loopEnabled = false;
  export let loopStart = 0;
  export let loopEnd = 8;
  export let playbackBeat = -1;
  /** @type {(next: { enabled?: boolean, start?: number, end?: number }) => void | Promise<void>} */
  export let onLoopBraceChange = () => {};

  const pxPerQuarter = 14;
  const rowHeightPx = 11;
  const keyboardWidthPx = 44;
  const rulerHeightPx = 28;
  const handleWidthPx = 10;

  /** @type {HTMLElement | null} */
  let scrollElement = null;
  /** @type {"move" | "start" | "end" | null} */
  let dragMode = null;
  let dragPointerId = -1;
  let dragStartBeat = 0;
  let dragAnchorStart = 0;
  let dragAnchorEnd = 0;
  let displayStart = loopStart;
  let displayEnd = loopEnd;

  $: if (dragMode === null) {
    displayStart = loopStart;
    displayEnd = loopEnd;
  }

  $: scheduled = buildPhraseSchedule({
    notes,
    rowMuted,
    rowReversed,
    rowTimingOffset,
    stepDurationFraction,
    stepTimingMultiplier,
    stepVelocity,
    stepMuted,
    stepSkipped,
    stepProbability,
    stepCycle,
    stepCycleOffset,
    pulseIndex,
    lengthQuarters,
  });

  $: pitchRange = pitchRangeForSchedule(scheduled);
  $: pitchSpan = pitchRange.maxMidi - pitchRange.minMidi + 1;
  $: rollWidthPx = lengthQuarters * pxPerQuarter;
  $: rollHeightPx = pitchSpan * rowHeightPx;
  $: loopSpan = Math.max(1, displayEnd - displayStart);
  $: loopLeftPx = displayStart * pxPerQuarter;
  $: loopWidthPx = loopSpan * pxPerQuarter;
  $: showPlaybackPlayhead = playbackBeat >= 0;
  $: playbackPlayheadLeftPx = playbackBeat * pxPerQuarter;

  /** @param {number} midi */
  function pitchTopPx(midi) {
    return (pitchRange.maxMidi - midi) * rowHeightPx;
  }

  /** @param {number} start @param {number} end */
  function noteWidthPx(start, end) {
    return Math.max(1, (end - start) * pxPerQuarter - 1);
  }

  /** @param {number} midi */
  function keyboardRowClass(midi) {
    return isBlackKey(midi)
      ? "bg-zinc-950/90 border-b border-zinc-800/80"
      : "bg-zinc-800/70 border-b border-zinc-700/50";
  }

  /** @param {{ enabled?: boolean, start?: number, end?: number }} next */
  async function commitLoopBrace(next) {
    const clamped = clampLoopBrace(
      next.start ?? loopStart,
      next.end ?? loopEnd,
      lengthQuarters,
    );

    await onLoopBraceChange({
      enabled: next.enabled ?? loopEnabled,
      start: clamped.start,
      end: clamped.end,
    });
  }

  /** @param {PointerEvent} event @param {"move" | "start" | "end"} mode */
  function beginLoopDrag(event, mode) {
    if (!scrollElement) return;

    dragMode = mode;
    dragPointerId = event.pointerId;
    dragStartBeat = beatFromClientX(event.clientX, scrollElement, pxPerQuarter);
    dragAnchorStart = displayStart;
    dragAnchorEnd = displayEnd;

    event.currentTarget?.setPointerCapture?.(event.pointerId);
    event.preventDefault();
  }

  /** @param {PointerEvent} event */
  function moveLoopDrag(event) {
    if (dragMode === null || event.pointerId !== dragPointerId || !scrollElement) return;

    const beat = beatFromClientX(event.clientX, scrollElement, pxPerQuarter);

    if (dragMode === "move") {
      const delta = beat - dragStartBeat;

      if (delta === 0) return;

      const span = dragAnchorEnd - dragAnchorStart;
      let nextStart = dragAnchorStart + delta;
      let nextEnd = nextStart + span;

      if (nextStart < 0) {
        nextStart = 0;
        nextEnd = span;
      }

      if (nextEnd > lengthQuarters) {
        nextEnd = lengthQuarters;
        nextStart = nextEnd - span;
      }

      displayStart = nextStart;
      displayEnd = nextEnd;
      return;
    }

    if (dragMode === "start") {
      displayStart = Math.min(beat, displayEnd - 1);
      return;
    }

    displayEnd = Math.max(beat, displayStart + 1);
  }

  /** @param {PointerEvent} event */
  async function endLoopDrag(event) {
    if (dragMode === null || event.pointerId !== dragPointerId) return;

    event.currentTarget?.releasePointerCapture?.(event.pointerId);

    const mode = dragMode;
    dragMode = null;
    dragPointerId = -1;

    await commitLoopBrace({ start: displayStart, end: displayEnd });

    if (mode === "move" && !loopEnabled) {
      await commitLoopBrace({ enabled: true, start: displayStart, end: displayEnd });
    }
  }

  async function toggleLoopEnabled() {
    await commitLoopBrace({ enabled: !loopEnabled });
  }

  /** @param {KeyboardEvent} event */
  async function handleLoopKeydown(event) {
    if (!loopEnabled) return;

    let delta = 0;

    if (event.key === "ArrowLeft") delta = -1;
    if (event.key === "ArrowRight") delta = 1;

    if (delta === 0) return;

    event.preventDefault();

    const span = displayEnd - displayStart;
    let nextStart = displayStart + delta;
    let nextEnd = nextStart + span;

    if (nextStart < 0) {
      nextStart = 0;
      nextEnd = span;
    }

    if (nextEnd > lengthQuarters) {
      nextEnd = lengthQuarters;
      nextStart = nextEnd - span;
    }

    await commitLoopBrace({ start: nextStart, end: nextEnd });
  }

  $: pitchRows = Array.from({ length: pitchSpan }, (_, index) => pitchRange.maxMidi - index);
  $: quarterLines = Array.from({ length: lengthQuarters + 1 }, (_, quarter) => quarter);
  $: barLines = Array.from({ length: Math.floor(lengthQuarters / 4) + 1 }, (_, bar) => bar * 4);
</script>

<section class="mt-6 flex min-h-0 w-full flex-1 flex-col">
  <div class="mb-2 flex shrink-0 items-baseline justify-between gap-3">
    <div class="flex items-center gap-3">
      <p class="text-xs font-medium uppercase tracking-widest text-zinc-500">Output preview</p>
      <button
        type="button"
        class="rounded-md border px-2 py-0.5 text-[10px] font-medium uppercase tracking-wide transition-colors outline-none focus-visible:ring-1 focus-visible:ring-emerald-400 {loopEnabled
          ? 'border-emerald-500/50 bg-emerald-500/15 text-emerald-300'
          : 'border-zinc-700 bg-zinc-900 text-zinc-500 hover:border-zinc-600 hover:text-zinc-300'}"
        onclick={toggleLoopEnabled}
      >
        Loop {loopEnabled ? "on" : "off"}
      </button>
    </div>
    <p class="text-xs text-zinc-600">
      {lengthQuarters} quarter notes · loop {displayStart}–{displayEnd} ({loopSpan} beats)
    </p>
  </div>

  <div
    class="flex h-0 min-h-0 flex-1 flex-col overflow-hidden rounded-xl border border-zinc-800 bg-zinc-950/80"
  >
    <div class="h-0 min-h-0 flex-1 overflow-y-auto overscroll-contain">
      <div class="flex">
        <div
          class="shrink-0 border-r border-zinc-800 bg-zinc-900/90"
          style:width="{keyboardWidthPx}px"
        >
        <div style:height="{rulerHeightPx}px" class="border-b border-zinc-800"></div>
        <div class="relative" style:height="{rollHeightPx}px">
          {#each pitchRows as midi (midi)}
            <div
              class="absolute right-0 left-0 flex items-center justify-end pr-1.5 {keyboardRowClass(midi)}"
              style:top="{pitchTopPx(midi)}px"
              style:height="{rowHeightPx}px"
            >
              {#if midi % 12 === 0}
                <span class="text-[9px] font-medium text-zinc-500">{midiToNoteName(midi)}</span>
              {/if}
            </div>
          {/each}
        </div>
        </div>

        <div
          bind:this={scrollElement}
          class="min-w-0 flex-1 overflow-x-auto overflow-y-hidden"
        >
        <div class="relative" style:width="{rollWidthPx}px">
          <div
            class="relative border-b border-zinc-800 bg-zinc-900/95"
            style:height="{rulerHeightPx}px"
          >
            {#each barLines as bar (bar)}
              <div
                class="pointer-events-none absolute top-0 bottom-0 border-l border-zinc-700/80"
                style:left="{bar * pxPerQuarter}px"
              >
                <span class="absolute top-1 left-1 text-[9px] font-medium text-zinc-500">{bar}</span>
              </div>
            {/each}

            <!-- svelte-ignore a11y_no_noninteractive_tabindex -->
            <!-- svelte-ignore a11y_no_noninteractive_element_interactions -->
            <div
              tabindex="0"
              role="group"
              aria-label="Loop brace"
              class="absolute inset-y-1 z-20 touch-none select-none outline-none"
              style:left="{loopLeftPx}px"
              style:width="{loopWidthPx}px"
              onkeydown={handleLoopKeydown}
              onpointermove={moveLoopDrag}
              onpointerup={endLoopDrag}
              onpointercancel={endLoopDrag}
            >
              <div
                class="absolute inset-x-0 top-0 h-2 rounded-sm border transition-colors {loopEnabled
                  ? 'border-emerald-400/80 bg-emerald-400/90'
                  : 'border-zinc-500/70 bg-zinc-500/60'}"
              >
                <button
                  type="button"
                  aria-label="Loop start"
                  class="absolute top-1/2 left-0 z-30 h-4 w-2.5 -translate-x-1/2 -translate-y-1/2 cursor-ew-resize rounded-sm border border-zinc-900/40 bg-zinc-200 shadow-sm"
                  onpointerdown={(event) => beginLoopDrag(event, "start")}
                ></button>

                <button
                  type="button"
                  aria-label="Move loop brace"
                  class="absolute inset-x-2 inset-y-0 cursor-grab active:cursor-grabbing"
                  onpointerdown={(event) => beginLoopDrag(event, "move")}
                  ondblclick={toggleLoopEnabled}
                ></button>

                <button
                  type="button"
                  aria-label="Loop end"
                  class="absolute top-1/2 right-0 z-30 h-4 w-2.5 translate-x-1/2 -translate-y-1/2 cursor-ew-resize rounded-sm border border-zinc-900/40 bg-zinc-200 shadow-sm"
                  onpointerdown={(event) => beginLoopDrag(event, "end")}
                ></button>
              </div>
            </div>
          </div>

          <div class="relative" style:height="{rollHeightPx}px">
            {#if showPlaybackPlayhead}
              <div
                class="pointer-events-none absolute top-0 bottom-0 z-40 w-px bg-zinc-100/90 shadow-[0_0_6px_rgba(255,255,255,0.35)]"
                style:left="{playbackPlayheadLeftPx}px"
              ></div>
            {/if}

            {#if loopEnabled}
              <div
                class="pointer-events-none absolute inset-y-0 z-10 border-x border-emerald-400/35 bg-emerald-400/8"
                style:left="{loopLeftPx}px"
                style:width="{loopWidthPx}px"
              ></div>
            {/if}

            {#each pitchRows as midi (midi)}
              <div
                class="pointer-events-none absolute right-0 left-0 {keyboardRowClass(midi)}"
                style:top="{pitchTopPx(midi)}px"
                style:height="{rowHeightPx}px"
              ></div>
            {/each}

            {#each quarterLines as quarter (quarter)}
              <div
                class="pointer-events-none absolute top-0 bottom-0 border-l {quarter % 4 === 0
                  ? 'border-zinc-600/70'
                  : 'border-zinc-800/90'}"
                style:left="{quarter * pxPerQuarter}px"
              ></div>
            {/each}

            {#each scheduled as note (`${note.row}-${note.start}-${note.midi}`)}
              {@const noteActive =
                note.velocity > 0 && isScheduledNoteActiveAtBeat(note, playbackBeat)}
              {@const noteAccent = rowAccentFor(note.row, rowColorsEnabled)}
              <div
                class="absolute rounded-[2px] border transition-[box-shadow,border-color,filter] duration-150 {noteActive
                  ? noteAccent.pianoNoteActive
                  : noteAccent.pianoNoteIdle}"
                style:left="{note.start * pxPerQuarter}px"
                style:top="{pitchTopPx(note.midi) + 1}px"
                style:width="{noteWidthPx(note.start, note.end)}px"
                style:height="{rowHeightPx - 2}px"
                style:opacity="{noteActive ? 1 : Math.max(0.2, note.velocity / 127)}"
                title="{midiToNoteName(note.midi)} · vel {note.velocity}"
              ></div>
            {/each}
          </div>
        </div>
        </div>
      </div>
    </div>
  </div>
</section>
