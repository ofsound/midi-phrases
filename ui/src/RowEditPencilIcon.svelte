<script>
  /** Small edit pencil — row header piano-roll affordance. */
  /** @type {{ class?: string }} */
  let { class: className = "h-2.5 w-2.5" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 10 10"
  class={className}
  aria-hidden="true"
  fill="none"
  stroke="currentColor"
  stroke-width="1.25"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M6.4 1.4 8.6 3.6 2.9 9.3 1 10l.7-1.9L6.4 1.4z" />
  <path d="M2.5 8.5h5" />
</svg>
