import { describe, expect, it } from "vitest";
import {
  interpolateShapeYAtX,
  midiFromRollY,
  shapeNoteUpdatesFromStroke,
} from "./rowPianoRollShape.js";

describe("midiFromRollY", () => {
  it("maps roll y to midi using the visible top pitch", () => {
    expect(midiFromRollY(0, 16, 72)).toBe(72);
    expect(midiFromRollY(16, 16, 72)).toBe(71);
    expect(midiFromRollY(32.4, 16, 72)).toBe(70);
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

describe("shapeNoteUpdatesFromStroke", () => {
  it("updates only steps beneath the drawn horizontal span", () => {
    const points = [
      { x: 36, y: 0 },
      { x: 180, y: 32 },
    ];

    const updates = shapeNoteUpdatesFromStroke(points, 4, 72, 16, 72);

    expect(updates).toEqual([
      { step: 0, midi: 72 },
      { step: 1, midi: 71 },
      { step: 2, midi: 70 },
    ]);
  });
});
