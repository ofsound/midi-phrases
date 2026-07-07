export const defaultDrumRowPitchLockEnabled = false;
export const drumRowPitchLockBaseMidi = 36;

/** @param {number} row */
export function drumRowPitchLockNoteForRow(row) {
  const clampedRow = Math.min(3, Math.max(0, Math.round(row)));

  return drumRowPitchLockBaseMidi + clampedRow;
}

/** @param {{ midi: number, row: number }[]} events @param {boolean} enabled */
export function applyDrumRowPitchLock(events, enabled) {
  if (!enabled) return events;

  return events.map((event) => ({
    ...event,
    midi: drumRowPitchLockNoteForRow(event.row),
  }));
}
