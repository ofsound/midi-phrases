<script>
  /** Small edit pencil — row header piano-roll affordance. */
  /** @type {{ class?: string }} */
  let { class: className = "" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="2 2 20 20"
  class="block shrink-0 {className}"
  aria-hidden="true"
  fill="none"
  stroke="currentColor"
  stroke-width="2.25"
  stroke-linecap="round"
  stroke-linejoin="round"
>
  <path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z" />
</svg>
