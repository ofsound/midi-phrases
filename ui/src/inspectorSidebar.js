/**
 * @param {import('./rowAccentTheme.js').RowAccent} accent
 * @param {boolean} active
 * @param {{ disabled?: boolean }} [options]
 */
export function inspectorToggleClasses(accent, active, { disabled = false } = {}) {
  const base = `flex h-9 min-w-0 flex-1 items-center justify-center gap-1.5 rounded-md border px-2 transition-[background-color,border-color,color,box-shadow] outline-none ${accent.ringFocusWithWidth}`;

  if (disabled) {
    return `${base} mp-control-gradient-muted border-border-subtle text-text-faint`;
  }

  if (active) {
    return `${base} ${accent.bgAccentStrong} ${accent.borderFocus} border text-white ${accent.playbackGlow}`;
  }

  return `${base} mp-control-gradient border-border text-text-secondary hover:border-border-strong hover:text-text`;
}
