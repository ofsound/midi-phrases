#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace WebViewHoverSupport
{
/** Updates WebKit :hover state for an embedded WKWebView (macOS). No-op on other platforms. */
#if JUCE_MAC
void pokeWebKitHoverState (juce::Component* platformViewComponent);
void applyNativeCursorToWebView (juce::Component* platformViewComponent, const juce::String& cursorName);

/** Ensures the host NSWindow delivers mouseMoved: events so WebKit/JUCE can update the cursor
    while no mouse button is held. The standalone window does not enable this by default, which
    leaves the cursor frozen until a drag is in progress. Returns true once configured. */
bool ensureWindowAcceptsMouseMoved (juce::Component* platformViewComponent);
#else
inline void pokeWebKitHoverState (juce::Component*) {}
inline void applyNativeCursorToWebView (juce::Component*, const juce::String&) {}
inline bool ensureWindowAcceptsMouseMoved (juce::Component*) { return true; }
#endif
} // namespace WebViewHoverSupport
