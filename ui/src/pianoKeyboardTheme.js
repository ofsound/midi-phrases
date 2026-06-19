/** Virtual piano key surfaces — fixed high contrast in every app color theme. */

export const pianoWhiteKeyClass =
  "bg-gradient-to-b from-piano-key-white to-piano-key-white-muted border-r border-b border-piano-key-white-border transition-[filter,background-color] duration-75 hover:brightness-[0.98] active:brightness-95";

export const pianoBlackKeyClass =
  "bg-gradient-to-b from-piano-key-black-muted to-piano-key-black border border-piano-key-black-border shadow-md transition-[filter,background-color] duration-75 hover:brightness-110 active:brightness-125";

/** Opaque disabled styling — do not use opacity on black keys (white-key seams show through). */
export const pianoWhiteKeyDisabledClass =
  "pointer-events-none bg-gradient-to-b from-[color-mix(in_srgb,var(--color-piano-key-white)_35%,var(--color-app))] to-[color-mix(in_srgb,var(--color-piano-key-white-muted)_35%,var(--color-app))] border-r border-b border-[color-mix(in_srgb,var(--color-piano-key-white-border)_35%,transparent)] saturate-75 brightness-95";

export const pianoBlackKeyDisabledClass =
  "pointer-events-none bg-gradient-to-b from-[color-mix(in_srgb,var(--color-piano-key-black-muted)_35%,var(--color-piano-key-white))] to-[color-mix(in_srgb,var(--color-piano-key-black)_35%,var(--color-piano-key-white))] border border-[color-mix(in_srgb,var(--color-piano-key-black-border)_50%,var(--color-piano-key-white-border))] shadow-none saturate-75 brightness-105";

/** Covers the white-key seam centered under each black key. */
export const pianoBlackKeySeamMaskClass = "pointer-events-none absolute top-0 z-[5] bg-piano-key-white";
