#include "PluginEditor.h"
#include "WebViewHoverSupport.h"

#include <algorithm>

//==============================================================================
#if JUCE_WEB_BROWSER

namespace
{
constexpr int defaultEditorWidth = 1670;
constexpr int defaultPluginEditorHeight = 980;
constexpr int defaultStandaloneEditorHeight = 1044;

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
    webView = std::make_unique<juce::WebBrowserComponent> (
        WebViewResources::makeBrowserOptions (processorRef, *this));
    addAndMakeVisible (*webView);

    setLookAndFeel (&hostCursorLookAndFeel);
    webView->setLookAndFeel (&hostCursorLookAndFeel);

    processorRef.setWebHostCursorHandler ([this] (const juce::String& cursorName)
    {
        applyHostCursorFromWeb (cursorName);
    });
    processorRef.setWebEditorFullscreenHandler ([this] (const int mode)
    {
        return handleEditorFullscreenRequest (mode);
    });
    processorRef.setWebEditorScaleMinimumHandler ([this] (const int minWidth, const int minHeight)
    {
        return handleEditorScaleMinimumRequest (minWidth, minHeight);
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

#if JUCE_WEB_BROWSER
    scaleMinimumWidth = defaultEditorWidth;
    scaleMinimumHeight = processorRef.hasStandaloneTransport() ? defaultStandaloneEditorHeight
                                                               : defaultPluginEditorHeight;
    applyNormalResizeLimits();
#else
    setResizeLimits (1000, 480, 2000, 1280);
#endif
    setResizable (true, true);
    setSize (defaultEditorWidth,
             processorRef.hasStandaloneTransport() ? defaultStandaloneEditorHeight
                                                   : defaultPluginEditorHeight);
}

PluginEditor::~PluginEditor()
{
#if JUCE_WEB_BROWSER
    processorRef.setWebHostCursorHandler (nullptr);
    processorRef.setWebEditorFullscreenHandler (nullptr);
    processorRef.setWebEditorScaleMinimumHandler (nullptr);

    if (webView != nullptr)
        webView->setLookAndFeel (nullptr);

    setLookAndFeel (nullptr);
#endif
}

#if JUCE_WEB_BROWSER
juce::File PluginEditor::getDefaultProjectsDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("ofsound")
        .getChildFile ("MIDI Phrases");
}

juce::Array<juce::File> PluginEditor::getSiblingProjectFiles() const
{
    const auto directory = currentProjectFile.existsAsFile()
                               ? currentProjectFile.getParentDirectory()
                               : getDefaultProjectsDirectory();
    juce::Array<juce::File> files;

    if (directory.isDirectory())
        directory.findChildFiles (files, juce::File::findFiles, false, "*.midiphrases");

    std::sort (files.begin(), files.end(), [] (const juce::File& left, const juce::File& right)
    {
        return left.getFileName().compareNatural (right.getFileName(), true) < 0;
    });
    return files;
}

bool PluginEditor::hasPreviousProject() const
{
    return getSiblingProjectFiles().size() > 1;
}

bool PluginEditor::hasNextProject() const
{
    return getSiblingProjectFiles().size() > 1;
}

juce::String PluginEditor::getCurrentProjectFileName() const
{
    return currentProjectFile.existsAsFile() ? currentProjectFile.getFileName() : juce::String();
}

juce::var PluginEditor::projectOperationResult (const bool success,
                                                 const juce::String& errorMessage)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("success", success ? 1 : 0);
    object->setProperty ("error", errorMessage);
    return juce::var (object.release());
}

bool PluginEditor::loadProjectFile (const juce::File& file, juce::String& errorMessage)
{
    constexpr size_t maximumProjectBytes = 16 * 1024 * 1024;

    if (! file.existsAsFile())
    {
        errorMessage = "That project file no longer exists.";
        return false;
    }

    if (file.getSize() <= 0 || static_cast<juce::uint64> (file.getSize()) > maximumProjectBytes)
    {
        errorMessage = "The selected file is empty or too large to be a MIDI Phrases project.";
        return false;
    }

    juce::MemoryBlock data;
    if (! file.loadFileAsData (data))
    {
        errorMessage = "The project file could not be read.";
        return false;
    }

    const auto xml = juce::parseXML (
        juce::String::createStringFromData (data.getData(), static_cast<int> (data.getSize())));
    if (xml == nullptr || ! xml->hasTagName ("MidiPhrases"))
    {
        errorMessage = "The selected file is not a MIDI Phrases project.";
        return false;
    }

    processorRef.setStateInformation (data.getData(), static_cast<int> (data.getSize()));
    currentProjectFile = file;

    if (! xml->hasAttribute ("projectName"))
    {
        processorRef.setProjectMetadata (file.getFileNameWithoutExtension(),
                                         {},
                                         {},
                                         {},
                                         "dark",
                                         100,
                                         false);
    }

    return true;
}

bool PluginEditor::saveProjectFile (const juce::File& file, juce::String& errorMessage)
{
    if (! file.getParentDirectory().createDirectory())
    {
        errorMessage = "The project folder could not be created.";
        return false;
    }

    juce::MemoryBlock data;
    processorRef.getStateInformation (data);
    juce::TemporaryFile temporaryFile (file);

    if (! temporaryFile.getFile().replaceWithData (data.getData(), data.getSize())
        || ! temporaryFile.overwriteTargetFileWithTemporary())
    {
        errorMessage = "The project file could not be written.";
        return false;
    }

    currentProjectFile = file;
    return true;
}

void PluginEditor::showSaveProjectDialog (
    const juce::Array<juce::var>& args,
    juce::WebBrowserComponent::NativeFunctionCompletion complete)
{
    if (projectFileChooser != nullptr)
    {
        complete (projectOperationResult (false, "A project dialog is already open."));
        return;
    }

    const auto now = juce::Time::getCurrentTime().toISO8601 (true);
    const auto name = args.size() > 0 ? args[0].toString().trim() : processorRef.getProjectName();
    const auto description = args.size() > 1 ? args[1].toString() : processorRef.getProjectDescription();
    const auto theme = args.size() > 2 ? args[2].toString() : processorRef.getProjectThemeMode();
    const auto scale = args.size() > 3 ? static_cast<int> (args[3]) : processorRef.getProjectUiScalePercent();
    const auto stretch = args.size() > 4 ? static_cast<int> (args[4]) != 0
                                         : processorRef.getProjectStretchStepsToFit();
    const auto createdAt = processorRef.getProjectCreatedAt().isNotEmpty()
                               ? processorRef.getProjectCreatedAt()
                               : now;
    auto directory = currentProjectFile.existsAsFile() ? currentProjectFile.getParentDirectory()
                                                        : getDefaultProjectsDirectory();
    directory.createDirectory();
    auto legalName = juce::File::createLegalFileName (
        name.isNotEmpty() ? name : juce::String ("Untitled Project"));
    if (legalName.isEmpty())
        legalName = "Untitled Project";
    const auto suggestedFile = currentProjectFile.existsAsFile()
                                   ? currentProjectFile
                                   : directory.getChildFile (legalName + ".midiphrases");

    projectFileChooser = std::make_unique<juce::FileChooser> (
        "Save MIDI Phrases Project", suggestedFile, "*.midiphrases", true);
    const juce::Component::SafePointer<PluginEditor> safeThis (this);
    projectFileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis,
         completion = std::move (complete),
         name,
         description,
         createdAt,
         now,
         theme,
         scale,
         stretch] (const juce::FileChooser& chooser) mutable
        {
            if (safeThis == nullptr)
                return;

            auto file = chooser.getResult();
            safeThis->projectFileChooser.reset();
            if (file == juce::File())
            {
                completion (projectOperationResult (false));
                return;
            }

            if (! file.hasFileExtension (".midiphrases"))
                file = file.withFileExtension (".midiphrases");

            safeThis->processorRef.setProjectMetadata (
                name, description, createdAt, now, theme, scale, stretch);
            juce::String error;
            const auto saved = safeThis->saveProjectFile (file, error);
            completion (projectOperationResult (saved, error));
        });
}

void PluginEditor::showLoadProjectDialog (
    juce::WebBrowserComponent::NativeFunctionCompletion complete)
{
    if (projectFileChooser != nullptr)
    {
        complete (projectOperationResult (false, "A project dialog is already open."));
        return;
    }

    const auto start = currentProjectFile.existsAsFile() ? currentProjectFile
                                                          : getDefaultProjectsDirectory();
    projectFileChooser = std::make_unique<juce::FileChooser> (
        "Load MIDI Phrases Project", start, "*.midiphrases", true);
    const juce::Component::SafePointer<PluginEditor> safeThis (this);
    projectFileChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, completion = std::move (complete)] (const juce::FileChooser& chooser) mutable
        {
            if (safeThis == nullptr)
                return;

            const auto file = chooser.getResult();
            safeThis->projectFileChooser.reset();
            if (file == juce::File())
            {
                completion (projectOperationResult (false));
                return;
            }

            juce::String error;
            const auto loaded = safeThis->loadProjectFile (file, error);
            completion (projectOperationResult (loaded, error));
        });
}

void PluginEditor::createNewProject (
    juce::WebBrowserComponent::NativeFunctionCompletion complete)
{
    if (projectFileChooser != nullptr)
    {
        complete (projectOperationResult (false, "Close the open project dialog first."));
        return;
    }

    processorRef.resetProject();
    currentProjectFile = juce::File();
    complete (projectOperationResult (true));
}

void PluginEditor::cycleProject (
    const int direction,
    juce::WebBrowserComponent::NativeFunctionCompletion complete)
{
    const auto files = getSiblingProjectFiles();

    if (files.size() <= 1)
    {
        complete (projectOperationResult (
            false,
            files.isEmpty() ? juce::String ("No saved projects were found in the projects folder.")
                            : juce::String()));
        return;
    }

    auto currentIndex = files.indexOf (currentProjectFile);
    int nextIndex = 0;

    if (currentIndex < 0)
    {
        nextIndex = direction < 0 ? files.size() - 1 : 0;
    }
    else
    {
        nextIndex = currentIndex + (direction < 0 ? -1 : 1);

        if (nextIndex < 0)
            nextIndex = files.size() - 1;
        else if (nextIndex >= files.size())
            nextIndex = 0;
    }

    if (currentIndex >= 0 && nextIndex == currentIndex)
    {
        complete (projectOperationResult (false));
        return;
    }

    juce::String error;
    const auto loaded = loadProjectFile (files[nextIndex], error);
    complete (projectOperationResult (loaded, error));
}
#endif

#if JUCE_WEB_BROWSER
void PluginEditor::applyNormalResizeLimits()
{
    setResizeLimits (scaleMinimumWidth, scaleMinimumHeight, 4096, 2400);
}

void PluginEditor::applyFullscreenResizeLimits (const juce::Rectangle<int> targetBounds)
{
    setResizeLimits (1000,
                     480,
                     juce::jmax (2000, targetBounds.getWidth()),
                     juce::jmax (1280, targetBounds.getHeight()));
}

juce::Rectangle<int> PluginEditor::getDisplayUserBounds() const
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();

    if (const auto* display = displays.getDisplayForRect (getScreenBounds()))
        return display->userBounds.toNearestInt();

    return { 0, 0, juce::jmax (getWidth(), 2000), juce::jmax (getHeight(), 1280) };
}

juce::ResizableWindow* PluginEditor::getStandaloneFullscreenWindow() const
{
    if (! processorRef.hasStandaloneTransport())
        return nullptr;

    auto* topLevelComponent = getTopLevelComponent();

    if (topLevelComponent == nullptr || topLevelComponent == this)
        return nullptr;

    return dynamic_cast<juce::ResizableWindow*> (topLevelComponent);
}

juce::DocumentWindow* PluginEditor::getStandaloneDocumentWindow() const
{
    if (! processorRef.hasStandaloneTransport())
        return nullptr;

    auto* topLevelComponent = getTopLevelComponent();

    if (topLevelComponent == nullptr || topLevelComponent == this)
        return nullptr;

    return dynamic_cast<juce::DocumentWindow*> (topLevelComponent);
}

void PluginEditor::setStandaloneWrapperChromeVisible (const bool shouldBeVisible)
{
    auto* standaloneWindow = getStandaloneDocumentWindow();

    if (standaloneWindow == nullptr)
        return;

    if (! shouldBeVisible)
    {
        if (standaloneTitleBarHeightBeforeFullscreen < 0)
            standaloneTitleBarHeightBeforeFullscreen = standaloneWindow->getTitleBarHeight();

        standaloneWindow->setTitleBarButtonsRequired (0, false);
        standaloneWindow->setTitleBarHeight (0);
    }
    else
    {
        standaloneWindow->setTitleBarHeight (standaloneTitleBarHeightBeforeFullscreen > 0
                                                 ? standaloneTitleBarHeightBeforeFullscreen
                                                 : 26);
        standaloneWindow->setTitleBarButtonsRequired (juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton,
                                                      false);
        standaloneTitleBarHeightBeforeFullscreen = -1;
    }

    for (int childIndex = 0; childIndex < standaloneWindow->getNumChildComponents(); ++childIndex)
    {
        if (auto* button = dynamic_cast<juce::Button*> (standaloneWindow->getChildComponent (childIndex)))
        {
            if (button->getButtonText() == "Options")
                button->setVisible (shouldBeVisible);
        }
    }

    standaloneWindow->resized();
}

void PluginEditor::nudgeWebViewAfterLayoutChange()
{
    resized();

    if (webView != nullptr)
        webView->evaluateJavascript ("window.dispatchEvent(new Event('resize')); void 0;");
}

juce::var PluginEditor::createEditorFullscreenState() const
{
    const auto* standaloneWindow = getStandaloneFullscreenWindow();
    const auto nativeFullscreenActive = standaloneWindow != nullptr && standaloneWindow->isFullScreen();

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("enabled", editorFullscreen || nativeFullscreenActive ? 1 : 0);
    object->setProperty ("native", nativeFullscreenActive ? 1 : 0);
    object->setProperty ("available", 1);
    object->setProperty ("width", getWidth());
    object->setProperty ("height", getHeight());
    return juce::var (object.release());
}

juce::var PluginEditor::setEditorFullscreen (const bool shouldBeFullscreen)
{
    if (shouldBeFullscreen == editorFullscreen)
        return createEditorFullscreenState();

    if (shouldBeFullscreen)
    {
        preFullscreenEditorBounds = getBounds();
        editorFullscreen = true;
        const auto displayBounds = getDisplayUserBounds();
        applyFullscreenResizeLimits (displayBounds);

        if (auto* standaloneWindow = getStandaloneFullscreenWindow())
        {
            standaloneNativeFullscreen = true;
            setStandaloneWrapperChromeVisible (false);
            standaloneWindow->setFullScreen (true);
            standaloneWindow->setContentComponentSize (displayBounds.getWidth(), displayBounds.getHeight());
            nudgeWebViewAfterLayoutChange();
            return createEditorFullscreenState();
        }

        setSize (displayBounds.getWidth(), displayBounds.getHeight());
        nudgeWebViewAfterLayoutChange();
        return createEditorFullscreenState();
    }

    if (standaloneNativeFullscreen)
    {
        if (auto* standaloneWindow = getStandaloneFullscreenWindow())
            standaloneWindow->setFullScreen (false);

        standaloneNativeFullscreen = false;
        setStandaloneWrapperChromeVisible (true);
    }

    const auto restoreBounds = preFullscreenEditorBounds;
    editorFullscreen = false;
    applyNormalResizeLimits();

    if (! restoreBounds.isEmpty())
        setSize (restoreBounds.getWidth(), restoreBounds.getHeight());

    nudgeWebViewAfterLayoutChange();
    return createEditorFullscreenState();
}

juce::var PluginEditor::handleEditorFullscreenRequest (const int mode)
{
    if (mode < 0)
        return createEditorFullscreenState();

    return setEditorFullscreen (mode != 0);
}

juce::var PluginEditor::handleEditorScaleMinimumRequest (const int minWidth, const int minHeight)
{
    const auto previousMinimumWidth = scaleMinimumWidth;
    const auto previousMinimumHeight = scaleMinimumHeight;
    const auto followsScaleMinimum = getWidth() == previousMinimumWidth
                                  && getHeight() == previousMinimumHeight;

    scaleMinimumWidth = juce::jlimit (900, 2400, minWidth);
    scaleMinimumHeight = juce::jlimit (480, 1800, minHeight);

    if (! editorFullscreen)
    {
        applyNormalResizeLimits();
        const auto nextWidth = followsScaleMinimum ? scaleMinimumWidth
                                                   : juce::jmax (getWidth(), scaleMinimumWidth);
        const auto nextHeight = followsScaleMinimum ? scaleMinimumHeight
                                                    : juce::jmax (getHeight(), scaleMinimumHeight);
        setSize (nextWidth, nextHeight);
    }

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("available", 1);
    object->setProperty ("minWidth", scaleMinimumWidth);
    object->setProperty ("minHeight", scaleMinimumHeight);
    return juce::var (object.release());
}
#endif

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
