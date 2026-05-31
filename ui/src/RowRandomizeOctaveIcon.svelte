<script>
  /** Octave-shift randomization icon for row notes. */
  /** @type {{ class?: string }} */
  let { class: className = "h-6 w-6" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
>
  <path
    d="M12 6v9"
    fill="none"
    stroke="currentColor"
    stroke-width="2.25"
    stroke-linecap="round"
  />
  <ellipse
    cx="9"
    cy="16"
    rx="3"
    ry="2.4"
    fill="none"
    stroke="currentColor"
    stroke-width="2.25"
  />
  <path
    d="M6.5 8.5 4 6l2.5-2.5M4 6h4"
    fill="none"
    stroke="currentColor"
    stroke-width="2.25"
    stroke-linecap="round"
    stroke-linejoin="round"
  />
  <path
    d="m17.5 15.5 2.5 2.5-2.5 2.5M20 18h-4"
    fill="none"
    stroke="currentColor"
    stroke-width="2.25"
    stroke-linecap="round"
    stroke-linejoin="round"
  />
  <circle cx="17.5" cy="6" r="1.3" fill="currentColor" />
  <circle cx="20.5" cy="6" r="1.3" fill="currentColor" />
</svg>
