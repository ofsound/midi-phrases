export const themeModes = ["light", "dark", "alt"];
export const defaultThemeMode = "dark";

const themeModeStorageKey = "midi-phrases-theme-mode";

/** @param {unknown} value */
export function isThemeMode(value) {
  return typeof value === "string" && themeModes.includes(value);
}

export function storedThemeMode() {
  try {
    const stored = localStorage.getItem(themeModeStorageKey);
    return isThemeMode(stored) ? stored : defaultThemeMode;
  } catch {
    return defaultThemeMode;
  }
}

/**
 * @param {unknown} value
 * @param {{ persist?: boolean }} [options]
 */
export function applyThemeMode(value, options = {}) {
  const next = isThemeMode(value) ? value : defaultThemeMode;
  document.documentElement.setAttribute("data-theme", next);

  if (options.persist ?? true) {
    try {
      localStorage.setItem(themeModeStorageKey, next);
    } catch {
      // Local storage is optional in embedded WebViews.
    }
  }

  return next;
}
