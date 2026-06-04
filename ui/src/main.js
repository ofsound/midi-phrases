import "./app.css";
import { mount } from "svelte";
import App from "./App.svelte";
import { installCursorSync } from "./cursor.js";

installCursorSync();
mount(App, { target: document.getElementById("app") });
