<script>
  /**
   * Combination-mode glyphs (Cross-Mod, Hocket, Bloom, Counter, Echo, Round Robin, Weave).
   *
   * @typedef {'crossMod' | 'hocket' | 'bloom' | 'counter' | 'echo' | 'roundRobin' | 'weave'} CombinationModeIconKind
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
    <!-- Carrier row timing; pitch, velocity, and duration cross from other rows. -->
    <circle cx="4" cy="7" r="1.6" fill="currentColor" />
    <circle cx="4" cy="12" r="1.6" fill="currentColor" />
    <circle cx="4" cy="17" r="1.6" fill="currentColor" />
    <circle cx="20" cy="7" r="1.6" fill="currentColor" />
    <circle cx="20" cy="12" r="1.6" fill="currentColor" />
    <circle cx="20" cy="17" r="1.6" fill="currentColor" />
    <path
      d="M5.6 7 C10 7 14 9.5 18.4 12"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 12 C10 12 14 14.5 18.4 17"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 17 C10 17 14 14.5 18.4 7"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
  {:else if kind === "hocket"}
    <!-- Long gestures broken into alternating row handoffs. -->
    <path d="M4 6 H7.25" stroke="currentColor" stroke-width="1.35" stroke-linecap="round" opacity="0.7" />
    <path d="M4 12 H7.25" stroke="currentColor" stroke-width="1.35" stroke-linecap="round" opacity="0.7" />
    <path d="M4 18 H7.25" stroke="currentColor" stroke-width="1.35" stroke-linecap="round" opacity="0.7" />
    <path
      d="M7.25 6 C10 6 10 12 12.75 12 C15.5 12 15.5 18 18.25 18"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M7.25 18 C10 18 10 12 12.75 12 C15.5 12 15.5 6 18.25 6"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.55"
    />
    <circle cx="4" cy="6" r="1.45" fill="currentColor" opacity="0.7" />
    <circle cx="4" cy="12" r="1.45" fill="currentColor" opacity="0.7" />
    <circle cx="4" cy="18" r="1.45" fill="currentColor" opacity="0.7" />
    <path d="M11.7 9.75 V14.25" stroke="currentColor" stroke-width="1.15" stroke-linecap="round" opacity="0.4" />
    <path d="M14.3 9.75 V14.25" stroke="currentColor" stroke-width="1.15" stroke-linecap="round" opacity="0.4" />
    <circle cx="20" cy="6" r="1.45" fill="currentColor" />
    <circle cx="20" cy="12" r="1.45" fill="currentColor" />
    <circle cx="20" cy="18" r="1.45" fill="currentColor" />
  {:else if kind === "bloom"}
    <!-- Source onset with trailing scale-neighbor ornaments. -->
    <circle cx="4" cy="12" r="1.6" fill="currentColor" />
    <path
      d="M5.6 12 H12"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M12 12 C14.5 10.5 16.5 9.25 18.65 8.75"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M12 12 C14.5 13.5 16.5 14.75 18.65 15.25"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <circle cx="20" cy="8.75" r="1.35" fill="currentColor" />
    <circle cx="20" cy="15.25" r="1.35" fill="currentColor" />
  {:else if kind === "counter"}
    <!-- Call on the beat, offbeat response after. -->
    <path
      d="M3 11 H21"
      stroke="currentColor"
      stroke-width="1.2"
      stroke-linecap="round"
      opacity="0.45"
    />
    <circle cx="4" cy="11" r="1.6" fill="currentColor" />
    <path
      d="M5.6 11 C10 11 14 13.5 18.4 16"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <circle cx="20" cy="16" r="1.6" fill="currentColor" />
    <path d="M20 11 V13.5" stroke="currentColor" stroke-width="1.2" stroke-linecap="round" opacity="0.45" />
  {:else if kind === "echo"}
    <!-- One carrier event multiplied into staggered, fading repeats. -->
    <path
      d="M4 18 H20"
      stroke="currentColor"
      stroke-width="1.2"
      stroke-linecap="round"
      opacity="0.35"
    />
    <path d="M5.5 13 V18" stroke="currentColor" stroke-width="1.45" stroke-linecap="round" />
    <path d="M9.5 12 V18" stroke="currentColor" stroke-width="1.45" stroke-linecap="round" opacity="0.72" />
    <path d="M13.5 10.75 V18" stroke="currentColor" stroke-width="1.45" stroke-linecap="round" opacity="0.5" />
    <path d="M17.5 9.5 V18" stroke="currentColor" stroke-width="1.45" stroke-linecap="round" opacity="0.32" />
  {:else if kind === "roundRobin"}
    <!-- Phrases advance one row at a time with a short weighted overlap. -->
    <path
      d="M4 7 H8.5 C10.4 7 11.6 9 13.5 9 H20"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M4 13 H10.5 C12.4 13 13.6 15 15.5 15 H20"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.72"
    />
    <circle cx="4" cy="7" r="1.45" fill="currentColor" />
    <circle cx="4" cy="13" r="1.45" fill="currentColor" opacity="0.72" />
    <circle cx="20" cy="9" r="1.45" fill="currentColor" />
    <circle cx="20" cy="15" r="1.45" fill="currentColor" opacity="0.72" />
    <path
      d="M10.5 18.25 C12.5 20 15.8 20 17.75 18.25"
      stroke="currentColor"
      stroke-width="1.2"
      stroke-linecap="round"
      opacity="0.45"
    />
    <path
      d="M17.75 18.25 H15.45 M17.75 18.25 V20.55"
      stroke="currentColor"
      stroke-width="1.2"
      stroke-linecap="round"
      opacity="0.45"
    />
  {:else}
    <!-- Competing same-time events woven into one weighted winner. -->
    <circle cx="4" cy="4.5" r="1.6" fill="currentColor" />
    <circle cx="4" cy="8.25" r="1.6" fill="currentColor" />
    <circle cx="4" cy="12" r="1.6" fill="currentColor" />
    <circle cx="4" cy="15.75" r="1.6" fill="currentColor" />
    <circle cx="4" cy="19.5" r="1.6" fill="currentColor" />
    <path
      d="M5.6 4.5 C9 4.5 11 8.25 13.5 12"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 8.25 C8.75 8.75 11 10.25 13.5 12"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 12 H13.5"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 15.75 C8.75 13.25 11 13.75 13.5 12"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M5.6 19.5 C9 19.5 11 15.75 13.5 12"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M13.5 12 H18.4"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <circle cx="20" cy="12" r="1.6" fill="currentColor" />
  {/if}
</svg>
