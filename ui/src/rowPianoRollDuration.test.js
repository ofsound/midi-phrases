import { describe, expect, it } from "vitest";
import { durationFractionFromRailX } from "./rowPianoRollDuration.js";

describe("durationFractionFromRailX", () => {
  it("maps positions across the duration rail", () => {
    expect(durationFractionFromRailX(125, 100, 100)).toBe(0.25);
    expect(durationFractionFromRailX(150, 100, 100)).toBe(0.5);
    expect(durationFractionFromRailX(200, 100, 100)).toBe(1);
  });

  it("clamps positions outside the duration rail", () => {
    expect(durationFractionFromRailX(50, 100, 100)).toBe(0);
    expect(durationFractionFromRailX(250, 100, 100)).toBe(1);
    expect(durationFractionFromRailX(100, 100, 0)).toBe(0);
  });
});
