<script>
  
  /**
   * @typedef {Object} Props
   * @property {boolean} [active] - When true, draws a diagonal hatch (e.g. over the duration track only).
   */

  /** @type {Props} */
  let { active = false } = $props();
</script>

{#if active}
  <div
    class="step-muted-overlay pointer-events-none absolute inset-0 z-30 rounded-[inherit]"
    aria-hidden="true"
  ></div>
{/if}

<style>
  .step-muted-overlay {
    background: repeating-linear-gradient(
      -45deg,
      color-mix(in srgb, var(--color-app) 15%, transparent),
      color-mix(in srgb, var(--color-app) 15%, transparent) 2px,
      color-mix(in srgb, var(--color-text) 14%, transparent) 2px,
      color-mix(in srgb, var(--color-text) 14%, transparent) 5px
    );
    box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--color-text) 12%, transparent);
  }
</style>
