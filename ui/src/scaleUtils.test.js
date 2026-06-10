import { describe, expect, it } from "vitest";
import {
  isChromaticScaleMode,
  isMidiInScale,
  scaleDegreeDelta,
  snapMidiToScale,
  transposeMidiByScaleDegrees,
} from "./scaleUtils.js";

describe("transposeMidiByScaleDegrees", () => {
  it("moves by semitone in chromatic mode", () => {
    expect(transposeMidiByScaleDegrees(60, 1, 0, 0)).toBe(61);
    expect(transposeMidiByScaleDegrees(60, -1, 0, 0)).toBe(59);
  });

  it("moves by scale degree in major mode", () => {
    expect(transposeMidiByScaleDegrees(60, 1, 0, 1)).toBe(62);
    expect(transposeMidiByScaleDegrees(60, 2, 0, 1)).toBe(64);
    expect(transposeMidiByScaleDegrees(64, -1, 0, 1)).toBe(62);
  });
});

describe("scaleDegreeDelta", () => {
  it("counts semitones in chromatic mode", () => {
    expect(scaleDegreeDelta(60, 63, 0, 0)).toBe(3);
  });

  it("counts scale degrees in major mode", () => {
    expect(scaleDegreeDelta(60, 64, 0, 1)).toBe(2);
    expect(scaleDegreeDelta(64, 60, 0, 1)).toBe(-2);
  });
});

describe("snapMidiToScale", () => {
  it("snaps off-scale notes to the nearest in-scale pitch", () => {
    expect(snapMidiToScale(61, 0, 1)).toBe(60);
  });
});

describe("isChromaticScaleMode", () => {
  it("is true only for chromatic mode index", () => {
    expect(isChromaticScaleMode(0)).toBe(true);
    expect(isChromaticScaleMode(1)).toBe(false);
  });
});

describe("isMidiInScale", () => {
  it("includes every pitch class in chromatic mode", () => {
    expect(isMidiInScale(61, 0, 0)).toBe(true);
  });

  it("filters to the active scale in diatonic modes", () => {
    expect(isMidiInScale(60, 0, 1)).toBe(true);
    expect(isMidiInScale(61, 0, 1)).toBe(false);
    expect(isMidiInScale(64, 4, 11)).toBe(true);
    expect(isMidiInScale(63, 4, 11)).toBe(false);
  });
});
