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
  stroke-width="2.5"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M5 6.5 H10.5 M7.5 4 10.5 6.5 7.5 9" />
  <path d="M19 17.5 H13.5 M16.5 15 13.5 17.5 16.5 20" />
</svg>
