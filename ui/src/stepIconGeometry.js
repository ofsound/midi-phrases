/** Shared card bounds for step view mode toggle icons (24×24 viewBox). */
export const stepIconViewBox = "0 0 24 24";
export const stepIconStrokeWidth = 1.8;
export const stepIconHalfStroke = stepIconStrokeWidth / 2;
export const stepIconX = 5;
export const stepIconY = 3.5;
export const stepIconWidth = 14;
export const stepIconHeight = 17;
export const stepIconRx = 2;

/** Solid fill rect matching the outer edge of the stroked card. */
export const stepIconOuterRect = {
  x: stepIconX - stepIconHalfStroke,
  y: stepIconY - stepIconHalfStroke,
  width: stepIconWidth + stepIconStrokeWidth,
  height: stepIconHeight + stepIconStrokeWidth,
  rx: stepIconRx + stepIconHalfStroke,
};
