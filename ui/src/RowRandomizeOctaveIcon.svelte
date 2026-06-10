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
  fill="none"
  stroke="currentColor"
  stroke-width="2.5"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M7 11 V5.5 M4 7.5 7 4.5 10 7.5" />
  <path d="M17 13 V18.5 M14 16 17 19 20 16" />
</svg>
