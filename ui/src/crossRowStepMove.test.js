import { describe, expect, it } from "vitest";
import { moveStepBetweenRows } from "./crossRowStepMove.js";

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
