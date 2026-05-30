<script>
  /** When true, draws a diagonal hatch (e.g. over the duration track only). */
  export let active = false;
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
      rgb(0 0 0 / 0.15),
      rgb(0 0 0 / 0.15) 2px,
      rgb(255 255 255 / 0.14) 2px,
      rgb(255 255 255 / 0.14) 5px
    );
    box-shadow: inset 0 0 0 1px rgb(255 255 255 / 0.12);
  }
</style>
