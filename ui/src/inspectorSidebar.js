/**
 * Inactive control face shared by inspector toggles, seed targets, and seed actions.
 * @param {{ disabled?: boolean }} [options]
 */
export function inspectorInactiveControlClasses({ disabled = false } = {}) {
  if (disabled) {
    return "mp-control-gradient-muted border-border-subtle text-text-faint";
  }

  return "mp-control-gradient border-border text-text-secondary hover:border-border-strong hover:text-text";
}

/**
 * Compact labeled action button (e.g. seed panel New Notes / Randomize).
 * @param {{ disabled?: boolean }} [options]
 */
export function inspectorTextButtonClasses({ disabled = false } = {}) {
  return `flex h-9 items-center justify-center rounded-md border px-3 text-xs font-semibold leading-none tracking-normal transition-[background-color,border-color,color,box-shadow] outline-none focus-visible:ring-1 focus-visible:ring-focus-ring disabled:cursor-default disabled:opacity-40 ${inspectorInactiveControlClasses({ disabled })}`;
}

/**
 * @param {import('./rowAccentTheme.js').RowAccent} accent
 */
function inspectorControlButtonBase(accent) {
  return `flex items-center justify-center rounded-md border transition-[background-color,border-color,color,box-shadow] outline-none ${accent.ringFocusWithWidth}`;
}

/**
 * @param {import('./rowAccentTheme.js').RowAccent} accent
 * @param {boolean} active
 * @param {{ disabled?: boolean }} [options]
 */
export function inspectorToggleClasses(accent, active, { disabled = false } = {}) {
  const base = `${inspectorControlButtonBase(accent)} h-9 min-w-0 flex-1 gap-1.5 px-2`;

  if (disabled) {
    return `${base} ${inspectorInactiveControlClasses({ disabled: true })}`;
  }

  if (active) {
    return `${base} ${accent.bgAccentStrong} ${accent.borderFocus} border text-white ${accent.playbackGlow}`;
  }

  return `${base} ${inspectorInactiveControlClasses()}`;
}

/**
 * Octave shift controls in the step inspector keyboard header.
 * @param {import('./rowAccentTheme.js').RowAccent} accent
 */
export function inspectorOctaveButtonClasses(accent) {
  return `${inspectorControlButtonBase(accent)} h-10 w-10 shrink-0 text-base font-semibold tabular-nums ${inspectorInactiveControlClasses()}`;
}
