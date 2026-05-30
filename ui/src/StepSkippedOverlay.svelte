<script>
  /** When true, draws horizontal skip stripes over the step card. */
  export let active = false;
</script>

{#if active}
  <div
    class="step-skipped-overlay pointer-events-none absolute inset-0 z-30 rounded-[inherit]"
    aria-hidden="true"
  ></div>
{/if}

<style>
  .step-skipped-overlay {
    background: repeating-linear-gradient(
      0deg,
      rgb(0 0 0 / 0.22),
      rgb(0 0 0 / 0.22) 3px,
      rgb(148 163 184 / 0.16) 3px,
      rgb(148 163 184 / 0.16) 7px
    );
    box-shadow: inset 0 0 0 2px rgb(148 163 184 / 0.35);
  }
</style>
