import { describe, expect, it } from "vitest";
import {
  buildRecordPianoKeys,
  pianoKeyboardAspectRatio,
  pianoWhiteKeyWidthToHeight,
  recordPianoMidiRange,
  scalePreviewMidiRange,
  stepInspectorMidiRange,
} from "./pianoKeyboardLayout.js";

describe("pianoKeyboardAspectRatio", () => {
  it("scales with the visible white-key count", () => {
    const { whiteCount } = buildRecordPianoKeys(36, 83);
    expect(pianoKeyboardAspectRatio(whiteCount)).toBeCloseTo(whiteCount * pianoWhiteKeyWidthToHeight);
  });

  it("matches layout ranges used by the keyboards", () => {
    const recordRange = recordPianoMidiRange(3);
    const inspectorRange = stepInspectorMidiRange(0);
    const previewRange = scalePreviewMidiRange(0);

    const record = buildRecordPianoKeys(recordRange.lowest, recordRange.highest);
    const inspector = buildRecordPianoKeys(inspectorRange.lowest, inspectorRange.highest);
    const preview = buildRecordPianoKeys(previewRange.lowest, previewRange.highest);

    expect(pianoKeyboardAspectRatio(record.whiteCount)).toBeGreaterThan(
      pianoKeyboardAspectRatio(inspector.whiteCount),
    );
    expect(pianoKeyboardAspectRatio(inspector.whiteCount)).toBeGreaterThan(
      pianoKeyboardAspectRatio(preview.whiteCount),
    );
  });
});
