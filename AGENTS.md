# CLAUDE.md

This file provides guidance for AI coding agents working in this repository. See also `AGENTS.md` (kept in sync).

## About This Project

**MIDI Phrases** by **ofsound** — a MIDI effect (no audio DSP; dummy audio out bus for host compatibility) that sequences and interleaves separate phrases into a composition. Built on the [Pamplejuce](https://github.com/sudara/pamplejuce) template with JUCE 8, CMake, C++23, Catch2 tests, and Standalone / AU / VST3 / AUv3 / CLAP formats.

**Hosts:** Ableton — use **VST3** only (Live does not expose AU MIDI-out). Live cannot place third-party plugins in the built-in MIDI-effects row; load **MIDI Phrases** in the **instrument** slot on one MIDI track, put your synth on a second track, and set **MIDI From** → first track → **MIDI Phrases** (Monitor **In**) — [Ableton guide](https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in).

Plugin-specific logic lives in `source/`. The editor is a native JUCE interface in `source/ui/`. CI is macOS-only for now; code signing is disabled until release.

## Build Commands (VS Code / Cursor)

Use the **CMake Tools** extension (`.vscode/settings.json` points at `Builds/` and Ninja).

**macOS prerequisites** (once): Xcode or Xcode Command Line Tools, Homebrew CMake ≥ 3.25, and Ninja:

```bash
xcode-select --install          # if needed
brew install cmake ninja
```

```bash
# One-time: fetch submodules
git submodule update --init --recursive

# Configure (after CMakeLists.txt changes)
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build (Standalone, AU, VST3, AUv3, CLAP)
cmake --build Builds

# Run tests
ctest --test-dir Builds --verbose --output-on-failure
# or: ./Builds/Tests

# Run a single test
./Builds/Tests "[Plugin instance]"

# Release build (for DAW performance testing)
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds
```

On macOS for universal binary: add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` to configure.

Built plugins are copied to `~/Library/Audio/Plug-Ins/` when `COPY_PLUGIN_AFTER_BUILD` is on (Debug builds too).

## Native JUCE frontend

The editor lives in `source/ui/`. It uses the Familjen Grotesk fonts embedded from `assets/fonts/`, three theme palettes, and custom-painted controls. There is no Svelte project, npm build, browser resource provider, or embedded web bundle. `JUCE_WEB_BROWSER=0` is required.

- `PhrasesStyle.h` — palette, type, icons, and shared controls.
- `PhrasesNativeEditor` — layout, inspector, seed, recording keyboard, and project chrome.
- `PhrasesGrid` — phrase rows, step cards, and hit testing.
- `PhrasesPreview` — piano-roll preview. Keep it consistent with `PluginProcessor` scheduling; tests cover that parity.
- `PhrasesModel` — row and step snapshots, selection, and undo gestures.
- `PluginEditor` — hosts the native view, project files, fullscreen, and scaling.

Melatonin Inspector is not linked. The `melatonin_inspector` submodule may still exist in `.gitmodules`.

## Agent workflow (rebuild every time)

After any change to `source/`, `tests/`, `assets/`, `CMakeLists.txt`, or plugin-related CMake modules:

1. Reconfigure only if CMake or the font list changed: `cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug`
2. **Always** build all formats: `cmake --build Builds` (Standalone, AU, VST3, AUv3, CLAP — installs AU/VST3 to `~/Library/Audio/Plug-Ins/`). **Never** use only `MidiPhrases_Standalone`; DAW plugins will stay stale.
3. Fix compile warnings before finishing; do not leave rebuilds to the user unless the toolchain is missing.

Run `./Builds/Tests` when test or processor behavior changes.

See `.cursor/rules/rebuild-after-changes.mdc` for the DAW reload note. After a rebuild, quit and reopen the standalone, and remove and reopen the plugin in the DAW.

## Project Structure

- `source/` — PluginProcessor, PluginEditor, native UI, scheduling
- `tests/` — Catch2 tests
- `benchmarks/` — Catch2 benchmarks
- `cmake/`, `cmake_project/` — CMake modules (tests, assets, icons)
- `modules/` — `clap-juce-extensions` submodule
- `JUCE/` — JUCE submodule
- `assets/fonts/` — Familjen Grotesk faces embedded as binary data
- `scripts/` — `setup-git-hooks.sh`, etc.
- `packaging/` — Installer resources

## Architecture

**SharedCode Library**: The `SharedCode` INTERFACE library links plugin source code to both the main plugin target and the Tests target, avoiding ODR violations.

**CMake Modules**:
- `PamplejuceVersion.cmake` - Reads VERSION file, optional auto-bump patch level
- `Assets.cmake` - Optional helper that can embed every file under `assets/`. The plugin lists its fonts explicitly in `CMakeLists.txt` instead.
- `Tests.cmake` - Configures Catch2 test target
- `Benchmarks.cmake` - Configures Catch2 benchmark target
- `PamplejuceIPP.cmake` - Intel IPP integration (optional)

**Test Discovery**: Uses `catch_discover_tests()` with `PRE_TEST` discovery mode for Xcode compatibility.

## Key Configuration

Edit `CMakeLists.txt` to customize:
- `PROJECT_NAME` - Internal name (no spaces)
- `PRODUCT_NAME` - Display name in DAWs (can have spaces)
- `COMPANY_NAME` - Used for bundle name
- `BUNDLE_ID` - macOS bundle identifier
- `FORMATS` - Plugin formats to build (default: Standalone AU VST3 AUv3; CLAP via `clap_juce_extensions_plugin`)
- `PLUGIN_MANUFACTURER_CODE` / `PLUGIN_CODE` - 4-character plugin IDs

Version is read from the `VERSION` file in project root.

## Code Quality

Always resolve any compile warnings encountered during builds. Warnings should be treated as errors and fixed before considering a task complete.

Warnings-as-errors must be enforced for owned targets in CMake. If a local build reports a warning in project source, tests, or benchmarks, fix it before finishing. Generated or vendor warnings may be isolated explicitly, but do not weaken warnings for project-owned code.

Note: LSP/clangd often reports false positive diagnostic errors (like "undeclared identifier", "file not found") because it doesn't have full context of the JUCE module system. Ignore these unless the actual build fails.

## Includes

JUCE modules include common standard library headers (`<vector>`, `<algorithm>`, `<string>`, `<memory>`, etc.) so you don't need to add those explicitly in JUCE code. Adding them is harmless but redundant.

## Threading Model

JUCE plugins have two main threads:

- **Audio thread**: Runs `processBlock` — must be realtime-safe (see below). Never block, allocate, or lock.
- **Message thread**: Runs UI callbacks, parameter listeners, and timer callbacks. Owns the `MessageManager`.

To communicate between them:
- **Simple values**: Use `std::atomic` or JUCE's `AudioParameterFloat`/`AudioParameterBool` (which are atomic under the hood)
- **Larger data**: Use a lock-free queue (e.g. `moodycamel::ReaderWriterQueue`) to pass data from message → audio thread
- **Audio → UI updates**: Use `juce::AsyncUpdater` or `juce::Timer` on the message thread to poll state — never call UI code from the audio thread

## Processor State Ownership

Any state read by `processBlock()` must be owned by the audio thread or copied into an audio-thread snapshot. UI callbacks, parameter listeners, timers, and state loading must not mutate containers or objects that `processBlock()` reads directly.

For sequencer/grid data:
- Define a fixed maximum capacity before audio-thread use
- Use fixed-size storage for audio-visible state
- Send edits to the audio thread through a bounded queue or publish immutable snapshots
- Apply queued edits at the start of `processBlock()`
- Never call `std::vector::resize`, `insert`, `erase`, `push_back`, or any allocating operation on data used by the audio thread

If a feature needs both the piano-roll preview and audio scheduling, keep `PhrasesPreview` and `PluginProcessor` on one tested model. Do not change one without a parity check.

Timer and paint paths that run every frame should stay lightweight. Avoid rebuilding the processor or allocating full schedules on each tick.

## Realtime Safety

For anything in the audio thread / hot DSP path (e.g. `processBlock`):
- Allocate in constructors or `prepareToPlay`, not while rendering audio
- Avoid dynamic allocations and container growth (`std::vector::push_back`, map insertion, string building)
- Prefer fixed-size storage (`std::array`, preallocated buffers, fixed-capacity queues)
- Keep operations deterministic and lock-free where possible

## Adding Dependencies

**JUCE Modules** live in `modules/` as git submodules. Add with `git submodule add`, then `add_subdirectory` and link to `SharedCode` in `CMakeLists.txt`. Some useful ones:

- [melatonin_blur](https://github.com/sudara/melatonin_blur) — fast cross-platform blurs for C++ UI (shadows, glows, frosted glass)
- [melatonin_perfetto](https://github.com/sudara/melatonin_perfetto) — performance tracing with Perfetto, great for profiling `processBlock` and paint calls
- [gin](https://github.com/FigBug/gin) — large collection of utilities (DSP, UI components, LookAndFeel, etc.)

**Non-JUCE C++ libraries** should be added via [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) which is already configured. CPM downloads and caches dependencies at configure time — no submodule needed:

```cmake
CPMAddPackage("gh:nlohmann/json@3.11.3")
target_link_libraries(SharedCode INTERFACE nlohmann_json::nlohmann_json)
```

Some useful CPM libraries:
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing/serialization
- [cameron314/readerwriterqueue](https://github.com/cameron314/readerwriterqueue) — lock-free single-producer/single-consumer queue, ideal for audio↔message thread communication

## Code Style

- **C++:** `.clang-format` with Allman-style braces, 4-space indentation, no column limit. Match the existing `source/ui` patterns.
