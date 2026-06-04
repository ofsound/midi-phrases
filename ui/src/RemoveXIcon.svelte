<script>
  /** Rounded-stroke X — step remove and pattern clear. */
  /** @type {{ class?: string }} */
  let { class: className = "h-2 w-2" } = $props();
</script>

<svg viewBox="0 0 10 10" class={className} aria-hidden="true">
  <path
    d="M2 2 L8 8 M8 2 L2 8"
    fill="none"
    stroke="currentColor"
    stroke-width="1.75"
    stroke-linecap="round"
  />
</svg>
