import {
  SHADOW_ITEM_MARKER_PROPERTY_NAME,
  SHADOW_PLACEHOLDER_ITEM_ID,
} from "svelte-dnd-action";

/** @param {{ id: string } & Record<string, unknown>} item */
export function isShadowItem(item) {
  return item.id === SHADOW_PLACEHOLDER_ITEM_ID || Boolean(item[SHADOW_ITEM_MARKER_PROPERTY_NAME]);
}

/** @param {{ id: string }[]} items */
export function withoutShadowItems(items) {
  return items.filter((item) => !isShadowItem(item));
}

/** @param {string[]} orderedIds @param {string[]} knownIds */
export function sanitizeOrderedIds(orderedIds, knownIds) {
  const known = new Set(knownIds);

  return orderedIds.filter((id) => known.has(id));
}

/**
 * Horizontal center of the insertion gap before `index`.
 *
 * @param {number[]} cellWidths
 * @param {number} index
 * @param {number} leadingInset
 * @param {number} gap
 */
export function placementIndicatorLeftPx(cellWidths, index, leadingInset, gap) {
  const clampedIndex = Math.min(cellWidths.length, Math.max(0, Math.trunc(index)));
  let left = 0;

  for (let cell = 0; cell < clampedIndex; cell += 1) {
    left += cellWidths[cell] + (cell === 0 ? leadingInset : gap);
  }

  return left + (clampedIndex === 0 ? leadingInset : gap) / 2;
}

/**
 * Insertion boundary from a horizontal pointer coordinate and visible step rects.
 * The left half of a step resolves before that step; the right half resolves after it.
 *
 * @param {{ step: number, left: number, width: number }[]} cells
 * @param {number} clientX
 * @param {number} maxIndex
 */
export function insertionIndexFromCellMidpoints(cells, clientX, maxIndex) {
  const clampedMax = Math.max(0, Math.trunc(maxIndex));
  const sortedCells = cells
    .filter((cell) => Number.isFinite(cell.step) && Number.isFinite(cell.left) && cell.width > 0)
    .slice()
    .sort((left, right) => left.left - right.left);

  if (sortedCells.length === 0) return 0;

  let insertionIndex = 0;

  for (const cell of sortedCells) {
    const step = Math.min(clampedMax, Math.max(0, Math.trunc(cell.step)));

    if (clientX < cell.left + cell.width / 2) {
      return step;
    }

    insertionIndex = Math.min(clampedMax, step + 1);
  }

  return insertionIndex;
}
