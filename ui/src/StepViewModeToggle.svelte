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

<!-- Ornate physical control deck chassis: shifted down 5px to overflow the header -->
<div 
  class="relative flex flex-col items-center select-none group translate-y-[5px] z-30"
  aria-label="Step view mode"
>
  <!-- Main Ornate Toggle Plate (Oval) -->
  <div 
    class="relative flex items-center justify-center p-2 rounded-full border border-border/30 bg-surface-muted/20 backdrop-blur-md shadow-2xl transition-colors hover:border-border/40"
  >
    
    <!-- Left LED Status Indicator -->
    <div class="flex flex-col items-center justify-center gap-1 pl-3.5 pr-2">
      <div 
        class="h-1.5 w-1.5 rounded-full transition-all duration-300"
        style={!compact 
          ? "background-color: var(--color-accent); box-shadow: 0 0 10px var(--color-accent), 0 0 3px var(--color-accent); transform: scale(1.15);" 
          : "background-color: var(--color-border); opacity: 0.3;"}
      ></div>
      <span class="text-[7px] font-bold text-text-muted/50 uppercase tracking-wider">Full</span>
    </div>

    <!-- The Recessed Track -->
    <div 
      class="relative flex h-10 w-28 items-center rounded-full p-[3px] bg-black/60 border border-border/30 shadow-[inset_0_2px_4px_rgba(0,0,0,0.7)] overflow-hidden"
    >
      <!-- Sliding Active Shuttle -->
      <div 
        class="absolute top-[3px] bottom-[3px] w-[50px] rounded-full transition-all duration-300 cubic-bezier(0.25, 1, 0.5, 1) pointer-events-none active-shuttle"
        style="
          left: {compact ? 'calc(100% - 53px)' : '3px'};
          background: linear-gradient(135deg, color-mix(in srgb, var(--color-accent) 85%, #fff), var(--color-accent));
          box-shadow: 0 0 12px color-mix(in srgb, var(--color-accent) 60%, transparent), inset 0 1px 0 rgba(255, 255, 255, 0.4);
          border: 1px solid color-mix(in srgb, var(--color-accent) 90%, #fff);
        "
      ></div>

      <!-- Left button (Full step controls) -->
      <button
        type="button"
        role="radio"
        aria-checked={!compact}
        aria-label="Full step controls"
        title="Show full step controls with duration, note, and velocity"
        data-cursor="pointer"
        class="relative z-10 flex h-full w-[50px] items-center justify-center rounded-full outline-none transition-colors duration-200 {!compact ? 'text-zinc-950 font-extrabold' : 'text-text-muted/60 hover:text-text-secondary'}"
        onclick={() => {
          if (compact) onChange(false);
        }}
      >
        <FullStepControlsIcon class="pointer-events-none h-5 w-5" />
      </button>

      <!-- Center tiny tick line -->
      <div class="absolute left-1/2 top-1/2 -translate-x-1/2 -translate-y-1/2 w-[1px] h-3 bg-zinc-800/40 pointer-events-none"></div>

      <!-- Right button (Compact steps) -->
      <button
        type="button"
        role="radio"
        aria-checked={compact}
        aria-label="Compact steps"
        title="Stretch compact steps to fit the row width"
        data-cursor="pointer"
        class="relative z-10 flex h-full w-[50px] items-center justify-center rounded-full outline-none transition-colors duration-200 {compact ? 'text-zinc-950 font-extrabold' : 'text-text-muted/60 hover:text-text-secondary'}"
        onclick={() => {
          if (!compact) onChange(true);
        }}
      >
        <StretchToFitIcon class="pointer-events-none h-5 w-5" />
      </button>
    </div>

    <!-- Right LED Status Indicator -->
    <div class="flex flex-col items-center justify-center gap-1 pl-2 pr-3.5">
      <div 
        class="h-1.5 w-1.5 rounded-full transition-all duration-300"
        style={compact 
          ? "background-color: var(--color-accent); box-shadow: 0 0 10px var(--color-accent), 0 0 3px var(--color-accent); transform: scale(1.15);" 
          : "background-color: var(--color-border); opacity: 0.3;"}
      ></div>
      <span class="text-[7px] font-bold text-text-muted/50 uppercase tracking-wider">Comp</span>
    </div>

  </div>
</div>
