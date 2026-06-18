<script>
  import muteIconSrc from "./assets/mute-icon.png";

  /** Speaker with X — step footer mute (from assets/mute-icon.png). */
  /** @type {{ class?: string }} */
  let { class: className = "h-3 w-3" } = $props();
</script>

<span
  class="step-mute-icon inline-block shrink-0 bg-current {className}"
  style:--step-mute-icon="url({muteIconSrc})"
  aria-hidden="true"
></span>

<style>
  .step-mute-icon {
    transform: scale(0.68);
    transform-origin: center;
    mask-image: var(--step-mute-icon);
    -webkit-mask-image: var(--step-mute-icon);
    mask-size: contain;
    -webkit-mask-size: contain;
    mask-repeat: no-repeat;
    -webkit-mask-repeat: no-repeat;
    mask-position: center;
    -webkit-mask-position: center;
  }
</style>
