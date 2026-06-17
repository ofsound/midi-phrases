<script>
  /** Freeform shape-drawing icon for the row piano roll. */
  /** @type {{ class?: string }} */
  let { class: className = "h-4 w-4" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
  fill="none"
  stroke="currentColor"
  stroke-width="2"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M4 18 C8 6, 12 6, 16 12 S20 18, 20 18" />
  <circle cx="4" cy="18" r="1.5" fill="currentColor" stroke="none" />
  <circle cx="10" cy="8" r="1.5" fill="currentColor" stroke="none" />
  <circle cx="16" cy="12" r="1.5" fill="currentColor" stroke="none" />
  <circle cx="20" cy="18" r="1.5" fill="currentColor" stroke="none" />
</svg>
