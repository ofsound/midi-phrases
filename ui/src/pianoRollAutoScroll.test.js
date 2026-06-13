import { describe, expect, it } from "vitest";
import { pagedPlaybackScrollLeft } from "./pianoRollAutoScroll.js";

describe("pagedPlaybackScrollLeft", () => {
  it("keeps the current page while the playhead remains visible", () => {
    expect(pagedPlaybackScrollLeft(199, 100, 100, 400)).toBe(100);
  });

  it("jumps forward by a page when the playhead reaches the right edge", () => {
    expect(pagedPlaybackScrollLeft(200, 100, 100, 400)).toBe(200);
  });

  it("uses full viewport jumps from a manually offset scroll position", () => {
    expect(pagedPlaybackScrollLeft(175, 75, 100, 400)).toBe(175);
  });

  it("jumps to the page containing the playhead if a frame skips ahead", () => {
    expect(pagedPlaybackScrollLeft(361, 100, 100, 400)).toBe(300);
  });

  it("jumps back to the page containing the playhead after loop wrap", () => {
    expect(pagedPlaybackScrollLeft(24, 300, 100, 400)).toBe(0);
  });

  it("uses full viewport jumps when paging backward from a manual offset", () => {
    expect(pagedPlaybackScrollLeft(220, 350, 100, 400)).toBe(150);
  });

  it("clamps to the end of the roll", () => {
    expect(pagedPlaybackScrollLeft(520, 300, 100, 430)).toBe(430);
  });

  it("ignores zero-width viewports", () => {
    expect(pagedPlaybackScrollLeft(200, 100, 0, 400)).toBe(100);
  });
});
