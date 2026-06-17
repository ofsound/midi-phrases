export const uiDesignWidthPx = 1670;
export const uiDesignHeightPluginPx = 980;
export const uiDesignHeightStandalonePx = 1044;
export const uiMinScale = 0.7;
export const uiMaxScale = 1;
export const uiBaseFontSizePx = 16;
export const uiScaleStorageKey = "midiPhrasesUiScalePreset";

export const uiScalePresetOptions = [
  { value: "70", label: "70", scale: 0.7, minWidthPx: 1050, minHeightPluginPx: 620, minHeightStandalonePx: 660 },
  { value: "80", label: "80", scale: 0.8, minWidthPx: 1200, minHeightPluginPx: 700, minHeightStandalonePx: 744 },
  { value: "90", label: "90", scale: 0.9, minWidthPx: 1350, minHeightPluginPx: 790, minHeightStandalonePx: 836 },
  { value: "100", label: "100", scale: 1, minWidthPx: 1500, minHeightPluginPx: 875, minHeightStandalonePx: 935 },
];

export const uiScaleState = $state({
  scale: 1,
  presetValue: "100",
  widthPx: uiDesignWidthPx,
  heightPx: uiDesignHeightPluginPx,
  standaloneTransportAvailable: false,
});

/** @param {number} value @param {number} min @param {number} max */
function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

/** @param {string | number | null | undefined} value */
export function uiScalePresetForValue(value) {
  const normalized = String(value ?? "");

  return uiScalePresetOptions.find((option) => option.value === normalized) ?? uiScalePresetOptions.at(-1);
}

/**
 * @param {{ widthPx: number, heightPx: number, standaloneTransportAvailable?: boolean }} params
 */
export function computeUiScale({ widthPx, heightPx, standaloneTransportAvailable = false }) {
  void widthPx;
  void heightPx;
  void standaloneTransportAvailable;

  return uiScaleState.scale;
}

/** @param {number} scale */
export function applyUiScaleToDocument(scale) {
  if (typeof document === "undefined") return;

  const root = document.documentElement;
  root.style.fontSize = `${uiBaseFontSizePx * scale}px`;
  root.style.setProperty("--mp-ui-scale", String(scale));
}

export function storedUiScalePreset() {
  if (typeof localStorage === "undefined") return "100";

  return uiScalePresetForValue(localStorage.getItem(uiScaleStorageKey))?.value ?? "100";
}

/**
 * @param {string | number} presetValue
 * @param {{ persist?: boolean }} [options]
 */
export function setUiScalePreset(presetValue, { persist = true } = {}) {
  const preset = uiScalePresetForValue(presetValue);
  const scale = clamp(preset.scale, uiMinScale, uiMaxScale);

  uiScaleState.presetValue = preset.value;
  uiScaleState.scale = scale;
  applyUiScaleToDocument(scale);

  if (persist && typeof localStorage !== "undefined") {
    localStorage.setItem(uiScaleStorageKey, preset.value);
  }

  return preset;
}

/** @param {{ standaloneTransportAvailable?: boolean }} [params] */
export function currentUiScaleMinimumSize({ standaloneTransportAvailable = uiScaleState.standaloneTransportAvailable } = {}) {
  const preset = uiScalePresetForValue(uiScaleState.presetValue);

  return {
    widthPx: preset.minWidthPx,
    heightPx: standaloneTransportAvailable ? preset.minHeightStandalonePx : preset.minHeightPluginPx,
  };
}

/**
 * @param {{ widthPx: number, heightPx: number, standaloneTransportAvailable?: boolean }} params
 */
export function setUiViewportSize({ widthPx, heightPx, standaloneTransportAvailable = false }) {
  uiScaleState.widthPx = widthPx;
  uiScaleState.heightPx = heightPx;
  uiScaleState.standaloneTransportAvailable = standaloneTransportAvailable;

  return uiScaleState.scale;
}

/** @param {number} basePx */
export function scaledPx(basePx) {
  return basePx * uiScaleState.scale;
}
