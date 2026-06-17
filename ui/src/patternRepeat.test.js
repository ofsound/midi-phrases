import { describe, expect, it } from "vitest";
import {
  isScheduledNoteActiveAtPatternBeat,
  mapPlaybackBeatForPianoRoll,
  patternRepeatLengthQuarters,
  positiveMod,
  rowPatternRepeatLengthQuarters,
} from "./phraseSchedule.js";
import { defaultStepTimingMultiplierIndex } from "./stepCellLayout.js";

describe("patternRepeatLengthQuarters", () => {
  it("returns the row cycle when one row is active", () => {
    const repeat = patternRepeatLengthQuarters({
      stepTimingMultiplier: [
        [
          defaultStepTimingMultiplierIndex,
          defaultStepTimingMultiplierIndex,
        ],
        [],
        [],
        [],
      ],
      rowMuted: [false, true, true, true],
      stepSkipped: [[false, false], [], [], []],
      stepCycle: [[1, 1], [], [], []],
      pulseIndex: 1,
    });

    expect(repeat).toBe(2);
  });

  it("uses lcm across active rows", () => {
    const repeat = patternRepeatLengthQuarters({
      stepTimingMultiplier: [
        [defaultStepTimingMultiplierIndex],
        [defaultStepTimingMultiplierIndex, defaultStepTimingMultiplierIndex],
        [],
        [],
      ],
      rowMuted: [false, false, true, true],
      stepSkipped: [[false], [false, false], [], []],
      stepCycle: [[1], [1, 1], [], []],
      pulseIndex: 1,
    });

    expect(repeat).toBe(2);
  });

  it("extends row repeat for step cycle patterns", () => {
    const repeat = rowPatternRepeatLengthQuarters(
      [defaultStepTimingMultiplierIndex],
      [false],
      [3],
      1,
    );

    expect(repeat).toBe(3);
  });
});

describe("mapPlaybackBeatForPianoRoll", () => {
  it("wraps transport beats to the pattern repeat when loop is off", () => {
    expect(
      mapPlaybackBeatForPianoRoll(305, {
        loopEnabled: false,
        patternRepeatLengthQuarters: 8,
        previewLengthQuarters: 300,
      }),
    ).toBe(1);
  });

  it("keeps loop-brace beats unchanged", () => {
    expect(
      mapPlaybackBeatForPianoRoll(12, {
        loopEnabled: true,
        patternRepeatLengthQuarters: 8,
        previewLengthQuarters: 300,
      }),
    ).toBe(12);
  });

  it("caps repeat length to the preview window", () => {
    expect(
      mapPlaybackBeatForPianoRoll(350, {
        loopEnabled: false,
        patternRepeatLengthQuarters: 384,
        previewLengthQuarters: 300,
      }),
    ).toBe(50);
  });
});

describe("positiveMod", () => {
  it("returns non-negative remainders", () => {
    expect(positiveMod(-1, 8)).toBe(7);
    expect(positiveMod(9, 8)).toBe(1);
  });
});

describe("isScheduledNoteActiveAtPatternBeat", () => {
  it("matches wrapped note activity", () => {
    const note = { start: 2, end: 3, midi: 60, velocity: 100, row: 0, step: 0 };

    expect(isScheduledNoteActiveAtPatternBeat(note, 10, 8)).toBe(true);
    expect(isScheduledNoteActiveAtPatternBeat(note, 11, 8)).toBe(false);
  });
});
