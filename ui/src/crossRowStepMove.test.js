import { describe, expect, it } from "vitest";
import { duplicateStepBetweenRows, moveStepBetweenRows } from "./crossRowStepMove.js";

describe("moveStepBetweenRows", () => {
  it("moves every aligned step value to an exact target boundary", () => {
    const result = moveStepBetweenRows(
      {
        notes: [[60, 62], [65, 67]],
        velocity: [[80, 81], [90, 91]],
        muted: [[false, true], [false, false]],
      },
      [["a", "b"], ["c", "d"]],
      0,
      1,
      "b",
      ["c", "b", "d"],
    );

    expect(result).toEqual({
      matrices: {
        notes: [[60], [65, 62, 67]],
        velocity: [[80], [90, 81, 91]],
        muted: [[false], [false, true, false]],
      },
      stepIds: [["a"], ["c", "b", "d"]],
    });
  });

  it("moves a step into an empty target row", () => {
    const result = moveStepBetweenRows(
      {
        notes: [[60, 62], []],
        velocity: [[80, 81], []],
      },
      [["a", "b"], []],
      0,
      1,
      "a",
      ["a"],
    );

    expect(result).toEqual({
      matrices: {
        notes: [[62], [60]],
        velocity: [[81], [80]],
      },
      stepIds: [["b"], ["a"]],
    });
  });

  it("rejects a target order that loses or duplicates steps", () => {
    expect(
      moveStepBetweenRows(
        { notes: [[60], [65, 67]] },
        [["a"], ["b", "c"]],
        0,
        1,
        "a",
        ["a", "b", "b"],
      ),
    ).toBeNull();
  });
});

describe("duplicateStepBetweenRows", () => {
  it("copies a step into an empty target row", () => {
    const result = duplicateStepBetweenRows(
      {
        notes: [[60], []],
        velocity: [[80], []],
      },
      [["a"], []],
      0,
      1,
      "a",
      ["a"],
      "copy-a",
    );

    expect(result).toEqual({
      matrices: {
        notes: [[60], [60]],
        velocity: [[80], [80]],
      },
      stepIds: [["a"], ["copy-a"]],
    });
  });

  it("copies every aligned value into another row without removing the source", () => {
    const result = duplicateStepBetweenRows(
      {
        notes: [[60, 62], [65, 67]],
        velocity: [[80, 81], [90, 91]],
      },
      [["a", "b"], ["c", "d"]],
      0,
      1,
      "b",
      ["c", "b", "d"],
      "copy-b",
    );

    expect(result).toEqual({
      matrices: {
        notes: [[60, 62], [65, 62, 67]],
        velocity: [[80, 81], [90, 81, 91]],
      },
      stepIds: [["a", "b"], ["c", "copy-b", "d"]],
    });
  });

  it("maps a move-style same-row preview to the matching copy boundary", () => {
    const result = duplicateStepBetweenRows(
      { notes: [[60, 62, 64, 65]] },
      [["a", "b", "c", "d"]],
      0,
      0,
      "b",
      ["a", "c", "d", "b"],
      "copy-b",
    );

    expect(result).toEqual({
      matrices: { notes: [[60, 62, 64, 65, 62]] },
      stepIds: [["a", "b", "c", "d", "copy-b"]],
    });
  });

  it("accepts the stationary-source insertion boundary used by copy-drag previews", () => {
    const result = duplicateStepBetweenRows(
      { notes: [[60, 62, 64]] },
      [["a", "b", "c"]],
      0,
      0,
      "b",
      ["a", "b", "c"],
      "copy-b",
      3,
    );

    expect(result).toEqual({
      matrices: { notes: [[60, 62, 64, 62]] },
      stepIds: [["a", "b", "c", "copy-b"]],
    });
  });
});
