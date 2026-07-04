import { describe, expect, it } from "vitest";
import { buildRecordPianoKeys } from "./pianoKeyboardLayout.js";
import {
  clampScrollLeft,
  highestVisibleMidiFromScrollLeft,
  horizontalWheelDeltaPx,
  lowestVisibleMidiFromScrollLeft,
  octaveScrollDeltaPx,
  pianoKeyboardContentWidthPx,
  pianoKeyboardFullMidiRange,
  scrollLeftForLowestMidi,
  whiteKeyWidthPx,
} from "./pianoKeyboardScroll.js";

describe("pianoKeyboardScroll", () => {
  const range = pianoKeyboardFullMidiRange();
  const layout = buildRecordPianoKeys(range.lowest, range.highest);
  const keyWidth = whiteKeyWidthPx(150);

  it("sizes keys from keyboard height", () => {
    expect(whiteKeyWidthPx(150)).toBeCloseTo(150 * (23 / 150));
    expect(pianoKeyboardContentWidthPx(layout.whiteCount, keyWidth)).toBeCloseTo(
      layout.whiteCount * keyWidth,
    );
  });

  it("maps scroll position to visible MIDI range", () => {
    const scrollLeft = scrollLeftForLowestMidi(36, layout.whites, keyWidth);
    const lowest = lowestVisibleMidiFromScrollLeft(scrollLeft, layout.whites, keyWidth);
    const highest = highestVisibleMidiFromScrollLeft(scrollLeft, keyWidth * 21, layout.whites, keyWidth);

    expect(lowest).toBe(36);
    expect(highest).toBeGreaterThan(lowest);
  });

  it("clamps scroll within content bounds", () => {
    const contentWidth = pianoKeyboardContentWidthPx(layout.whiteCount, keyWidth);
    const viewportWidth = keyWidth * 21;

    expect(clampScrollLeft(-10, contentWidth, viewportWidth)).toBe(0);
    expect(clampScrollLeft(contentWidth, contentWidth, viewportWidth)).toBe(
      contentWidth - viewportWidth,
    );
  });

  it("uses seven white keys per octave scroll step", () => {
    expect(octaveScrollDeltaPx(keyWidth)).toBeCloseTo(7 * keyWidth);
  });

  it("prefers horizontal wheel delta and supports shift+vertical", () => {
    expect(
      horizontalWheelDeltaPx(
        /** @type {WheelEvent} */ ({
          deltaX: 12,
          deltaY: 2,
          shiftKey: false,
        }),
      ),
    ).toBe(12);

    expect(
      horizontalWheelDeltaPx(
        /** @type {WheelEvent} */ ({
          deltaX: 0,
          deltaY: 18,
          shiftKey: true,
        }),
      ),
    ).toBe(18);

    expect(
      horizontalWheelDeltaPx(
        /** @type {WheelEvent} */ ({
          deltaX: 0,
          deltaY: 18,
          shiftKey: false,
        }),
      ),
    ).toBe(0);
  });
});
