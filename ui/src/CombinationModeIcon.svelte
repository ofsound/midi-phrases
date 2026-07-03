<script>
  /**
   * Combination-mode glyphs (Cross-Mod, Canon, Retro-Inv, Hocket, Tendril, Echo, Weave).
   *
   * @typedef {'crossMod' | 'canon' | 'retroInv' | 'hocket' | 'tendril' | 'echo' | 'weave'} CombinationModeIconKind
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
  {:else if kind === "canon"}
    <!-- Leader imitated by a delayed follower row. -->
    <path
      d="M4 8 H10.5 C13 8 14.5 11 17 11 H20"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M4 14 H10.5 C13 14 14.5 17 17 17 H20"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.62"
    />
    <circle cx="4" cy="8" r="1.55" fill="currentColor" />
    <circle cx="20" cy="11" r="1.55" fill="currentColor" />
    <circle cx="4" cy="14" r="1.55" fill="currentColor" opacity="0.62" />
    <circle cx="20" cy="17" r="1.55" fill="currentColor" opacity="0.62" />
    <path d="M10.2 5.8 V18.2" stroke="currentColor" stroke-width="1.1" stroke-linecap="round" opacity="0.35" />
    <path d="M13.8 5.8 V18.2" stroke="currentColor" stroke-width="1.1" stroke-linecap="round" opacity="0.35" />
  {:else if kind === "retroInv"}
    <!-- Reversed contour reflected around the following row. -->
    <path
      d="M4 7 C8 7 10 12 12 12 C14 12 16 17 20 17"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M4 17 C8 17 10 12 12 12 C14 12 16 7 20 7"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.58"
    />
    <circle cx="4" cy="7" r="1.5" fill="currentColor" />
    <circle cx="4" cy="17" r="1.5" fill="currentColor" opacity="0.58" />
    <circle cx="20" cy="7" r="1.5" fill="currentColor" opacity="0.58" />
    <circle cx="20" cy="17" r="1.5" fill="currentColor" />
    <path d="M12 5.25 V18.75" stroke="currentColor" stroke-width="1.1" stroke-linecap="round" opacity="0.4" />
    <path
      d="M9.25 10.15 L12 12 L14.75 10.15"
      stroke="currentColor"
      stroke-width="1.1"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.45"
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
  {:else if kind === "tendril"}
    <!-- A source curls toward a neighboring row, then resolves as a small phraselet. -->
    <circle cx="4" cy="12" r="1.55" fill="currentColor" />
    <path
      d="M5.55 12 C8.2 11.9 9.25 8.4 11.75 8.7 C14.1 8.95 14.15 13.25 16.55 13.45 C18.05 13.58 18.9 12.25 20 10.75"
      stroke="currentColor"
      stroke-width="1.45"
      stroke-linecap="round"
      stroke-linejoin="round"
    />
    <path
      d="M8.2 15.6 C10.1 17.55 13.35 17.35 15.2 15.25"
      stroke="currentColor"
      stroke-width="1.2"
      stroke-linecap="round"
      stroke-linejoin="round"
      opacity="0.5"
    />
    <circle cx="11.75" cy="8.7" r="1.25" fill="currentColor" opacity="0.72" />
    <circle cx="16.55" cy="13.45" r="1.35" fill="currentColor" />
    <circle cx="20" cy="10.75" r="1.25" fill="currentColor" opacity="0.58" />
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
