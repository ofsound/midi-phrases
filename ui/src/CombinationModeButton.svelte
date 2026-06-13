<script>
  /**
   * Hardware-style rail toggle with immediate pressed feedback.
   * Defers the toggle callback so paint can run before heavy parent work.
   *
   * @typedef {Object} Props
   * @property {boolean} pressed
   * @property {string} [ariaLabel]
   * @property {string} [title]
   * @property {string} [class]
   * @property {string} [faceClass]
   * @property {() => void | Promise<void>} [onToggle]
   * @property {import('svelte').Snippet} [children]
   */

  /** @type {Props} */
  let {
    pressed = false,
    ariaLabel = "",
    title = "",
    class: className = "",
    faceClass = "",
    onToggle = () => {},
    children,
  } = $props();

  /** @type {boolean | null} */
  let optimisticPressed = $state(null);

  let displayPressed = $derived(optimisticPressed ?? pressed);

  $effect(() => {
    pressed;
    optimisticPressed = null;
  });

  /** @param {PointerEvent} event */
  function onPointerDown(event) {
    event.preventDefault();
    optimisticPressed = !displayPressed;

    requestAnimationFrame(() => {
      onToggle();
    });
  }
</script>

<button
  type="button"
  class="combination-mode-button {className}"
  aria-label={ariaLabel}
  aria-pressed={displayPressed}
  {title}
  data-cursor="pointer"
  onpointerdown={onPointerDown}
>
  <span class="combination-mode-button-face {faceClass}">
    {@render children?.()}
  </span>
</button>
