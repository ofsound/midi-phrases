#pragma once
#include "PluginProcessor.h"
#include "ui/PhrasesNativeEditor.h"

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    using Completion = std::function<void (juce::var)>;
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showSaveProjectDialog (const juce::Array<juce::var>&, Completion);
    void showLoadProjectDialog (Completion);
    void createNewProject (Completion);
    void cycleProject (int direction, Completion);
    bool hasPreviousProject() const;
    bool hasNextProject() const;
    juce::String getCurrentProjectFileName() const;
    juce::var handleEditorFullscreenRequest (int mode);
    juce::var handleEditorScaleMinimumRequest (int minWidth, int minHeight);

private:
    PluginProcessor& processorRef;
    std::unique_ptr<phrases::ui::NativeEditor> nativeView;
    std::unique_ptr<juce::FileChooser> projectFileChooser;
    juce::File currentProjectFile;
    juce::MemoryBlock savedState;
    juce::Rectangle<int> preFullscreenEditorBounds;
    int standaloneTitleBarHeightBeforeFullscreen = -1;
    int scaleMinimumWidth = 1690, scaleMinimumHeight = 980;
    bool editorFullscreen = false, standaloneNativeFullscreen = false;
    void projectCommand (int command);
    void updateScale();
    void applyNormalResizeLimits();
    void applyFullscreenResizeLimits (juce::Rectangle<int>);
    juce::Rectangle<int> getDisplayUserBounds() const;
    juce::ResizableWindow* getStandaloneFullscreenWindow() const;
    juce::DocumentWindow* getStandaloneDocumentWindow() const;
    void setStandaloneWrapperChromeVisible (bool);
    void refreshNativeLayout();
    juce::var createEditorFullscreenState() const;
    juce::var setEditorFullscreen (bool);
    juce::File getDefaultProjectsDirectory() const;
    juce::Array<juce::File> getSiblingProjectFiles() const;
    bool loadProjectFile (const juce::File&, juce::String&);
    bool saveProjectFile (const juce::File&, juce::String&);
    juce::var projectOperationResult (bool, const juce::String& errorMessage = {}) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
