import { defaultPulseIndex, pulseQuartersForIndex } from "./pulseLayout.js";
import { rowStepVisualLayout, rowTimingOffsetQuarters } from "./phraseSchedule.js";
import {
  defaultRowTimingOffsetIndex,
  stepTimingMultiplierCount,
  stepTimingMultiplierQuarterStep,
} from "./stepCellLayout.js";

/**
 * Snap a horizontal piano-roll resize gesture to the timing-multiplier grid.
 *
 * @param {number} initialIndex
 * @param {number} deltaPx
 * @param {number} pxPerQuarter
 * @param {number} [pulseIndex]
 */
export function timingMultiplierIndexAfterRollResize(
  initialIndex,
  deltaPx,
  pxPerQuarter,
  pulseIndex = defaultPulseIndex,
) {
  const pixelsPerMultiplierStep =
    pulseQuartersForIndex(pulseIndex) * stepTimingMultiplierQuarterStep * pxPerQuarter;

  if (!Number.isFinite(pixelsPerMultiplierStep) || pixelsPerMultiplierStep <= 0) {
    return Math.min(stepTimingMultiplierCount - 1, Math.max(0, Math.round(initialIndex)));
  }

  const deltaSteps = Math.round(deltaPx / pixelsPerMultiplierStep);

  return Math.min(
    stepTimingMultiplierCount - 1,
    Math.max(0, Math.round(initialIndex) + deltaSteps),
  );
}

/**
 * Preview shortening the step before a note being dragged left. The preceding
 * step can shrink to 0.25×; reaching its start restores its original width so
 * the caller can switch to the normal step-reorder behavior.
 *
 * @param {number} precedingMultiplierIndex
 * @param {number} dragDeltaPx
 * @param {number} pxPerQuarter
 * @param {number} distanceToPrecedingStartPx
 * @param {number} [pulseIndex]
 */
export function precedingStepResizeForNoteDrag(
  precedingMultiplierIndex,
  dragDeltaPx,
  pxPerQuarter,
  distanceToPrecedingStartPx,
  pulseIndex = defaultPulseIndex,
) {
  const normalizedIndex = Math.min(
    stepTimingMultiplierCount - 1,
    Math.max(0, Math.round(precedingMultiplierIndex)),
  );
  const distanceLeftPx = Math.max(0, -dragDeltaPx);
  const crossedPrecedingStart =
    dragDeltaPx < 0 && distanceLeftPx >= Math.max(0, distanceToPrecedingStartPx);

  if (crossedPrecedingStart) {
    return { multiplierIndex: normalizedIndex, crossedPrecedingStart: true };
  }

  const pixelsPerMultiplierStep =
    pulseQuartersForIndex(pulseIndex) * stepTimingMultiplierQuarterStep * pxPerQuarter;

  if (!Number.isFinite(pixelsPerMultiplierStep) || pixelsPerMultiplierStep <= 0) {
    return { multiplierIndex: normalizedIndex, crossedPrecedingStart: false };
  }

  const shortenedBySteps = Math.round(distanceLeftPx / pixelsPerMultiplierStep);

  return {
    multiplierIndex: Math.max(0, normalizedIndex - shortenedBySteps),
    crossedPrecedingStart: false,
  };
}

/**
 * Preview expanding the step before a note being dragged right. Expansion is
 * capped at 4×; dragging beyond the next unavailable increment restores the
 * original width so the caller can resume normal step reordering.
 *
 * @param {number} precedingMultiplierIndex
 * @param {number} dragDeltaPx
 * @param {number} pxPerQuarter
 * @param {number} [pulseIndex]
 */
export function precedingStepExpansionForNoteDrag(
  precedingMultiplierIndex,
  dragDeltaPx,
  pxPerQuarter,
  pulseIndex = defaultPulseIndex,
) {
  const normalizedIndex = Math.min(
    stepTimingMultiplierCount - 1,
    Math.max(0, Math.round(precedingMultiplierIndex)),
  );
  const pixelsPerMultiplierStep =
    pulseQuartersForIndex(pulseIndex) * stepTimingMultiplierQuarterStep * pxPerQuarter;

  if (!Number.isFinite(pixelsPerMultiplierStep) || pixelsPerMultiplierStep <= 0) {
    return { multiplierIndex: normalizedIndex, exceededMaximum: false };
  }

  const expandedBySteps = Math.max(0, Math.round(dragDeltaPx / pixelsPerMultiplierStep));
  const availableSteps = stepTimingMultiplierCount - 1 - normalizedIndex;

  if (expandedBySteps > availableSteps) {
    return { multiplierIndex: normalizedIndex, exceededMaximum: true };
  }

  return {
    multiplierIndex: normalizedIndex + expandedBySteps,
    exceededMaximum: false,
  };
}

/**
 * @param {number[]} stepTimingMultiplier
 * @param {boolean[]} [stepSkipped]
 * @param {number} [pulseIndex]
 * @param {number} [rowTimingOffset]
 */
export function buildRowRollTimeline(
  stepTimingMultiplier,
  _stepSkipped = [],
  pulseIndex = defaultPulseIndex,
  rowTimingOffset = defaultRowTimingOffsetIndex,
) {
  const { stepStartQuarters, stepLengthQuarters, cycleLengthQuarters } = rowStepVisualLayout(
    stepTimingMultiplier,
    pulseIndex,
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
    : rowOffsetQuarters;
  const timelineLengthQuarters = Math.max(contentEndQuarters, 0);

  return {
    slots,
    cycleLengthQuarters,
    rowOffsetQuarters,
    timelineLengthQuarters,
  };
}
