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

  const modes = [
    {
      compact: false,
      label: "Full step controls",
      title: "Show full step controls with duration, note, and velocity",
      Icon: FullStepControlsIcon,
    },
    {
      compact: true,
      label: "Compact steps",
      title: "Stretch compact steps to fit the row width",
      Icon: StretchToFitIcon,
    },
  ];
</script>

<div class="flex gap-1" role="radiogroup" aria-label="Step view mode">
  {#each modes as mode (mode.compact ? "compact" : "full")}
    {@const selected = compact === mode.compact}
    <button
      type="button"
      role="radio"
      aria-checked={selected}
      aria-label={mode.label}
      title={mode.title}
      data-cursor="pointer"
      class="flex h-8 w-8 shrink-0 items-center justify-center rounded-md border bg-gradient-to-b outline-none transition-[border-color,box-shadow,color,background-color] duration-75 {accent.ringFocusWithWidth || 'focus-visible:ring-1 focus-visible:ring-focus-ring'} {selected
        ? `${accent.dragBorder} ${accent.dragShadow} from-surface-subtle/70 to-surface text-text`
        : 'border-border/90 from-surface-muted/40 to-app text-text-muted hover:border-border-strong hover:text-text-secondary'}"
      onclick={() => {
        if (!selected) onChange(mode.compact);
      }}
    >
      <mode.Icon class="pointer-events-none h-4 w-4" />
    </button>
  {/each}
</div>
