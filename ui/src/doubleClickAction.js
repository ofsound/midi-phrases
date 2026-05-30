/** @typedef {{ intervalMs?: number, maxDistancePx?: number, disabled?: boolean, shouldIgnore?: (event: PointerEvent) => boolean, onDoubleClick?: (event: PointerEvent) => void }} DoubleClickParams */

const DEFAULT_INTERVAL_MS = 400;
const DEFAULT_DISTANCE_PX = 16;

/**
 * Detects double primary-button taps/clicks via pointerup timing.
 * More reliable than `dblclick` in embedded JUCE WebViews on empty regions.
 *
 * @type {import('svelte/action').Action<HTMLElement, DoubleClickParams>}
 */
export function doubleClick(node, params = {}) {
  let intervalMs = params.intervalMs ?? DEFAULT_INTERVAL_MS;
  let maxDistancePx = params.maxDistancePx ?? DEFAULT_DISTANCE_PX;
  let disabled = params.disabled ?? false;
  let shouldIgnore = params.shouldIgnore ?? (() => false);
  let onDoubleClick = params.onDoubleClick ?? (() => {});

  let lastUpTime = 0;
  let lastUpX = 0;
  let lastUpY = 0;

  /** @param {PointerEvent} event */
  function handlePointerUp(event) {
    if (disabled || event.button !== 0 || shouldIgnore(event)) return;

    const now = performance.now();
    const elapsed = now - lastUpTime;
    const distance = Math.hypot(event.clientX - lastUpX, event.clientY - lastUpY);

    if (lastUpTime > 0 && elapsed <= intervalMs && distance <= maxDistancePx) {
      lastUpTime = 0;
      onDoubleClick(event);
      return;
    }

    lastUpTime = now;
    lastUpX = event.clientX;
    lastUpY = event.clientY;
  }

  node.addEventListener("pointerup", handlePointerUp);

  return {
    /** @param {DoubleClickParams} next */
    update(next) {
      intervalMs = next.intervalMs ?? DEFAULT_INTERVAL_MS;
      maxDistancePx = next.maxDistancePx ?? DEFAULT_DISTANCE_PX;
      disabled = next.disabled ?? false;
      shouldIgnore = next.shouldIgnore ?? shouldIgnore;
      onDoubleClick = next.onDoubleClick ?? onDoubleClick;
    },
    destroy() {
      node.removeEventListener("pointerup", handlePointerUp);
    },
  };
}
