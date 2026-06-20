/**
 * @typedef {{ widthPx: number, gapBeforePx: number, html: string }} BulkStepGhostSnapshot
 */

/**
 * @param {ParentNode | null | undefined} zone
 * @param {string[]} blockIds
 * @param {(px: number) => number} scalePx
 * @param {number} insertGapPx
 * @param {number} leadingPaddingPx
 * @returns {Map<string, BulkStepGhostSnapshot>}
 */
export function captureBulkStepGhostSnapshots(
  zone,
  blockIds,
  scalePx,
  insertGapPx,
  leadingPaddingPx,
) {
  /** @type {Map<string, BulkStepGhostSnapshot>} */
  const snapshots = new Map();

  if (!zone) return snapshots;

  blockIds.forEach((stepId, index) => {
    const cell = zone.querySelector(`[data-bulk-step-cell][data-step-id="${stepId}"]`);
    const inner = cell?.querySelector(".pointer-events-auto");

    if (!(cell instanceof HTMLElement) || !(inner instanceof HTMLElement)) return;

    snapshots.set(stepId, {
      widthPx: scalePx(cell.offsetWidth),
      gapBeforePx: index === 0 ? scalePx(leadingPaddingPx) : scalePx(insertGapPx),
      html: inner.innerHTML,
    });
  });

  return snapshots;
}

/**
 * @param {string[]} blockIds
 * @param {Map<string, BulkStepGhostSnapshot>} snapshots
 */
export function bulkGhostLayoutEntries(blockIds, snapshots) {
  return blockIds.map((stepId) => ({
    stepId,
    widthPx: snapshots.get(stepId)?.widthPx ?? 0,
    gapBeforePx: snapshots.get(stepId)?.gapBeforePx ?? 0,
  }));
}

/**
 * @param {HTMLElement} element
 * @param {string[]} blockIds
 * @param {Map<string, BulkStepGhostSnapshot>} snapshots
 * @param {boolean} [isDuplicate]
 */
export function applyBulkStepDragGhost(element, blockIds, snapshots, isDuplicate = false) {
  if (blockIds.length < 2 || snapshots.size === 0) return;

  const ghostKey = blockIds.join("|");

  if (
    element.dataset.bulkGhostKey === ghostKey
    && element.querySelector("[data-bulk-drag-ghost-row]")
  ) {
    return;
  }

  element.dataset.bulkDragGhost = "true";
  element.dataset.bulkGhostKey = ghostKey;
  element.style.setProperty("display", "flex", "important");
  element.style.setProperty("flex-direction", "row", "important");
  element.style.setProperty("align-items", "stretch", "important");
  element.style.setProperty("width", "auto", "important");
  element.style.setProperty("height", "auto", "important");
  element.style.setProperty("margin", "0", "important");
  element.style.setProperty("padding", "0", "important");
  element.style.setProperty("background", "transparent", "important");
  element.style.setProperty(
    "box-shadow",
    isDuplicate
      ? "0 10px 28px color-mix(in srgb, var(--color-text-secondary, #94a3b8) 35%, transparent)"
      : "0 10px 28px color-mix(in srgb, #000 38%, transparent)",
    "important",
  );
  element.style.setProperty("opacity", "1", "important");
  element.style.setProperty("visibility", "visible", "important");
  element.style.setProperty("outline", "none", "important");

  const row = document.createElement("div");
  row.setAttribute("data-bulk-drag-ghost-row", "true");
  row.className = "flex items-stretch";

  let totalWidth = 0;

  for (const stepId of blockIds) {
    const snapshot = snapshots.get(stepId);

    if (!snapshot) continue;

    const shell = document.createElement("div");
    shell.style.flexShrink = "0";
    shell.style.width = `${snapshot.widthPx}px`;
    shell.style.minWidth = `${snapshot.widthPx}px`;
    shell.style.maxWidth = `${snapshot.widthPx}px`;
    shell.style.marginLeft = `${snapshot.gapBeforePx}px`;
    totalWidth += snapshot.widthPx + snapshot.gapBeforePx;

    const content = document.createElement("div");
    content.className = "pointer-events-none h-full overflow-visible";
    content.innerHTML = snapshot.html;
    content.querySelectorAll("[data-remove-button], [data-insert-slot], [data-multiplier-resize]")
      .forEach((node) => {
        if (node instanceof HTMLElement) node.style.display = "none";
      });
    content.querySelectorAll(".border-2").forEach((node) => {
      if (node instanceof HTMLElement) {
        node.style.setProperty("outline", "none");
        node.style.setProperty("--tw-ring-shadow", "0 0 #0000");
        node.style.setProperty("--tw-ring-width", "0px");
      }
    });
    shell.appendChild(content);
    row.appendChild(shell);
  }

  element.replaceChildren(row);
  element.style.setProperty("width", `${totalWidth}px`, "important");
}
