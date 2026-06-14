<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { defaultPulseIndex, pulseNoteKinds, pulseOptions } from "./pulseLayout.js";
  import PulseNoteIcon from "./PulseNoteIcon.svelte";

  
  
  /**
   * @typedef {Object} Props
   * @property {any} [value]
   * @property {string} [ariaLabel]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {(index: number) => void | Promise<void>} [onValueChange]
   */

  /** @type {Props} */
  let {
    value = defaultPulseIndex,
    ariaLabel = "Pulse length",
    accent = emeraldRowAccent,
    onValueChange = () => {}
  } = $props();
</script>

<div class="flex gap-1" role="radiogroup" aria-label={ariaLabel}>
  {#each pulseOptions as option (option.index)}
    {@const selected = value === option.index}
    {@const kind = pulseNoteKinds[option.index] ?? "quarter"}
    <button
      type="button"
      role="radio"
      aria-checked={selected}
      aria-label={option.label}
      title={option.label}
      data-cursor="pointer"
      class="flex h-8 w-8 shrink-0 items-center justify-center rounded-md border bg-gradient-to-b outline-none transition-[border-color,box-shadow,color,background-color] duration-75 {accent.ringFocusWithWidth} {selected
        ? `${accent.dragBorder} ${accent.dragShadow} from-surface-subtle/70 to-surface text-text`
        : 'border-border/90 from-surface-muted/40 to-app text-text-muted hover:border-border-strong hover:text-text-secondary'}"
      onclick={() => {
        if (!selected) onValueChange(option.index);
      }}
    >
      <PulseNoteIcon kind={kind} class="h-5 w-5" />
    </button>
  {/each}
</div>
