<script>
  import { inspectorInactiveControlClasses } from "./inspectorSidebar.js";
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
    rowTargets = [true, false, false, false],
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

  /** @param {MouseEvent} event @param {number} row */
  function handleRowTargetClick(event, row) {
    onRowTargetToggle(row, { shiftKey: event.shiftKey });
  }
</script>

<div class="seed-row-target-rail {className}">
  <div class="seed-row-target-strip" role="group" aria-label="Seed rows">
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
        class="seed-row-target-btn seed-row-target-row {targeted
          ? 'seed-row-target-row--active'
          : inspectorInactiveControlClasses({ disabled: busy })}"
        style:--row-accent={rowAccentColorVar(accent)}
        style:--row-accent-strong={rowAccentStrongColorVar(accent)}
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
      class="seed-row-target-btn seed-row-target-all {allRowsTargeted
        ? 'seed-row-target-all--active'
        : inspectorInactiveControlClasses({ disabled: busy })}"
      onclick={onToggleAllRowTargets}
    >
      All
    </button>
  </div>
</div>

<style>
  .seed-row-target-rail {
    display: block;
  }

  .seed-row-target-strip {
    display: flex;
    align-items: stretch;
    gap: 0.375rem;
  }

  .seed-row-target-btn {
    flex: 1 1 0;
    min-width: 0;
    height: 2.25rem;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 0;
    border-radius: 0.375rem;
    border-width: 1px;
    border-style: solid;
    font-family: var(--font-sans, ui-sans-serif, system-ui, sans-serif);
    font-size: 0.75rem;
    font-weight: 600;
    font-variant-numeric: tabular-nums;
    line-height: 1;
    outline: none;
    transition:
      background-color 120ms ease,
      border-color 120ms ease,
      box-shadow 120ms ease,
      color 120ms ease;
  }

  .seed-row-target-btn:focus-visible:not(.seed-row-target-row--active):not(.seed-row-target-all--active) {
    box-shadow: var(--shadow-accent-focus);
  }

  .seed-row-target-btn:disabled {
    cursor: default;
  }

  .seed-row-target-row--active:focus-visible {
    box-shadow:
      var(--shadow-accent-focus),
      0 0 0.35rem color-mix(in srgb, var(--row-accent) 34%, transparent);
  }

  .seed-row-target-row--active {
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
    letter-spacing: 0.01em;
  }

  .seed-row-target-all--active {
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
