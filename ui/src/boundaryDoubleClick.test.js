import { describe, expect, it } from "vitest";
import { boundaryDoubleClickCommand } from "./boundaryDoubleClick.js";

describe("boundaryDoubleClickCommand", () => {
  it("inserts a default step on an unmodified boundary double-click", () => {
    expect(boundaryDoubleClickCommand({
      altKey: false,
      insertStep: 1,
      stepCount: 2,
      maxStepCount: 64,
    })).toEqual({ type: "insert", insertStep: 1 });
  });

  it("duplicates the left step on Option-double-click between steps", () => {
    expect(boundaryDoubleClickCommand({
      altKey: true,
      insertStep: 1,
      stepCount: 2,
      maxStepCount: 64,
    })).toEqual({ type: "duplicate", insertStep: 1 });
  });

  it("duplicates the final step at the row-end boundary", () => {
    expect(boundaryDoubleClickCommand({
      altKey: true,
      insertStep: 3,
      stepCount: 3,
      maxStepCount: 64,
    })).toEqual({ type: "duplicate", insertStep: 3 });
  });

  it("inserts at the leading boundary", () => {
    expect(boundaryDoubleClickCommand({
      altKey: false,
      insertStep: 0,
      stepCount: 2,
      maxStepCount: 64,
    })).toEqual({ type: "insert", insertStep: 0 });
  });

  it("rejects leading-boundary duplicate and full rows", () => {
    expect(boundaryDoubleClickCommand({
      altKey: true,
      insertStep: 0,
      stepCount: 2,
      maxStepCount: 64,
    })).toBeNull();
    expect(boundaryDoubleClickCommand({
      altKey: true,
      insertStep: 64,
      stepCount: 64,
      maxStepCount: 64,
    })).toBeNull();
  });
});
