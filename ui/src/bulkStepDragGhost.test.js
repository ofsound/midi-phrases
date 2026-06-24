import { describe, expect, it } from "vitest";
import { applyDragCopyBadge, bulkGhostLayoutEntries } from "./bulkStepDragGhost.js";

function fakeElement() {
  /** @type {any[]} */
  const children = [];
  const element = {
    children,
    style: {
      values: new Map(),
      setProperty(name, value) {
        this.values.set(name, value);
      },
    },
    querySelectorAll(selector) {
      if (selector !== "[data-drag-copy-badge]") return [];

      return children.filter((child) => child.dataset.dragCopyBadge === "true");
    },
    appendChild(child) {
      child.remove = () => {
        const index = children.indexOf(child);

        if (index >= 0) children.splice(index, 1);
      };
      children.push(child);
    },
  };

  return element;
}

function fakeBadgeElement() {
  return {
    dataset: {},
    style: {
      values: new Map(),
      setProperty(name, value) {
        this.values.set(name, value);
      },
    },
    textContent: "",
    setAttribute() {},
    remove() {},
  };
}

describe("bulkGhostLayoutEntries", () => {
  it("maps snapshot widths into render order", () => {
    const snapshots = new Map([
      ["a", { widthPx: 50, gapBeforePx: 2, html: "" }],
      ["b", { widthPx: 70, gapBeforePx: 4, html: "" }],
    ]);

    expect(bulkGhostLayoutEntries(["a", "b"], snapshots)).toEqual([
      { stepId: "a", widthPx: 50, gapBeforePx: 2 },
      { stepId: "b", widthPx: 70, gapBeforePx: 4 },
    ]);
  });
});

describe("applyDragCopyBadge", () => {
  it("adds and removes a plus badge for duplicate drag previews", () => {
    const originalDocument = globalThis.document;
    // @ts-expect-error minimal fake document for this DOM-writing helper.
    globalThis.document = { createElement: () => fakeBadgeElement() };
    const element = fakeElement();

    try {
      applyDragCopyBadge(element, true);

      expect(element.children).toHaveLength(1);
      expect(element.children[0].textContent).toBe("+");
      expect(element.children[0].dataset.dragCopyBadge).toBe("true");

      applyDragCopyBadge(element, false);

      expect(element.children).toHaveLength(0);
    } finally {
      // @ts-expect-error restore possibly undefined document in the node test environment.
      globalThis.document = originalDocument;
    }
  });
});
