<script>
  import { midiToNoteName } from "./midiNoteNames.js";
  import { emeraldRowAccent } from "./rowAccentTheme.js";

  /**
   * @typedef {Object} Props
   * @property {number} [row]
   * @property {number} [step]
   * @property {number} [note]
   * @property {number} [probability]
   * @property {number} [cycle]
   * @property {number} [cycleOffset]
   * @property {import('./rowAccentTheme.js').RowAccent} [accent]
   * @property {() => void} [onClose]
   */

  /** @type {Props} */
  let {
    row = 0,
    step = 0,
    note = 60,
    probability = 100,
    cycle = 1,
    cycleOffset = 0,
    accent = emeraldRowAccent,
    onClose = () => {},
  } = $props();

  let noteLabel = $derived(midiToNoteName(note));
</script>

<section class="flex min-h-0 w-full flex-1 flex-col border-t border-border-subtle bg-app/90 px-6 py-5">
  <div class="mb-5 flex shrink-0 items-center justify-between gap-4">
    <div>
      <p class="text-xs font-medium uppercase tracking-widest text-text-muted">Step inspector</p>
      <h2 class="mt-1 text-2xl font-semibold leading-none text-text">
        Row {row + 1} · Step {step + 1} · {noteLabel}
      </h2>
    </div>
    <button
      type="button"
      data-cursor="pointer"
      aria-label="Close step inspector"
      class="flex h-8 w-8 shrink-0 items-center justify-center rounded-md border border-border bg-surface text-lg leading-none text-text-muted transition-colors outline-none hover:border-border-strong hover:text-text focus-visible:ring-1 focus-visible:ring-focus-ring"
      onclick={onClose}
    >
      X
    </button>
  </div>

  <div class="grid min-h-0 flex-1 grid-cols-3 gap-4">
    <div class="flex min-h-0 flex-col justify-between rounded-md border border-border-subtle bg-surface/80 p-4">
      <span class="text-xs font-semibold uppercase tracking-widest text-text-muted">Probability</span>
      <span class="mt-4 text-4xl font-semibold leading-none {accent.textAccent}">{probability}%</span>
    </div>
    <div class="flex min-h-0 flex-col justify-between rounded-md border border-border-subtle bg-surface/80 p-4">
      <span class="text-xs font-semibold uppercase tracking-widest text-text-muted">Cycle</span>
      <span class="mt-4 text-4xl font-semibold leading-none text-text">{cycle}</span>
    </div>
    <div class="flex min-h-0 flex-col justify-between rounded-md border border-border-subtle bg-surface/80 p-4">
      <span class="text-xs font-semibold uppercase tracking-widest text-text-muted">Cycle offset</span>
      <span class="mt-4 text-4xl font-semibold leading-none text-text">{cycleOffset + 1}</span>
    </div>
  </div>
</section>
