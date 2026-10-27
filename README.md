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

## Editor

The interface is native JUCE in `source/ui/`. Fonts are embedded from `assets/fonts/`. There is no browser UI or npm frontend build.

After UI changes, run `cmake --build Builds` so every format is updated (not standalone-only).

## AI agents

See **`CLAUDE.md`** and **`AGENTS.md`** (kept in sync) for build workflow and realtime rules. Cursor also loads `.cursor/rules/rebuild-after-changes.mdc`.

## Formats

Standalone, AU, VST3, AUv3, and CLAP (via clap-juce-extensions). CI is macOS-focused for now.

## Docs

- Agent / contributor guide: `CLAUDE.md`, `AGENTS.md`
- Pamplejuce template docs: [melatonin.dev/manuals/pamplejuce](https://melatonin.dev/manuals/pamplejuce)
