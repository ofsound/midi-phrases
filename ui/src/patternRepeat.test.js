import { describe, expect, it } from "vitest";
import {
  isScheduledNoteActiveAtBeat,
  isScheduledNoteActiveAtPatternBeat,
  isScheduledNoteActiveAtPlaybackBeat,
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
  it("wraps transport beats to the pattern length when loop is off", () => {
    expect(mapPlaybackBeatForPianoRoll(10, { patternLengthQuarters: 8 })).toBe(2);
    expect(mapPlaybackBeatForPianoRoll(16, { patternLengthQuarters: 8 })).toBe(0);
  });

  it("keeps loop-brace mapped beats unchanged", () => {
    expect(mapPlaybackBeatForPianoRoll(12, { loopEnabled: true })).toBe(12);
  });

  it("returns inactive beats unchanged", () => {
    expect(mapPlaybackBeatForPianoRoll(-1)).toBe(-1);
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

  it("does not mark future rendered repeats active at the displayed playhead", () => {
    const displayBeat = mapPlaybackBeatForPianoRoll(1, { patternLengthQuarters: 4 });
    const noteUnderPlayhead = { start: 1, end: 2, midi: 60, velocity: 100, row: 0, step: 0 };
    const futureRepeat = { ...noteUnderPlayhead, start: 5, end: 6 };

    expect(isScheduledNoteActiveAtPatternBeat(futureRepeat, displayBeat, 4)).toBe(true);
    expect(isScheduledNoteActiveAtBeat(noteUnderPlayhead, displayBeat)).toBe(true);
    expect(isScheduledNoteActiveAtBeat(futureRepeat, displayBeat)).toBe(false);
  });

  it("matches tiled repeats through the playback-beat helper used by the piano roll", () => {
    const displayBeat = mapPlaybackBeatForPianoRoll(9, { patternLengthQuarters: 4 });
    const tiledRepeat = { start: 9, end: 10, midi: 60, velocity: 100, row: 0, step: 0 };

    expect(
      isScheduledNoteActiveAtPlaybackBeat(tiledRepeat, displayBeat, {
        patternLengthQuarters: 4,
      }),
    ).toBe(true);
    expect(isScheduledNoteActiveAtBeat(tiledRepeat, displayBeat)).toBe(false);
  });
});
