import { describe, expect, it } from "vitest";
import {
  clampHundredScalePercent,
  clampStepProbabilityPercent,
  defaultStepProbabilityValue,
  maxPercentValue,
  maxStepProbabilityValue,
} from "./percentLimits.js";

describe("percent limits", () => {
  it("keeps shared percent controls capped at 99", () => {
    expect(maxPercentValue).toBe(99);
    expect(clampHundredScalePercent(100)).toBe(99);
  });

  it("allows step probability to reach 100 by default", () => {
    expect(maxStepProbabilityValue).toBe(100);
    expect(defaultStepProbabilityValue).toBe(100);
    expect(clampStepProbabilityPercent(99)).toBe(99);
    expect(clampStepProbabilityPercent(100)).toBe(100);
    expect(clampStepProbabilityPercent(101)).toBe(100);
    expect(clampStepProbabilityPercent(Number.NaN)).toBe(defaultStepProbabilityValue);
  });
});
