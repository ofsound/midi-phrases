# Native frontend migration

The existing frontend is the visual and behavioral specification. This is a
frontend conversion, not a redesign. Baseline Git revision is recorded in
`native-ui-reference/baseline.txt`; the original running standalone screenshot is
`native-ui-reference/web-standalone-original.png`.

## Acceptance requirements

- Preserve Familjen Grotesk typography, vector logo and icons, three palettes,
  honeycomb rails, gradients, outlines, spacing, and full/compact step rendering.
- Preserve four independently timed rows, selection and bulk operations, step
  insert/remove/reorder/copy, boundary resizing, duration/velocity/note gestures,
  mute/skip, cycle/probability inspector, scale picker and seed controls.
- Preserve patterns, loops, MIDI recording and computer keyboard, transport,
  processing controls, piano-roll editing/preview, undo/redo, project files,
  theme, scaling and fullscreen.
- Keep host/plugin identifiers and project serialization compatible. Route edits
  through existing processor setters; do not access audio-owned state from UI.
- Compare deterministic native renders with the web reference at matching sizes;
  test gesture cancellation, undo, recording conflicts and repeated lifetimes.
- Run all processor tests and build all formats, with warnings as errors.
- Remove the browser/frontend build only after native feature and visual checks.

## Reference measurements (100%)

Design size: 1690 × 980 (plugin), 1690 × 1044 (standalone). Four row
bands: 104 px each, 10 px gaps, 24 px top/bottom padding. Full cards:
96 px high, 8 px corner radius, 2 px border, 16 px insert gap. Quarter-grid
unit: 76.5 px before shared horizontal content fitting. Row leading controls:
127 px. Root scaling: 50–100%. Negative row offsets have visual compensation
only. The processor remains responsible for actual timing.

## Status

The shipping editor is the native JUCE UI in `source/ui/`. The Svelte app,
`assets/webview/ui.zip`, and the WebView resource provider have been removed.
Host identifiers and project serialization are unchanged.

Reference captures remain in `docs/native-ui-reference/`.
