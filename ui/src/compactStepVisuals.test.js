import { describe, expect, it } from "vitest";
import {
  compactStepDurationFillPercent,
  compactStepMinimumVelocityOpacity,
  compactStepSkippedOpacity,
  compactStepVelocityOpacity,
  durationBarFillOpacity,
  durationBarMinimumVelocityOpacity,
} from "./compactStepVisuals.js";

describe("durationBarFillOpacity", () => {
  it("maps MIDI velocity to increasing accent strength", () => {
    expect(durationBarFillOpacity(0)).toBe(durationBarMinimumVelocityOpacity);
    expect(durationBarFillOpacity(64)).toBeGreaterThan(durationBarFillOpacity(32));
    expect(durationBarFillOpacity(127)).toBe(1);
  });

  it("clamps invalid or out-of-range values", () => {
    expect(durationBarFillOpacity(-10)).toBe(durationBarMinimumVelocityOpacity);
    expect(durationBarFillOpacity(Number.NaN)).toBe(durationBarMinimumVelocityOpacity);
    expect(durationBarFillOpacity(200)).toBe(1);
  });
});

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

describe("compactStepDurationFillPercent", () => {
  it("maps duration fraction to a left-fill width percent", () => {
    expect(compactStepDurationFillPercent(1)).toBe(100);
    expect(compactStepDurationFillPercent(0.5)).toBe(50);
    expect(compactStepDurationFillPercent(0.25)).toBe(25);
  });

  it("hides fill for muted steps and zero duration", () => {
    expect(compactStepDurationFillPercent(0.75, true)).toBe(0);
    expect(compactStepDurationFillPercent(0)).toBe(0);
  });

  it("clamps invalid values", () => {
    expect(compactStepDurationFillPercent(-0.2)).toBe(0);
    expect(compactStepDurationFillPercent(1.5)).toBe(100);
    expect(compactStepDurationFillPercent(Number.NaN)).toBe(100);
  });
});
