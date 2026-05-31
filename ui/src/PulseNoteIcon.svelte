<script>
  /** @type {{ kind?: 'eighth' | 'quarter' | 'half' | 'whole', class?: string }} */
  let { kind = "quarter", class: className = "h-6 w-5" } = $props();
</script>

<svg
  class={className}
  viewBox="0 0 24 32"
  fill="none"
  xmlns="http://www.w3.org/2000/svg"
  aria-hidden="true"
>
  {#if kind === "eighth"}
    <ellipse cx="9" cy="24" rx="6" ry="4.5" fill="currentColor" />
    <path
      d="M14.25 22.5V7"
      stroke="currentColor"
      stroke-width="2"
      stroke-linecap="round"
    />
    <path
      d="M14.25 7C20.5 9.25 20.75 15.5 14.25 18.25"
      stroke="currentColor"
      stroke-width="2"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
  {:else if kind === "quarter"}
    <ellipse cx="9" cy="24" rx="6" ry="4.5" fill="currentColor" />
    <path
      d="M14.25 22.5V7"
      stroke="currentColor"
      stroke-width="2"
      stroke-linecap="round"
    />
  {:else if kind === "half"}
    <ellipse
      cx="9"
      cy="24"
      rx="6"
      ry="4.5"
      fill="none"
      stroke="currentColor"
      stroke-width="1.75"
    />
    <path
      d="M14.25 22.5V7"
      stroke="currentColor"
      stroke-width="2"
      stroke-linecap="round"
    />
  {:else}
    <ellipse
      cx="11"
      cy="22"
      rx="8"
      ry="5.5"
      fill="none"
      stroke="currentColor"
      stroke-width="1.75"
    />
  {/if}
</svg>
