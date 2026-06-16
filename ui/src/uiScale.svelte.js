export const uiDesignWidthPx = 1670;
export const uiDesignHeightPluginPx = 980;
export const uiDesignHeightStandalonePx = 1044;
export const uiMinScale = 0.6;
export const uiMaxScale = 1;
export const uiBaseFontSizePx = 16;

export const uiScaleState = $state({
  scale: 1,
  widthPx: uiDesignWidthPx,
  heightPx: uiDesignHeightPluginPx,
  standaloneTransportAvailable: false,
});

/** @param {number} value @param {number} min @param {number} max */
function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

/**
 * @param {{ widthPx: number, heightPx: number, standaloneTransportAvailable?: boolean }} params
 */
export function computeUiScale({ widthPx, heightPx, standaloneTransportAvailable = false }) {
  const designHeightPx = standaloneTransportAvailable
    ? uiDesignHeightStandalonePx
    : uiDesignHeightPluginPx;
  const widthScale = widthPx < uiDesignWidthPx ? widthPx / uiDesignWidthPx : 1;
  const heightScale = heightPx < designHeightPx ? heightPx / designHeightPx : 1;

  return clamp(Math.min(widthScale, heightScale), uiMinScale, uiMaxScale);
}

/** @param {number} scale */
export function applyUiScaleToDocument(scale) {
  if (typeof document === "undefined") return;

  const root = document.documentElement;
  root.style.fontSize = `${uiBaseFontSizePx * scale}px`;
  root.style.setProperty("--mp-ui-scale", String(scale));
}

/**
 * @param {{ widthPx: number, heightPx: number, standaloneTransportAvailable?: boolean }} params
 */
export function setUiViewportSize({ widthPx, heightPx, standaloneTransportAvailable = false }) {
  const scale = computeUiScale({ widthPx, heightPx, standaloneTransportAvailable });

  uiScaleState.widthPx = widthPx;
  uiScaleState.heightPx = heightPx;
  uiScaleState.standaloneTransportAvailable = standaloneTransportAvailable;
  uiScaleState.scale = scale;
  applyUiScaleToDocument(scale);

  return scale;
}

/** @param {number} basePx */
export function scaledPx(basePx) {
  return basePx * uiScaleState.scale;
}
