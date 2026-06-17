/** Virtual piano key surfaces — fixed high contrast in every app color theme. */

export const pianoWhiteKeyClass =
  "bg-gradient-to-b from-piano-key-white to-piano-key-white-muted border-r border-b border-piano-key-white-border transition-[filter,background-color,opacity] duration-75 hover:brightness-[0.98] active:brightness-95";

export const pianoBlackKeyClass =
  "bg-gradient-to-b from-piano-key-black-muted to-piano-key-black border border-piano-key-black-border shadow-md transition-[filter,background-color,opacity] duration-75 hover:brightness-110 active:brightness-125";

/** Piano roll sidebar row behind note labels. */
export const pianoRollKeyboardWhiteRowClass =
  "bg-piano-key-white border-b border-piano-key-white-border";

export const pianoRollKeyboardBlackRowClass =
  "bg-piano-key-black border-b border-piano-key-black-border";
