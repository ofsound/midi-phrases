<script>
  import { doubleClick } from "./doubleClickAction.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {boolean} [muted]
   * @property {boolean} [flipped]
   * @property {boolean} [disabled]
   * @property {string} [surfaceClass]
   * @property {string} [borderClass]
   * @property {string} [headerClass]
   * @property {(flipped: boolean) => void} [onFlipChange]
   * @property {import('svelte').Snippet} [front]
   * @property {import('svelte').Snippet} [backHeader]
   * @property {import('svelte').Snippet} [back]
   * @property {import('svelte').Snippet} [backFooter]
   */

  /** @type {Props} */
  let {
    accent = emeraldRowAccent,
    muted = false,
    flipped = false,
    disabled = false,
    surfaceClass = "bg-surface",
    borderClass = "border-border",
    headerClass = "border-b border-border-subtle bg-surface-muted/60",
    onFlipChange = () => {},
    front,
    backHeader,
    back,
    backFooter,
  } = $props();

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

  let backBodyDoubleClickOptions = $derived({
    disabled: disabled || !flipped,
    shouldIgnore: shouldIgnoreBackPointerDoubleClick,
    onDoubleClick: handleBackBodyDoubleClick,
  });

  let frontPointerClass = $derived(
    flipped ? "pointer-events-none" : "pointer-events-auto",
  );
  let backPointerClass = $derived(
    flipped ? "pointer-events-auto" : "pointer-events-none",
  );
</script>

<div
  class="flip-scene pointer-events-none relative h-full w-full min-w-0"
  style="perspective: 900px;"
>
  <div
    class="flip-inner pointer-events-none relative h-full w-full min-w-0 {flipped ? 'is-flipped' : ''}"
  >
    <div
      class="flip-face flip-front relative min-h-0 min-w-0 {frontPointerClass}"
      aria-hidden={flipped}
    >
      <div class="relative h-full min-h-0 w-full min-w-0">
        {@render front?.()}
      </div>
    </div>

    <div
      class="flip-face flip-back absolute inset-0 min-h-0 min-w-0 {backPointerClass}"
      aria-hidden={!flipped}
    >
      <div
        class="relative flex h-full min-w-0 flex-col overflow-hidden rounded-lg border-2 outline-none transition-[border-color,background-color,box-shadow,opacity] duration-75 {surfaceClass} {borderClass} {muted
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
          {@render backHeader?.()}
        </div>

        <!-- svelte-ignore a11y_no_static_element_interactions -->
        <div
          class="relative flex min-h-0 w-full min-w-0 flex-1 flex-col gap-1 px-1 py-1 {muted
            ? 'opacity-80'
            : ''}"
          use:doubleClick={backBodyDoubleClickOptions}
          ondblclick={handleBackBodyDoubleClick}
          title="Double-click empty area to close step settings"
        >
          {@render back?.()}
        </div>

        {@render backFooter?.()}
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
