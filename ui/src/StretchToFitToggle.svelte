<script>
  import StretchToFitIcon from "./StretchToFitIcon.svelte";

  let {
    enabled = false,
    onChange = () => {},
  } = $props();
</script>

<button
  type="button"
  aria-label={enabled ? "Use full step controls" : "Stretch steps to fit"}
  aria-pressed={enabled}
  title={enabled ? "Show full step controls" : "Stretch steps to fit"}
  data-cursor="pointer"
  class="flex h-8 w-8 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring {enabled
    ? 'text-text'
    : 'text-text-muted'}"
  onclick={() => onChange(!enabled)}
>
  <StretchToFitIcon class="pointer-events-none h-4 w-4" />
</button>
