/** Global pulse length in quarter notes for one 1× step; matches PluginProcessor pulse table. */
export const pulseQuartersValues = [0.5, 1, 2, 4];

export const pulseCount = pulseQuartersValues.length;

/** Quarter note */
export const defaultPulseIndex = 1;

export const pulseOptions = [
  { index: 0, label: "8th" },
  { index: 1, label: "Quarter" },
  { index: 2, label: "Half" },
  { index: 3, label: "Whole" },
];

/** @param {number} pulseIndex */
export function pulseQuartersForIndex(pulseIndex) {
  const index = Math.min(pulseCount - 1, Math.max(0, Math.trunc(pulseIndex)));

  return pulseQuartersValues[index] ?? 1;
}
