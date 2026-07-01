import { describe, expect, it } from "vitest";
import {
  buildRowRollTimeline,
  precedingStepExpansionForNoteDrag,
  precedingStepResizeForNoteDrag,
  timingMultiplierIndexAfterRollResize,
} from "./rowPianoRollTimeline.js";
import { defaultRowTimingOffsetIndex, rowTimingOffsetIndexForQuarters } from "./stepCellLayout.js";

describe("buildRowRollTimeline", () => {
  it("applies row timing offset to step start positions", () => {
    const { slots } = buildRowRollTimeline([3, 3], [], 1, rowTimingOffsetIndexForQuarters(0.25));

    expect(slots[0].startQuarters).toBe(0.25);
    expect(slots[1].startQuarters).toBe(1.25);
  });

  it("uses pulse length when computing slot durations", () => {
    const quarterPulse = buildRowRollTimeline([3], [], 1, defaultRowTimingOffsetIndex);
    const halfPulse = buildRowRollTimeline([3], [], 2, defaultRowTimingOffsetIndex);

    expect(quarterPulse.slots[0].lengthQuarters).toBe(1);
    expect(halfPulse.slots[0].lengthQuarters).toBe(2);
  });

  it("pushes and pulls later steps without changing their widths", () => {
    const original = buildRowRollTimeline([3, 7, 1], [], 1, defaultRowTimingOffsetIndex);
    const expanded = buildRowRollTimeline([5, 7, 1], [], 1, defaultRowTimingOffsetIndex);
    const contracted = buildRowRollTimeline([1, 7, 1], [], 1, defaultRowTimingOffsetIndex);

    expect(expanded.slots[1].startQuarters).toBe(original.slots[1].startQuarters + 0.5);
    expect(expanded.slots[2].startQuarters).toBe(original.slots[2].startQuarters + 0.5);
    expect(contracted.slots[1].startQuarters).toBe(original.slots[1].startQuarters - 0.5);
    expect(contracted.slots[2].startQuarters).toBe(original.slots[2].startQuarters - 0.5);
    expect(expanded.slots[1].lengthQuarters).toBe(original.slots[1].lengthQuarters);
    expect(expanded.slots[2].lengthQuarters).toBe(original.slots[2].lengthQuarters);
  });

  it("snaps resize gestures in either direction and clamps the result", () => {
    expect(timingMultiplierIndexAfterRollResize(3, 13, 100, 1)).toBe(4);
    expect(timingMultiplierIndexAfterRollResize(3, -13, 100, 1)).toBe(2);
    expect(timingMultiplierIndexAfterRollResize(0, -1000, 100, 1)).toBe(0);
    expect(timingMultiplierIndexAfterRollResize(15, 1000, 100, 1)).toBe(15);
  });

  it("shortens a preceding step until its start is crossed", () => {
    expect(precedingStepResizeForNoteDrag(7, -51, 100, 200, 1)).toEqual({
      multiplierIndex: 5,
      crossedPrecedingStart: false,
    });
    expect(precedingStepResizeForNoteDrag(7, -190, 100, 200, 1)).toEqual({
      multiplierIndex: 0,
      crossedPrecedingStart: false,
    });
  });

  it("restores the preceding width at its start so the steps can swap", () => {
    expect(precedingStepResizeForNoteDrag(7, -200, 100, 200, 1)).toEqual({
      multiplierIndex: 7,
      crossedPrecedingStart: true,
    });
  });

  it("expands a preceding step when its following step is dragged right", () => {
    expect(precedingStepExpansionForNoteDrag(3, 51, 100, 1)).toEqual({
      multiplierIndex: 5,
      exceededMaximum: false,
    });
    expect(precedingStepExpansionForNoteDrag(3, 300, 100, 1)).toEqual({
      multiplierIndex: 15,
      exceededMaximum: false,
    });
  });

  it("restores the preceding width when a right drag exceeds 4x", () => {
    expect(precedingStepExpansionForNoteDrag(3, 325, 100, 1)).toEqual({
      multiplierIndex: 3,
      exceededMaximum: true,
    });
  });
});
