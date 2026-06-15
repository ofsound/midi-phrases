#include "PluginEditor.h"
#include "WebViewHoverSupport.h"

//==============================================================================
#if JUCE_WEB_BROWSER

namespace
{
juce::MouseCursor mouseCursorFromWebName (const juce::String& name)
{
    if (name == "pointer")
        return juce::MouseCursor (juce::MouseCursor::PointingHandCursor);

    if (name == "ew-resize")
        return juce::MouseCursor (juce::MouseCursor::LeftRightResizeCursor);

    if (name == "ns-resize")
        return juce::MouseCursor (juce::MouseCursor::UpDownResizeCursor);

    if (name == "grab" || name == "grabbing")
        return juce::MouseCursor (juce::MouseCursor::DraggingHandCursor);

    if (name == "none")
    {
        juce::Image image (juce::Image::ARGB, 16, 16, true);
        image.clear (image.getBounds());
        return juce::MouseCursor (image, 8, 8);
    }

    return juce::MouseCursor (juce::MouseCursor::NormalCursor);
}
} // namespace

juce::MouseCursor PluginEditor::HostCursorLookAndFeel::getMouseCursorFor (juce::Component& component)
{
    if (editor.isWebViewHierarchy (component))
        return editor.getHostMouseCursor();

    return juce::LookAndFeel_V4::getMouseCursorFor (component);
}

bool PluginEditor::isWebViewHierarchy (const juce::Component& component) const
{
    if (webView == nullptr)
        return false;

    return &component == webView.get() || webView->isParentOf (&component);
}

void PluginEditor::applyHostCursorFromWeb (const juce::String& cursorName)
{
    const auto normalized = cursorName.isEmpty() ? juce::String ("default") : cursorName;

    if (normalized == lastHostCursorName)
        return;

    lastHostCursorName = normalized;
    hostMouseCursor = mouseCursorFromWebName (normalized);

    setMouseCursor (hostMouseCursor);

    if (webView != nullptr)
    {
        webView->setMouseCursor (hostMouseCursor);

        for (int i = 0; i < webView->getNumChildComponents(); ++i)
            if (auto* child = webView->getChildComponent (i))
                child->setMouseCursor (hostMouseCursor);

        WebViewHoverSupport::applyNativeCursorToWebView (webView->getChildComponent (0), normalized);
    }

    // Do not use forceMouseCursorUpdate() — it re-queries the WKWebView child and resets the arrow.
    juce::Desktop::getInstance().getMainMouseSource().showMouseCursor (hostMouseCursor);
}

void PluginEditor::pokeWebViewHoverAt (const juce::Point<float> localPos)
{
    if (webView == nullptr)
        return;

    WebViewHoverSupport::pokeWebKitHoverState (webView->getChildComponent (0));

    const auto script = juce::String::formatted (
        R"((function(x,y,w,h){if(typeof window.__mpOnMouseAt==='function')window.__mpOnMouseAt(x,y,w,h);})(%f,%f,%d,%d))",
        localPos.x,
        localPos.y,
        webView->getWidth(),
        webView->getHeight());

    webView->evaluateJavascript (script);
}

void PluginEditor::syncWebViewInteractionFromMouse()
{
    if (webView == nullptr || ! isShowing())
        return;

    if (! windowAcceptsMouseMoved)
        windowAcceptsMouseMoved = WebViewHoverSupport::ensureWindowAcceptsMouseMoved (webView->getChildComponent (0));

    const auto screenPos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().toFloat();
    const auto localPos = webView->getLocalPoint (nullptr, screenPos);
    const auto inside = webView->getLocalBounds().toFloat().contains (localPos);

    if (! inside)
    {
        if (mouseWasOverWebView)
        {
            mouseWasOverWebView = false;
            applyHostCursorFromWeb ("default");
            webView->evaluateJavascript ("window.__mpClearCursor?.()");
        }

        return;
    }

    mouseWasOverWebView = true;
    pokeWebViewHoverAt (localPos);
}

void PluginEditor::mouseMove (const juce::MouseEvent&)
{
    syncWebViewInteractionFromMouse();
}

void PluginEditor::timerCallback()
{
    syncWebViewInteractionFromMouse();
}

#endif

//==============================================================================
PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    juce::ignoreUnused (processorRef);

#if JUCE_WEB_BROWSER
    webView = std::make_unique<juce::WebBrowserComponent> (WebViewResources::makeBrowserOptions (processorRef));
    addAndMakeVisible (*webView);

    setLookAndFeel (&hostCursorLookAndFeel);
    webView->setLookAndFeel (&hostCursorLookAndFeel);

    processorRef.setWebHostCursorHandler ([this] (const juce::String& cursorName)
    {
        applyHostCursorFromWeb (cursorName);
    });

   #if JUCE_DEBUG && defined (MIDI_PHRASES_UI_DEV_SERVER)
    webView->goToURL ("http://localhost:5174");
   #else
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
   #endif

    startTimerHz (60);
#else
    fallbackLabel.setText ("WebView is not available on this platform.\nEnable JUCE_WEB_BROWSER.",
                           juce::dontSendNotification);
    fallbackLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (fallbackLabel);
#endif

    setResizeLimits (1000, 480, 2000, 1280);
    setResizable (true, true);
    setSize (1670, processorRef.hasStandaloneTransport() ? 1044 : 980);
}

PluginEditor::~PluginEditor()
{
#if JUCE_WEB_BROWSER
    processorRef.setWebHostCursorHandler (nullptr);

    if (webView != nullptr)
        webView->setLookAndFeel (nullptr);

    setLookAndFeel (nullptr);
#endif
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void PluginEditor::resized()
{
#if JUCE_WEB_BROWSER
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
#else
    fallbackLabel.setBounds (getLocalBounds());
#endif
}
