/** Wait for extracted stylesheets before first paint (Vite may emit CSS after the entry script). */
export function waitForStylesheets() {
  const links = [...document.querySelectorAll('link[rel="stylesheet"]')];
  const unloaded = links.filter((link) => !link.sheet);

  if (unloaded.length === 0) {
    return Promise.resolve();
  }

  return Promise.all(
    unloaded.map(
      (link) =>
        new Promise((resolve) => {
          link.addEventListener("load", resolve, { once: true });
          link.addEventListener("error", resolve, { once: true });
        }),
    ),
  );
}

/** Two animation frames so flex layout settles before the WebView is shown. */
export function waitForLayoutFrames() {
  return new Promise((resolve) => {
    requestAnimationFrame(() => {
      requestAnimationFrame(resolve);
    });
  });
}

export async function revealUiWhenReady() {
  await waitForLayoutFrames();
  document.documentElement.classList.add("ui-ready");
}
