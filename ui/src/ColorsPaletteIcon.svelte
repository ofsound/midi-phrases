<script>
  /** Painter palette icon for row color toggle. */
  /** @type {{ class?: string }} */
  let { class: className = "h-4 w-4" } = $props();
</script>

<svg
  xmlns="http://www.w3.org/2000/svg"
  viewBox="0 0 24 24"
  class={className}
  aria-hidden="true"
>
  <path
    d="M17.8 8.2C19.8 10.2 20.2 13.1 18.9 15.7c-1 2-2.7 3.1-4.3 3.6l-.8.25q-1.6 1.8-3.2 0l-.8-.25c-1.6-.5-3.3-1.6-4.3-3.6C4.2 13.1 4.6 10.2 6.6 8.2 8.4 6.6 10.5 5.75 12.2 5.75S16 6.6 17.8 8.2Z"
    fill="none"
    stroke="currentColor"
    stroke-width="2.25"
    stroke-linejoin="round"
  />
  <circle cx="8.75" cy="9.25" r="1.2" fill="currentColor" />
  <circle cx="11.25" cy="7.75" r="1.2" fill="currentColor" />
  <circle cx="13.75" cy="7.75" r="1.2" fill="currentColor" />
  <circle cx="16.25" cy="9.25" r="1.2" fill="currentColor" />
</svg>
