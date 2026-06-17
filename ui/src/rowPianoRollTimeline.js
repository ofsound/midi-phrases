import { defaultPulseIndex, pulseQuartersForIndex } from "./pulseLayout.js";
import { rowStepLayout, rowTimingOffsetQuarters } from "./phraseSchedule.js";

/**
 * @param {number[]} stepTimingMultiplier
 * @param {boolean[]} [stepSkipped]
 * @param {number} [pulseIndex]
 * @param {number} [rowTimingOffset]
 */
export function buildRowRollTimeline(
  stepTimingMultiplier,
  stepSkipped = [],
  pulseIndex = defaultPulseIndex,
  rowTimingOffset = 3,
) {
  const { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters } = rowStepLayout(
    stepTimingMultiplier,
    pulseIndex,
    stepSkipped,
  );
  const rowOffsetQuarters = rowTimingOffsetQuarters(rowTimingOffset, pulseIndex);

  /** @type {import("./rowPianoRollShape.js").RowRollStepSlot[]} */
  const slots = stepStartQuarters.map((startQuarters, step) => ({
    step,
    startQuarters: startQuarters + rowOffsetQuarters,
    lengthQuarters: stepLengthQuarters[step] ?? pulseQuartersForIndex(pulseIndex),
  }));

  const lastSlot = slots[slots.length - 1];
  const contentEndQuarters = lastSlot
    ? lastSlot.startQuarters + lastSlot.lengthQuarters
    : cycleLengthQuarters + rowOffsetQuarters;
  const timelineLengthQuarters = Math.max(
    cycleLengthQuarters + rowOffsetQuarters,
    contentEndQuarters,
    0,
  );

  return {
    slots,
    cycleLengthQuarters,
    rowOffsetQuarters,
    timelineLengthQuarters,
  };
}
