<script>
  /** Right-pointing arrow: thin shaft joined to center of triangular head. */
  /** @type {{ class?: string }} */
  let { class: className = "h-3 w-3" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
>
  <path
    fill="currentColor"
    d="M3 9 10 9 10 4 21 12 10 20 10 15 3 15Z"
  />
</svg>
