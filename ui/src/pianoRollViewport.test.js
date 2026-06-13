import { describe, expect, it } from "vitest";
import { fittedPitchRangeForSchedule } from "./pianoRollViewport.js";

describe("fittedPitchRangeForSchedule", () => {
  it("uses a C3-C5 fallback for empty schedules", () => {
    expect(fittedPitchRangeForSchedule([])).toEqual({ minMidi: 48, maxMidi: 72 });
  });

  it("adds two semitones of padding and expands to at least 12 visible semitones", () => {
    expect(fittedPitchRangeForSchedule([{ midi: 60 }])).toEqual({ minMidi: 54, maxMidi: 65 });
  });

  it("uses the whole filtered schedule range without octave snapping", () => {
    expect(fittedPitchRangeForSchedule([{ midi: 57 }, { midi: 72 }])).toEqual({
      minMidi: 55,
      maxMidi: 74,
    });
  });

  it("clamps padding and minimum expansion to the MIDI note range", () => {
    expect(fittedPitchRangeForSchedule([{ midi: 0 }])).toEqual({ minMidi: 0, maxMidi: 11 });
    expect(fittedPitchRangeForSchedule([{ midi: 127 }])).toEqual({ minMidi: 116, maxMidi: 127 });
  });
});
