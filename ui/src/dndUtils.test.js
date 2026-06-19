import { describe, expect, it } from "vitest";
import { placementIndicatorLeftPx } from "./dndUtils.js";

describe("placementIndicatorLeftPx", () => {
  it("centers the indicator in the leading inset", () => {
    expect(placementIndicatorLeftPx([100, 80], 0, 12, 8)).toBe(6);
  });

  it("centers the indicator in a gap between steps", () => {
    expect(placementIndicatorLeftPx([100, 80], 1, 12, 8)).toBe(116);
  });

  it("supports the trailing insertion boundary", () => {
    expect(placementIndicatorLeftPx([100, 80], 2, 12, 8)).toBe(204);
  });
});
