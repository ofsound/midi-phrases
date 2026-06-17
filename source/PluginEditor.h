#pragma once

#include "PluginProcessor.h"

#if JUCE_WEB_BROWSER
 #include "WebViewResourceProvider.h"
#endif

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor
#if JUCE_WEB_BROWSER
    ,
                     private juce::Timer
#endif
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

#if JUCE_WEB_BROWSER
    void mouseMove (const juce::MouseEvent&) override;

    juce::MouseCursor getHostMouseCursor() const noexcept
    {
        return hostMouseCursor;
    }

    bool isWebViewHierarchy (const juce::Component& component) const;
    void applyHostCursorFromWeb (const juce::String& cursorName);
    juce::var handleEditorFullscreenRequest (int mode);
    juce::var handleEditorScaleMinimumRequest (int minWidth, int minHeight);
#endif

private:
    PluginProcessor& processorRef;

#if JUCE_WEB_BROWSER
    class HostCursorLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        explicit HostCursorLookAndFeel (PluginEditor& editorIn)
            : editor (editorIn)
        {
        }

        juce::MouseCursor getMouseCursorFor (juce::Component& component) override;

    private:
        PluginEditor& editor;
    };

    void timerCallback() override;
    void syncWebViewInteractionFromMouse();
    void pokeWebViewHoverAt (juce::Point<float> localPos);
    void applyNormalResizeLimits();
    void applyFullscreenResizeLimits (juce::Rectangle<int> targetBounds);
    juce::Rectangle<int> getDisplayUserBounds() const;
    juce::ResizableWindow* getStandaloneFullscreenWindow() const;
    juce::DocumentWindow* getStandaloneDocumentWindow() const;
    void setStandaloneWrapperChromeVisible (bool shouldBeVisible);
    juce::var createEditorFullscreenState() const;
    juce::var setEditorFullscreen (bool shouldBeFullscreen);

    HostCursorLookAndFeel hostCursorLookAndFeel { *this };
    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::MouseCursor hostMouseCursor { juce::MouseCursor::NormalCursor };
    juce::String lastHostCursorName;
    juce::Rectangle<int> preFullscreenEditorBounds;
    int standaloneTitleBarHeightBeforeFullscreen = -1;
    int scaleMinimumWidth = 1500;
    int scaleMinimumHeight = 875;
    bool mouseWasOverWebView = false;
    bool windowAcceptsMouseMoved = false;
    bool editorFullscreen = false;
    bool standaloneNativeFullscreen = false;
#else
    juce::Label fallbackLabel;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
