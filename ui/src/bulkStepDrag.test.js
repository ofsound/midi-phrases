import { describe, expect, it } from "vitest";
import {
  blockCrossRowInsertionIndex,
  blockDuplicateInsertionIndex,
  blockMoveOrder,
  duplicateBlockInRow,
  selectedIdsInRowOrder,
} from "./bulkStepDrag.js";

describe("selectedIdsInRowOrder", () => {
  it("keeps row order for the selected ids", () => {
    expect(selectedIdsInRowOrder(["a", "b", "c", "d"], ["d", "b"])).toEqual(["b", "d"]);
  });
});

describe("blockMoveOrder", () => {
  it("matches single-step move semantics", () => {
    expect(blockMoveOrder(["a", "b", "c", "d", "e"], ["b"], 3)).toEqual(["a", "c", "b", "d", "e"]);
  });

  it("moves a multi-step block as one unit", () => {
    expect(blockMoveOrder(["a", "b", "c", "d", "e"], ["b", "d"], 2)).toEqual([
      "a",
      "b",
      "d",
      "c",
      "e",
    ]);
    expect(blockMoveOrder(["a", "b", "c", "d", "e"], ["b", "d"], 5)).toEqual([
      "a",
      "c",
      "e",
      "b",
      "d",
    ]);
  });
});

describe("blockDuplicateInsertionIndex", () => {
  it("matches single-step duplicate semantics", () => {
    expect(blockDuplicateInsertionIndex(["a", "b", "c", "d"], ["b"], 3)).toBe(4);
    expect(blockDuplicateInsertionIndex(["a", "b", "c", "d"], ["b"], 1)).toBe(1);
  });

  it("maps a multi-step shadow index to the copy boundary", () => {
    expect(blockDuplicateInsertionIndex(["a", "b", "c", "d", "e"], ["b", "d"], 2)).toBe(2);
    expect(blockDuplicateInsertionIndex(["a", "b", "c", "d", "e"], ["b", "d"], 5)).toBe(5);
  });
});

describe("blockCrossRowInsertionIndex", () => {
  it("prefers the dragged preview id position", () => {
    expect(blockCrossRowInsertionIndex(["x", "y"], "b", ["x", "b", "y"], 0)).toBe(1);
  });

  it("falls back to the clamped shadow index", () => {
    expect(blockCrossRowInsertionIndex(["x", "y"], "b", ["x", "y"], 2)).toBe(2);
  });
});

describe("duplicateBlockInRow", () => {
  it("inserts aligned copies for every selected step", () => {
    const result = duplicateBlockInRow(
      {
        notes: [[60, 62, 64, 65]],
        velocity: [[80, 81, 82, 83]],
      },
      [["a", "b", "c", "d"]],
      0,
      ["b", "d"],
      2,
      () => "new",
    );

    expect(result).toEqual({
      matrices: {
        notes: [[60, 62, 62, 65, 64, 65]],
        velocity: [[80, 81, 81, 83, 82, 83]],
      },
      stepIds: [["a", "b", "new", "new", "c", "d"]],
      newIds: ["new", "new"],
    });
  });
});
