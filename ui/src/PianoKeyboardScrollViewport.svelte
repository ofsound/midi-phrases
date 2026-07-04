<script>
  import {
    clampScrollLeft,
    horizontalWheelDeltaPx,
    octaveScrollDeltaPx,
    pianoKeyboardContentWidthPx,
    whiteKeyWidthPx,
  } from "./pianoKeyboardScroll.js";

  /** @typedef {import('./pianoKeyboardScroll.js').PianoKeyboardScrollViewportHandle} PianoKeyboardScrollViewportHandle */

  /**
   * @typedef {Object} Props
   * @property {number} whiteCount
   * @property {number} [initialScrollLeftPx]
   * @property {(scrollLeftPx: number, viewportWidthPx: number, keyWidthPx: number) => void} [onScrollChange]
   * @property {(handle: PianoKeyboardScrollViewportHandle) => void} [onViewportReady]
   * @property {import('svelte').Snippet<[{ keyWidthPx: number }]>} children
   * @property {string} [class]
   */

  /** @type {Props} */
  let {
    whiteCount,
    initialScrollLeftPx = 0,
    onScrollChange = () => {},
    onViewportReady = () => {},
    children,
    class: className = "",
  } = $props();

  /** @type {HTMLElement | null} */
  let scrollElement = null;

  let viewportWidthPx = $state(0);
  let viewportHeightPx = $state(0);
  let scrollLeftPx = $state(0);
  let appliedInitialScrollLeftPx = /** @type {number | null} */ (null);

  let keyWidthPx = $derived(whiteKeyWidthPx(viewportHeightPx));
  let contentWidthPx = $derived(pianoKeyboardContentWidthPx(whiteCount, keyWidthPx));

  /** @returns {PianoKeyboardScrollViewportHandle} */
  function createViewportHandle() {
    return {
      getScrollLeft: () => scrollElement?.scrollLeft ?? scrollLeftPx,
      getViewportWidth: () => scrollElement?.clientWidth ?? viewportWidthPx,
      getKeyWidthPx: () => whiteKeyWidthPx(scrollElement?.clientHeight ?? viewportHeightPx),
      scrollToLeft: (scrollLeft) => {
        applyScrollLeft(scrollLeft);
      },
      scrollByOctave: (octaveDelta) => {
        const currentScrollLeft = scrollElement?.scrollLeft ?? scrollLeftPx;
        const currentKeyWidthPx = whiteKeyWidthPx(scrollElement?.clientHeight ?? viewportHeightPx);

        applyScrollLeft(currentScrollLeft + octaveDelta * octaveScrollDeltaPx(currentKeyWidthPx));
      },
    };
  }

  /** @param {HTMLElement} node */
  function updateViewportMetrics(node) {
    viewportWidthPx = node.clientWidth;
    viewportHeightPx = node.clientHeight;
    scrollLeftPx = node.scrollLeft;
  }

  /** @param {number} scrollLeft */
  function applyScrollLeft(scrollLeft) {
    if (!scrollElement) return;

    const nextScrollLeft = clampScrollLeft(scrollLeft, contentWidthPx, viewportWidthPx);

    scrollElement.scrollLeft = nextScrollLeft;
    scrollLeftPx = nextScrollLeft;
    onScrollChange(nextScrollLeft, viewportWidthPx, keyWidthPx);
  }

  /** @param {HTMLElement} node */
  function scrollElementAttachment(node) {
    scrollElement = node;
    updateViewportMetrics(node);
    onViewportReady(createViewportHandle());

    const resizeObserver = new ResizeObserver(() => {
      updateViewportMetrics(node);
    });
    resizeObserver.observe(node);

    return () => {
      resizeObserver.disconnect();

      if (scrollElement === node) {
        scrollElement = null;
      }
    };
  }

  /** @param {HTMLElement} node */
  function wheelAttachment(node) {
    /** @param {WheelEvent} event */
    function onWheel(event) {
      const delta = horizontalWheelDeltaPx(event);

      if (delta === 0) return;

      event.preventDefault();
      applyScrollLeft(node.scrollLeft + delta);
    }

    node.addEventListener("wheel", onWheel, { passive: false });

    return () => {
      node.removeEventListener("wheel", onWheel);
    };
  }

  /** @param {Event} event */
  function handleScroll(event) {
    const node = /** @type {HTMLElement} */ (event.currentTarget);

    scrollLeftPx = node.scrollLeft;
    onScrollChange(scrollLeftPx, viewportWidthPx, keyWidthPx);
  }

  $effect(() => {
    if (!scrollElement || keyWidthPx <= 0) return;
    if (initialScrollLeftPx === appliedInitialScrollLeftPx) return;

    applyScrollLeft(initialScrollLeftPx);
    appliedInitialScrollLeftPx = initialScrollLeftPx;
  });
</script>

<div
  {@attach scrollElementAttachment}
  {@attach wheelAttachment}
  class="piano-keyboard-scroll {className}"
  onscroll={handleScroll}
>
  <div
    class="piano-keyboard-scroll-surface relative h-full"
    style:width="{contentWidthPx}px"
    style:--piano-white-key-width="{keyWidthPx}px"
  >
    {@render children({ keyWidthPx })}
  </div>
</div>
