/**
 * Returns the horizontal piano-roll scroll position that keeps a playback
 * playhead visible using page-sized jumps.
 *
 * @param {number} playheadLeftPx
 * @param {number} viewportScrollLeftPx
 * @param {number} viewportWidthPx
 * @param {number} maxScrollLeftPx
 */
export function pagedPlaybackScrollLeft(
  playheadLeftPx,
  viewportScrollLeftPx,
  viewportWidthPx,
  maxScrollLeftPx,
) {
  if (viewportWidthPx <= 0) return viewportScrollLeftPx;

  const viewportRightPx = viewportScrollLeftPx + viewportWidthPx;
  const playheadIsVisible =
    playheadLeftPx >= viewportScrollLeftPx && playheadLeftPx < viewportRightPx;

  if (playheadIsVisible) return viewportScrollLeftPx;

  if (playheadLeftPx >= viewportRightPx) {
    const pagesPastRight =
      Math.floor((playheadLeftPx - viewportRightPx) / viewportWidthPx) + 1;

    return Math.min(
      viewportScrollLeftPx + pagesPastRight * viewportWidthPx,
      Math.max(0, maxScrollLeftPx),
    );
  }

  const pagesBeforeLeft =
    Math.floor((viewportScrollLeftPx - playheadLeftPx) / viewportWidthPx) + 1;

  return Math.max(0, viewportScrollLeftPx - pagesBeforeLeft * viewportWidthPx);
}
