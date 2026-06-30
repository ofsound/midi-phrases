<script>
  import { rowAccentColorVar, rowAccentFor } from "./rowAccentTheme.js";

  /**
   * Compact hardware-style row target toggles for seed mode.
   *
   * @typedef {Object} Props
   * @property {boolean[]} [rowTargets]
   * @property {boolean} [rowColorsEnabled]
   * @property {boolean} [busy]
   * @property {(row: number, options: { shiftKey: boolean }) => void} [onRowTargetToggle]
   * @property {() => void} [onToggleAllRowTargets]
   * @property {string} [class]
   */

  /** @type {Props} */
  let {
    rowTargets = [true, true, true, true],
    rowColorsEnabled = true,
    busy = false,
    onRowTargetToggle = () => {},
    onToggleAllRowTargets = () => {},
    class: className = "",
  } = $props();

  let allRowsTargeted = $derived(rowTargets.every(Boolean));

  /** @param {import('./rowAccentTheme.js').RowAccent} accent */
  function rowAccentStrongColorVar(accent) {
    const token = accent.bgAccent.replace(/^bg-/, "");
    return `var(--color-${token}-strong)`;
  }

  /** @param {import('./rowAccentTheme.js').RowAccent} accent */
  function rowAccentSoftColorVar(accent) {
    const token = accent.bgAccent.replace(/^bg-/, "");
    return `var(--color-${token}-soft, var(--color-${token}))`;
  }

  /** @param {MouseEvent} event @param {number} row */
  function handleRowTargetClick(event, row) {
    onRowTargetToggle(row, { shiftKey: event.shiftKey });
  }
</script>

<div class="seed-row-target-rail {className}">
  <p class="seed-row-target-label">Seed rows</p>
  <div class="seed-row-target-strip" role="group" aria-label="Rows to seed">
    {#each [0, 1, 2, 3] as row (row)}
      {@const targeted = rowTargets[row] ?? false}
      {@const accent = rowAccentFor(row, rowColorsEnabled)}
      <button
        type="button"
        data-cursor="pointer"
        disabled={busy}
        aria-label={`Seed row ${row + 1}`}
        aria-pressed={targeted}
        title={targeted
          ? `Row ${row + 1} will receive seeded values. Shift-click to combine rows.`
          : `Select row ${row + 1} for seeding. Shift-click to combine rows.`}
        class="seed-row-target-btn seed-row-target-row"
        style:--row-accent={rowAccentColorVar(accent)}
        style:--row-accent-strong={rowAccentStrongColorVar(accent)}
        style:--row-accent-soft={rowAccentSoftColorVar(accent)}
        onclick={(event) => handleRowTargetClick(event, row)}
      >
        {row + 1}
      </button>
    {/each}
    <button
      type="button"
      data-cursor="pointer"
      disabled={busy}
      aria-label={allRowsTargeted ? "Disable seeding for all rows" : "Enable seeding for all rows"}
      aria-pressed={allRowsTargeted}
      title={allRowsTargeted ? "Disable seeding for all rows" : "Enable seeding for all rows"}
      class="seed-row-target-btn seed-row-target-all"
      onclick={onToggleAllRowTargets}
    >
      All
    </button>
  </div>
</div>

<style>
  .seed-row-target-rail {
    display: grid;
    gap: 0.35rem;
  }

  .seed-row-target-label {
    margin: 0;
    font-size: 9px;
    font-weight: 500;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    color: var(--color-text-muted);
  }

  .seed-row-target-strip {
    --target-height: 1.875rem;
    display: flex;
    align-items: stretch;
    gap: 0.3rem;
  }

  .seed-row-target-btn {
    flex: 1 1 0;
    min-width: 0;
    height: var(--target-height);
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 0;
    border-radius: 2px;
    font-family: var(--font-sans, ui-sans-serif, system-ui, sans-serif);
    font-size: 0.8125rem;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
    line-height: 1;
    outline: none;
    transition:
      background-color 120ms ease,
      border-color 120ms ease,
      box-shadow 120ms ease,
      color 120ms ease;
  }

  .seed-row-target-row {
    border: 1.5px solid color-mix(in srgb, var(--row-accent) 78%, transparent);
    background: color-mix(in srgb, var(--row-accent) 28%, var(--color-field));
    color: var(--row-accent-strong);
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, var(--row-accent-soft) 24%, transparent),
      inset 0 -1px 2px color-mix(in srgb, var(--color-app) 36%, transparent);
  }

  .seed-row-target-row:not([aria-pressed="true"]):hover:not(:disabled) {
    background: color-mix(in srgb, var(--row-accent) 42%, var(--color-field));
    border-color: color-mix(in srgb, var(--row-accent-strong) 88%, transparent);
    color: var(--row-accent-strong);
  }

  .seed-row-target-row:focus-visible {
    box-shadow:
      var(--shadow-accent-focus),
      0 0 0.35rem color-mix(in srgb, var(--row-accent) 34%, transparent);
  }

  .seed-row-target-row:disabled {
    opacity: 0.4;
    cursor: default;
  }

  .seed-row-target-row[aria-pressed="true"] {
    border-color: color-mix(in srgb, var(--row-accent-strong) 94%, #fff);
    background: linear-gradient(
      180deg,
      color-mix(in srgb, var(--row-accent) 82%, #fff) 0%,
      var(--row-accent) 54%,
      var(--row-accent-strong) 100%
    );
    color: var(--color-control-primary-text);
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, #fff 38%, transparent),
      inset 0 -1.5px 2px color-mix(in srgb, var(--row-accent-strong) 52%, transparent),
      0 0 0.55rem color-mix(in srgb, var(--row-accent) 48%, transparent);
  }

  .seed-row-target-all {
    flex: 1.15 1 0;
    border: 1px solid color-mix(in srgb, var(--color-border-subtle) 78%, transparent);
    background: color-mix(in srgb, var(--color-surface-subtle) 38%, transparent);
    color: color-mix(in srgb, var(--color-text) 72%, transparent);
    font-size: 0.625rem;
    font-weight: 700;
    letter-spacing: 0.14em;
    text-transform: uppercase;
    box-shadow: inset 0 1px 1.5px color-mix(in srgb, var(--color-text) 10%, transparent);
  }

  .seed-row-target-all:hover:not(:disabled) {
    background: color-mix(in srgb, var(--color-control-ghost-hover) 48%, transparent);
    color: var(--color-text);
  }

  .seed-row-target-all:focus-visible {
    box-shadow: var(--shadow-accent-focus);
  }

  .seed-row-target-all:disabled {
    opacity: 0.4;
    cursor: default;
  }

  .seed-row-target-all[aria-pressed="true"] {
    border-color: color-mix(in srgb, var(--color-accent) 86%, #fff);
    background: linear-gradient(
      180deg,
      color-mix(in srgb, var(--color-accent) 68%, #fff) 0%,
      var(--color-accent) 52%,
      color-mix(in srgb, var(--color-accent-strong) 82%, var(--color-accent)) 100%
    );
    color: var(--color-control-primary-text);
    box-shadow:
      inset 0 1px 0 color-mix(in srgb, #fff 36%, transparent),
      inset 0 -1.5px 2px color-mix(in srgb, var(--color-accent-strong) 48%, transparent),
      0 0 0.45rem color-mix(in srgb, var(--color-accent) 28%, transparent);
  }
</style>
