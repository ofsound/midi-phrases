<script>
  /** Left rotation icon for selected step order shift. */
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
  <path d="M16 5.5 19 8.5 16 11.5" />
  <path d="M5 8.5 H19" />
  <path d="M8 18.5 5 15.5 8 12.5" />
  <path d="M19 15.5 H5" />
</svg>
