import "./app.css";
import { mount } from "svelte";
import App from "./App.svelte";
import { installCursorSync } from "./cursor.js";
import { revealUiWhenReady, waitForStylesheets } from "./bootstrapUi.js";

installCursorSync();

waitForStylesheets().then(async () => {
  mount(App, { target: document.getElementById("app") });
  await revealUiWhenReady();
});
