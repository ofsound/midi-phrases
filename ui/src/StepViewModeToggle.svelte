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

  const lanePatternId = "step-view-mode-lane-ticks";
  const whiteRingClipId = "step-view-mode-white-ring";

  function toggle() {
    onChange(!compact);
  }
</script>

<div class="relative z-30 flex h-[4.5rem] select-none items-end">
  <button
    type="button"
    class="step-view-mode-frame h-[4.5rem] w-[10.5rem] rounded-full shadow-lg backdrop-blur-md outline-none"
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
      viewBox="0 0 168 72"
      preserveAspectRatio="none"
      aria-hidden="true"
    >
      <defs>
        <clipPath id={whiteRingClipId} clipPathUnits="userSpaceOnUse">
          <path
            fill-rule="evenodd"
            d="M 36 0 H 132 A 36 36 0 0 1 168 36 A 36 36 0 0 1 132 72 H 36 A 36 36 0 0 1 0 36 A 36 36 0 0 1 36 0 Z M 38 8 H 130 A 28 28 0 0 1 158 36 A 28 28 0 0 1 130 64 H 38 A 28 28 0 0 1 10 36 A 28 28 0 0 1 38 8 Z"
          />
        </clipPath>
        <pattern
          id={lanePatternId}
          width="2.8"
          height="14"
          patternUnits="userSpaceOnUse"
        >
          <line
            x1="1.2"
            y1="2"
            x2="1.2"
            y2="12"
            stroke="var(--color-app)"
            stroke-width="0.65"
            stroke-dasharray="1.2 1.6"
            stroke-linecap="butt"
            opacity="0.86"
          />
        </pattern>
      </defs>

      <g clip-path={`url(#${whiteRingClipId})`}>
        <!-- Four horizontal lanes — logo P track-divider rhythm -->
        <rect x="6" y="6" width="156" height="13.5" fill={`url(#${lanePatternId})`} />
        <line x1="6" y1="20.75" x2="162" y2="20.75" stroke="var(--color-app)" stroke-width="0.45" opacity="0.38" />
        <rect x="6" y="21.25" width="156" height="13.5" fill={`url(#${lanePatternId})`} />
        <line x1="6" y1="36" x2="162" y2="36" stroke="var(--color-app)" stroke-width="0.45" opacity="0.38" />
        <rect x="6" y="36.5" width="156" height="13.5" fill={`url(#${lanePatternId})`} />
        <line x1="6" y1="51.25" x2="162" y2="51.25" stroke="var(--color-app)" stroke-width="0.45" opacity="0.38" />
        <rect x="6" y="51.75" width="156" height="13.5" fill={`url(#${lanePatternId})`} />
      </g>
    </svg>

    <div
      class="step-view-mode-track absolute inset-x-2.5 inset-y-2 grid grid-cols-2 rounded-full border border-border/30 bg-zinc-950 p-0.5 shadow-[inset_0_2px_4px_rgba(0,0,0,0.7)]"
      aria-hidden="true"
    >
      <div
        class="pointer-events-none absolute top-1 bottom-1 left-1 w-[calc(50%-0.25rem)] rounded-full transition-transform duration-300 ease-out {compact
          ? 'translate-x-full'
          : ''}"
        style="
          background: linear-gradient(135deg, color-mix(in srgb, var(--color-accent) 85%, #fff), var(--color-accent));
          box-shadow: 0 0 0.75rem color-mix(in srgb, var(--color-accent) 60%, transparent), inset 0 1px 0 rgba(255, 255, 255, 0.4);
          border: 1px solid color-mix(in srgb, var(--color-accent) 90%, #fff);
        "
      ></div>

      <div
        class="icon-slot pointer-events-auto relative z-10 flex items-center justify-center rounded-full transition-colors duration-200 {!compact
          ? 'text-zinc-950'
          : 'text-text-muted/60 hover:text-text-secondary'}"
      >
        <FullStepControlsIcon class="pointer-events-none h-6 w-6" />
      </div>

      <div
        class="icon-slot pointer-events-auto relative z-10 flex items-center justify-center rounded-full transition-colors duration-200 {compact
          ? 'text-zinc-950'
          : 'text-text-muted/60 hover:text-text-secondary'}"
      >
        <StretchToFitIcon class="pointer-events-none h-6 w-6" />
      </div>
    </div>
  </button>
</div>

<style>
  /* Outer racetrack — logo track-shoulder outline + track-bed white fill */
  .step-view-mode-frame {
    position: relative;
    isolation: isolate;
    border: 4px solid var(--color-accent);
    background-color: var(--color-text);
    box-shadow:
      0 0 4px color-mix(in srgb, var(--color-accent) 18%, transparent),
      0 4px 14px color-mix(in srgb, var(--color-app) 55%, transparent);
    filter: drop-shadow(0 1px 1px color-mix(in srgb, var(--color-app) 55%, transparent));
    transition:
      border-color 150ms ease,
      box-shadow 150ms ease;
  }

  .track-bed-lanes {
    z-index: 0;
    overflow: hidden;
  }

  .step-view-mode-track {
    z-index: 1;
  }

  .step-view-mode-frame:hover {
    box-shadow:
      0 0 8px color-mix(in srgb, var(--color-accent) 28%, transparent),
      0 4px 14px color-mix(in srgb, var(--color-app) 55%, transparent);
  }

  .step-view-mode-frame:focus-visible {
    box-shadow:
      0 0 0 2px var(--color-app),
      0 0 0 4px color-mix(in srgb, var(--color-accent) 55%, transparent),
      0 0 8px color-mix(in srgb, var(--color-accent) 28%, transparent);
  }
</style>
