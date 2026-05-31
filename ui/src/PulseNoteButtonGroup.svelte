<script>
  import { emeraldRowAccent } from "./rowAccentTheme.js";
  import { defaultPulseIndex, pulseNoteKinds, pulseOptions } from "./pulseLayout.js";
  import PulseNoteIcon from "./PulseNoteIcon.svelte";

  export let value = defaultPulseIndex;
  export let ariaLabel = "Pulse length";
  /** @type {import('./rowAccentTheme.js').RowAccent} */
  export let accent = emeraldRowAccent;
  /** @type {(index: number) => void | Promise<void>} */
  export let onValueChange = () => {};
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
      class="flex h-8 w-8 shrink-0 cursor-pointer items-center justify-center rounded-md border bg-gradient-to-b outline-none transition-[border-color,box-shadow,color,background-color] duration-75 {accent.ringFocusWithWidth} {selected
        ? `${accent.dragBorder} ${accent.dragShadow} from-zinc-600/70 to-zinc-900 text-zinc-100`
        : 'border-zinc-700/90 from-zinc-800/40 to-zinc-950 text-zinc-500 hover:border-zinc-600 hover:text-zinc-300'}"
      onclick={() => {
        if (!selected) onValueChange(option.index);
      }}
    >
      <PulseNoteIcon kind={kind} class="h-5 w-5" />
    </button>
  {/each}
</div>
