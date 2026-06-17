<script>
  let {
    value = "100",
    options = [],
    onValueChange = () => {},
  } = $props();

  /** @param {string} next */
  function selectScale(next) {
    if (next !== value) onValueChange(next);
  }
</script>

<div
  role="group"
  aria-label="UI scale"
  class="flex h-8 shrink-0 items-center rounded-md border border-border-subtle bg-field p-0.5"
>
  {#each options as option (option.value)}
    {@const selected = value === option.value}
    <button
      type="button"
      aria-pressed={selected}
      aria-label={`${option.label} percent UI scale`}
      title={`${option.label}% UI scale`}
      data-cursor="pointer"
      class="flex h-7 min-w-6 items-center justify-center rounded-[3px] px-1 text-[9px] font-bold leading-none outline-none transition-[background-color,color,box-shadow] focus-visible:ring-1 focus-visible:ring-focus-ring {selected
        ? 'bg-control-primary text-control-primary-text shadow-[0_0_8px_color-mix(in_srgb,var(--color-control-primary)_45%,transparent)]'
        : 'text-text-faint hover:bg-control-ghost-hover hover:text-text-secondary'}"
      onclick={() => selectScale(option.value)}
    >
      {option.label}
    </button>
  {/each}
</div>
