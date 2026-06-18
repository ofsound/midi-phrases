import { describe, expect, it } from "vitest";
import {
  compactStepMinimumVelocityOpacity,
  compactStepSkippedOpacity,
  compactStepVelocityOpacity,
} from "./compactStepVisuals.js";

describe("compactStepVelocityOpacity", () => {
  it("maps MIDI velocity to increasing accent strength", () => {
    expect(compactStepVelocityOpacity(0)).toBe(compactStepMinimumVelocityOpacity);
    expect(compactStepVelocityOpacity(64)).toBeGreaterThan(compactStepVelocityOpacity(32));
    expect(compactStepVelocityOpacity(127)).toBe(1);
  });

  it("clamps invalid or out-of-range values", () => {
    expect(compactStepVelocityOpacity(-10)).toBe(compactStepMinimumVelocityOpacity);
    expect(compactStepVelocityOpacity(Number.NaN)).toBe(compactStepMinimumVelocityOpacity);
    expect(compactStepVelocityOpacity(200)).toBe(1);
  });

  it("uses a distinct faint fill for skipped steps", () => {
    expect(compactStepVelocityOpacity(127, true)).toBe(compactStepSkippedOpacity);
  });
});
