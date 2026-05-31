<script>
  /** Dice icon for row step-order randomization. */
  /** @type {{ class?: string }} */
  let { class: className = "h-6 w-6" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
>
  <rect
    x="4"
    y="4"
    width="16"
    height="16"
    rx="3"
    fill="none"
    stroke="currentColor"
    stroke-width="2.25"
  />
  <circle cx="8.5" cy="8.5" r="1.3" fill="currentColor" />
  <circle cx="15.5" cy="8.5" r="1.3" fill="currentColor" />
  <circle cx="12" cy="12" r="1.3" fill="currentColor" />
  <circle cx="8.5" cy="15.5" r="1.3" fill="currentColor" />
  <circle cx="15.5" cy="15.5" r="1.3" fill="currentColor" />
</svg>
