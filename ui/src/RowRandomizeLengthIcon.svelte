<script>
  /** Random step-length multiplier icon for selected steps. */
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
  <text
    x="12"
    y="14.4"
    text-anchor="middle"
    dominant-baseline="middle"
    fill="currentColor"
    font-family="ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace"
    font-size="6.5"
    font-weight="800"
  >
    n*x
  </text>
</svg>
