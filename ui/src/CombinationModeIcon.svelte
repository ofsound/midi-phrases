<script>
  /**
   * Combination-mode glyphs (Cross-Mod, Bloom, Counter, Echo, Weave).
   *
   * @typedef {'crossMod' | 'bloom' | 'counter' | 'echo' | 'weave'} CombinationModeIconKind
   * @typedef {Object} Props
   * @property {CombinationModeIconKind} kind
   * @property {string} [class]
   */

  /** @type {Props} */
  let { kind, class: className = "combination-mode-icon" } = $props();
</script>

<svg
  class={className}
  viewBox="0 0 24 24"
  fill="none"
  xmlns="http://www.w3.org/2000/svg"
  aria-hidden="true"
>
  {#if kind === "crossMod"}
    <!-- Attributes cross-routed between phrase rows. -->
    <path d="M7 4 V20" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" opacity="0.55" />
    <path d="M17 4 V20" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" opacity="0.55" />
    <circle cx="7" cy="7" r="1.6" fill="currentColor" />
    <circle cx="7" cy="12" r="1.6" fill="currentColor" />
    <circle cx="7" cy="17" r="1.6" fill="currentColor" />
    <circle cx="17" cy="7" r="1.6" fill="currentColor" />
    <circle cx="17" cy="12" r="1.6" fill="currentColor" />
    <circle cx="17" cy="17" r="1.6" fill="currentColor" />
    <path d="M8.8 7.8 L15.2 11.2" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" />
    <path d="M8.8 12 L15.2 15.4" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" />
    <path d="M8.8 16.2 L15.2 7.8" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" />
    <path d="M14.2 10.6 L15.2 11.2 L14.6 12.2" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" />
    <path d="M14.2 14.8 L15.2 15.4 L14.6 16.4" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" />
    <path d="M14.2 7.2 L15.2 7.8 L14.6 6.8" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" stroke-linejoin="round" />
  {:else if kind === "bloom"}
    <!-- A source onset opens into scale-neighbor ornaments. -->
    <path d="M12 17 V7" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" opacity="0.65" />
    <circle cx="12" cy="17" r="2.1" fill="currentColor" />
    <path
      d="M12 11 C9.5 9.5 7.5 7.5 6.5 5"
      stroke="currentColor"
      stroke-width="1.6"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M12 11 C14.5 9.5 16.5 7.5 17.5 5"
      stroke="currentColor"
      stroke-width="1.6"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <circle cx="6.5" cy="5" r="1.8" fill="currentColor" opacity="0.72" />
    <circle cx="17.5" cy="5" r="1.8" fill="currentColor" opacity="0.72" />
  {:else if kind === "counter"}
    <!-- A main phrase answered by an offset counterline. -->
    <path d="M4 8 H9.5" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" />
    <path d="M14.5 8 H20" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" />
    <circle cx="4" cy="8" r="1.55" fill="currentColor" />
    <circle cx="14.5" cy="8" r="1.55" fill="currentColor" />
    <path
      d="M8 16 C10.5 12.5 13.5 12.5 16 16"
      stroke="currentColor"
      stroke-width="1.7"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.72"
    />
    <circle cx="8" cy="16" r="1.55" fill="currentColor" opacity="0.72" />
    <circle cx="16" cy="16" r="1.55" fill="currentColor" opacity="0.72" />
  {:else if kind === "echo"}
    <!-- One carrier event multiplied into staggered repeats. -->
    <path d="M4 18 H20" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" opacity="0.35" />
    <path d="M5 15 V18" stroke="currentColor" stroke-width="2.75" stroke-linecap="round" />
    <path d="M10 13.5 V18" stroke="currentColor" stroke-width="2.25" stroke-linecap="round" opacity="0.78" />
    <path d="M15 12 V18" stroke="currentColor" stroke-width="1.85" stroke-linecap="round" opacity="0.55" />
    <path d="M19.5 10.5 V18" stroke="currentColor" stroke-width="1.45" stroke-linecap="round" opacity="0.35" />
    <path
      d="M6.5 10.5 C9 8.5 12.5 8.5 18 7.5"
      stroke="currentColor"
      stroke-width="1.35"
      stroke-linecap="round"
      opacity="0.45"
    />
  {:else}
    <!-- Competing same-time events woven into one weighted winner. -->
    <path
      d="M4 6 C9 6 11.5 10 14 12 C11.5 14 9 18 4 18"
      stroke="currentColor"
      stroke-width="1.7"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M4 12 H13"
      stroke="currentColor"
      stroke-width="1.7"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M4 18 C9 18 11.5 14 14 12 C16.5 10 18.5 10 20 10"
      stroke="currentColor"
      stroke-width="1.7"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <circle cx="20" cy="10" r="2.1" fill="currentColor" />
  {/if}
</svg>
