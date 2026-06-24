<script>
  import CompactStepIcon from "./CompactStepIcon.svelte";
  import FullStepControlsIcon from "./FullStepControlsIcon.svelte";
  import { interfaceAccent } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {boolean} [compact] - Compact stretch-to-fit step cells when true; full step controls when false.
   * @property {"default" | "logo"} [size] - "logo" matches the 2rem MIDI Phrases wordmark height.
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(compact: boolean) => void} [onChange]
   */

  /** @type {Props} */
  let {
    compact = false,
    size = "default",
    accent = interfaceAccent,
    onChange = () => {},
  } = $props();

  function toggle() {
    onChange(!compact);
  }

  const lanePatternId = "step-view-mode-lane-ticks";
  const whiteRingClipId = "step-view-mode-white-ring";
</script>

<div
  class="step-view-mode-root relative z-30 flex select-none items-start {size === 'logo'
    ? 'step-view-mode-root-logo'
    : ''}"
>
  <button
    type="button"
    class="step-view-mode-frame rounded-full outline-none"
    aria-label={compact
      ? "Compact steps enabled. Click to show full step controls."
      : "Full step controls enabled. Click to stretch compact steps to fit."}
    aria-pressed={compact}
    title={compact
      ? "Show full step controls with duration, note, and velocity"
      : "Stretch compact steps to fit the row width"}
    data-cursor="pointer"
    onclick={toggle}
  >
    <svg
      class="track-bed-lanes pointer-events-none absolute inset-0 h-full w-full rounded-full"
      viewBox="0 0 134 58"
      preserveAspectRatio="none"
      aria-hidden="true"
    >
      <defs>
        <linearGradient
          id="step-view-mode-white-ring-shade"
          x1="67"
          y1="0"
          x2="67"
          y2="58"
          gradientUnits="userSpaceOnUse"
        >
          <stop offset="0%" stop-color="var(--color-border-subtle)" />
          <stop offset="42%" stop-color="var(--color-text-muted)" />
          <stop offset="100%" stop-color="var(--color-border)" />
        </linearGradient>
        <clipPath id={whiteRingClipId} clipPathUnits="userSpaceOnUse">
          <path
            fill-rule="evenodd"
            d="M 29 0 H 105 A 29 29 0 0 1 134 29 A 29 29 0 0 1 105 58 H 29 A 29 29 0 0 1 0 29 A 29 29 0 0 1 29 0 Z M 31 7 H 103 A 22 22 0 0 1 125 29 A 22 22 0 0 1 103 51 H 31 A 22 22 0 0 1 9 29 A 22 22 0 0 1 31 7 Z"
          />
        </clipPath>
        <pattern
          id={lanePatternId}
          width="2.8"
          height="11"
          patternUnits="userSpaceOnUse"
        >
          <line
            x1="1.2"
            y1="1.5"
            x2="1.2"
            y2="9.5"
            stroke="var(--color-app)"
            stroke-width="0.65"
            stroke-dasharray="1.2 1.6"
            stroke-linecap="butt"
            opacity="0.62"
          />
        </pattern>
      </defs>

      <g clip-path={`url(#${whiteRingClipId})`}>
        <rect x="0" y="0" width="134" height="58" fill="url(#step-view-mode-white-ring-shade)" />
        <rect x="5" y="5" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
        <line x1="5" y1="16.75" x2="129" y2="16.75" stroke="var(--color-app)" stroke-width="0.45" opacity="0.26" />
        <rect x="5" y="17.25" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
        <line x1="5" y1="29" x2="129" y2="29" stroke="var(--color-app)" stroke-width="0.45" opacity="0.26" />
        <rect x="5" y="29.5" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
        <line x1="5" y1="41.25" x2="129" y2="41.25" stroke="var(--color-app)" stroke-width="0.45" opacity="0.26" />
        <rect x="5" y="41.75" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
      </g>
    </svg>

    <div class="step-view-mode-track absolute grid grid-cols-2 rounded-full border border-border/30 bg-black/60" aria-hidden="true">
      <div
        class="step-view-mode-thumb pointer-events-none absolute rounded-full {compact
          ? 'step-view-mode-thumb-compact'
          : ''}"
      ></div>

      <div
        class="icon-slot pointer-events-auto relative z-10 rounded-full {!compact
          ? 'text-zinc-950'
          : 'text-text-muted/60'}"
      >
        <FullStepControlsIcon class="pointer-events-none" />
      </div>

      <div
        class="icon-slot pointer-events-auto relative z-10 rounded-full {compact
          ? 'text-zinc-950'
          : 'text-text-muted/60'}"
      >
        <CompactStepIcon class="pointer-events-none" />
      </div>
    </div>
  </button>
</div>

<style>
  .step-view-mode-root {
    --svm: 0.85;
    --svm-extra-h: 4px;
    height: calc(3.6rem * var(--svm) + var(--svm-extra-h));
  }

  .step-view-mode-root-logo {
    --svm: calc(2rem / 3.6rem);
    --svm-extra-h: 0px;
  }

  /* Outer racetrack — track-bed fill + depth shadow */
  .step-view-mode-frame {
    position: relative;
    isolation: isolate;
    overflow: hidden;
    width: calc(7.35rem * var(--svm));
    height: calc(3.6rem * var(--svm) + var(--svm-extra-h));
    border: none;
    background-color: var(--color-border);
    box-shadow:
      0 calc(3px * var(--svm)) calc(11px * var(--svm)) color-mix(in srgb, var(--color-app) 55%, transparent),
      inset 0 calc(-2px * var(--svm)) calc(5px * var(--svm))
        color-mix(in srgb, var(--color-app) 24%, transparent);
  }

  .track-bed-lanes {
    z-index: 0;
    overflow: hidden;
  }

  .step-view-mode-track {
    --svm-track-pad: calc(0.125rem * var(--svm));
    --svm-thumb-gap: calc(0.25rem * var(--svm));
    z-index: 1;
    contain: layout style paint;
    inset: calc(7px * var(--svm)) calc(10.5px * var(--svm));
    padding: var(--svm-track-pad);
    box-shadow: inset 0 calc(2px * var(--svm)) calc(4px * var(--svm)) rgba(0, 0, 0, 0.7);
  }

  .step-view-mode-thumb {
    top: var(--svm-track-pad);
    bottom: var(--svm-track-pad);
    left: var(--svm-track-pad);
    width: calc((100% - 2 * var(--svm-track-pad) - var(--svm-thumb-gap)) / 2);
    transform: translate3d(0, 0, 0);
    transition: transform 300ms ease-out;
    will-change: transform;
    backface-visibility: hidden;
    border: calc(1px * var(--svm)) solid color-mix(in srgb, var(--color-accent) 86%, #fff);
    background: linear-gradient(
      180deg,
      color-mix(in srgb, var(--color-accent) 68%, #fff) 0%,
      var(--color-accent) 52%,
      color-mix(in srgb, var(--color-accent-strong) 82%, var(--color-accent)) 100%
    );
    box-shadow:
      inset 0 calc(1px * var(--svm)) 0 color-mix(in srgb, #fff 36%, transparent),
      inset 0 calc(-1.5px * var(--svm)) calc(2px * var(--svm))
        color-mix(in srgb, var(--color-accent-strong) 48%, transparent),
      0 calc(1px * var(--svm)) calc(2px * var(--svm))
        color-mix(in srgb, var(--color-accent) 28%, transparent),
      0 0 calc(0.55rem * var(--svm)) color-mix(in srgb, var(--color-accent) 42%, transparent);
  }

  .step-view-mode-thumb-compact {
    /* 100% is thumb width; add the inter-thumb gap so both ovals share the same inset. */
    transform: translate3d(calc(100% + var(--svm-thumb-gap)), 0, 0);
  }

  .icon-slot {
    contain: layout style paint;
    display: grid;
    place-items: center;
    min-width: 0;
    min-height: 0;
  }

  .icon-slot :global(svg) {
    display: block;
    width: calc(1.25rem * var(--svm));
    height: calc(1.25rem * var(--svm));
    flex-shrink: 0;
  }

  .step-view-mode-frame:focus-visible {
    box-shadow:
      0 0 0 calc(2px * var(--svm)) var(--color-focus-ring),
      0 calc(3px * var(--svm)) calc(11px * var(--svm)) color-mix(in srgb, var(--color-app) 55%, transparent),
      inset 0 calc(-2px * var(--svm)) calc(5px * var(--svm))
        color-mix(in srgb, var(--color-app) 24%, transparent);
  }
</style>
