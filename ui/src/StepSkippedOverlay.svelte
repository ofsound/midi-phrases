<script>
  /**
   * @typedef {Object} Props
   * @property {boolean} [active] - When true, dims the step with a strong inset shadow (skipped).
   */

  /** @type {Props} */
  let { active = false } = $props();
</script>

{#if active}
  <div
    class="step-skipped-overlay pointer-events-none absolute inset-0 z-30 rounded-[inherit]"
    aria-hidden="true"
  ></div>
{/if}

<style>
  .step-skipped-overlay {
    background: color-mix(in srgb, var(--color-app) 44%, transparent);
    box-shadow:
      inset 0 3px 10px 1px color-mix(in srgb, var(--color-app) 52%, transparent),
      inset 0 1px 2px color-mix(in srgb, var(--color-app) 32%, transparent),
      inset 0 0 0 1px color-mix(in srgb, var(--color-app) 24%, transparent);
  }
</style>
