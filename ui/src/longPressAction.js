/** @typedef {{ duration?: number, disabled?: boolean, onLongPress?: () => void }} LongPressParams */

const DEFAULT_DURATION_MS = 800;
const MOVE_THRESHOLD_PX = 12;

/**
 * Fires `onLongPress` after the pointer is held without significant movement.
 * Ignores non-primary buttons and interactive descendants (buttons, inputs, etc.).
 *
 * @type {import('svelte/action').Action<HTMLElement, LongPressParams>}
 */
export function longPress(node, params = {}) {
  let duration = params.duration ?? DEFAULT_DURATION_MS;
  let disabled = params.disabled ?? false;
  let onLongPress = params.onLongPress ?? (() => {});

  /** @type {ReturnType<typeof setTimeout> | null} */
  let timer = null;
  let startX = 0;
  let startY = 0;
  /** @type {number | null} */
  let activePointerId = null;

  function clearTimer() {
    if (timer === null) return;

    clearTimeout(timer);
    timer = null;
  }

  /** @param {PointerEvent} event */
  function shouldIgnoreTarget(event) {
    const target = event.target;

    if (!(target instanceof Element)) return false;

    return Boolean(
      target.closest(
        "button, input, textarea, select, a, [contenteditable='true'], [data-no-long-press]",
      ),
    );
  }

  /** @param {PointerEvent} event */
  function handlePointerDown(event) {
    if (disabled || event.button !== 0 || shouldIgnoreTarget(event)) return;

    activePointerId = event.pointerId;
    startX = event.clientX;
    startY = event.clientY;
    clearTimer();

    timer = setTimeout(() => {
      timer = null;
      activePointerId = null;
      onLongPress();
    }, duration);
  }

  /** @param {PointerEvent} event */
  function handlePointerMove(event) {
    if (timer === null || event.pointerId !== activePointerId) return;

    if (Math.hypot(event.clientX - startX, event.clientY - startY) > MOVE_THRESHOLD_PX) {
      clearTimer();
      activePointerId = null;
    }
  }

  function handlePointerEnd() {
    clearTimer();
    activePointerId = null;
  }

  node.addEventListener("pointerdown", handlePointerDown);
  node.addEventListener("pointermove", handlePointerMove);
  node.addEventListener("pointerup", handlePointerEnd);
  node.addEventListener("pointercancel", handlePointerEnd);

  return {
    /** @param {LongPressParams} next */
    update(next) {
      duration = next.duration ?? DEFAULT_DURATION_MS;
      disabled = next.disabled ?? false;
      onLongPress = next.onLongPress ?? onLongPress;
    },
    destroy() {
      clearTimer();
      node.removeEventListener("pointerdown", handlePointerDown);
      node.removeEventListener("pointermove", handlePointerMove);
      node.removeEventListener("pointerup", handlePointerEnd);
      node.removeEventListener("pointercancel", handlePointerEnd);
    },
  };
}
