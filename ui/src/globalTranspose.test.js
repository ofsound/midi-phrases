import { describe, expect, it } from "vitest";
import {
  applyGlobalTranspose,
  clampGlobalTransposeSemitones,
  formatGlobalTransposeSemitones,
} from "./globalTranspose.js";

describe("global transpose", () => {
  it("applies a remembered semitone offset and clamps only output notes", () => {
    const source = [{ midi: 60 }, { midi: 125 }];

    expect(applyGlobalTranspose(source, 5).map((event) => event.midi)).toEqual([65, 127]);
    expect(source.map((event) => event.midi)).toEqual([60, 125]);
  });

  it("clamps and formats the stored offset", () => {
    expect(clampGlobalTransposeSemitones(99)).toBe(48);
    expect(clampGlobalTransposeSemitones(-99)).toBe(-48);
    expect(formatGlobalTransposeSemitones(5)).toBe("+5");
    expect(formatGlobalTransposeSemitones(0)).toBe("0");
  });
});
