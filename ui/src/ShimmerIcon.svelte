<script>
  /** Stacked wavy lines for shimmer delay taps — solid base fading upward. */
  /** @type {{ class?: string }} */
  let { class: className = "combination-mode-icon" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
  fill="none"
  stroke="currentColor"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path
    d="M4 7.5 C7 5 10 10 13 7.5 S19 5 20 7.5"
    stroke-width="1.45"
    opacity="0.38"
  />
  <path
    d="M4 12 C7 9.5 10 14.5 13 12 S19 9.5 20 12"
    stroke-width="1.85"
    opacity="0.68"
  />
  <path
    d="M4 16.5 C7 14 10 19 13 16.5 S19 14 20 16.5"
    stroke-width="2.25"
    opacity="1"
  />
</svg>
