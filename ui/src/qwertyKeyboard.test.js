import { describe, expect, it } from "vitest";
import {
  clampQwertyOctaveOffset,
  midiFromQwertyCode,
  qwertyBaseMidi,
  qwertyOctaveDownCode,
  qwertyOctaveUpCode,
} from "./qwertyKeyboard.js";

describe("qwertyKeyboard", () => {
  it("maps A to the base note at octave offset zero", () => {
    expect(midiFromQwertyCode("KeyA", 0)).toBe(qwertyBaseMidi);
  });

  it("maps W to the semitone above the base note", () => {
    expect(midiFromQwertyCode("KeyW", 0)).toBe(qwertyBaseMidi + 1);
  });

  it("shifts mapped notes by octave offset", () => {
    expect(midiFromQwertyCode("KeyA", 1)).toBe(qwertyBaseMidi + 12);
    expect(midiFromQwertyCode("KeyA", -1)).toBe(qwertyBaseMidi - 12);
  });

  it("clamps octave offsets so mapped notes stay in MIDI range", () => {
    expect(clampQwertyOctaveOffset(-10)).toBeLessThan(0);
    expect(midiFromQwertyCode("Semicolon", clampQwertyOctaveOffset(10))).not.toBeNull();
    expect(midiFromQwertyCode("Semicolon", clampQwertyOctaveOffset(-10))).not.toBeNull();
  });

  it("identifies octave shift keys", () => {
    expect(qwertyOctaveDownCode).toBe("KeyZ");
    expect(qwertyOctaveUpCode).toBe("KeyX");
  });
});
