<script>
  /** Standard piano-roll edit mode icon. */
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
  <path d="M4 7h16" />
  <path d="M4 12h16" />
  <path d="M4 17h16" />
  <rect x="7" y="9.5" width="7" height="3" rx="0.75" fill="currentColor" stroke="none" />
</svg>
