import { stepAtRollX } from "./rowPianoRollShape.js";
import {
  precedingStepExpansionForNoteDrag,
  precedingStepResizeForNoteDrag,
  timingMultiplierIndexAfterRollResize,
} from "./rowPianoRollTimeline.js";
import {
  defaultStepTimingMultiplierIndex,
} from "./stepCellLayout.js";

export const compactStepMoveThresholdPx = 4;

/**
 * @param {number} deltaX
 * @param {number} deltaY
 */
export function compactStepMoveDominates(deltaX, deltaY) {
  return (
    Math.abs(deltaX) >= compactStepMoveThresholdPx
    && Math.abs(deltaX) > Math.abs(deltaY)
  );
}

/**
 * @param {number} stepCount
 * @param {{ mode?: string, step?: number, targetStep?: number } | null | undefined} drag
 * @returns {number[] | null}
 */
export function compactPreviewStepOrder(stepCount, drag) {
  if (
    !drag
    || drag.mode !== "move"
    || drag.step === undefined
    || drag.targetStep === undefined
    || drag.targetStep === drag.step
  ) {
    return null;
  }

  const order = Array.from({ length: stepCount }, (_, index) => index);
  const [moved] = order.splice(drag.step, 1);
  order.splice(drag.targetStep, 0, moved);

  return order;
}

/**
 * @param {{ mode?: string, previewPreviousStep?: number, previewPreviousMultiplierIndex?: number, originalPreviousMultiplierIndex?: number } | null | undefined} drag
 * @param {number[]} stepTimingMultiplier
 * @returns {number[] | null}
 */
export function compactTimingMultipliersDuringMove(drag, stepTimingMultiplier) {
  if (
    !drag
    || drag.mode !== "move"
    || drag.previewPreviousStep === undefined
    || drag.previewPreviousStep < 0
    || drag.previewPreviousMultiplierIndex === undefined
    || drag.previewPreviousMultiplierIndex === drag.originalPreviousMultiplierIndex
  ) {
    return null;
  }

  const preview = stepTimingMultiplier.slice();
  preview[drag.previewPreviousStep] = drag.previewPreviousMultiplierIndex;

  return preview;
}

/**
 * Preview horizontal step move / preceding-step resize for compact rows.
 *
 * @param {object} drag
 * @param {number} rollX
 * @param {number} clientX
 * @param {import("./rowPianoRollShape.js").RowRollStepSlot[]} originalSlots
 * @param {number} pxPerQuarter
 * @param {number} pulseIndex
 */
export function previewCompactStepMove(
  drag,
  rollX,
  clientX,
  originalSlots,
  pxPerQuarter,
  pulseIndex,
) {
  const dragDeltaX = clientX - drag.startX;
  let targetStep = stepAtRollX(rollX, originalSlots, pxPerQuarter);
  let previewPreviousMultiplierIndex = drag.originalPreviousMultiplierIndex;

  if (dragDeltaX < 0 && drag.previewPreviousStep >= 0) {
    const precedingSlot = originalSlots[drag.previewPreviousStep];
    const draggedSlot = originalSlots[drag.step];
    const draggedStartXPx = Math.max(
      0,
      draggedSlot.startQuarters * pxPerQuarter + dragDeltaX,
    );
    targetStep = stepAtRollX(draggedStartXPx, originalSlots, pxPerQuarter);
    const distanceToPrecedingStartPx =
      (draggedSlot.startQuarters - precedingSlot.startQuarters) * pxPerQuarter;
    const precedingResize = precedingStepResizeForNoteDrag(
      drag.originalPreviousMultiplierIndex,
      dragDeltaX,
      pxPerQuarter,
      distanceToPrecedingStartPx,
      pulseIndex,
    );

    previewPreviousMultiplierIndex = precedingResize.multiplierIndex;

    if (!precedingResize.crossedPrecedingStart) {
      targetStep = drag.step;
    }
  } else if (dragDeltaX > 0 && drag.previewPreviousStep >= 0) {
    const draggedSlot = originalSlots[drag.step];
    const draggedStartXPx =
      draggedSlot.startQuarters * pxPerQuarter + dragDeltaX;
    targetStep = stepAtRollX(draggedStartXPx, originalSlots, pxPerQuarter);
    const precedingExpansion = precedingStepExpansionForNoteDrag(
      drag.originalPreviousMultiplierIndex,
      dragDeltaX,
      pxPerQuarter,
      pulseIndex,
    );

    previewPreviousMultiplierIndex = precedingExpansion.multiplierIndex;

    if (!precedingExpansion.exceededMaximum) {
      targetStep = drag.step;
    }
  }

  return {
    didDrag: true,
    targetStep: targetStep < 0 ? drag.step : targetStep,
    previewPreviousMultiplierIndex,
  };
}

/**
 * @param {number} initialIndex
 * @param {number} deltaPx
 * @param {number} pxPerQuarter
 * @param {number} pulseIndex
 */
export function previewCompactStepBoundaryResize(
  initialIndex,
  deltaPx,
  pxPerQuarter,
  pulseIndex,
) {
  return timingMultiplierIndexAfterRollResize(
    initialIndex,
    deltaPx,
    pxPerQuarter,
    pulseIndex,
  );
}

/**
 * @param {number} step
 * @param {"start" | "end"} edge
 */
export function compactBoundaryResizeStepIndex(step, edge) {
  return edge === "start" ? step - 1 : step;
}

/**
 * @param {number} step
 * @param {"start" | "end"} edge
 * @param {number[]} stepTimingMultiplier
 */
export function compactBoundaryResizeInitialIndex(step, edge, stepTimingMultiplier) {
  const resizedStep = compactBoundaryResizeStepIndex(step, edge);

  if (resizedStep < 0) return defaultStepTimingMultiplierIndex;

  return stepTimingMultiplier[resizedStep] ?? defaultStepTimingMultiplierIndex;
}
