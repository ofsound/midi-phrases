<script>
  /** Two opposing block arrows based on the step skip arrow shape. */
  /** @type {{ class?: string }} */
  let { class: className = "h-5 w-5" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
>
  <path
    fill="currentColor"
    d="M3 5.5 13.5 5.5 13.5 3 22 8.5 13.5 14 13.5 11.5 3 11.5Z"
  />
  <path
    fill="currentColor"
    d="M21 18.5 10.5 18.5 10.5 21 2 15.5 10.5 10 10.5 12.5 21 12.5Z"
  />
</svg>
