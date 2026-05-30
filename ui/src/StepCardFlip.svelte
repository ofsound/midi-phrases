<script>
  import { doubleClick } from "./doubleClickAction.js";
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

  /** @param {MouseEvent} event */
  function shouldIgnoreBackDoubleClick(event) {
    const target = event.target;

    if (!(target instanceof Element)) return true;

    return Boolean(
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [role='slider'], [data-no-flip-close]",
      ),
    );
  }

  /** @param {MouseEvent} event */
  function handleBackHeaderDoubleClick(event) {
    if (disabled || !flipped || shouldIgnoreBackDoubleClick(event)) return;

    event.preventDefault();
    closeFlip();
  }

  /** @param {MouseEvent | PointerEvent} event */
  function handleBackBodyDoubleClick(event) {
    if (disabled || !flipped || shouldIgnoreBackDoubleClick(event)) return;

    event.preventDefault();
    closeFlip();
  }

  /** @param {PointerEvent} event */
  function shouldIgnoreBackPointerDoubleClick(event) {
    return shouldIgnoreBackDoubleClick(event);
  }

  $: backBodyDoubleClickOptions = {
    disabled: disabled || !flipped,
    shouldIgnore: shouldIgnoreBackPointerDoubleClick,
    onDoubleClick: handleBackBodyDoubleClick,
  };

  $: longPressOptions = {
    duration: longPressMs,
    disabled,
    onLongPress: requestFlip,
  };

  /**
   * Each face stays visible for its outgoing flip, then hides after transitionend
   * so WebView does not flash blank or bleed the hidden face (display:none breaks 3D).
   */
  let frontHidden = false;
  let backHidden = true;

  $: if (flipped) {
    backHidden = false;
  } else {
    frontHidden = false;
  }

  /** @param {TransitionEvent} event */
  function handleFlipTransitionEnd(event) {
    if (event.propertyName !== "transform") return;

    frontHidden = flipped;
    backHidden = !flipped;
  }
</script>

<div
  class="flip-scene pointer-events-none relative h-full w-full min-w-0"
  style="perspective: 900px;"
>
  <div
    class="flip-inner pointer-events-none relative h-full w-full min-w-0 {flipped ? 'is-flipped' : ''}"
    ontransitionend={handleFlipTransitionEnd}
  >
    <div
      class="flip-face flip-front relative min-h-0 min-w-0 {frontHidden
        ? 'pointer-events-none hidden'
        : flipped
          ? 'pointer-events-none'
          : 'pointer-events-auto'}"
      aria-hidden={flipped}
    >
      <div class="relative h-full min-h-0 w-full min-w-0">
        <slot name="front" />
        <StepSkippedOverlay active={stepSkipped && !flipped} />
        <StepMutedOverlay active={stepSilenced && !stepSkipped && !flipped} />
      </div>
    </div>

    <div
      class="flip-face flip-back absolute inset-0 min-h-0 min-w-0 {backHidden
        ? 'pointer-events-none hidden'
        : flipped
          ? 'pointer-events-auto'
          : 'pointer-events-none'}"
      aria-hidden={!flipped}
    >
      <div
        class="relative flex h-full min-w-0 flex-col overflow-hidden rounded-lg border-2 outline-none {surfaceClass} {borderClass} {muted
          ? ''
          : accent.cellFocusWithinBorder}"
      >
        <!-- svelte-ignore a11y_no_static_element_interactions -->
        <div
          class="flex h-5 w-full shrink-0 items-center justify-start gap-1 px-1 {headerClass}"
          use:doubleClick={backBodyDoubleClickOptions}
          ondblclick={handleBackHeaderDoubleClick}
          title="Double-click to close step settings"
          aria-label="Step settings. Double-click to close."
        >
          <slot name="back-header" />
        </div>

        <!-- svelte-ignore a11y_no_static_element_interactions -->
        <div
          class="relative flex min-h-0 w-full min-w-0 flex-1 flex-col gap-1 px-1 py-1 {muted
            ? 'opacity-80'
            : ''}"
          use:longPress={longPressOptions}
          use:doubleClick={backBodyDoubleClickOptions}
          ondblclick={handleBackBodyDoubleClick}
          title="Double-click empty area to close step settings"
        >
          <slot name="back" />
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
    transform: translateZ(0.1px);
  }

  .flip-back {
    transform: rotateY(180deg) translateZ(0.1px);
  }
</style>
