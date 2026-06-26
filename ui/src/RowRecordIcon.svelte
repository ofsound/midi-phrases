<script>
  /** Record (circle) / stop (square) for row MIDI capture. */
  /** @type {{ class?: string, recording?: boolean, compact?: boolean }} */
  let { class: className = "h-6 w-6", recording = false, compact = false } = $props();

  const viewBox = $derived(
    compact ? (recording ? "7 7 10 10" : "5.5 5.5 13 13") : "0 0 24 24",
  );
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox={viewBox}
  class={className}
  aria-hidden="true"
  fill="currentColor"
>
  {#if recording}
    <rect x="7" y="7" width="10" height="10" rx="1.5" />
  {:else}
    <circle cx="12" cy="12" r="6.5" />
  {/if}
</svg>
