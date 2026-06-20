<script>
  import FullStepControlsIcon from "./FullStepControlsIcon.svelte";
  import StretchToFitIcon from "./StretchToFitIcon.svelte";
  import { interfaceAccent } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {boolean} [compact] - Compact stretch-to-fit step cells when true; full step controls when false.
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(compact: boolean) => void} [onChange]
   */

  /** @type {Props} */
  let {
    compact = false,
    accent = interfaceAccent,
    onChange = () => {},
  } = $props();

  function toggle() {
    onChange(!compact);
  }

  const lanePatternId = "step-view-mode-lane-ticks";
  const whiteRingClipId = "step-view-mode-white-ring";
</script>

<div class="relative z-30 flex h-[3.6rem] select-none items-end">
  <button
    type="button"
    class="step-view-mode-frame h-[3.6rem] w-[8.4rem] rounded-full shadow-lg backdrop-blur-md outline-none"
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
          <stop offset="0%" stop-color="var(--color-surface-subtle)" />
          <stop offset="40%" stop-color="var(--color-text)" />
          <stop offset="100%" stop-color="var(--color-border-subtle)" />
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
            opacity="0.86"
          />
        </pattern>
      </defs>

      <g clip-path={`url(#${whiteRingClipId})`}>
        <rect x="0" y="0" width="134" height="58" fill="url(#step-view-mode-white-ring-shade)" />
        <rect x="5" y="5" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
        <line x1="5" y1="16.75" x2="129" y2="16.75" stroke="var(--color-app)" stroke-width="0.45" opacity="0.38" />
        <rect x="5" y="17.25" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
        <line x1="5" y1="29" x2="129" y2="29" stroke="var(--color-app)" stroke-width="0.45" opacity="0.38" />
        <rect x="5" y="29.5" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
        <line x1="5" y1="41.25" x2="129" y2="41.25" stroke="var(--color-app)" stroke-width="0.45" opacity="0.38" />
        <rect x="5" y="41.75" width="124" height="10.5" fill={`url(#${lanePatternId})`} />
      </g>
    </svg>

    <div
      class="step-view-mode-track absolute inset-x-[9px] inset-y-[7px] grid grid-cols-2 rounded-full border border-border/30 bg-black/60 p-0.5 shadow-[inset_0_2px_4px_rgba(0,0,0,0.7)]"
      aria-hidden="true"
    >
      <div
        class="step-view-mode-thumb pointer-events-none absolute top-0.5 bottom-0.5 left-0.5 w-[calc(50%-0.2rem)] rounded-full {compact
          ? 'step-view-mode-thumb-compact'
          : ''}"
      ></div>

      <div
        class="icon-slot pointer-events-auto relative z-10 flex shrink-0 items-center justify-center rounded-full {!compact
          ? 'text-white'
          : 'text-text-muted/60 hover:text-text-secondary'}"
      >
        <FullStepControlsIcon class="pointer-events-none block h-5 w-5 shrink-0" />
      </div>

      <div
        class="icon-slot pointer-events-auto relative z-10 flex shrink-0 items-center justify-center rounded-full {compact
          ? 'text-white'
          : 'text-text-muted/60 hover:text-text-secondary'}"
      >
        <StretchToFitIcon class="pointer-events-none block h-5 w-5 shrink-0" />
      </div>
    </div>
  </button>
</div>

<style>
  /* Outer racetrack — soft accent glow + track-bed white fill */
  .step-view-mode-frame {
    position: relative;
    isolation: isolate;
    border: none;
    background-color: var(--color-text);
    box-shadow:
      0 0 7px color-mix(in srgb, var(--color-accent) 38%, transparent),
      0 0 16px color-mix(in srgb, var(--color-accent) 20%, transparent),
      0 0 26px color-mix(in srgb, var(--color-accent) 10%, transparent),
      0 3px 11px color-mix(in srgb, var(--color-app) 55%, transparent),
      inset 0 -2px 5px color-mix(in srgb, var(--color-app) 24%, transparent);
    transition: box-shadow 150ms ease;
  }

  .step-view-mode-frame::before {
    content: "";
    position: absolute;
    inset: -1px;
    border-radius: inherit;
    pointer-events: none;
    z-index: -1;
    box-shadow:
      0 0 10px color-mix(in srgb, var(--color-accent) 42%, transparent),
      0 0 20px color-mix(in srgb, var(--color-accent) 22%, transparent);
  }

  .step-view-mode-frame::after {
    content: "";
    position: absolute;
    inset: 1px;
    border-radius: inherit;
    pointer-events: none;
    z-index: 0;
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, #fff 38%, transparent),
      inset 0 -1px 2px color-mix(in srgb, var(--color-app) 18%, transparent);
    mask-image: radial-gradient(ellipse 58% 52% at 50% 50%, transparent 68%, #000 70%);
    -webkit-mask-image: radial-gradient(ellipse 58% 52% at 50% 50%, transparent 68%, #000 70%);
  }

  .track-bed-lanes {
    z-index: 0;
    overflow: hidden;
  }

  .step-view-mode-track {
    z-index: 1;
    contain: layout style paint;
  }

  .step-view-mode-thumb {
    transform: translate3d(0, 0, 0);
    transition: transform 300ms ease-out;
    will-change: transform;
    backface-visibility: hidden;
    border: 1px solid color-mix(in srgb, var(--color-accent) 86%, #fff);
    background: linear-gradient(
      180deg,
      color-mix(in srgb, var(--color-accent) 68%, #fff) 0%,
      var(--color-accent) 52%,
      color-mix(in srgb, var(--color-accent-strong) 82%, var(--color-accent)) 100%
    );
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, #fff 36%, transparent),
      inset 0 -1.5px 2px color-mix(in srgb, var(--color-accent-strong) 48%, transparent),
      0 1px 2px color-mix(in srgb, var(--color-accent) 28%, transparent),
      0 0 0.55rem color-mix(in srgb, var(--color-accent) 42%, transparent);
  }

  .step-view-mode-thumb-compact {
    transform: translate3d(100%, 0, 0);
  }

  .icon-slot {
    contain: layout style paint;
  }

  .step-view-mode-frame:hover {
    box-shadow:
      0 0 9px color-mix(in srgb, var(--color-accent) 46%, transparent),
      0 0 18px color-mix(in srgb, var(--color-accent) 26%, transparent),
      0 0 28px color-mix(in srgb, var(--color-accent) 14%, transparent),
      0 3px 11px color-mix(in srgb, var(--color-app) 55%, transparent),
      inset 0 -2px 5px color-mix(in srgb, var(--color-app) 24%, transparent);
  }

  .step-view-mode-frame:hover::before {
    box-shadow:
      0 0 12px color-mix(in srgb, var(--color-accent) 50%, transparent),
      0 0 24px color-mix(in srgb, var(--color-accent) 28%, transparent);
  }

  .step-view-mode-frame:focus-visible {
    box-shadow:
      0 0 0 2px var(--color-app),
      0 0 8px color-mix(in srgb, var(--color-accent) 40%, transparent),
      0 0 18px color-mix(in srgb, var(--color-accent) 22%, transparent),
      0 0 28px color-mix(in srgb, var(--color-accent) 12%, transparent);
  }
</style>
