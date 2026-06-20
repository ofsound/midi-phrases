import { describe, expect, it } from "vitest";
import { rowStepLayout } from "./phraseSchedule.js";
import { buildRowRollTimeline } from "./rowPianoRollTimeline.js";
import {
  beatLineQuarters,
  interpolateShapeYAtX,
  measureLineQuarters,
  midiFromPitchDragDelta,
  midiFromRollY,
  rollLengthQuartersForCycle,
  shapeNoteUpdatesFromStroke,
  shapeVelocityUpdatesFromStroke,
  insertStepIndexFromRollX,
  pianoRollNoteDragTooltipVisible,
  stepAtRollX,
  stepSlotCenterXPx,
  velocityFromRollY,
  velocityYInRoll,
} from "./rowPianoRollShape.js";

describe("midiFromRollY", () => {
  it("maps roll y to midi using the visible top pitch", () => {
    expect(midiFromRollY(0, 16, 72)).toBe(72);
    expect(midiFromRollY(16, 16, 72)).toBe(71);
    expect(midiFromRollY(32.4, 16, 72)).toBe(70);
  });
});

describe("midiFromPitchDragDelta", () => {
  it("never returns an out-of-scale preview note", () => {
    expect(midiFromPitchDragDelta(60, 1, 0, 1)).toBe(60);
    expect(midiFromPitchDragDelta(60, 2, 0, 1)).toBe(62);
    expect(midiFromPitchDragDelta(64, 1, 0, 1)).toBe(65);
  });

  it("remains chromatic in chromatic mode", () => {
    expect(midiFromPitchDragDelta(60, 1, 0, 0)).toBe(61);
  });
});

describe("velocityFromRollY", () => {
  it("maps roll y to velocity with the top as maximum", () => {
    expect(velocityFromRollY(0, 100)).toBe(127);
    expect(velocityFromRollY(50, 100)).toBe(64);
    expect(velocityFromRollY(100, 100)).toBe(0);
  });
});

describe("velocityYInRoll", () => {
  it("maps velocity back to roll y positions", () => {
    expect(velocityYInRoll(127, 100)).toBe(0);
    expect(velocityYInRoll(0, 100)).toBe(100);
  });
});

describe("interpolateShapeYAtX", () => {
  it("linearly interpolates between stroke points", () => {
    const points = [
      { x: 0, y: 0 },
      { x: 100, y: 100 },
    ];

    expect(interpolateShapeYAtX(points, 50)).toBe(50);
    expect(interpolateShapeYAtX(points, -10)).toBe(0);
    expect(interpolateShapeYAtX(points, 120)).toBe(100);
  });
});

describe("rollLengthQuartersForCycle", () => {
  it("pads the timeline to whole 4-quarter measures", () => {
    expect(rollLengthQuartersForCycle(3)).toBe(4);
    expect(rollLengthQuartersForCycle(5)).toBe(8);
    expect(rollLengthQuartersForCycle(8)).toBe(8);
  });
});

describe("measureLineQuarters", () => {
  it("places measure boundaries every four quarter notes", () => {
    expect(measureLineQuarters(5)).toEqual([0, 4, 8]);
    expect(beatLineQuarters(5)).toEqual([0, 1, 2, 3, 4, 5, 6, 7, 8]);
  });
});

describe("buildRowRollTimeline", () => {
  it("positions steps using multiplier times pulse in quarter notes", () => {
    const multipliers = [5, 3, 7];
    const { slots, timelineLengthQuarters } = buildRowRollTimeline(multipliers, [], 1, 3);
    const layout = rowStepLayout(multipliers, 1, []);

    expect(slots.map((slot) => slot.startQuarters)).toEqual(layout.stepStartQuarters);
    expect(slots.map((slot) => slot.lengthQuarters)).toEqual(layout.stepLengthQuarters);
    expect(timelineLengthQuarters).toBe(layout.cycleLengthQuarters);
  });
});

describe("stepAtRollX", () => {
  it("maps x positions to steps using quarter-note slot widths", () => {
    const { slots } = buildRowRollTimeline([5, 3, 7, 15], [], 1, 3);
    const pxPerQuarter = 28;

    expect(stepAtRollX(0, slots, pxPerQuarter)).toBe(0);
    expect(stepAtRollX(stepSlotCenterXPx(slots[1], pxPerQuarter), slots, pxPerQuarter)).toBe(1);
    expect(stepAtRollX(10_000, slots, pxPerQuarter)).toBe(3);
  });
});

describe("insertStepIndexFromRollX", () => {
  it("inserts before the step under the click and appends past row content", () => {
    const { slots } = buildRowRollTimeline([5, 3, 7, 15], [], 1, 3);
    const pxPerQuarter = 28;
    const lastSlot = slots[slots.length - 1];
    const contentEndPx = (lastSlot.startQuarters + lastSlot.lengthQuarters) * pxPerQuarter;

    expect(insertStepIndexFromRollX(0, slots, pxPerQuarter)).toBe(0);
    expect(
      insertStepIndexFromRollX(stepSlotCenterXPx(slots[1], pxPerQuarter), slots, pxPerQuarter),
    ).toBe(1);
    expect(insertStepIndexFromRollX(contentEndPx - 1, slots, pxPerQuarter)).toBe(3);
    expect(insertStepIndexFromRollX(contentEndPx, slots, pxPerQuarter)).toBe(4);
    expect(insertStepIndexFromRollX(10_000, slots, pxPerQuarter)).toBe(4);
  });

  it("returns index 0 for an empty row", () => {
    const { slots } = buildRowRollTimeline([], [], 1, 3);

    expect(insertStepIndexFromRollX(40, slots, 28)).toBe(0);
  });
});

describe("shapeNoteUpdatesFromStroke", () => {
  it("updates only steps beneath the drawn horizontal span", () => {
    const { slots } = buildRowRollTimeline([7, 7, 7, 7], [], 1, 3);
    const pxPerQuarter = 28;
    const points = [
      { x: stepSlotCenterXPx(slots[0], pxPerQuarter), y: 0 },
      { x: stepSlotCenterXPx(slots[2], pxPerQuarter), y: 32 },
    ];

    const updates = shapeNoteUpdatesFromStroke(points, slots, pxPerQuarter, 16, 72);

    expect(updates).toEqual([
      { step: 0, midi: 72 },
      { step: 1, midi: 71 },
      { step: 2, midi: 70 },
    ]);
  });
});

describe("shapeVelocityUpdatesFromStroke", () => {
  it("updates velocities only for steps beneath the drawn horizontal span", () => {
    const { slots } = buildRowRollTimeline([7, 7, 7, 7], [], 1, 3);
    const pxPerQuarter = 28;
    const points = [
      { x: stepSlotCenterXPx(slots[0], pxPerQuarter), y: 0 },
      { x: stepSlotCenterXPx(slots[2], pxPerQuarter), y: 100 },
    ];

    const updates = shapeVelocityUpdatesFromStroke(points, slots, pxPerQuarter, 100);

    expect(updates).toEqual([
      { step: 0, velocity: 127 },
      { step: 1, velocity: 64 },
      { step: 2, velocity: 0 },
    ]);
  });
});

describe("pianoRollNoteDragTooltipVisible", () => {
  it("shows when the pitch row is too short for the 10px label", () => {
    expect(pianoRollNoteDragTooltipVisible(12, 80, "C4")).toBe(true);
    expect(pianoRollNoteDragTooltipVisible(18, 80, "C4")).toBe(false);
  });

  it("shows when the note pill is too narrow", () => {
    expect(pianoRollNoteDragTooltipVisible(20, 32, "C4")).toBe(true);
    expect(pianoRollNoteDragTooltipVisible(20, 48, "C4")).toBe(false);
  });

  it("shows when the label would truncate inside a wide-enough pill", () => {
    // Real note names are at most 4 chars; use a 5-char label to exercise the width estimate.
    expect(pianoRollNoteDragTooltipVisible(20, 40, "C#4xx")).toBe(true);
    expect(pianoRollNoteDragTooltipVisible(20, 44, "C#4xx")).toBe(false);
  });
});
