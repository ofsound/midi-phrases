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
 *   textAccentLight: string,
 *   textAccentFocus: string,
 *   bgAccent: string,
 *   bgAccentStrong: string,
 *   dragBorder: string,
 *   dragShadow: string,
 *   playbackGlow: string,
 *   pianoNoteIdle: string,
 *   pianoNoteActive: string,
 * }} RowAccent */

/** Default row accent — palette tokens live in app.css @theme. */
/** @type {RowAccent} */
export const emeraldRowAccent = {
  borderActive: "border-accent-400",
  borderFocus: "border-accent-500",
  borderFocusVisible: "focus-visible:border-accent-500",
  cellFocusWithinBorder: "focus-within:has-[:focus-visible]:border-accent-500",
  ringFocus: "focus-visible:ring-accent-400",
  ringFocusWithWidth: "focus-visible:ring-1 focus-visible:ring-accent-400",
  insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-accent-500",
  controlFocus: "focus:border-accent-500 focus:ring-1 focus:ring-accent-500",
  textAccent: "text-accent-400",
  textAccentLight: "text-accent-300",
  textAccentFocus: "focus-visible:text-accent-300",
  bgAccent: "bg-accent-400",
  bgAccentStrong: "bg-accent-500",
  dragBorder: "border-accent-400",
  dragShadow: "shadow-accent-drag",
  playbackGlow: "shadow-accent-playback",
  pianoNoteIdle: "border-accent-300/20 bg-accent-400/85 shadow-[inset_0_1px_0_rgba(255,255,255,0.12)]",
  pianoNoteActive: "border-accent-200/90 bg-accent-300 shadow-accent-piano-active",
};

/** Rows 2–4 (1-based): blue, orange, violet when row colors are enabled. */
/** @type {RowAccent[]} */
export const altRowAccents = [
  {
    borderActive: "border-blue-400",
    borderFocus: "border-blue-500",
    borderFocusVisible: "focus-visible:border-blue-500",
    cellFocusWithinBorder: "focus-within:has-[:focus-visible]:border-blue-500",
    ringFocus: "focus-visible:ring-blue-400",
    ringFocusWithWidth: "focus-visible:ring-1 focus-visible:ring-blue-400",
    insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-blue-500",
    controlFocus: "focus:border-blue-500 focus:ring-1 focus:ring-blue-500",
    textAccent: "text-blue-400",
    textAccentLight: "text-blue-300",
    textAccentFocus: "focus-visible:text-blue-300",
    bgAccent: "bg-blue-400",
    bgAccentStrong: "bg-blue-500",
    dragBorder: "border-blue-400",
    dragShadow: "shadow-[0_0_10px_rgba(59,130,246,0.4)]",
    playbackGlow: "shadow-[0_0_14px_rgba(59,130,246,0.55)]",
    pianoNoteIdle: "border-blue-300/25 bg-blue-500/85 shadow-[inset_0_1px_0_rgba(255,255,255,0.12)]",
    pianoNoteActive: "border-blue-200/90 bg-blue-300 shadow-[0_0_12px_rgba(59,130,246,0.65)]",
  },
  {
    borderActive: "border-orange-400",
    borderFocus: "border-orange-500",
    borderFocusVisible: "focus-visible:border-orange-500",
    cellFocusWithinBorder: "focus-within:has-[:focus-visible]:border-orange-500",
    ringFocus: "focus-visible:ring-orange-400",
    ringFocusWithWidth: "focus-visible:ring-1 focus-visible:ring-orange-400",
    insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-orange-500",
    controlFocus: "focus:border-orange-500 focus:ring-1 focus:ring-orange-500",
    textAccent: "text-orange-400",
    textAccentLight: "text-orange-300",
    textAccentFocus: "focus-visible:text-orange-300",
    bgAccent: "bg-orange-400",
    bgAccentStrong: "bg-orange-500",
    dragBorder: "border-orange-400",
    dragShadow: "shadow-[0_0_10px_rgba(249,115,22,0.4)]",
    playbackGlow: "shadow-[0_0_14px_rgba(249,115,22,0.55)]",
    pianoNoteIdle: "border-orange-300/25 bg-orange-500/85 shadow-[inset_0_1px_0_rgba(255,255,255,0.12)]",
    pianoNoteActive: "border-orange-200/90 bg-orange-300 shadow-[0_0_12px_rgba(249,115,22,0.65)]",
  },
  {
    borderActive: "border-violet-400",
    borderFocus: "border-violet-500",
    borderFocusVisible: "focus-visible:border-violet-500",
    cellFocusWithinBorder: "focus-within:has-[:focus-visible]:border-violet-500",
    ringFocus: "focus-visible:ring-violet-400",
    ringFocusWithWidth: "focus-visible:ring-1 focus-visible:ring-violet-400",
    insertZoneFocus: "focus-visible:ring-1 focus-visible:ring-violet-500",
    controlFocus: "focus:border-violet-500 focus:ring-1 focus:ring-violet-500",
    textAccent: "text-violet-400",
    textAccentLight: "text-violet-300",
    textAccentFocus: "focus-visible:text-violet-300",
    bgAccent: "bg-violet-400",
    bgAccentStrong: "bg-violet-500",
    dragBorder: "border-violet-400",
    dragShadow: "shadow-[0_0_10px_rgba(139,92,246,0.4)]",
    playbackGlow: "shadow-[0_0_14px_rgba(139,92,246,0.55)]",
    pianoNoteIdle: "border-violet-300/25 bg-violet-500/85 shadow-[inset_0_1px_0_rgba(255,255,255,0.12)]",
    pianoNoteActive: "border-violet-200/90 bg-violet-300 shadow-[0_0_12px_rgba(139,92,246,0.65)]",
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
export const rowMutedOverlayClasses = "pointer-events-none absolute inset-0 z-10 rounded-xl bg-zinc-950/50";

/** Icon toggles: off/on gray levels only (no hover; no row accent). */
export const toggleIconRestClasses = "text-zinc-500";
export const toggleIconActiveClasses = "text-zinc-300";

/** Row power toggle when row is off — darker gray only (on uses row accent). */
export const rowPowerToggleOffClasses = "text-zinc-600";

/** On/off toggle stays above {@link rowMutedOverlayClasses}. */
export const rowMuteControlClasses = "relative z-20 flex h-10 w-10 shrink-0 items-center justify-center rounded-md border-0 bg-transparent p-0 outline-none focus-visible:ring-1 focus-visible:ring-zinc-500";

/** Bordered control in the global header (e.g. row reverse). */
export const rowReverseControlClasses = "relative z-20 flex h-9 min-w-[3.75rem] shrink-0 items-center justify-center rounded-lg border bg-zinc-900 px-2 transition-colors outline-none hover:border-zinc-600";

/** Row reverse / randomize icon actions — borderless, equal hit target. */
export const rowActionIconControlClasses = "relative z-20 flex h-9 w-9 shrink-0 items-center justify-center border-0 bg-transparent p-0 outline-none transition-colors focus:outline-none focus-visible:outline-none";
