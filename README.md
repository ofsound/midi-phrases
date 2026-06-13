# MIDI Phrases

**MIDI Phrases** by [ofsound](https://ofsound.com) — a MIDI effect plugin that sequences and interleaves phrases into a composition. Built on [Pamplejuce](https://github.com/sudara/pamplejuce) (JUCE 8, CMake, C++23, Catch2).

## Host notes

- **Ableton Live** — use **VST3** only (no AU MIDI-out). Load on the **instrument** track, route MIDI to a second track with your synth ([Ableton guide](https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in)).

## Quick start

```bash
git submodule update --init --recursive
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds
./Builds/Tests
```

Built AU/VST3 are copied to `~/Library/Audio/Plug-Ins/` when `COPY_PLUGIN_AFTER_BUILD` is on.

**Standalone (Debug):** `Builds/MidiPhrases_artefacts/Debug/Standalone/MIDI Phrases.app`

## WebView UI (`ui/`)

The editor is a JUCE `WebBrowserComponent` running a **Svelte 5** app (Vite, Tailwind). Production builds embed `assets/webview/ui.zip`; Debug can hot-reload from Vite on port **5174**.

```bash
cd ui && npm install
./scripts/setup-git-hooks.sh   # once: pre-commit runs svelte-check when ui/ is staged
cd ui && npm run dev           # hot reload (Debug + dev server)
cd ui && npm run check         # svelte-check
cd ui && npm run build         # dist + ui.zip
```

After UI changes, run `cmake --build Builds` so every format embeds the new zip (not standalone-only).

## AI agents

See **`CLAUDE.md`** and **`AGENTS.md`** (kept in sync) for build workflow, realtime rules, and Svelte 5 conventions. Cursor also loads `.cursor/rules/` (rebuild + `ui/**` Svelte guidance).

## Formats

Standalone, AU, VST3, AUv3, and CLAP (via clap-juce-extensions). CI is macOS-focused for now.

## Docs

- Agent / contributor guide: `CLAUDE.md`, `AGENTS.md`
- Pamplejuce template docs: [melatonin.dev/manuals/pamplejuce](https://melatonin.dev/manuals/pamplejuce)
