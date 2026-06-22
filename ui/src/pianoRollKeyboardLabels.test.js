import { describe, expect, it } from "vitest";
import {
  isPianoRollKeyboardLabelMidi,
  pianoRollKeyboardLabelFontSizePx,
  pianoRollKeyboardLabelForMidi,
  pianoRollKeyboardOctave,
} from "./pianoRollKeyboardLabels.js";

describe("pianoRollKeyboardLabels", () => {
  it("marks C naturals only", () => {
    expect(isPianoRollKeyboardLabelMidi(36)).toBe(true);
    expect(isPianoRollKeyboardLabelMidi(37)).toBe(false);
    expect(isPianoRollKeyboardLabelMidi(48)).toBe(true);
  });

  it("formats octave labels", () => {
    expect(pianoRollKeyboardOctave(36)).toBe(1);
    expect(pianoRollKeyboardLabelForMidi(36)).toBe("C1");
    expect(pianoRollKeyboardLabelForMidi(60)).toBe("C3");
  });

  it("keeps a readable font size at compressed row heights", () => {
    expect(pianoRollKeyboardLabelFontSizePx(14)).toBe(9);
    expect(pianoRollKeyboardLabelFontSizePx(7)).toBe(7);
    expect(pianoRollKeyboardLabelFontSizePx(3)).toBe(6);
  });
});
