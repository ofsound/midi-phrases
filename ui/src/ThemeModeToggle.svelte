<script>
  const themeModeOptions = [
    { value: "light", label: "L", title: "Light theme" },
    { value: "dark", label: "D", title: "Dark theme" },
    { value: "alt", label: "A", title: "Alt theme" },
  ];

  let {
    value = "dark",
    onValueChange = () => {},
  } = $props();

  /** @param {string} next */
  function selectThemeMode(next) {
    if (next !== value) {
      onValueChange(next);
    }
  }
</script>

<div
  role="group"
  aria-label="Theme mode"
  class="flex h-5 shrink-0 items-center rounded-sm border border-border-subtle bg-field p-0.5"
>
  {#each themeModeOptions as option (option.value)}
    {@const selected = value === option.value}
    <button
      type="button"
      aria-pressed={selected}
      aria-label={option.title}
      title={option.title}
      data-cursor="pointer"
      class="flex h-4 w-4 items-center justify-center rounded-[2px] text-[9px] leading-none font-bold transition-[background-color,color,box-shadow] outline-none focus-visible:ring-1 focus-visible:ring-focus-ring {selected
        ? 'bg-control-primary text-control-primary-text shadow-[0_0_8px_color-mix(in_srgb,var(--color-control-primary)_45%,transparent)]'
        : 'text-text-faint hover:bg-control-ghost-hover hover:text-text-secondary'}"
      onclick={() => selectThemeMode(option.value)}
    >
      {option.label}
    </button>
  {/each}
</div>
