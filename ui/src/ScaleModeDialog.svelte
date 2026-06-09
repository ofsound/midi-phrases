<script>
  import { keyCenters, scaleModes, scaleName } from "./scaleUtils.js";

  let {
    root = 0,
    modeIndex = 0,
    patternSlot = 0,
    onClose = () => {},
    onChange = () => {},
  } = $props();

  let titleId = "scale-mode-title";
  let selectedName = $derived(scaleName(root, modeIndex));

  function closeOnEscape(event) {
    if (event.key !== "Escape") return;

    event.preventDefault();
    onClose();
  }
</script>

<svelte:window onkeydown={closeOnEscape} />

<div class="fixed inset-0 z-[80] flex items-center justify-center bg-zinc-950/45 p-4 backdrop-blur-[14px] sm:p-6">
  <button
    type="button"
    aria-label="Dismiss scale mode"
    class="absolute inset-0 border-0 bg-transparent"
    onclick={onClose}
  ></button>

  <div
    role="dialog"
    aria-modal="true"
    aria-labelledby={titleId}
    tabindex="-1"
    class="relative z-[1] flex max-h-[min(90vh,42rem)] w-full max-w-[min(58rem,calc(100vw-2rem))] flex-col overflow-hidden border border-zinc-700/90 bg-zinc-950/88 shadow-[0_24px_80px_rgba(0,0,0,0.55)] backdrop-blur-[22px]"
  >
    <div class="flex items-start justify-between gap-4 border-b border-zinc-800 px-5 py-4 sm:px-6">
      <div class="min-w-0">
        <p class="text-xs font-semibold uppercase tracking-[0.28em] text-zinc-500">Pattern {patternSlot + 1}</p>
        <h2 id={titleId} class="mt-1 text-2xl font-semibold text-zinc-100">Scale Mode</h2>
      </div>
      <button
        type="button"
        aria-label="Close scale mode"
        data-cursor="pointer"
        class="flex h-8 w-8 shrink-0 items-center justify-center border border-zinc-700 bg-zinc-900 text-zinc-400 outline-none transition-colors hover:border-zinc-500 hover:text-zinc-100 focus:ring-1 focus:ring-accent-400"
        onclick={onClose}
      >
        <svg
          class="h-4 w-4"
          viewBox="0 0 24 24"
          fill="none"
          stroke="currentColor"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
          aria-hidden="true"
        >
          <path d="M18 6 6 18" />
          <path d="m6 6 12 12" />
        </svg>
      </button>
    </div>

    <div class="grid min-h-0 gap-6 overflow-auto p-5 sm:p-6">
      <div class="grid gap-3">
        <div class="flex items-end justify-between gap-4">
          <div>
            <p class="text-xs font-semibold uppercase tracking-[0.28em] text-zinc-500">Key center</p>
            <h3 class="mt-1 text-lg font-semibold text-zinc-100">{keyCenters[root]?.label ?? "C"}</h3>
          </div>
          <div class="text-right text-sm font-semibold text-accent-300">{selectedName}</div>
        </div>
        <div class="grid grid-cols-6 gap-2 sm:grid-cols-12">
          {#each keyCenters as key (key.value)}
            <button
              type="button"
              aria-label={`Set key center to ${key.label}`}
              aria-pressed={root === key.value}
              data-cursor="pointer"
              class="flex h-11 items-center justify-center border text-sm font-semibold outline-none transition-colors focus:ring-1 focus:ring-accent-400 {root === key.value
                ? 'border-accent-400 bg-accent-400 text-zinc-950'
                : 'border-zinc-700 bg-zinc-900 text-zinc-200 hover:border-zinc-500'}"
              onclick={() => onChange(key.value, modeIndex)}
            >
              {key.label}
            </button>
          {/each}
        </div>
      </div>

      <div class="grid gap-3">
        <div>
          <p class="text-xs font-semibold uppercase tracking-[0.28em] text-zinc-500">Mode</p>
          <h3 class="mt-1 text-lg font-semibold text-zinc-100">{scaleModes[modeIndex]?.label ?? "Chromatic"}</h3>
        </div>
        <div class="grid gap-2 sm:grid-cols-2 lg:grid-cols-4">
          {#each scaleModes as mode, index (mode.label)}
            <button
              type="button"
              aria-label={`Set mode to ${mode.label}`}
              aria-pressed={modeIndex === index}
              data-cursor="pointer"
              class="flex min-h-14 items-center justify-between gap-3 border px-3 py-2 text-left outline-none transition-colors focus:ring-1 focus:ring-accent-400 {modeIndex === index
                ? 'border-accent-400 bg-accent-400 text-zinc-950'
                : 'border-zinc-700 bg-zinc-900 text-zinc-200 hover:border-zinc-500'}"
              onclick={() => onChange(root, index)}
            >
              <span class="min-w-0 text-sm font-semibold leading-tight">{mode.label}</span>
              <span class="shrink-0 text-xs font-bold uppercase opacity-70">{mode.shortLabel}</span>
            </button>
          {/each}
        </div>
      </div>
    </div>
  </div>
</div>
