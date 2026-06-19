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
</script>

<div
  class="relative z-30 flex select-none translate-y-[3px]"
  role="radiogroup"
  aria-label="Step view mode"
>
  <div
    class="rounded-full border border-border/30 bg-surface-muted/20 p-1 shadow-lg backdrop-blur-md transition-colors hover:border-border/40"
  >
    <div
      class="relative grid h-8 w-16 grid-cols-2 rounded-full border border-border/30 bg-black/60 p-0.5 shadow-[inset_0_2px_4px_rgba(0,0,0,0.7)]"
    >
      <div
        class="pointer-events-none absolute top-0.5 bottom-0.5 left-0.5 w-[calc(50%-0.125rem)] rounded-full transition-transform duration-300 ease-out {compact
          ? 'translate-x-full'
          : ''}"
        style="
          background: linear-gradient(135deg, color-mix(in srgb, var(--color-accent) 85%, #fff), var(--color-accent));
          box-shadow: 0 0 0.75rem color-mix(in srgb, var(--color-accent) 60%, transparent), inset 0 1px 0 rgba(255, 255, 255, 0.4);
          border: 1px solid color-mix(in srgb, var(--color-accent) 90%, #fff);
        "
      ></div>

      <button
        type="button"
        role="radio"
        aria-checked={!compact}
        aria-label="Full step controls"
        title="Show full step controls with duration, note, and velocity"
        data-cursor="pointer"
        class="relative z-10 flex items-center justify-center rounded-full outline-none transition-colors duration-200 {!compact
          ? 'text-zinc-950'
          : 'text-text-muted/60 hover:text-text-secondary'}"
        onclick={() => {
          if (compact) onChange(false);
        }}
      >
        <FullStepControlsIcon class="pointer-events-none h-4 w-4" />
      </button>

      <button
        type="button"
        role="radio"
        aria-checked={compact}
        aria-label="Compact steps"
        title="Stretch compact steps to fit the row width"
        data-cursor="pointer"
        class="relative z-10 flex items-center justify-center rounded-full outline-none transition-colors duration-200 {compact
          ? 'text-zinc-950'
          : 'text-text-muted/60 hover:text-text-secondary'}"
        onclick={() => {
          if (!compact) onChange(true);
        }}
      >
        <StretchToFitIcon class="pointer-events-none h-4 w-4" />
      </button>
    </div>
  </div>
</div>
