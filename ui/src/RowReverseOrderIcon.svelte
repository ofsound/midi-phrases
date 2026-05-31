<script>
  /** Opposing arrows for reverse step order. */
  /** @type {{ class?: string }} */
  let { class: className = "h-6 w-6" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
  fill="none"
  stroke="currentColor"
  stroke-width="2.25"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M4 8.5h11M11 5.5 15 8.5 11 11.5" />
  <path d="M20 15.5H9M13 12.5 9 15.5 13 18.5" />
</svg>
