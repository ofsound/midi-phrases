<script>
  import { pianoKeyboardAspectRatio } from "./pianoKeyboardLayout.js";

  /**
   * @typedef {Object} Props
   * @property {number} whiteCount
   * @property {import('svelte').Snippet} children
   * @property {string} [class]
   */

  /** @type {Props} */
  let { whiteCount, children, class: className = "" } = $props();

  let aspectRatio = $derived(pianoKeyboardAspectRatio(whiteCount));
</script>

<div class="piano-keyboard-fit {className}">
  <div
    class="piano-keyboard-aspect"
    style:--piano-aspect={aspectRatio}
  >
    {@render children()}
  </div>
</div>
