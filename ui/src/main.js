import "./app.css";
import { mount } from "svelte";
import App from "./App.svelte";
import { installCursorManager } from "./cursor.js";

installCursorManager();
mount(App, { target: document.getElementById("app") });
