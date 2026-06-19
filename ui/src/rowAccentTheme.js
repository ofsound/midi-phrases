/** @typedef {{
 *   borderActive: string,
 *   borderFocus: string,
 *   borderFocusVisible: string,
 *   cellFocusWithinBorder: string,
 *   ringFocus: string,
 *   ringFocusWithWidth: string,
 *   insertZoneFocus: string,
 *   controlFocus: string,
 *   textAccent: string,
 *   textAccentStrong: string,
 *   textAccentLight: string,
 *   textAccentFocus: string,
 *   bgAccent: string,
 *   bgAccentStrong: string,
 *   dragBorder: string,
 *   dragShadow: string,
 *   playbackGlow: string,
 *   selectionShell: string,
 *   selectionRing: string,
 *   selectionBorder: string,
 *   pianoNoteIdle: string,
 *   pianoNoteActive: string,
 *   velocityBarFill: string,
 *   velocityBarFillPreview: string,
 *   durationTrackBg: string,
 *   durationBorder: string,
 * }} RowAccent */

/** Global interface accent — intentionally distinct from any row palette. */
/** @type {RowAccent} */
export const interfaceAccent = {
  borderActive: "border-accent",
  borderFocus: "border-accent-strong",
  borderFocusVisible: "",
  cellFocusWithinBorder: "",
  ringFocus: "",
  ringFocusWithWidth: "",
  insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-focus-ring",
  controlFocus: "focus:border-focus-ring focus:ring-1 focus:ring-focus-ring",
  textAccent: "text-accent",
  textAccentStrong: "text-accent-strong",
  textAccentLight: "text-accent",
  textAccentFocus: "focus-visible:text-accent",
  bgAccent: "bg-accent",
  bgAccentStrong: "bg-accent-strong",
  dragBorder: "border-accent",
  dragShadow: "shadow-accent-drag",
  playbackGlow: "shadow-accent-playback",
  selectionShell: "shadow-[0_0_0_1px_color-mix(in_srgb,var(--color-accent)_55%,transparent),0_0_8px_color-mix(in_srgb,var(--color-accent)_18%,transparent)]",
  selectionRing: "ring-1 ring-accent/28",
  selectionBorder: "border-accent/55",
  pianoNoteIdle: "border-accent/20 bg-accent/85 shadow-[inset_0_1px_0_color-mix(in_srgb,var(--color-text)_12%,transparent)]",
  pianoNoteActive: "border-link-hover/90 bg-link-hover shadow-accent-piano-active",
  velocityBarFill: "bg-[color-mix(in_srgb,var(--color-accent)_32%,transparent)]",
  velocityBarFillPreview: "bg-[color-mix(in_srgb,var(--color-accent)_56%,transparent)]",
  durationTrackBg: "bg-[color-mix(in_srgb,var(--color-accent)_16%,var(--color-surface))]",
  durationBorder: "border border-accent",
};

/** Row 1 accent — palette tokens live in semantic-theme.generated.css. */
/** @type {RowAccent} */
export const emeraldRowAccent = {
  borderActive: "border-row-accent-1",
  borderFocus: "border-row-accent-1-strong",
  borderFocusVisible: "",
  cellFocusWithinBorder: "",
  ringFocus: "",
  ringFocusWithWidth: "",
  insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-row-accent-1-strong",
  controlFocus: "focus:border-row-accent-1-strong focus:ring-1 focus:ring-row-accent-1-strong",
  textAccent: "text-row-accent-1",
  textAccentStrong: "text-row-accent-1-strong",
  textAccentLight: "text-row-accent-1-soft",
  textAccentFocus: "focus-visible:text-row-accent-1-soft",
  bgAccent: "bg-row-accent-1",
  bgAccentStrong: "bg-row-accent-1-strong",
  dragBorder: "border-row-accent-1",
  dragShadow: "shadow-[0_0_10px_color-mix(in_srgb,var(--color-row-accent-1)_40%,transparent)]",
  playbackGlow: "shadow-[0_0_14px_color-mix(in_srgb,var(--color-row-accent-1)_55%,transparent)]",
  selectionShell: "shadow-[0_0_0_1px_color-mix(in_srgb,var(--color-row-accent-1)_55%,transparent),0_0_8px_color-mix(in_srgb,var(--color-row-accent-1)_18%,transparent)]",
  selectionRing: "ring-1 ring-row-accent-1/28",
  selectionBorder: "border-row-accent-1/55",
  pianoNoteIdle: "border-row-accent-1-soft/25 bg-row-accent-1-strong/85 shadow-[inset_0_1px_0_color-mix(in_srgb,var(--color-text)_12%,transparent)]",
  pianoNoteActive: "border-row-accent-1-soft/90 bg-row-accent-1-soft shadow-[0_0_12px_color-mix(in_srgb,var(--color-row-accent-1)_65%,transparent)]",
  velocityBarFill: "bg-[color-mix(in_srgb,var(--color-row-accent-1)_32%,transparent)]",
  velocityBarFillPreview: "bg-[color-mix(in_srgb,var(--color-row-accent-1)_56%,transparent)]",
  durationTrackBg: "bg-[color-mix(in_srgb,var(--color-row-accent-1)_16%,var(--color-surface))]",
  durationBorder: "border border-row-accent-1",
};

/** Rows 2-4 (1-based): alternate row accents when row colors are enabled. */
/** @type {RowAccent[]} */
export const altRowAccents = [
  {
    borderActive: "border-row-accent-2",
    borderFocus: "border-row-accent-2-strong",
    borderFocusVisible: "",
    cellFocusWithinBorder: "",
    ringFocus: "",
    ringFocusWithWidth: "",
    insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-row-accent-2-strong",
    controlFocus: "focus:border-row-accent-2-strong focus:ring-1 focus:ring-row-accent-2-strong",
    textAccent: "text-row-accent-2",
    textAccentStrong: "text-row-accent-2-strong",
    textAccentLight: "text-row-accent-2-soft",
    textAccentFocus: "focus-visible:text-row-accent-2-soft",
    bgAccent: "bg-row-accent-2",
    bgAccentStrong: "bg-row-accent-2-strong",
    dragBorder: "border-row-accent-2",
    dragShadow: "shadow-[0_0_10px_color-mix(in_srgb,var(--color-row-accent-2)_40%,transparent)]",
    playbackGlow: "shadow-[0_0_14px_color-mix(in_srgb,var(--color-row-accent-2)_55%,transparent)]",
    selectionShell: "shadow-[0_0_0_1px_color-mix(in_srgb,var(--color-row-accent-2)_55%,transparent),0_0_8px_color-mix(in_srgb,var(--color-row-accent-2)_18%,transparent)]",
    selectionRing: "ring-1 ring-row-accent-2/28",
    selectionBorder: "border-row-accent-2/55",
    pianoNoteIdle: "border-row-accent-2-soft/25 bg-row-accent-2-strong/85 shadow-[inset_0_1px_0_color-mix(in_srgb,var(--color-text)_12%,transparent)]",
    pianoNoteActive: "border-row-accent-2-soft/90 bg-row-accent-2-soft shadow-[0_0_12px_color-mix(in_srgb,var(--color-row-accent-2)_65%,transparent)]",
    velocityBarFill: "bg-[color-mix(in_srgb,var(--color-row-accent-2)_32%,transparent)]",
    velocityBarFillPreview: "bg-[color-mix(in_srgb,var(--color-row-accent-2)_56%,transparent)]",
    durationTrackBg: "bg-[color-mix(in_srgb,var(--color-row-accent-2)_16%,var(--color-surface))]",
    durationBorder: "border border-row-accent-2",
  },
  {
    borderActive: "border-row-accent-3",
    borderFocus: "border-row-accent-3-strong",
    borderFocusVisible: "",
    cellFocusWithinBorder: "",
    ringFocus: "",
    ringFocusWithWidth: "",
    insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-row-accent-3-strong",
    controlFocus: "focus:border-row-accent-3-strong focus:ring-1 focus:ring-row-accent-3-strong",
    textAccent: "text-row-accent-3",
    textAccentStrong: "text-row-accent-3-strong",
    textAccentLight: "text-row-accent-3-soft",
    textAccentFocus: "focus-visible:text-row-accent-3-soft",
    bgAccent: "bg-row-accent-3",
    bgAccentStrong: "bg-row-accent-3-strong",
    dragBorder: "border-row-accent-3",
    dragShadow: "shadow-[0_0_10px_color-mix(in_srgb,var(--color-row-accent-3)_40%,transparent)]",
    playbackGlow: "shadow-[0_0_14px_color-mix(in_srgb,var(--color-row-accent-3)_55%,transparent)]",
    selectionShell: "shadow-[0_0_0_1px_color-mix(in_srgb,var(--color-row-accent-3)_55%,transparent),0_0_8px_color-mix(in_srgb,var(--color-row-accent-3)_18%,transparent)]",
    selectionRing: "ring-1 ring-row-accent-3/28",
    selectionBorder: "border-row-accent-3/55",
    pianoNoteIdle: "border-row-accent-3-soft/25 bg-row-accent-3-strong/85 shadow-[inset_0_1px_0_color-mix(in_srgb,var(--color-text)_12%,transparent)]",
    pianoNoteActive: "border-row-accent-3-soft/90 bg-row-accent-3-soft shadow-[0_0_12px_color-mix(in_srgb,var(--color-row-accent-3)_65%,transparent)]",
    velocityBarFill: "bg-[color-mix(in_srgb,var(--color-row-accent-3)_32%,transparent)]",
    velocityBarFillPreview: "bg-[color-mix(in_srgb,var(--color-row-accent-3)_56%,transparent)]",
    durationTrackBg: "bg-[color-mix(in_srgb,var(--color-row-accent-3)_16%,var(--color-surface))]",
    durationBorder: "border border-row-accent-3",
  },
  {
    borderActive: "border-row-accent-4",
    borderFocus: "border-row-accent-4-strong",
    borderFocusVisible: "",
    cellFocusWithinBorder: "",
    ringFocus: "",
    ringFocusWithWidth: "",
    insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-row-accent-4-strong",
    controlFocus: "focus:border-row-accent-4-strong focus:ring-1 focus:ring-row-accent-4-strong",
    textAccent: "text-row-accent-4",
    textAccentStrong: "text-row-accent-4-strong",
    textAccentLight: "text-row-accent-4-soft",
    textAccentFocus: "focus-visible:text-row-accent-4-soft",
    bgAccent: "bg-row-accent-4",
    bgAccentStrong: "bg-row-accent-4-strong",
    dragBorder: "border-row-accent-4",
    dragShadow: "shadow-[0_0_10px_color-mix(in_srgb,var(--color-row-accent-4)_40%,transparent)]",
    playbackGlow: "shadow-[0_0_14px_color-mix(in_srgb,var(--color-row-accent-4)_55%,transparent)]",
    selectionShell: "shadow-[0_0_0_1px_color-mix(in_srgb,var(--color-row-accent-4)_55%,transparent),0_0_8px_color-mix(in_srgb,var(--color-row-accent-4)_18%,transparent)]",
    selectionRing: "ring-1 ring-row-accent-4/28",
    selectionBorder: "border-row-accent-4/55",
    pianoNoteIdle: "border-row-accent-4-soft/25 bg-row-accent-4-strong/85 shadow-[inset_0_1px_0_color-mix(in_srgb,var(--color-text)_12%,transparent)]",
    pianoNoteActive: "border-row-accent-4-soft/90 bg-row-accent-4-soft shadow-[0_0_12px_color-mix(in_srgb,var(--color-row-accent-4)_65%,transparent)]",
    velocityBarFill: "bg-[color-mix(in_srgb,var(--color-row-accent-4)_32%,transparent)]",
    velocityBarFillPreview: "bg-[color-mix(in_srgb,var(--color-row-accent-4)_56%,transparent)]",
    durationTrackBg: "bg-[color-mix(in_srgb,var(--color-row-accent-4)_16%,var(--color-surface))]",
    durationBorder: "border border-row-accent-4",
  },
];

/** @param {number} row @param {boolean} rowColorsEnabled */
export function rowAccentFor(row, rowColorsEnabled) {
  if (!rowColorsEnabled || row <= 0) {
    return emeraldRowAccent;
  }

  return altRowAccents[row - 1] ?? emeraldRowAccent;
}

/** Dims row transport controls when muted; does not cover step cells. */
export const rowMutedOverlayClasses = "pointer-events-none absolute inset-0 z-10 rounded-xl bg-app/50";

/** Icon toggles: off/on gray levels only (no hover; no row accent). */
export const toggleIconRestClasses = "text-text-muted";
export const toggleIconActiveClasses = "text-text-secondary";

/** Row power toggle when row is off — darker gray only (on uses row accent). */
export const rowPowerToggleOffClasses = "text-text-faint";

/** On/off toggle stays above {@link rowMutedOverlayClasses}. */
export const rowMuteControlClasses = "relative z-20 flex h-[1.6875rem] w-[1.6875rem] shrink-0 items-center justify-center rounded-md border-0 bg-transparent p-0 outline-none focus-visible:ring-1 focus-visible:ring-focus-ring";

/** Bordered control in the global header (e.g. row reverse). */
export const rowReverseControlClasses = "relative z-20 flex h-9 min-w-[3.75rem] shrink-0 items-center justify-center rounded-lg border bg-surface px-2 transition-colors outline-none hover:border-border-strong";

/** Row reverse / randomize icon actions — borderless, equal hit target. */
export const rowActionIconControlClasses = "relative z-20 flex h-6 w-6 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors focus:outline-none focus-visible:outline-none";
