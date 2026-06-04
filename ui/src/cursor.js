const dragCursorMap = {
  grab: "grabbing",
};

let installed = false;
let hoverCursor = "";
let activeCursor = "";

function isDisabledCursorElement(element) {
  return (
    (element instanceof HTMLButtonElement && element.disabled)
    || element.getAttribute("aria-disabled") === "true"
  );
}

function cursorForElement(element) {
  if (isDisabledCursorElement(element)) return "default";

  return element.getAttribute("data-cursor") || "";
}

function syncCursorElement(element) {
  if (!(element instanceof HTMLElement)) return;
  if (!element.hasAttribute("data-cursor")) return;

  const cursor = cursorForElement(element);
  if (element.style.cursor !== cursor) element.style.cursor = cursor;
}

function syncCursorTree(root) {
  if (!(root instanceof Element)) return;

  syncCursorElement(root);

  for (const element of root.querySelectorAll("[data-cursor]")) {
    syncCursorElement(element);
  }
}

function applyCursor() {
  const cursor = activeCursor || hoverCursor;

  if (cursor) {
    document.body.dataset.cursor = cursor;
    document.body.style.cursor = cursor;
  } else {
    delete document.body.dataset.cursor;
    document.body.style.removeProperty("cursor");
  }
}

function cursorFromTarget(target) {
  if (!(target instanceof Element)) return "";

  const cursorElement = target.closest("[data-cursor]");
  if (!cursorElement) return "";

  return cursorForElement(cursorElement);
}

function setHoverCursor(cursor) {
  hoverCursor = cursor;
  applyCursor();
}

export function setActiveCursor(cursor) {
  activeCursor = cursor;
  applyCursor();
}

export function clearActiveCursor(cursor) {
  if (cursor === undefined || activeCursor === cursor) {
    activeCursor = "";
    applyCursor();
  }
}

export function installCursorManager() {
  if (installed) return;

  installed = true;

  const syncHoverCursor = (event) => {
    setHoverCursor(cursorFromTarget(event.target));
  };

  const beginPointerCursor = (event) => {
    const cursor = cursorFromTarget(event.target);
    if (!cursor || cursor === "default") return;

    setActiveCursor(dragCursorMap[cursor] || cursor);
  };

  const endPointerCursor = () => {
    clearActiveCursor();
  };

  syncCursorTree(document.documentElement);

  const observer = new MutationObserver((mutations) => {
    for (const mutation of mutations) {
      if (mutation.type === "childList") {
        for (const node of mutation.addedNodes) {
          syncCursorTree(node);
        }

        continue;
      }

      syncCursorTree(mutation.target);
    }
  });

  observer.observe(document.documentElement, {
    attributes: true,
    attributeFilter: ["aria-disabled", "data-cursor", "disabled", "style"],
    childList: true,
    subtree: true,
  });

  document.addEventListener("pointerover", syncHoverCursor, true);
  document.addEventListener("pointermove", syncHoverCursor, true);
  document.addEventListener("pointerdown", beginPointerCursor, true);
  document.addEventListener("pointerup", endPointerCursor, true);
  document.addEventListener("pointercancel", endPointerCursor, true);
  document.addEventListener("mousemove", syncHoverCursor, true);

  window.addEventListener("blur", () => {
    hoverCursor = "";
    activeCursor = "";
    applyCursor();
  });

  window.addEventListener("mouseout", (event) => {
    if (event.relatedTarget === null) setHoverCursor("");
  });
}
