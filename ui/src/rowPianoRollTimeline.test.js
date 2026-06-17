import { describe, expect, it } from "vitest";
import { buildRowRollTimeline } from "./rowPianoRollTimeline.js";

describe("buildRowRollTimeline", () => {
  it("applies row timing offset to step start positions", () => {
    const { slots } = buildRowRollTimeline([3, 3], [], 1, 4);

    expect(slots[0].startQuarters).toBe(0.25);
    expect(slots[1].startQuarters).toBe(1.25);
  });

  it("uses pulse length when computing slot durations", () => {
    const quarterPulse = buildRowRollTimeline([3], [], 1, 3);
    const halfPulse = buildRowRollTimeline([3], [], 2, 3);

    expect(quarterPulse.slots[0].lengthQuarters).toBe(1);
    expect(halfPulse.slots[0].lengthQuarters).toBe(2);
  });
});
