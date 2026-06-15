import { describe, expect, it } from "vitest";
import {
  cycleGatePasses,
  cycleLengthFromCellElements,
  cycleLengthFromClientX,
  cycleMaskFromLegacyOffset,
  isCycleCellActive,
  normalizeCyclePattern,
  normalizeEditorCyclePattern,
  resizeCyclePattern,
  toggleCycleCell,
} from "./cyclePattern.js";

describe("cycle pattern", () => {
  it("plays only the first repetition for [x] [ ] [ ]", () => {
    const {cycle, mask} = normalizeCyclePattern(3, 0b001);

    expect(cycleGatePasses(0, cycle, mask)).toBe(true);
    expect(cycleGatePasses(1, cycle, mask)).toBe(false);
    expect(cycleGatePasses(2, cycle, mask)).toBe(false);
    expect(cycleGatePasses(3, cycle, mask)).toBe(true);
  });

  it("plays the first two repetitions for [x] [x] [ ]", () => {
    const {cycle, mask} = normalizeCyclePattern(3, 0b011);

    expect(cycleGatePasses(0, cycle, mask)).toBe(true);
    expect(cycleGatePasses(1, cycle, mask)).toBe(true);
    expect(cycleGatePasses(2, cycle, mask)).toBe(false);
    expect(cycleGatePasses(3, cycle, mask)).toBe(true);
  });

  it("migrates legacy single offsets to one-hot masks", () => {
    expect(cycleMaskFromLegacyOffset(2, 4)).toBe(0b100);
  });

  it("toggles cells inside the active length", () => {
    expect(isCycleCellActive(3, 0b101, 1)).toBe(false);
    expect(toggleCycleCell(3, 0b101, 1)).toEqual({cycle: 3, mask: 0b111});
  });

  it("keeps at least one active step when resizing", () => {
    expect(resizeCyclePattern(3, 0b100, 2)).toEqual({cycle: 2, mask: 1});
  });

  it("clamps editor cycle length to eight cells", () => {
    expect(normalizeEditorCyclePattern(16, 0b11111111)).toEqual({cycle: 8, mask: 0b11111111});
  });

  it("maps pointer x position to cycle length from cell elements", () => {
    const cells = [
      {getBoundingClientRect: () => ({left: 0, right: 10, width: 10})},
      {getBoundingClientRect: () => ({left: 10, right: 20, width: 10})},
      {getBoundingClientRect: () => ({left: 20, right: 30, width: 10})},
    ];

    expect(cycleLengthFromCellElements(4, cells)).toBe(1);
    expect(cycleLengthFromCellElements(11, cells)).toBe(2);
    expect(cycleLengthFromCellElements(25, cells)).toBe(3);
  });

  it("toggles the last cell in the active pattern length", () => {
    expect(toggleCycleCell(3, 0b011, 2)).toEqual({cycle: 3, mask: 0b111});
    expect(toggleCycleCell(3, 0b111, 2)).toEqual({cycle: 3, mask: 0b011});
  });
});
