import { describe, expect, it } from "vitest";
import { bulkGhostLayoutEntries } from "./bulkStepDragGhost.js";

describe("bulkGhostLayoutEntries", () => {
  it("maps snapshot widths into render order", () => {
    const snapshots = new Map([
      ["a", { widthPx: 50, gapBeforePx: 2, html: "" }],
      ["b", { widthPx: 70, gapBeforePx: 4, html: "" }],
    ]);

    expect(bulkGhostLayoutEntries(["a", "b"], snapshots)).toEqual([
      { stepId: "a", widthPx: 50, gapBeforePx: 2 },
      { stepId: "b", widthPx: 70, gapBeforePx: 4 },
    ]);
  });
});
