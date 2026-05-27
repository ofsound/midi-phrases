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
