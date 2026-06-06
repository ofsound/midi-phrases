#include "WebViewHoverSupport.h"

#if JUCE_MAC
 #include <juce_gui_extra/juce_gui_extra.h>

 #import <Cocoa/Cocoa.h>

namespace WebViewHoverSupport
{
namespace
{
NSView* viewFromPlatformComponent (juce::Component* platformViewComponent)
{
    if (platformViewComponent == nullptr)
        return nil;

    auto* nsViewComponent = dynamic_cast<juce::NSViewComponent*> (platformViewComponent);

    if (nsViewComponent == nullptr)
        return nil;

    return (__bridge NSView*) nsViewComponent->getView();
}

NSCursor* nsCursorForName (const juce::String& cursorName)
{
    if (cursorName == "pointer")
        return [NSCursor pointingHandCursor];

    if (cursorName == "ew-resize")
        return [NSCursor resizeLeftRightCursor];

    if (cursorName == "ns-resize")
        return [NSCursor resizeUpDownCursor];

    if (cursorName == "grabbing")
        return [NSCursor closedHandCursor];

    if (cursorName == "grab")
        return [NSCursor openHandCursor];

    if (cursorName == "none")
    {
        static NSCursor* transparentCursor = nil;
        static dispatch_once_t onceToken;

        dispatch_once (&onceToken, ^{
            const auto size = NSMakeSize (16.0, 16.0);
            NSImage* const image = [[NSImage alloc] initWithSize: size];
            [image lockFocus];
            [[NSColor clearColor] set];
            NSRectFill (NSMakeRect (0.0, 0.0, size.width, size.height));
            [image unlockFocus];
            transparentCursor = [[NSCursor alloc] initWithImage: image hotSpot: NSMakePoint (8.0, 8.0)];
        });

        return transparentCursor;
    }

    return [NSCursor arrowCursor];
}
} // namespace

void pokeWebKitHoverState (juce::Component* platformViewComponent)
{
    NSView* const view = viewFromPlatformComponent (platformViewComponent);

    if (view == nil)
        return;

    JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
    if ([view respondsToSelector: @selector (_updateMouseoverWithFakeEvent)])
        [view performSelector: @selector (_updateMouseoverWithFakeEvent)];
    JUCE_END_IGNORE_WARNINGS_GCC_LIKE
}

void applyNativeCursorToWebView (juce::Component* platformViewComponent, const juce::String& cursorName)
{
    NSView* const view = viewFromPlatformComponent (platformViewComponent);

    if (view == nil)
        return;

    NSCursor* const cursor = nsCursorForName (cursorName);
    [cursor set];

    if (NSWindow* const window = [view window])
        [window invalidateCursorRectsForView: view];
}

bool ensureWindowAcceptsMouseMoved (juce::Component* platformViewComponent)
{
    NSView* const view = viewFromPlatformComponent (platformViewComponent);

    if (view == nil)
        return false;

    NSWindow* const window = [view window];

    if (window == nil)
        return false;

    if (! [window acceptsMouseMovedEvents])
        [window setAcceptsMouseMovedEvents: YES];

    return true;
}
} // namespace WebViewHoverSupport

#endif
