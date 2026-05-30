<script>
  import { longPress } from "./longPressAction.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import StepMutedOverlay from "./StepMutedOverlay.svelte";
  import StepSkippedOverlay from "./StepSkippedOverlay.svelte";

  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  export let muted = false;
  export let flipped = false;
  export let disabled = false;
  export let longPressMs = 800;
  export let surfaceClass = "bg-zinc-900";
  export let borderClass = "border-zinc-700";
  export let headerClass = "border-b border-zinc-800 bg-zinc-800/60";
  export let stepSilenced = false;
  export let stepSkipped = false;
  /** @type {(flipped: boolean) => void} */
  export let onFlipChange = () => {};

  function requestFlip() {
    onFlipChange(!flipped);
  }

  function closeFlip() {
    if (flipped) {
      onFlipChange(false);
    }
  }

  $: longPressOptions = {
    duration: longPressMs,
    disabled,
    onLongPress: requestFlip,
  };
</script>

<div class="flip-scene relative h-full w-full min-w-0" style="perspective: 900px;">
  <div class="flip-inner relative h-full w-full min-w-0 {flipped ? 'is-flipped' : ''}">
    <div
      class="flip-face flip-front relative min-h-0 min-w-0 {flipped
        ? 'pointer-events-none'
        : ''}"
      aria-hidden={flipped}
    >
      <div class="relative h-full min-h-0 w-full min-w-0">
        <slot name="front" />
        <StepSkippedOverlay active={stepSkipped} />
        <StepMutedOverlay active={stepSilenced && !stepSkipped} />
      </div>
    </div>

    <div
      class="flip-face flip-back absolute inset-0 min-h-0 min-w-0 {flipped
        ? ''
        : 'pointer-events-none'}"
      aria-hidden={!flipped}
      use:longPress={longPressOptions}
    >
      <div
        class="relative flex h-full min-w-0 flex-col overflow-hidden rounded-lg border-2 outline-none {surfaceClass} {borderClass} {muted
          ? ''
          : accent.cellFocusWithinBorder}"
      >
        <div
          class="flex h-5 w-full shrink-0 items-center justify-start px-1 {headerClass}"
          use:longPress={longPressOptions}
        >
          <button
            type="button"
            aria-label="Close step settings"
            class="z-10 flex h-4 w-4 shrink-0 items-center justify-start p-0 text-zinc-400 transition-colors outline-none hover:text-zinc-200"
            onpointerdown={(event) => event.stopPropagation()}
            onclick={closeFlip}
          >
            <svg viewBox="0 0 10 10" class="pointer-events-none h-2 w-2" aria-hidden="true">
              <path
                d="M2 2 L8 8 M8 2 L2 8"
                fill="none"
                stroke="currentColor"
                stroke-width="1.75"
                stroke-linecap="round"
              />
            </svg>
          </button>
        </div>

        <div
          class="relative flex min-h-0 flex-1 flex-col gap-1 px-1 py-1 {muted ? 'opacity-80' : ''}"
          use:longPress={longPressOptions}
        >
          <slot name="back" />
          <StepSkippedOverlay active={stepSkipped} />
          <StepMutedOverlay active={stepSilenced && !stepSkipped} />
        </div>
      </div>
    </div>
  </div>
</div>

<style>
  .flip-inner {
    transform-style: preserve-3d;
    transition: transform 0.55s cubic-bezier(0.4, 0, 0.2, 1);
  }

  .flip-inner.is-flipped {
    transform: rotateY(180deg);
  }

  .flip-face {
    backface-visibility: hidden;
    -webkit-backface-visibility: hidden;
  }

  .flip-back {
    transform: rotateY(180deg);
  }
</style>
