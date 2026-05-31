export const loopBraceSnapQuarters = 0.5;

/** @param {number} value */
export function snapLoopBraceBeat(value) {
  return Math.round(value / loopBraceSnapQuarters) * loopBraceSnapQuarters;
}

/** @param {number} start @param {number} end @param {number} maxBeat @param {number} [minSpan] */
export function clampLoopBrace(start, end, maxBeat, minSpan = loopBraceSnapQuarters) {
  let nextStart = Math.max(0, snapLoopBraceBeat(start));
  let nextEnd = Math.max(nextStart + minSpan, snapLoopBraceBeat(end));
  nextEnd = Math.min(maxBeat, nextEnd);
  nextStart = Math.min(nextStart, nextEnd - minSpan);

  return { start: nextStart, end: nextEnd };
}

/** @param {number} clientX @param {HTMLElement} scrollElement @param {number} pxPerQuarter */
export function beatFromClientX(clientX, scrollElement, pxPerQuarter) {
  const rect = scrollElement.getBoundingClientRect();
  const x = clientX - rect.left + scrollElement.scrollLeft;

  return Math.max(0, snapLoopBraceBeat(x / pxPerQuarter));
}
