import "./app.css";
import { mount } from "svelte";
import App from "./App.svelte";
import { installCursorSync } from "./cursor.js";
import { revealUiWhenReady, waitForStylesheets } from "./bootstrapUi.js";
import { applyThemeMode, storedThemeMode } from "./themeMode.js";

applyThemeMode(storedThemeMode(), { persist: false });
installCursorSync();

waitForStylesheets().then(async () => {
  mount(App, { target: document.getElementById("app") });
  await revealUiWhenReady();
});
