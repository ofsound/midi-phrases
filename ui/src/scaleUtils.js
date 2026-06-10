export const keyCenters = [
  {label: "C", value: 0},
  {label: "C#", value: 1},
  {label: "D", value: 2},
  {label: "D#", value: 3},
  {label: "E", value: 4},
  {label: "F", value: 5},
  {label: "F#", value: 6},
  {label: "G", value: 7},
  {label: "G#", value: 8},
  {label: "A", value: 9},
  {label: "A#", value: 10},
  {label: "B", value: 11},
];

export const scaleModes = [
  {label: "Chromatic", shortLabel: "Chrom", intervals: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]},
  {label: "Major", shortLabel: "Maj", intervals: [0, 2, 4, 5, 7, 9, 11]},
  {label: "Natural Minor", shortLabel: "Min", intervals: [0, 2, 3, 5, 7, 8, 10]},
  {label: "Dorian", shortLabel: "Dor", intervals: [0, 2, 3, 5, 7, 9, 10]},
  {label: "Phrygian", shortLabel: "Phr", intervals: [0, 1, 3, 5, 7, 8, 10]},
  {label: "Lydian", shortLabel: "Lyd", intervals: [0, 2, 4, 6, 7, 9, 11]},
  {label: "Mixolydian", shortLabel: "Mix", intervals: [0, 2, 4, 5, 7, 9, 10]},
  {label: "Locrian", shortLabel: "Loc", intervals: [0, 1, 3, 5, 6, 8, 10]},
  {label: "Harmonic Minor", shortLabel: "Har", intervals: [0, 2, 3, 5, 7, 8, 11]},
  {label: "Melodic Minor", shortLabel: "Mel", intervals: [0, 2, 3, 5, 7, 9, 11]},
  {label: "Major Pentatonic", shortLabel: "Maj P", intervals: [0, 2, 4, 7, 9]},
  {label: "Minor Pentatonic", shortLabel: "Min P", intervals: [0, 3, 5, 7, 10]},
  {label: "Blues", shortLabel: "Blue", intervals: [0, 3, 5, 6, 7, 10]},
  {label: "Whole Tone", shortLabel: "Whole", intervals: [0, 2, 4, 6, 8, 10]},
  {label: "Diminished Whole-Half", shortLabel: "W-H", intervals: [0, 2, 3, 5, 6, 8, 9, 11]},
  {label: "Diminished Half-Whole", shortLabel: "H-W", intervals: [0, 1, 3, 4, 6, 7, 9, 10]},
];

export const defaultScaleRoot = 0;
export const defaultScaleModeIndex = 0;

export function clampScaleRoot(root) {
  const value = Number.parseInt(String(root), 10);
  return Number.isNaN(value) ? defaultScaleRoot : Math.min(11, Math.max(0, value));
}

export function clampScaleModeIndex(modeIndex) {
  const value = Number.parseInt(String(modeIndex), 10);
  return Number.isNaN(value) ? defaultScaleModeIndex : Math.min(scaleModes.length - 1, Math.max(0, value));
}

export function isChromaticScaleMode(modeIndex) {
  return clampScaleModeIndex(modeIndex) === defaultScaleModeIndex;
}

export function scaleName(root, modeIndex) {
  const key = keyCenters[clampScaleRoot(root)]?.label ?? "C";
  const mode = scaleModes[clampScaleModeIndex(modeIndex)]?.label ?? "Chromatic";
  return `${key} ${mode}`;
}

function pitchClassInScale(midi, root, modeIndex) {
  const mode = scaleModes[clampScaleModeIndex(modeIndex)] ?? scaleModes[0];
  const pitchClass = ((Math.round(midi) % 12) + 12) % 12;
  const relative = (pitchClass - clampScaleRoot(root) + 12) % 12;
  return mode.intervals.includes(relative);
}

export function isMidiInScale(midi, root, modeIndex) {
  return pitchClassInScale(midi, root, modeIndex);
}

function clampMidi(note) {
  return Math.min(127, Math.max(0, Math.round(note)));
}

/** Snap to the nearest in-scale MIDI note (chromatic mode is a no-op). */
export function snapMidiToScale(note, root, modeIndex) {
  const clamped = clampMidi(note);

  if (pitchClassInScale(clamped, root, modeIndex)) {
    return clamped;
  }

  for (let distance = 1; distance <= 6; distance += 1) {
    const up = clamped + distance;
    const down = clamped - distance;
    const upInScale = up <= 127 && pitchClassInScale(up, root, modeIndex);
    const downInScale = down >= 0 && pitchClassInScale(down, root, modeIndex);

    if (upInScale && downInScale) {
      return down;
    }

    if (upInScale) {
      return up;
    }

    if (downInScale) {
      return down;
    }
  }

  return clamped;
}

export function transposeMidiByScaleDegrees(note, degreeDelta, root, modeIndex) {
  const delta = Math.round(degreeDelta);
  let current = clampMidi(note);

  if (delta === 0) return current;

  const direction = delta > 0 ? 1 : -1;
  let remaining = Math.abs(delta);

  while (remaining > 0) {
    if ((direction > 0 && current >= 127) || (direction < 0 && current <= 0)) {
      return current;
    }

    current += direction;

    if (pitchClassInScale(current, root, modeIndex)) {
      remaining -= 1;
    }
  }

  return current;
}

/** Signed scale-degree steps from one MIDI note to another (chromatic mode counts semitones). */
export function scaleDegreeDelta(fromNote, toNote, root, modeIndex) {
  const from = clampMidi(fromNote);
  const to = clampMidi(toNote);

  if (from === to) return 0;

  const direction = to > from ? 1 : -1;
  let current = from;
  let degrees = 0;

  while ((direction > 0 && current < to) || (direction < 0 && current > to)) {
    if ((direction > 0 && current >= 127) || (direction < 0 && current <= 0)) {
      break;
    }

    current += direction;

    if (pitchClassInScale(current, root, modeIndex)) {
      degrees += direction;
    }
  }

  return degrees;
}

/** Echo / Cross-Mod pitch: apply a row's scale-degree contour to the carrier note. */
export function echoNoteFromModStep(carrierNote, modBaseNote, modStepNote, root, modeIndex) {
  const degreeDelta = scaleDegreeDelta(modBaseNote, modStepNote, root, modeIndex);
  return transposeMidiByScaleDegrees(carrierNote, degreeDelta, root, modeIndex);
}
