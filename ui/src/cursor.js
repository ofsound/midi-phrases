import { getNativeFunction } from "@juce/index.js";

const dragCursorMap = {
  grab: "grabbing",
};

let activeCursor = "";
let setHostCursorNative = null;
let lastHoverChain = [];

function hasNativeFunction(name) {
  return (
    window.__JUCE__?.initialisationData?.__juce__functions?.includes?.(name) ??
    false
  );
}

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

function cursorFromTarget(target) {
  if (!(target instanceof Element)) return "default";

  const cursorElement = target.closest("[data-cursor], button, [role='button']");
  if (!cursorElement) return "default";

  return cursorForElement(cursorElement) || "pointer";
}

function clientCoordsFromHost(hostX, hostY, hostW, hostH) {
  const root = document.documentElement;
  const clientW = root.clientWidth;
  const clientH = root.clientHeight;

  if (hostW <= 0 || hostH <= 0 || clientW <= 0 || clientH <= 0) {
    return { x: hostX, y: hostY };
  }

  return {
    x: hostX * (clientW / hostW),
    y: hostY * (clientH / hostH),
  };
}

function cursorAtPoint(x, y) {
  const element = document.elementFromPoint(x, y);
  if (!(element instanceof Element)) return "";

  const cursorElement = element.closest("[data-cursor], button, [role='button']");
  if (!cursorElement) return "";

  return cursorForElement(cursorElement) || "pointer";
}

/** WKWebView in JUCE often skips :hover until click; mirror hover for Tailwind. */
function updateHoverChainAtPoint(x, y) {
  const hit = document.elementFromPoint(x, y);
  const nextChain = [];

  for (let node = hit instanceof Element ? hit : null; node; node = node.parentElement) {
    nextChain.push(node);
  }

  for (const prev of lastHoverChain) {
    if (!nextChain.includes(prev)) prev.removeAttribute("data-mp-hover");
  }

  for (const next of nextChain) {
    next.setAttribute("data-mp-hover", "");
  }

  lastHoverChain = nextChain;
}

/** JUCE WKWebView ignores CSS cursor on many controls; inline style is required. */
export function syncInlineCursors(root = document.documentElement) {
  if (!(root instanceof Element)) return;

  const visit = (element) => {
    if (!(element instanceof HTMLElement)) return;

    let cursor = "";

    if (element.hasAttribute("data-cursor")) {
      cursor = cursorForElement(element);
    } else if (
      element instanceof HTMLButtonElement
      || element.getAttribute("role") === "button"
    ) {
      cursor = isDisabledCursorElement(element) ? "default" : "pointer";
    }

    if (cursor) element.style.cursor = cursor;
  };

  visit(root);

  if (root instanceof HTMLElement || root instanceof Document) {
    for (const element of root.querySelectorAll("[data-cursor], button, [role='button']")) {
      visit(element);
    }
  }
}

function applyBodyCursor(cursor) {
  if (cursor) {
    document.body.dataset.cursor = cursor;
    document.body.style.cursor = cursor;
  } else {
    delete document.body.dataset.cursor;
    document.body.style.removeProperty("cursor");
  }
}

function reportHostCursor(cursor) {
  const name = cursor || "default";
  applyBodyCursor(name === "default" ? "" : name);
  setHostCursorNative?.(name);
}

function syncHoverFromTarget(target) {
  if (activeCursor) {
    reportHostCursor(activeCursor);
    return;
  }

  reportHostCursor(cursorFromTarget(target));
}

/** Polled from C++ every frame while the mouse is over the web view. */
export function updateInteractionAtPoint(hostX, hostY, hostW, hostH) {
  const { x, y } = clientCoordsFromHost(hostX, hostY, hostW, hostH);

  updateHoverChainAtPoint(x, y);

  if (activeCursor) {
    reportHostCursor(activeCursor);
    return activeCursor;
  }

  const cursor = cursorAtPoint(x, y);
  reportHostCursor(cursor);
  return cursor || "default";
}

export function clearInteractionHover() {
  for (const node of lastHoverChain) node.removeAttribute("data-mp-hover");

  lastHoverChain = [];

  if (!activeCursor) reportHostCursor("default");
}

export function setActiveCursor(cursor) {
  activeCursor = dragCursorMap[cursor] || cursor;
  reportHostCursor(activeCursor);
}

export function clearActiveCursor(cursor) {
  if (cursor === undefined || activeCursor === cursor) {
    activeCursor = "";
    reportHostCursor("default");
  }
}

export function clearSyncedCursor() {
  clearInteractionHover();
}

let installed = false;

export function installCursorSync() {
  if (installed) return;

  installed = true;
  syncInlineCursors();

  if (hasNativeFunction("setHostCursor")) {
    setHostCursorNative = getNativeFunction("setHostCursor");
  }

  const observer = new MutationObserver((mutations) => {
    for (const mutation of mutations) {
      if (mutation.type === "childList") {
        for (const node of mutation.addedNodes) syncInlineCursors(node);
      } else {
        syncInlineCursors(mutation.target);
      }
    }
  });

  observer.observe(document.documentElement, {
    attributes: true,
    attributeFilter: ["aria-disabled", "data-cursor", "disabled"],
    childList: true,
    subtree: true,
  });

  const onPointerHover = (event) => {
    syncHoverFromTarget(event.target);
  };

  const onPointerDown = (event) => {
    const cursor = cursorFromTarget(event.target);
    if (cursor === "default") return;

    setActiveCursor(cursor);
  };

  const onPointerUp = () => {
    clearActiveCursor();
  };

  document.addEventListener("pointerover", onPointerHover, true);
  document.addEventListener("pointermove", onPointerHover, true);
  document.addEventListener("pointerdown", onPointerDown, true);
  document.addEventListener("pointerup", onPointerUp, true);
  document.addEventListener("pointercancel", onPointerUp, true);

  window.addEventListener("blur", () => {
    activeCursor = "";
    clearSyncedCursor();
  });

  window.addEventListener("mouseout", (event) => {
    if (event.relatedTarget === null) clearSyncedCursor();
  });

  window.__mpOnMouseAt = updateInteractionAtPoint;
  window.__mpClearCursor = clearSyncedCursor;
}
