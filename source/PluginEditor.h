#pragma once
#include "PluginProcessor.h"
#include "ui/PhrasesNativeEditor.h"

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr int designWidth = 1692;
    static constexpr int designHeight = designWidth * 3 / 4;
    struct FittedPlacement
    {
        float scale = 1.0f;
        float x = 0.0f;
        float y = 0.0f;
    };
    // Largest uniform scale that keeps the design inside the available view, centered.
    static FittedPlacement fitDesignToAvailable (float availableWidth, float availableHeight, float contentWidth, float contentHeight)
    {
        if (! (availableWidth > 0.0f && availableHeight > 0.0f && contentWidth > 0.0f && contentHeight > 0.0f))
            return {};
        FittedPlacement placement;
        placement.scale = juce::jmin (availableWidth / contentWidth, availableHeight / contentHeight);
        placement.x = (availableWidth - contentWidth * placement.scale) * 0.5f;
        placement.y = (availableHeight - contentHeight * placement.scale) * 0.5f;
        return placement;
    }

    using Completion = std::function<void (juce::var)>;
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showSaveProjectDialog (Completion);
    void showLoadProjectDialog (Completion);
    void createNewProject (Completion);
    void cycleProject (int direction, Completion);
    bool hasPreviousProject() const;
    bool hasNextProject() const;
    juce::String getCurrentProjectFileName() const;
    juce::var handleEditorFullscreenRequest (int mode);

private:
    PluginProcessor& processorRef;
    std::unique_ptr<phrases::ui::NativeEditor> nativeView;
    std::unique_ptr<juce::FileChooser> projectFileChooser;
    juce::File currentProjectFile;
    juce::MemoryBlock savedState;
    juce::Rectangle<int> preFullscreenEditorBounds;
    int standaloneTitleBarHeightBeforeFullscreen = -1;
    bool editorFullscreen = false, standaloneNativeFullscreen = false;
    void projectCommand (int command);
    void saveProject (Completion);
    void applyNormalResizeLimits();
    void applyFullscreenResizeLimits (juce::Rectangle<int>);
    juce::Rectangle<int> getDisplayUserBounds() const;
    juce::ResizableWindow* getStandaloneFullscreenWindow() const;
    juce::DocumentWindow* getStandaloneDocumentWindow() const;
    void setStandaloneWrapperChromeVisible (bool);
    juce::var createEditorFullscreenState() const;
    juce::var setEditorFullscreen (bool);
    juce::File getDefaultProjectsDirectory() const;
    juce::Array<juce::File> getSiblingProjectFiles() const;
    bool loadProjectFile (const juce::File&, juce::String&);
    bool saveProjectFile (const juce::File&, const juce::String& name, juce::String& error);
    juce::var projectOperationResult (bool, const juce::String& errorMessage = {}) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
